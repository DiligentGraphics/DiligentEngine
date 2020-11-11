/*
 *  Copyright 2019-2020 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *  
 *      http://www.apache.org/licenses/LICENSE-2.0
 *  
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  In no event and under no legal theory, whether in tort (including negligence), 
 *  contract, or otherwise, unless required by applicable law (such as deliberate 
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental, 
 *  or consequential damages of any character arising as a result of this License or 
 *  out of the use or inability to use the software (including but not limited to damages 
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and 
 *  all other commercial damages or losses), even if such Contributor has been advised 
 *  of the possibility of such damages.
 */

#include <mutex>
#include <unordered_map>
#include <deque>
#include <vector>

#include "TextureUploaderD3D12_Vk.hpp"
#include "ThreadSignal.hpp"
#include "GraphicsAccessories.hpp"

namespace Diligent
{

namespace
{

class UploadTexture : public UploadBufferBase
{
public:
    UploadTexture(IReferenceCounters*     pRefCounters,
                  const UploadBufferDesc& Desc,
                  ITexture*               pStagingTexture) :
        // clang-format off
        UploadBufferBase {pRefCounters, Desc},
        m_pStagingTexture{pStagingTexture}
    // clang-format on
    {
    }

    ~UploadTexture()
    {
        for (Uint32 Slice = 0; Slice < m_Desc.ArraySize; ++Slice)
        {
            for (Uint32 Mip = 0; Mip < m_Desc.MipLevels; ++Mip)
            {
                DEV_CHECK_ERR(!IsMapped(Mip, Slice), "Releasing mapped staging texture");
            }
        }
    }

    void WaitForMap()
    {
        m_TextureMappedSignal.Wait();
    }

    void SignalMapped()
    {
        m_TextureMappedSignal.Trigger();
    }

    void SignalCopyScheduled(Uint64 FenceValue)
    {
        m_CopyScheduledFenceValue = FenceValue;
        m_CopyScheduledSignal.Trigger();
    }

    void Unmap(IDeviceContext* pDeviceContext, Uint32 Mip, Uint32 Slice)
    {
        VERIFY(IsMapped(Mip, Slice), "This subresource is not mapped");
        pDeviceContext->UnmapTextureSubresource(m_pStagingTexture, Mip, Slice);
        SetMappedData(Mip, Slice, MappedTextureSubresource{});
    }

    void Map(IDeviceContext* pDeviceContext, Uint32 Mip, Uint32 Slice)
    {
        VERIFY(!IsMapped(Mip, Slice), "This subresource is already mapped");
        MappedTextureSubresource MappedData;
        pDeviceContext->MapTextureSubresource(m_pStagingTexture, Mip, Slice, MAP_WRITE, MAP_FLAG_NO_OVERWRITE, nullptr, MappedData);
        SetMappedData(Mip, Slice, MappedData);
    }

    void Reset()
    {
        m_CopyScheduledSignal.Reset();
        m_TextureMappedSignal.Reset();
        m_CopyScheduledFenceValue = 0;
        UploadBufferBase::Reset();
    }

    virtual void WaitForCopyScheduled() override final
    {
        m_CopyScheduledSignal.Wait();
    }

    ITexture* GetStagingTexture() { return m_pStagingTexture; }

    bool DbgIsCopyScheduled() const
    {
        return m_CopyScheduledSignal.IsTriggered();
    }

    bool DbgIsMapped()
    {
        return m_TextureMappedSignal.IsTriggered();
    }

    Uint64 GetCopyScheduledFenceValue() const
    {
        VERIFY(m_CopyScheduledFenceValue != 0, "Fence value has not been initialized");
        return m_CopyScheduledFenceValue;
    }

private:
    ThreadingTools::Signal m_CopyScheduledSignal;
    ThreadingTools::Signal m_TextureMappedSignal;

    RefCntAutoPtr<ITexture> m_pStagingTexture;
    Uint64                  m_CopyScheduledFenceValue = 0;
};

} // namespace


struct TextureUploaderD3D12_Vk::InternalData
{
    struct PendingBufferOperation
    {
        enum Operation
        {
            Copy,
            Map
        } operation;
        RefCntAutoPtr<UploadTexture> pUploadTexture;
        RefCntAutoPtr<ITexture>      pDstTexture;
        Uint32                       DstSlice = 0;
        Uint32                       DstMip   = 0;

