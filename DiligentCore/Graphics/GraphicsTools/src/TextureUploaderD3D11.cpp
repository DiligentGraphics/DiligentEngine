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

#include "pch.h"

#include <unordered_map>
#include <deque>
#include <mutex>
#include <atlbase.h>
#include <vector>
#include <d3d11.h>

#include "TextureUploaderD3D11.hpp"
#include "RenderDeviceD3D11.h"
#include "DeviceContextD3D11.h"
#include "TextureD3D11.h"
#include "DXGITypeConversions.hpp"
#include "ThreadSignal.hpp"
#include "GraphicsAccessories.hpp"

namespace Diligent
{


namespace
{

class UploadBufferD3D11 : public UploadBufferBase
{
public:
    UploadBufferD3D11(IReferenceCounters* pRefCounters, const UploadBufferDesc& Desc, ID3D11Texture2D* pStagingTexture) :
        UploadBufferBase{pRefCounters, Desc},
        m_pStagingTexture{pStagingTexture}
    {}

    ~UploadBufferD3D11()
    {
    }

    // http://en.cppreference.com/w/cpp/thread/condition_variable
    void WaitForMap()
    {
        m_BufferMappedSignal.Wait();
    }

    void SignalMapped()
    {
        m_BufferMappedSignal.Trigger();
    }

    void SignalCopyScheduled()
    {
        m_CopyScheduledSignal.Trigger();
    }

    virtual void WaitForCopyScheduled() override final
    {
        m_CopyScheduledSignal.Wait();
    }

    bool DbgIsCopyScheduled()
    {
        return m_CopyScheduledSignal.IsTriggered();
    }

    bool DbgIsMapped()
    {
        return m_BufferMappedSignal.IsTriggered();
    }

    void Reset()
    {
        m_BufferMappedSignal.Reset();
        m_CopyScheduledSignal.Reset();
        UploadBufferBase::Reset();
    }

    ID3D11Texture2D* GetStagingTex() { return m_pStagingTexture; }

private:
    ThreadingTools::Signal   m_BufferMappedSignal;
    ThreadingTools::Signal   m_CopyScheduledSignal;
    CComPtr<ID3D11Texture2D> m_pStagingTexture;
};

} // namespace


struct TextureUploaderD3D11::InternalData
{
    struct PendingBufferOperation
    {
        enum Operation
        {
            Map,
            Copy
        } operation;
        RefCntAutoPtr<UploadBufferD3D11> pUploadBuffer;
        CComPtr<ID3D11Resource>          pd3d11NativeDstTexture;
        Uint32                           DstMip       = 0;
        Uint32                           DstSlice     = 0;
        Uint32                           DstMipLevels = 0;

        // clang-format off
        PendingBufferOperation(Operation op, UploadBufferD3D11* pBuff) :
            operation    {op   },
            pUploadBuffer{pBuff}
        {}
        PendingBufferOperation(Operation op, UploadBufferD3D11* pBuff, ID3D11Resource* pd3d11DstTex, Uint32 Mip, Uint32 Slice, Uint32 MipLevels) :
            operation             {op          },
            pUploadBuffer         {pBuff       },
            pd3d11NativeDstTexture{pd3d11DstTex},
            DstMip                {Mip         },
            DstSlice              {Slice       },
            DstMipLevels          {MipLevels   }
        {}
        // clang-format on
    };

    InternalData(IRenderDevice* pDevice)
    {
        RefCntAutoPtr<IRenderDeviceD3D11> pDeviceD3D11(pDevice, IID_RenderDeviceD3D11);
        m_pd3d11NativeDevice = pDeviceD3D11->GetD3D11Device();
    }

    CComPtr<ID3D11Device> m_pd3d11NativeDevice;

    void SwapMapQueues()
    {
        std::lock_guard<std::mutex> QueueLock(m_PendingOperationsMtx);
        m_PendingOperations.swap(m_InWorkOperations);
    }

    void EnqueCopy(UploadBufferD3D11* pUploadBuffer, ID3D11Resource* pd3d11DstTex, Uint32 Mip, Uint32 Slice, Uint32 MipLevels)
    {
        std::lock_guard<std::mutex> QueueLock(m_PendingOperationsMtx);
        m_PendingOperations.emplace_back(PendingBufferOperation::Operation::Copy, pUploadBuffer, pd3d11DstTex, Mip, Slice, MipLevels);
    }

