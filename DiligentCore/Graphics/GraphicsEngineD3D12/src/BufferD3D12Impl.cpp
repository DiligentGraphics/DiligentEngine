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
#include "BufferD3D12Impl.hpp"
#include "RenderDeviceD3D12Impl.hpp"
#include "DeviceContextD3D12Impl.hpp"
#include "D3D12TypeConversions.hpp"
#include "BufferViewD3D12Impl.hpp"
#include "GraphicsAccessories.hpp"
#include "DXGITypeConversions.hpp"
#include "EngineMemory.h"
#include "StringTools.hpp"

namespace Diligent
{

BufferD3D12Impl::BufferD3D12Impl(IReferenceCounters*        pRefCounters,
                                 FixedBlockMemoryAllocator& BuffViewObjMemAllocator,
                                 RenderDeviceD3D12Impl*     pRenderDeviceD3D12,
                                 const BufferDesc&          BuffDesc,
                                 const BufferData*          pBuffData /*= nullptr*/) :
    // clang-format off
    TBufferBase
    {
        pRefCounters,
        BuffViewObjMemAllocator,
        pRenderDeviceD3D12,
        BuffDesc,
        false
    },
    m_DynamicData
    (
        BuffDesc.Usage == USAGE_DYNAMIC ? (1 + pRenderDeviceD3D12->GetNumDeferredContexts()) : 0,
        D3D12DynamicAllocation{},
        STD_ALLOCATOR_RAW_MEM(D3D12DynamicAllocation, GetRawAllocator(), "Allocator for vector<DynamicAllocation>")
    )
// clang-format on
{
    ValidateBufferInitData(BuffDesc, pBuffData);

    if (m_Desc.Usage == USAGE_UNIFIED)
    {
        LOG_ERROR_AND_THROW("Unified resources are not supported in Direct3D12");
    }

    if (m_Desc.Usage == USAGE_IMMUTABLE)
        VERIFY(pBuffData != nullptr && pBuffData->pData != nullptr, "Initial data must not be null for immutable buffers");
    if (m_Desc.Usage == USAGE_DYNAMIC)
        VERIFY(pBuffData == nullptr || pBuffData->pData == nullptr, "Initial data must be null for dynamic buffers");

    Uint32 AlignmentMask = 1;
    if (m_Desc.BindFlags & BIND_UNIFORM_BUFFER)
        AlignmentMask = 255;

    if (m_Desc.Usage == USAGE_STAGING)
    {
        VERIFY(m_Desc.CPUAccessFlags == CPU_ACCESS_WRITE || m_Desc.CPUAccessFlags == CPU_ACCESS_READ,
               "Exactly one of the CPU_ACCESS_WRITE or CPU_ACCESS_READ flags must be specified for a staging buffer");

        if (m_Desc.CPUAccessFlags == CPU_ACCESS_WRITE)
        {
            VERIFY(pBuffData == nullptr || pBuffData->pData == nullptr, "CPU-writable staging buffers must be updated via map");

            AlignmentMask = D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1;
        }
    }

    if (AlignmentMask != 1)
        m_Desc.uiSizeInBytes = (m_Desc.uiSizeInBytes + AlignmentMask) & (~AlignmentMask);

    if (m_Desc.Usage == USAGE_DYNAMIC && (m_Desc.BindFlags & (BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS)) == 0)
    {
        // Dynamic constant/vertex/index buffers are suballocated in the upload heap when Map() is called.
        // Dynamic buffers with SRV or UAV flags need to be allocated in GPU-only memory
        // Dynamic upload heap buffer is always in D3D12_RESOURCE_STATE_GENERIC_READ state

        SetState(RESOURCE_STATE_GENERIC_READ);
        VERIFY_EXPR(m_DynamicData.size() == 1 + pRenderDeviceD3D12->GetNumDeferredContexts());
    }
    else
    {
        D3D12_RESOURCE_DESC D3D12BuffDesc = {};
        D3D12BuffDesc.Dimension           = D3D12_RESOURCE_DIMENSION_BUFFER;
        D3D12BuffDesc.Alignment           = 0;
        D3D12BuffDesc.Width               = m_Desc.uiSizeInBytes;
        D3D12BuffDesc.Height              = 1;
        D3D12BuffDesc.DepthOrArraySize    = 1;
        D3D12BuffDesc.MipLevels           = 1;
        D3D12BuffDesc.Format              = DXGI_FORMAT_UNKNOWN;
        D3D12BuffDesc.SampleDesc.Count    = 1;
        D3D12BuffDesc.SampleDesc.Quality  = 0;
        // Layout must be D3D12_TEXTURE_LAYOUT_ROW_MAJOR, as buffer memory layouts are
        // understood by applications and row-major texture data is commonly marshaled through buffers.
        D3D12BuffDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        D3D12BuffDesc.Flags  = D3D12_RESOURCE_FLAG_NONE;
        if (m_Desc.BindFlags & BIND_UNORDERED_ACCESS)
            D3D12BuffDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        if (!(m_Desc.BindFlags & BIND_SHADER_RESOURCE))
            D3D12BuffDesc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

        auto* pd3d12Device = pRenderDeviceD3D12->GetD3D12Device();

        D3D12_HEAP_PROPERTIES HeapProps;
        if (m_Desc.Usage == USAGE_STAGING)
            HeapProps.Type = m_Desc.CPUAccessFlags == CPU_ACCESS_READ ? D3D12_HEAP_TYPE_READBACK : D3D12_HEAP_TYPE_UPLOAD;
        else
            HeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        if (HeapProps.Type == D3D12_HEAP_TYPE_READBACK)
            SetState(RESOURCE_STATE_COPY_DEST);
        else if (HeapProps.Type == D3D12_HEAP_TYPE_UPLOAD)
            SetState(RESOURCE_STATE_GENERIC_READ);
        HeapProps.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        HeapProps.CreationNodeMask     = 1;
        HeapProps.VisibleNodeMask      = 1;

        bool bInitializeBuffer = (pBuffData != nullptr && pBuffData->pData != nullptr && pBuffData->DataSize > 0);
        if (bInitializeBuffer)
            SetState(RESOURCE_STATE_COPY_DEST);

        if (!IsInKnownState())
            SetState(RESOURCE_STATE_UNDEFINED);

        auto D3D12State = ResourceStateFlagsToD3D12ResourceStates(GetState());

        auto hr = pd3d12Device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE,
                                                        &D3D12BuffDesc, D3D12State, nullptr,
                                                        __uuidof(m_pd3d12Resource),
                                                        reinterpret_cast<void**>(static_cast<ID3D12Resource**>(&m_pd3d12Resource)));
        if (FAILED(hr))
            LOG_ERROR_AND_THROW("Failed to create D3D12 buffer");

        if (*m_Desc.Name != 0)
            m_pd3d12Resource->SetName(WidenString(m_Desc.Name).c_str());

        if (bInitializeBuffer)
        {
            D3D12_HEAP_PROPERTIES UploadHeapProps;
            UploadHeapProps.Type                 = D3D12_HEAP_TYPE_UPLOAD;
            UploadHeapProps.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            UploadHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            UploadHeapProps.CreationNodeMask     = 1;
            UploadHeapProps.VisibleNodeMask      = 1;

            D3D12BuffDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
            CComPtr<ID3D12Resource> UploadBuffer;
            hr = pd3d12Device->CreateCommittedResource(&UploadHeapProps, D3D12_HEAP_FLAG_NONE,
                                                       &D3D12BuffDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, __uuidof(UploadBuffer),
                                                       reinterpret_cast<void**>(static_cast<ID3D12Resource**>(&UploadBuffer)));
            if (FAILED(hr))
                LOG_ERROR_AND_THROW("Failed to create uload buffer");

            void* DestAddress = nullptr;

            hr = UploadBuffer->Map(0, nullptr, &DestAddress);
            if (FAILED(hr))
                LOG_ERROR_AND_THROW("Failed to map uload buffer");
            memcpy(DestAddress, pBuffData->pData, pBuffData->DataSize);
            UploadBuffer->Unmap(0, nullptr);

            auto InitContext = pRenderDeviceD3D12->AllocateCommandContext();
            // copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default buffer
            VERIFY_EXPR(CheckState(RESOURCE_STATE_COPY_DEST));
            // We MUST NOT call TransitionResource() from here, because
            // it will call AddRef() and potentially Release(), while
            // the object is not constructed yet
            InitContext->CopyResource(m_pd3d12Resource, UploadBuffer);

            // Command list fence should only be signaled when submitting cmd list
            // from the immediate context, otherwise the basic requirement will be violated
            // as in the scenario below
            // See http://diligentgraphics.com/diligent-engine/architecture/d3d12/managing-resource-lifetimes/
            //
            //  Signaled Fence  |        Immediate Context               |            InitContext            |
            //                  |                                        |                                   |
            //    N             |  Draw(ResourceX)                       |                                   |
            //                  |  Release(ResourceX)                    |                                   |
            //                  |   - (ResourceX, N) -> Release Queue    |                                   |
            //                  |                                        | CopyResource()                    |
            //   N+1            |                                        | CloseAndExecuteCommandContext()   |
            //                  |                                        |                                   |
            //   N+2            |  CloseAndExecuteCommandContext()       |                                   |
            //                  |   - Cmd list is submitted with number  |                                   |
            //                  |     N+1, but resource it references    |                                   |
            //                  |     was added to the delete queue      |                                   |
            //                  |     with value N                       |                                   |
            Uint32 QueueIndex = 0;
            pRenderDeviceD3D12->CloseAndExecuteTransientCommandContext(QueueIndex, std::move(InitContext));

            // Add reference to the object to the release queue to keep it alive
            // until copy operation is complete. This must be done after
            // submitting command list for execution!
            pRenderDeviceD3D12->SafeReleaseDeviceObject(std::move(UploadBuffer), Uint64{1} << QueueIndex);
        }

        if (m_Desc.BindFlags & BIND_UNIFORM_BUFFER)
        {
            m_CBVDescriptorAllocation = pRenderDeviceD3D12->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            CreateCBV(m_CBVDescriptorAllocation.GetCpuHandle());
        }
    }
}