        // clang-format off
        PendingBufferOperation(Operation op, UploadTexture* pUploadTex) :
            operation     {op        },
            pUploadTexture{pUploadTex}
        {}
        PendingBufferOperation(Operation op, UploadTexture* pUploadTex, ITexture* pDstTex, Uint32 dstSlice, Uint32 dstMip) :
            operation      {op        },
            pUploadTexture {pUploadTex},
            pDstTexture    {pDstTex   },
            DstSlice       {dstSlice  },
            DstMip         {dstMip    }
        {}
        // clang-format on
    };

    InternalData(IRenderDevice* pDevice)
    {
        FenceDesc fenceDesc;
        fenceDesc.Name = "Texture uploader sync fence";
        pDevice->CreateFence(fenceDesc, &m_pFence);
    }

    ~InternalData()
    {
        for (auto it : m_UploadTexturesCache)
        {
            if (it.second.size())
            {
                const auto& desc    = it.first;
                auto&       FmtInfo = GetTextureFormatAttribs(desc.Format);
                LOG_INFO_MESSAGE("TextureUploaderD3D12_Vk: releasing ", it.second.size(), ' ',
                                 desc.Width, 'x', desc.Height, 'x', desc.Depth, ' ', FmtInfo.Name,
                                 " upload buffer(s)", (it.second.size() == 1 ? "" : "s"));
            }
        }
    }

    std::vector<PendingBufferOperation>& SwapMapQueues()
    {
        std::lock_guard<std::mutex> QueueLock(m_PendingOperationsMtx);
        m_PendingOperations.swap(m_InWorkOperations);
        return m_InWorkOperations;
    }

    void EnqueCopy(UploadTexture* pUploadBuffer, ITexture* pDstTex, Uint32 dstSlice, Uint32 dstMip)
    {
        std::lock_guard<std::mutex> QueueLock(m_PendingOperationsMtx);
        m_PendingOperations.emplace_back(PendingBufferOperation::Operation::Copy, pUploadBuffer, pDstTex, dstSlice, dstMip);
    }

    void EnqueMap(UploadTexture* pUploadBuffer)
    {
        std::lock_guard<std::mutex> QueueLock(m_PendingOperationsMtx);
        m_PendingOperations.emplace_back(PendingBufferOperation::Operation::Map, pUploadBuffer);
    }

    Uint64 SignalFence(IDeviceContext* pContext)
    {
        // Fences can't be accessed from multiple threads simultaneously even
        // when protected by mutex
        auto FenceValue = m_NextFenceValue++;
        pContext->SignalFence(m_pFence, FenceValue);
        return FenceValue;
    }

    void UpdatedCompletedFenceValue()
    {
        // Fences can't be accessed from multiple threads simultaneously even
        // when protected by mutex
        m_CompletedFenceValue = m_pFence->GetCompletedValue();
    }

    RefCntAutoPtr<UploadTexture> FindCachedUploadTexture(const UploadBufferDesc& Desc)
    {
        RefCntAutoPtr<UploadTexture> pUploadTexture;
        std::lock_guard<std::mutex>  CacheLock(m_UploadTexturesCacheMtx);
        auto                         DequeIt = m_UploadTexturesCache.find(Desc);
        if (DequeIt != m_UploadTexturesCache.end())
        {
            auto& Deque = DequeIt->second;
            if (!Deque.empty())
            {
                auto& FrontBuff = Deque.front();
                if (FrontBuff->GetCopyScheduledFenceValue() <= m_CompletedFenceValue)
                {
                    pUploadTexture = std::move(FrontBuff);
                    Deque.pop_front();
                    pUploadTexture->Reset();
                }
            }
        }

        return pUploadTexture;
    }

    void RecycleUploadTexture(UploadTexture* pUploadTexture)
    {
        std::lock_guard<std::mutex> CacheLock(m_UploadTexturesCacheMtx);
        auto&                       Deque = m_UploadTexturesCache[pUploadTexture->GetDesc()];
        Deque.emplace_back(pUploadTexture);
    }

    Uint32 GetNumPendingOperations()
    {
        std::lock_guard<std::mutex> QueueLock(m_PendingOperationsMtx);
        return static_cast<Uint32>(m_PendingOperations.size());
    }