    void EnqueMap(UploadBufferD3D11* pUploadBuffer, PendingBufferOperation::Operation Op)
    {
        std::lock_guard<std::mutex> QueueLock(m_PendingOperationsMtx);
        m_PendingOperations.emplace_back(Op, pUploadBuffer);
    }

    void Execute(ID3D11DeviceContext* pd3d11NativeCtx, PendingBufferOperation& OperationInfo, bool ExecuteImmediately);

    void ExecuteImmediately(IDeviceContext* pContext, PendingBufferOperation& OperationInfo)
    {
        RefCntAutoPtr<IDeviceContextD3D11> pContextD3D11(pContext, IID_DeviceContextD3D11);
        if (pContextD3D11)
        {
            auto* pd3d11NativeCtx = pContextD3D11->GetD3D11DeviceContext();
            Execute(pd3d11NativeCtx, OperationInfo, true /*ExecuteImmediately*/);
        }
        else
        {
            UNEXPECTED("Failed to query IID_DeviceContextD3D11 interface from the device context. "
                       "Is it really Diligent::IDeviceContextD3D11 interface?");
        }
    }

    std::mutex                          m_PendingOperationsMtx;
    std::vector<PendingBufferOperation> m_PendingOperations;
    std::vector<PendingBufferOperation> m_InWorkOperations;