static BufferDesc BufferDescFromD3D12Resource(BufferDesc BuffDesc, ID3D12Resource* pd3d12Buffer)
{
    VERIFY(BuffDesc.Usage != USAGE_DYNAMIC, "Dynamic buffers cannot be attached to native d3d12 resource");

    auto D3D12BuffDesc = pd3d12Buffer->GetDesc();
    VERIFY(D3D12BuffDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER, "D3D12 resource is not a buffer");

    VERIFY(BuffDesc.uiSizeInBytes == 0 || BuffDesc.uiSizeInBytes == D3D12BuffDesc.Width, "Buffer size specified by the BufferDesc (", BuffDesc.uiSizeInBytes, ") does not match d3d12 resource size (", D3D12BuffDesc.Width, ")");
    BuffDesc.uiSizeInBytes = static_cast<Uint32>(D3D12BuffDesc.Width);

    if (D3D12BuffDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
    {
        VERIFY(BuffDesc.BindFlags == 0 || (BuffDesc.BindFlags & BIND_UNORDERED_ACCESS), "BIND_UNORDERED_ACCESS flag is not specified by the BufferDesc, while d3d12 resource was created with D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS flag");
        BuffDesc.BindFlags |= BIND_UNORDERED_ACCESS;
    }
    if (D3D12BuffDesc.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE)
    {
        VERIFY(!(BuffDesc.BindFlags & BIND_SHADER_RESOURCE), "BIND_SHADER_RESOURCE flag is specified by the BufferDesc, while d3d12 resource was created with D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE flag");
        BuffDesc.BindFlags &= ~BIND_SHADER_RESOURCE;
    }

    if ((BuffDesc.BindFlags & BIND_UNORDERED_ACCESS) || (BuffDesc.BindFlags & BIND_SHADER_RESOURCE))
    {
        if (BuffDesc.Mode == BUFFER_MODE_STRUCTURED || BuffDesc.Mode == BUFFER_MODE_FORMATTED)
        {
            VERIFY(BuffDesc.ElementByteStride != 0, "Element byte stride cannot be 0 for a structured or a formatted buffer");
        }
        else if (BuffDesc.Mode == BUFFER_MODE_RAW)
        {
        }
        else
        {
            UNEXPECTED("Buffer mode must be structured or formatted");
        }
    }

    return BuffDesc;
}

BufferD3D12Impl::BufferD3D12Impl(IReferenceCounters*        pRefCounters,
                                 FixedBlockMemoryAllocator& BuffViewObjMemAllocator,
                                 RenderDeviceD3D12Impl*     pRenderDeviceD3D12,
                                 const BufferDesc&          BuffDesc,
                                 RESOURCE_STATE             InitialState,
                                 ID3D12Resource*            pd3d12Buffer) :
    // clang-format off
    TBufferBase
    {
        pRefCounters,
        BuffViewObjMemAllocator,
        pRenderDeviceD3D12,
        BufferDescFromD3D12Resource(BuffDesc, pd3d12Buffer),
        false
    },
    m_DynamicData
    (
        BuffDesc.Usage == USAGE_DYNAMIC ? (1 + pRenderDeviceD3D12->GetNumDeferredContexts()) : 0,
        D3D12DynamicAllocation{},
        STD_ALLOCATOR_RAW_MEM(D3D12DynamicAllocation, GetRawAllocator(), "Allocator for vector<DynamicAllocation>")
    )
// clang-format on
{
    m_pd3d12Resource = pd3d12Buffer;
    SetState(InitialState);

    if (m_Desc.BindFlags & BIND_UNIFORM_BUFFER)
    {
        m_CBVDescriptorAllocation = pRenderDeviceD3D12->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        CreateCBV(m_CBVDescriptorAllocation.GetCpuHandle());
    }
}
BufferD3D12Impl::~BufferD3D12Impl()
{
    // D3D12 object can only be destroyed when it is no longer used by the GPU
    auto* pDeviceD3D12Impl = ValidatedCast<RenderDeviceD3D12Impl>(GetDevice());
    pDeviceD3D12Impl->SafeReleaseDeviceObject(std::move(m_pd3d12Resource), m_Desc.CommandQueueMask);
}

IMPLEMENT_QUERY_INTERFACE(BufferD3D12Impl, IID_BufferD3D12, TBufferBase)


void BufferD3D12Impl::CreateViewInternal(const BufferViewDesc& OrigViewDesc, IBufferView** ppView, bool bIsDefaultView)
{
    VERIFY(ppView != nullptr, "Null pointer provided");
    if (!ppView) return;
    VERIFY(*ppView == nullptr, "Overwriting reference to existing object may cause memory leaks");

    *ppView = nullptr;

    try
    {
        auto* pDeviceD3D12Impl  = ValidatedCast<RenderDeviceD3D12Impl>(GetDevice());
        auto& BuffViewAllocator = pDeviceD3D12Impl->GetBuffViewObjAllocator();
        VERIFY(&BuffViewAllocator == &m_dbgBuffViewAllocator, "Buff view allocator does not match allocator provided at buffer initialization");

        BufferViewDesc ViewDesc = OrigViewDesc;
        if (ViewDesc.ViewType == BUFFER_VIEW_UNORDERED_ACCESS)
        {
            auto UAVHandleAlloc = pDeviceD3D12Impl->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            CreateUAV(ViewDesc, UAVHandleAlloc.GetCpuHandle());
            *ppView = NEW_RC_OBJ(BuffViewAllocator, "BufferViewD3D12Impl instance", BufferViewD3D12Impl, bIsDefaultView ? this : nullptr)(GetDevice(), ViewDesc, this, std::move(UAVHandleAlloc), bIsDefaultView);
        }
        else if (ViewDesc.ViewType == BUFFER_VIEW_SHADER_RESOURCE)
        {
            auto SRVHandleAlloc = pDeviceD3D12Impl->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            CreateSRV(ViewDesc, SRVHandleAlloc.GetCpuHandle());
            *ppView = NEW_RC_OBJ(BuffViewAllocator, "BufferViewD3D12Impl instance", BufferViewD3D12Impl, bIsDefaultView ? this : nullptr)(GetDevice(), ViewDesc, this, std::move(SRVHandleAlloc), bIsDefaultView);
        }

        if (!bIsDefaultView && *ppView)
            (*ppView)->AddRef();
    }
    catch (const std::runtime_error&)
    {
        const auto* ViewTypeName = GetBufferViewTypeLiteralName(OrigViewDesc.ViewType);
        LOG_ERROR("Failed to create view \"", OrigViewDesc.Name ? OrigViewDesc.Name : "", "\" (", ViewTypeName, ") for buffer \"", m_Desc.Name, "\"");
    }
}

void BufferD3D12Impl::CreateUAV(BufferViewDesc& UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE UAVDescriptor)
{
    CorrectBufferViewDesc(UAVDesc);

    D3D12_UNORDERED_ACCESS_VIEW_DESC D3D12_UAVDesc;
    BufferViewDesc_to_D3D12_UAV_DESC(m_Desc, UAVDesc, D3D12_UAVDesc);

    auto* pDeviceD3D12 = static_cast<RenderDeviceD3D12Impl*>(GetDevice())->GetD3D12Device();
    pDeviceD3D12->CreateUnorderedAccessView(m_pd3d12Resource, nullptr, &D3D12_UAVDesc, UAVDescriptor);
}

void BufferD3D12Impl::CreateSRV(struct BufferViewDesc& SRVDesc, D3D12_CPU_DESCRIPTOR_HANDLE SRVDescriptor)
{
    CorrectBufferViewDesc(SRVDesc);

    D3D12_SHADER_RESOURCE_VIEW_DESC D3D12_SRVDesc;
    BufferViewDesc_to_D3D12_SRV_DESC(m_Desc, SRVDesc, D3D12_SRVDesc);

    auto* pDeviceD3D12 = static_cast<RenderDeviceD3D12Impl*>(GetDevice())->GetD3D12Device();
    pDeviceD3D12->CreateShaderResourceView(m_pd3d12Resource, &D3D12_SRVDesc, SRVDescriptor);
}

void BufferD3D12Impl::CreateCBV(D3D12_CPU_DESCRIPTOR_HANDLE CBVDescriptor)
{
    D3D12_CONSTANT_BUFFER_VIEW_DESC D3D12_CBVDesc;
    D3D12_CBVDesc.BufferLocation = m_pd3d12Resource->GetGPUVirtualAddress();
    D3D12_CBVDesc.SizeInBytes    = m_Desc.uiSizeInBytes;

    auto* pDeviceD3D12 = static_cast<RenderDeviceD3D12Impl*>(GetDevice())->GetD3D12Device();
    pDeviceD3D12->CreateConstantBufferView(&D3D12_CBVDesc, CBVDescriptor);
}

ID3D12Resource* BufferD3D12Impl::GetD3D12Buffer(Uint64& DataStartByteOffset, IDeviceContext* pContext)
{
    auto* pd3d12Resource = GetD3D12Resource();
    if (pd3d12Resource != nullptr)
    {
        VERIFY(m_Desc.Usage != USAGE_DYNAMIC || (m_Desc.BindFlags | (BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS)) != 0, "Expected non-dynamic buffer or a buffer with SRV or UAV bind flags");
        DataStartByteOffset = 0;
        return pd3d12Resource;
    }
    else
    {
        VERIFY(m_Desc.Usage == USAGE_DYNAMIC, "Dynamic buffer is expected");
        auto* pCtxD3D12 = ValidatedCast<DeviceContextD3D12Impl>(pContext);
#ifdef DILIGENT_DEVELOPMENT
        DvpVerifyDynamicAllocation(pCtxD3D12);
#endif
        auto ContextId      = pCtxD3D12->GetContextId();
        DataStartByteOffset = m_DynamicData[ContextId].Offset;
        return m_DynamicData[ContextId].pBuffer;
    }
}

#ifdef DILIGENT_DEVELOPMENT
void BufferD3D12Impl::DvpVerifyDynamicAllocation(DeviceContextD3D12Impl* pCtx) const
{
    auto ContextId    = pCtx->GetContextId();
    auto CurrentFrame = pCtx->GetCurrentFrameNumber();
    DEV_CHECK_ERR(m_DynamicData[ContextId].GPUAddress != 0, "Dynamic buffer '", m_Desc.Name, "' has not been mapped before its first use. Context Id: ", ContextId, ". Note: memory for dynamic buffers is allocated when a buffer is mapped.");
    DEV_CHECK_ERR(m_DynamicData[ContextId].DvpCtxFrameNumber == static_cast<Uint64>(CurrentFrame), "Dynamic allocation of dynamic buffer '", m_Desc.Name, "' in frame ", CurrentFrame, " is out-of-date. Note: contents of all dynamic resources is discarded at the end of every frame. A buffer must be mapped before its first use in any frame.");
    VERIFY(GetState() == RESOURCE_STATE_GENERIC_READ, "Dynamic buffers are expected to always be in RESOURCE_STATE_GENERIC_READ state");
}
#endif

void BufferD3D12Impl::SetD3D12ResourceState(D3D12_RESOURCE_STATES state)
{
    SetState(D3D12ResourceStatesToResourceStateFlags(state));
}

D3D12_RESOURCE_STATES BufferD3D12Impl::GetD3D12ResourceState() const
{
    return ResourceStateFlagsToD3D12ResourceStates(GetState());
}

} // namespace Diligent