    void Execute(IDeviceContext* pContext, PendingBufferOperation& OperationInfo);

private:
    std::mutex                          m_PendingOperationsMtx;
    std::vector<PendingBufferOperation> m_PendingOperations;
    std::vector<PendingBufferOperation> m_InWorkOperations;

    std::mutex                                                                     m_UploadTexturesCacheMtx;
    std::unordered_map<UploadBufferDesc, std::deque<RefCntAutoPtr<UploadTexture>>> m_UploadTexturesCache;

    RefCntAutoPtr<IFence> m_pFence;
    Uint64                m_NextFenceValue      = 1;
    Uint64                m_CompletedFenceValue = 0;
};

TextureUploaderD3D12_Vk::TextureUploaderD3D12_Vk(IReferenceCounters* pRefCounters, IRenderDevice* pDevice, const TextureUploaderDesc Desc) :
    TextureUploaderBase{pRefCounters, pDevice, Desc},
    m_pInternalData{new InternalData(pDevice)}
{
}

TextureUploaderD3D12_Vk::~TextureUploaderD3D12_Vk()
{
    auto NumPendingOperations = m_pInternalData->GetNumPendingOperations();
    if (NumPendingOperations != 0)
    {
        LOG_WARNING_MESSAGE("TextureUploaderD3D12_Vk::~TextureUploaderD3D12_Vk(): there ", (NumPendingOperations > 1 ? "are " : "is "),
                            NumPendingOperations, (NumPendingOperations > 1 ? " pending operations" : " pending operation"),
                            " in the queue. If other threads wait for ", (NumPendingOperations > 1 ? "these operations" : "this operation"),
                            ", they may deadlock.");
    }
}

void TextureUploaderD3D12_Vk::RenderThreadUpdate(IDeviceContext* pContext)
{
    auto& InWorkOperations = m_pInternalData->SwapMapQueues();
    if (!InWorkOperations.empty())
    {
        Uint32 NumCopyOperations = 0;
        for (auto& OperationInfo : InWorkOperations)
        {
            m_pInternalData->Execute(pContext, OperationInfo);
            if (OperationInfo.operation == InternalData::PendingBufferOperation::Copy)
                ++NumCopyOperations;
        }

        if (NumCopyOperations > 0)
        {
            // The buffer may be recycled immediately after the copy scheduled is signaled,
            // so we must signal the fence first.
            auto SignaledFenceValue = m_pInternalData->SignalFence(pContext);

            for (auto& OperationInfo : InWorkOperations)
            {
                if (OperationInfo.operation == InternalData::PendingBufferOperation::Copy)
                    OperationInfo.pUploadTexture->SignalCopyScheduled(SignaledFenceValue);
            }
        }

        InWorkOperations.clear();
    }

    // This must be called by the same thread that signals the fence
    m_pInternalData->UpdatedCompletedFenceValue();
}


void TextureUploaderD3D12_Vk::InternalData::Execute(IDeviceContext*         pContext,
                                                    PendingBufferOperation& OperationInfo)
{
    auto&       pUploadTex     = OperationInfo.pUploadTexture;
    const auto& StagingTexDesc = pUploadTex->GetDesc();

    switch (OperationInfo.operation)
    {
        case InternalData::PendingBufferOperation::Map:
        {
            for (Uint32 Slice = 0; Slice < StagingTexDesc.ArraySize; ++Slice)
            {
                for (Uint32 Mip = 0; Mip < StagingTexDesc.MipLevels; ++Mip)
                {
                    pUploadTex->Map(pContext, Mip, Slice);
                }
            }
            pUploadTex->SignalMapped();
        }
        break;

        case InternalData::PendingBufferOperation::Copy:
        {
            VERIFY(pUploadTex->DbgIsMapped(), "Upload texture must be copied only after it has been mapped");
            for (Uint32 Slice = 0; Slice < StagingTexDesc.ArraySize; ++Slice)
            {
                for (Uint32 Mip = 0; Mip < StagingTexDesc.MipLevels; ++Mip)
                {
                    pUploadTex->Unmap(pContext, Mip, Slice);

                    CopyTextureAttribs CopyInfo //
                        {
                            pUploadTex->GetStagingTexture(),
                            RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
                            OperationInfo.pDstTexture,
                            RESOURCE_STATE_TRANSITION_MODE_TRANSITION //
                        };
                    CopyInfo.SrcMipLevel = Mip;
                    CopyInfo.SrcSlice    = Slice;
                    CopyInfo.DstMipLevel = OperationInfo.DstMip + Mip;
                    CopyInfo.DstSlice    = OperationInfo.DstSlice + Slice;
                    pContext->CopyTexture(CopyInfo);
                }
            }
        }
        break;
    }
}

void TextureUploaderD3D12_Vk::AllocateUploadBuffer(IDeviceContext*         pContext,
                                                   const UploadBufferDesc& Desc,
                                                   IUploadBuffer**         ppBuffer)
{
    RefCntAutoPtr<UploadTexture> pUploadTexture = m_pInternalData->FindCachedUploadTexture(Desc);

    // No available buffer found in the cache
    if (!pUploadTexture)
    {
        TextureDesc StagingTexDesc;
        StagingTexDesc.Type           = Desc.ArraySize == 1 ? RESOURCE_DIM_TEX_2D : RESOURCE_DIM_TEX_2D_ARRAY;
        StagingTexDesc.Width          = Desc.Width;
        StagingTexDesc.Height         = Desc.Height;
        StagingTexDesc.Format         = Desc.Format;
        StagingTexDesc.MipLevels      = Desc.MipLevels;
        StagingTexDesc.ArraySize      = Desc.ArraySize;
        StagingTexDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        StagingTexDesc.Usage          = USAGE_STAGING;

        RefCntAutoPtr<ITexture> pStagingTexture;
        m_pDevice->CreateTexture(StagingTexDesc, nullptr, &pStagingTexture);

        LOG_INFO_MESSAGE("Created ", Desc.Width, "x", Desc.Height, 'x', Desc.Depth, ' ', Desc.MipLevels, "-mip ",
                         Desc.ArraySize, "-slice ",
                         GetTextureFormatAttribs(Desc.Format).Name, " staging texture");

        pUploadTexture = MakeNewRCObj<UploadTexture>()(Desc, pStagingTexture);
    }

    if (pContext != nullptr)
    {
        // Render thread
        InternalData::PendingBufferOperation MapOp{InternalData::PendingBufferOperation::Operation::Map, pUploadTexture};
        m_pInternalData->Execute(pContext, MapOp);
    }
    else
    {
        // Worker thread
        m_pInternalData->EnqueMap(pUploadTexture);
        pUploadTexture->WaitForMap();
    }
    *ppBuffer = pUploadTexture.Detach();
}

void TextureUploaderD3D12_Vk::ScheduleGPUCopy(IDeviceContext* pContext,
                                              ITexture*       pDstTexture,
                                              Uint32          ArraySlice,
                                              Uint32          MipLevel,
                                              IUploadBuffer*  pUploadBuffer)
{
    auto* pUploadTexture = ValidatedCast<UploadTexture>(pUploadBuffer);
    if (pContext != nullptr)
    {
        // Render thread
        InternalData::PendingBufferOperation CopyOp //
            {
                InternalData::PendingBufferOperation::Operation::Copy,
                pUploadTexture,
                pDstTexture,
                ArraySlice,
                MipLevel //
            };
        m_pInternalData->Execute(pContext, CopyOp);

        // The buffer may be recycled immediately after the copy scheduled is signaled,
        // so we must signal the fence first.
        auto SignaledFenceValue = m_pInternalData->SignalFence(pContext);
        pUploadTexture->SignalCopyScheduled(SignaledFenceValue);
        // This must be called by the same thread that signals the fence
        m_pInternalData->UpdatedCompletedFenceValue();
    }
    else
    {
        // Worker thread
        m_pInternalData->EnqueCopy(pUploadTexture, pDstTexture, ArraySlice, MipLevel);
    }
}

void TextureUploaderD3D12_Vk::RecycleBuffer(IUploadBuffer* pUploadBuffer)
{
    auto* pUploadTexture = ValidatedCast<UploadTexture>(pUploadBuffer);
    VERIFY(pUploadTexture->DbgIsCopyScheduled(), "Upload buffer must be recycled only after copy operation has been scheduled on the GPU");

    m_pInternalData->RecycleUploadTexture(pUploadTexture);
}

TextureUploaderStats TextureUploaderD3D12_Vk::GetStats()
{
    TextureUploaderStats Stats;
    Stats.NumPendingOperations = static_cast<Uint32>(m_pInternalData->GetNumPendingOperations());
    return Stats;
}

} // namespace Diligent