    std::mutex                                                                         m_UploadBuffCacheMtx;
    std::unordered_map<UploadBufferDesc, std::deque<RefCntAutoPtr<UploadBufferD3D11>>> m_UploadBufferCache;
};

TextureUploaderD3D11::TextureUploaderD3D11(IReferenceCounters* pRefCounters, IRenderDevice* pDevice, const TextureUploaderDesc Desc) :
    TextureUploaderBase{pRefCounters, pDevice, Desc},
    m_pInternalData{new InternalData(pDevice)}
{
}

TextureUploaderD3D11::~TextureUploaderD3D11()
{
    auto Stats = TextureUploaderD3D11::GetStats();
    if (Stats.NumPendingOperations != 0)
    {
        LOG_WARNING_MESSAGE("TextureUploaderD3D11::~TextureUploaderD3D11(): there ", (Stats.NumPendingOperations > 1 ? "are " : "is "),
                            Stats.NumPendingOperations, (Stats.NumPendingOperations > 1 ? " pending operations" : " pending operation"),
                            " in the queue. If other threads wait for ", (Stats.NumPendingOperations > 1 ? "these operations" : "this operation"),
                            ", they may deadlock.");
    }

    for (auto BuffQueueIt : m_pInternalData->m_UploadBufferCache)
    {
        if (BuffQueueIt.second.size())
        {
            const auto& desc    = BuffQueueIt.first;
            auto&       FmtInfo = m_pDevice->GetTextureFormatInfo(desc.Format);
            LOG_INFO_MESSAGE("TextureUploaderD3D11: releasing ", BuffQueueIt.second.size(), ' ', desc.Width, 'x', desc.Height, 'x', desc.Depth, ' ', FmtInfo.Name,
                             " staging texture", (BuffQueueIt.second.size() != 1 ? "s" : ""));
        }
    }
}

void TextureUploaderD3D11::RenderThreadUpdate(IDeviceContext* pContext)
{
    m_pInternalData->SwapMapQueues();
    if (!m_pInternalData->m_InWorkOperations.empty())
    {
        RefCntAutoPtr<IDeviceContextD3D11> pContextD3D11(pContext, IID_DeviceContextD3D11);

        auto* pd3d11NativeCtx = pContextD3D11->GetD3D11DeviceContext();

        for (auto& Operation : m_pInternalData->m_InWorkOperations)
        {
            m_pInternalData->Execute(pd3d11NativeCtx, Operation, false /*ExecuteImmediately*/);
        }

        m_pInternalData->m_InWorkOperations.clear();
    }
}

void TextureUploaderD3D11::InternalData::Execute(ID3D11DeviceContext*    pd3d11NativeCtx,
                                                 PendingBufferOperation& OperationInfo,
                                                 bool                    ExecuteImmediately)
{
    auto&       pBuffer        = OperationInfo.pUploadBuffer;
    const auto& UploadBuffDesc = pBuffer->GetDesc();

    switch (OperationInfo.operation)
    {
        case InternalData::PendingBufferOperation::Map:
        {
            bool AllMapped = true;
            for (Uint32 Slice = 0; Slice < UploadBuffDesc.ArraySize; ++Slice)
            {
                for (Uint32 Mip = 0; Mip < UploadBuffDesc.MipLevels; ++Mip)
                {
                    if (!pBuffer->IsMapped(Mip, Slice))
                    {
                        D3D11_MAPPED_SUBRESOURCE MappedData;

                        UINT Subres = D3D11CalcSubresource(static_cast<UINT>(Mip), static_cast<UINT>(Slice), static_cast<UINT>(UploadBuffDesc.MipLevels));
                        auto hr     = pd3d11NativeCtx->Map(pBuffer->GetStagingTex(), Subres, D3D11_MAP_WRITE,
                                                       ExecuteImmediately ? 0 : D3D11_MAP_FLAG_DO_NOT_WAIT,
                                                       &MappedData);
                        if (SUCCEEDED(hr))
                        {
                            pBuffer->SetMappedData(Mip, Slice, MappedTextureSubresource{MappedData.pData, MappedData.RowPitch, MappedData.DepthPitch});
                        }
                        else
                        {
                            VERIFY_EXPR(!ExecuteImmediately);
                            if (hr == DXGI_ERROR_WAS_STILL_DRAWING)
                            {
                                AllMapped = false;
                            }
                            else
                            {
                                LOG_ERROR("Unknown DX error when mapping staging texture: ", hr);
                            }
                        }
                    }
                }
            }

            if (AllMapped)
            {
                pBuffer->SignalMapped();
            }
            else
            {
                VERIFY_EXPR(!ExecuteImmediately);
                EnqueMap(pBuffer, OperationInfo.operation);
            }
        }
        break;

        case InternalData::PendingBufferOperation::Copy:
        {
            VERIFY(pBuffer->DbgIsMapped(), "Upload buffer must be copied only after it has been mapped");
            // Unmap all subresources first to avoid D3D11 warnings
            for (Uint32 Subres = 0; Subres < UploadBuffDesc.MipLevels * UploadBuffDesc.ArraySize; ++Subres)
            {
                pd3d11NativeCtx->Unmap(pBuffer->GetStagingTex(), Subres);
            }

            for (Uint32 Slice = 0; Slice < UploadBuffDesc.ArraySize; ++Slice)
            {
                for (Uint32 Mip = 0; Mip < UploadBuffDesc.MipLevels; ++Mip)
                {
                    UINT SrcSubres = D3D11CalcSubresource(
                        static_cast<UINT>(Mip),
                        static_cast<UINT>(Slice),
                        static_cast<UINT>(UploadBuffDesc.MipLevels));
                    UINT DstSubres = D3D11CalcSubresource(
                        static_cast<UINT>(OperationInfo.DstMip + Mip),
                        static_cast<UINT>(OperationInfo.DstSlice + Slice),
                        static_cast<UINT>(OperationInfo.DstMipLevels));
                    pd3d11NativeCtx->CopySubresourceRegion(OperationInfo.pd3d11NativeDstTexture, DstSubres,
                                                           0, 0, 0, // DstX, DstY, DstZ
                                                           pBuffer->GetStagingTex(),
                                                           SrcSubres,
                                                           nullptr // pSrcBox
                    );
                }
            }
            pBuffer->SignalCopyScheduled();
        }
        break;
    }
}

void TextureUploaderD3D11::AllocateUploadBuffer(IDeviceContext*         pContext,
                                                const UploadBufferDesc& Desc,
                                                IUploadBuffer**         ppBuffer)
{
    *ppBuffer = nullptr;

    RefCntAutoPtr<UploadBufferD3D11> pUploadBuffer;

    {
        std::lock_guard<std::mutex> CacheLock(m_pInternalData->m_UploadBuffCacheMtx);

        auto& Cache = m_pInternalData->m_UploadBufferCache;
        if (!Cache.empty())
        {
            auto DequeIt = Cache.find(Desc);
            if (DequeIt != Cache.end())
            {
                auto& Deque = DequeIt->second;
                if (!Deque.empty())
                {
                    pUploadBuffer = std::move(Deque.front());
                    Deque.pop_front();
                }
            }
        }
    }

    if (!pUploadBuffer)
    {
        // clang-format off
        D3D11_TEXTURE2D_DESC StagingTexDesc =
        {
            static_cast<UINT>(Desc.Width),
            static_cast<UINT>(Desc.Height),
            static_cast<UINT>(Desc.MipLevels),
            static_cast<UINT>(Desc.ArraySize),
            TexFormatToDXGI_Format(Desc.Format),
            {1, 0},     // DXGI_SAMPLE_DESC SampleDesc;
            D3D11_USAGE_STAGING,
            0,          // UINT BindFlags;
            D3D11_CPU_ACCESS_WRITE, //  UINT CPUAccessFlags;
            0,          // UINT MiscFlags;
        };
        // clang-format on

        CComPtr<ID3D11Texture2D> pStagingTex;

        HRESULT hr = m_pInternalData->m_pd3d11NativeDevice->CreateTexture2D(&StagingTexDesc, nullptr, &pStagingTex);
        if (FAILED(hr))
        {
            LOG_ERROR_MESSAGE("Failed to allocate staging D3D11 texture");
            return;
        }

        LOG_INFO_MESSAGE("TextureUploaderD3D11: created ", Desc.Width, 'x', Desc.Height, 'x', Desc.Depth, ' ',
                         Desc.MipLevels, "-mip ", Desc.ArraySize, "-slice ",
                         m_pDevice->GetTextureFormatInfo(Desc.Format).Name, " staging texture");

        pUploadBuffer = MakeNewRCObj<UploadBufferD3D11>()(Desc, pStagingTex);
    }

    if (pUploadBuffer)
    {
        if (pContext != nullptr)
        {
            // Main thread
            InternalData::PendingBufferOperation MapOp{InternalData::PendingBufferOperation::Map, pUploadBuffer};
            m_pInternalData->ExecuteImmediately(pContext, MapOp);
        }
        else
        {
            // Worker thread
            m_pInternalData->EnqueMap(pUploadBuffer, InternalData::PendingBufferOperation::Map);
            pUploadBuffer->WaitForMap();
        }
    }

    *ppBuffer = pUploadBuffer.Detach();
}

void TextureUploaderD3D11::ScheduleGPUCopy(IDeviceContext* pContext,
                                           ITexture*       pDstTexture,
                                           Uint32          ArraySlice,
                                           Uint32          MipLevel,
                                           IUploadBuffer*  pUploadBuffer)
{
    auto*                        pUploadBufferD3D11 = ValidatedCast<UploadBufferD3D11>(pUploadBuffer);
    RefCntAutoPtr<ITextureD3D11> pDstTexD3D11(pDstTexture, IID_TextureD3D11);
    auto*                        pd3d11NativeDstTex = pDstTexD3D11->GetD3D11Texture();
    const auto&                  DstTexDesc         = pDstTexture->GetDesc();
    if (pContext != nullptr)
    {
        // Main thread
        InternalData::PendingBufferOperation CopyOp //
            {
                InternalData::PendingBufferOperation::Copy,
                pUploadBufferD3D11,
                pd3d11NativeDstTex,
                MipLevel,
                ArraySlice,
                DstTexDesc.MipLevels //
            };
        m_pInternalData->ExecuteImmediately(pContext, CopyOp);
    }
    else
    {
        // Worker thread
        m_pInternalData->EnqueCopy(pUploadBufferD3D11, pd3d11NativeDstTex, MipLevel, ArraySlice, DstTexDesc.MipLevels);
    }
}

void TextureUploaderD3D11::RecycleBuffer(IUploadBuffer* pUploadBuffer)
{
    auto* pUploadBufferD3D11 = ValidatedCast<UploadBufferD3D11>(pUploadBuffer);
    VERIFY(pUploadBufferD3D11->DbgIsCopyScheduled(), "Upload buffer must be recycled only after copy operation has been scheduled on the GPU");
    pUploadBufferD3D11->Reset();

    std::lock_guard<std::mutex> CacheLock(m_pInternalData->m_UploadBuffCacheMtx);
    m_pInternalData->m_UploadBufferCache[pUploadBufferD3D11->GetDesc()].emplace_back(pUploadBufferD3D11);
}

TextureUploaderStats TextureUploaderD3D11::GetStats()
{
    TextureUploaderStats        Stats;
    std::lock_guard<std::mutex> QueueLock(m_pInternalData->m_PendingOperationsMtx);
    Stats.NumPendingOperations = static_cast<Uint32>(m_pInternalData->m_PendingOperations.size());

    return Stats;
}

} // namespace Diligent
