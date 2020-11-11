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
#include "TextureD3D12Impl.hpp"
#include "RenderDeviceD3D12Impl.hpp"
#include "DeviceContextD3D12Impl.hpp"
#include "D3D12TypeConversions.hpp"
#include "TextureViewD3D12Impl.hpp"
#include "DXGITypeConversions.hpp"
#include "d3dx12_win.h"
#include "EngineMemory.h"
#include "StringTools.hpp"

using namespace Diligent;

namespace Diligent
{

DXGI_FORMAT GetClearFormat(DXGI_FORMAT Fmt, D3D12_RESOURCE_FLAGS Flags)
{
    if (Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
    {
        // clang-format off
        switch (Fmt)
        {
            case DXGI_FORMAT_R32_TYPELESS:      return DXGI_FORMAT_D32_FLOAT;
            case DXGI_FORMAT_R16_TYPELESS:      return DXGI_FORMAT_D16_UNORM;
            case DXGI_FORMAT_R24G8_TYPELESS:    return DXGI_FORMAT_D24_UNORM_S8_UINT;
            case DXGI_FORMAT_R32G8X24_TYPELESS: return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
        }
        // clang-format on
    }
    else if (Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
    {
        // clang-format off
        switch (Fmt)
        {
            case DXGI_FORMAT_R32G32B32A32_TYPELESS: return DXGI_FORMAT_R32G32B32A32_FLOAT;
            case DXGI_FORMAT_R32G32B32_TYPELESS:    return DXGI_FORMAT_R32G32B32_FLOAT;  
            case DXGI_FORMAT_R16G16B16A16_TYPELESS: return DXGI_FORMAT_R16G16B16A16_FLOAT;
            case DXGI_FORMAT_R32G32_TYPELESS:       return DXGI_FORMAT_R32G32_FLOAT;     
            case DXGI_FORMAT_R10G10B10A2_TYPELESS:  return DXGI_FORMAT_R10G10B10A2_UNORM;
            case DXGI_FORMAT_R8G8B8A8_TYPELESS:     return DXGI_FORMAT_R8G8B8A8_UNORM;   
            case DXGI_FORMAT_R16G16_TYPELESS:       return DXGI_FORMAT_R16G16_FLOAT;     
            case DXGI_FORMAT_R32_TYPELESS:          return DXGI_FORMAT_R32_FLOAT;        
            case DXGI_FORMAT_R8G8_TYPELESS:         return DXGI_FORMAT_R8G8_UNORM;       
            case DXGI_FORMAT_R16_TYPELESS:          return DXGI_FORMAT_R16_FLOAT;        
            case DXGI_FORMAT_R8_TYPELESS:           return DXGI_FORMAT_R8_UNORM;         
            case DXGI_FORMAT_B8G8R8A8_TYPELESS:     return DXGI_FORMAT_B8G8R8A8_UNORM;   
            case DXGI_FORMAT_B8G8R8X8_TYPELESS:     return DXGI_FORMAT_B8G8R8X8_UNORM;
        }
        // clang-format on
    }
    return Fmt;
}

D3D12_RESOURCE_DESC TextureD3D12Impl::GetD3D12TextureDesc() const
{
    D3D12_RESOURCE_DESC Desc = {};

    Desc.Alignment = 0;
    if (m_Desc.Type == RESOURCE_DIM_TEX_1D_ARRAY || m_Desc.Type == RESOURCE_DIM_TEX_2D_ARRAY || m_Desc.Type == RESOURCE_DIM_TEX_CUBE || m_Desc.Type == RESOURCE_DIM_TEX_CUBE_ARRAY)
        Desc.DepthOrArraySize = (UINT16)m_Desc.ArraySize;
    else if (m_Desc.Type == RESOURCE_DIM_TEX_3D)
        Desc.DepthOrArraySize = (UINT16)m_Desc.Depth;
    else
        Desc.DepthOrArraySize = 1;

    if (m_Desc.Type == RESOURCE_DIM_TEX_1D || m_Desc.Type == RESOURCE_DIM_TEX_1D_ARRAY)
        Desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
    else if (m_Desc.Type == RESOURCE_DIM_TEX_2D || m_Desc.Type == RESOURCE_DIM_TEX_2D_ARRAY || m_Desc.Type == RESOURCE_DIM_TEX_CUBE || m_Desc.Type == RESOURCE_DIM_TEX_CUBE_ARRAY)
        Desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    else if (m_Desc.Type == RESOURCE_DIM_TEX_3D)
        Desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
    else
    {
        LOG_ERROR_AND_THROW("Unknown texture type");
    }

    Desc.Flags = D3D12_RESOURCE_FLAG_NONE;
    if (m_Desc.BindFlags & BIND_RENDER_TARGET)
        Desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    if (m_Desc.BindFlags & BIND_DEPTH_STENCIL)
        Desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    if ((m_Desc.BindFlags & BIND_UNORDERED_ACCESS) || (m_Desc.MiscFlags & MISC_TEXTURE_FLAG_GENERATE_MIPS))
        Desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    if ((m_Desc.BindFlags & BIND_SHADER_RESOURCE) == 0 && (m_Desc.BindFlags & BIND_DEPTH_STENCIL) != 0)
        Desc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

    auto Format = TexFormatToDXGI_Format(m_Desc.Format, m_Desc.BindFlags);
    if (Format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB && (Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS))
        Desc.Format = DXGI_FORMAT_R8G8B8A8_TYPELESS;
    else
        Desc.Format = Format;

    Desc.Height             = UINT{m_Desc.Height};
    Desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    Desc.MipLevels          = static_cast<UINT16>(m_Desc.MipLevels);
    Desc.SampleDesc.Count   = m_Desc.SampleCount;
    Desc.SampleDesc.Quality = 0;
    Desc.Width              = UINT64{m_Desc.Width};

    return Desc;
}

TextureD3D12Impl::TextureD3D12Impl(IReferenceCounters*        pRefCounters,
                                   FixedBlockMemoryAllocator& TexViewObjAllocator,
                                   RenderDeviceD3D12Impl*     pRenderDeviceD3D12,
                                   const TextureDesc&         TexDesc,
                                   const TextureData*         pInitData /*= nullptr*/) :
    TTextureBase{pRefCounters, TexViewObjAllocator, pRenderDeviceD3D12, TexDesc}
{
    if (m_Desc.Usage == USAGE_IMMUTABLE && (pInitData == nullptr || pInitData->pSubResources == nullptr))
        LOG_ERROR_AND_THROW("Immutable textures must be initialized with data at creation time: pInitData can't be null");

    if ((m_Desc.MiscFlags & MISC_TEXTURE_FLAG_GENERATE_MIPS) != 0)
    {
        if (m_Desc.Type != RESOURCE_DIM_TEX_2D && m_Desc.Type != RESOURCE_DIM_TEX_2D_ARRAY)
        {
            LOG_ERROR_AND_THROW("Mipmap generation is currently only supported for 2D textures and 2D texture arrays in d3d12 backend");
        }
    }

    D3D12_RESOURCE_DESC Desc = GetD3D12TextureDesc();

    auto* pd3d12Device = pRenderDeviceD3D12->GetD3D12Device();
    if (m_Desc.Usage == USAGE_IMMUTABLE || m_Desc.Usage == USAGE_DEFAULT || m_Desc.Usage == USAGE_DYNAMIC)
    {
        D3D12_CLEAR_VALUE  ClearValue  = {};
        D3D12_CLEAR_VALUE* pClearValue = nullptr;
        if (Desc.Flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL))
        {
            if (m_Desc.ClearValue.Format != TEX_FORMAT_UNKNOWN)
                ClearValue.Format = TexFormatToDXGI_Format(m_Desc.ClearValue.Format);
            else
            {
                auto Format       = TexFormatToDXGI_Format(m_Desc.Format, m_Desc.BindFlags);
                ClearValue.Format = GetClearFormat(Format, Desc.Flags);
            }

            if (Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
            {
                for (int i = 0; i < 4; ++i)
                    ClearValue.Color[i] = m_Desc.ClearValue.Color[i];
            }
            else if (Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
            {
                ClearValue.DepthStencil.Depth   = m_Desc.ClearValue.DepthStencil.Depth;
                ClearValue.DepthStencil.Stencil = m_Desc.ClearValue.DepthStencil.Stencil;
            }
            pClearValue = &ClearValue;
        }

        D3D12_HEAP_PROPERTIES HeapProps = {};

        HeapProps.Type                 = D3D12_HEAP_TYPE_DEFAULT;
        HeapProps.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        HeapProps.CreationNodeMask     = 1;
        HeapProps.VisibleNodeMask      = 1;

        bool bInitializeTexture = (pInitData != nullptr && pInitData->pSubResources != nullptr && pInitData->NumSubresources > 0);
        auto InitialState       = bInitializeTexture ? RESOURCE_STATE_COPY_DEST : RESOURCE_STATE_UNDEFINED;
        SetState(InitialState);
        auto D3D12State = ResourceStateFlagsToD3D12ResourceStates(InitialState);
        auto hr =
            pd3d12Device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &Desc, D3D12State, pClearValue, __uuidof(m_pd3d12Resource),
                                                  reinterpret_cast<void**>(static_cast<ID3D12Resource**>(&m_pd3d12Resource)));
        if (FAILED(hr))
            LOG_ERROR_AND_THROW("Failed to create D3D12 texture");

        if (*m_Desc.Name != 0)
            m_pd3d12Resource->SetName(WidenString(m_Desc.Name).c_str());

        if (bInitializeTexture)
        {
            Uint32 ExpectedNumSubresources = Uint32{Desc.MipLevels} * (Desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D ? 1 : Uint32{Desc.DepthOrArraySize});
            if (pInitData->NumSubresources != ExpectedNumSubresources)
                LOG_ERROR_AND_THROW("Incorrect number of subresources in init data. ", ExpectedNumSubresources, " expected, while ", pInitData->NumSubresources, " provided");

            UINT64 uploadBufferSize = 0;
            pd3d12Device->GetCopyableFootprints(&Desc, 0, pInitData->NumSubresources, 0, nullptr, nullptr, nullptr, &uploadBufferSize);

            D3D12_HEAP_PROPERTIES UploadHeapProps = {};

            UploadHeapProps.Type                 = D3D12_HEAP_TYPE_UPLOAD;
            UploadHeapProps.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            UploadHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            UploadHeapProps.CreationNodeMask     = 1;
            UploadHeapProps.VisibleNodeMask      = 1;

            D3D12_RESOURCE_DESC BufferDesc = {};

            BufferDesc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
            BufferDesc.Alignment          = 0;
            BufferDesc.Width              = uploadBufferSize;
            BufferDesc.Height             = 1;
            BufferDesc.DepthOrArraySize   = 1;
            BufferDesc.MipLevels          = 1;
            BufferDesc.Format             = DXGI_FORMAT_UNKNOWN;
            BufferDesc.SampleDesc.Count   = 1;
            BufferDesc.SampleDesc.Quality = 0;
            BufferDesc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            BufferDesc.Flags              = D3D12_RESOURCE_FLAG_NONE;

            CComPtr<ID3D12Resource> UploadBuffer;
            hr = pd3d12Device->CreateCommittedResource(&UploadHeapProps, D3D12_HEAP_FLAG_NONE,
                                                       &BufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
                                                       nullptr, __uuidof(UploadBuffer), reinterpret_cast<void**>(static_cast<ID3D12Resource**>(&UploadBuffer)));
            if (FAILED(hr))
                LOG_ERROR_AND_THROW("Failed to create committed resource in an upload heap");

            auto InitContext = pRenderDeviceD3D12->AllocateCommandContext();
            // copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default texture
            VERIFY_EXPR(CheckState(RESOURCE_STATE_COPY_DEST));
            std::vector<D3D12_SUBRESOURCE_DATA, STDAllocatorRawMem<D3D12_SUBRESOURCE_DATA>> D3D12SubResData(pInitData->NumSubresources, D3D12_SUBRESOURCE_DATA(), STD_ALLOCATOR_RAW_MEM(D3D12_SUBRESOURCE_DATA, GetRawAllocator(), "Allocator for vector<D3D12_SUBRESOURCE_DATA>"));
            for (size_t subres = 0; subres < D3D12SubResData.size(); ++subres)
            {
                D3D12SubResData[subres].pData      = pInitData->pSubResources[subres].pData;
                D3D12SubResData[subres].RowPitch   = pInitData->pSubResources[subres].Stride;
                D3D12SubResData[subres].SlicePitch = pInitData->pSubResources[subres].DepthStride;
            }
            auto UploadedSize = UpdateSubresources(InitContext->GetCommandList(), m_pd3d12Resource, UploadBuffer, 0, 0, pInitData->NumSubresources, D3D12SubResData.data());
            VERIFY(UploadedSize == uploadBufferSize, "Incorrect uploaded data size (", UploadedSize, "). ", uploadBufferSize, " is expected");

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

            // We MUST NOT call TransitionResource() from here, because
            // it will call AddRef() and potentially Release(), while
            // the object is not constructed yet
            // Add reference to the object to the release queue to keep it alive
            // until copy operation is complete.  This must be done after
            // submitting command list for execution!
            pRenderDeviceD3D12->SafeReleaseDeviceObject(std::move(UploadBuffer), Uint64{1} << QueueIndex);
        }
    }
    else if (m_Desc.Usage == USAGE_STAGING)
    {
        // Create staging buffer
        D3D12_HEAP_PROPERTIES StaginHeapProps = {};
        DEV_CHECK_ERR((m_Desc.CPUAccessFlags & (CPU_ACCESS_READ | CPU_ACCESS_WRITE)) == CPU_ACCESS_READ ||
                          (m_Desc.CPUAccessFlags & (CPU_ACCESS_READ | CPU_ACCESS_WRITE)) == CPU_ACCESS_WRITE,
                      "Exactly one of CPU_ACCESS_READ or CPU_ACCESS_WRITE flags must be specified");

        RESOURCE_STATE InitialState = RESOURCE_STATE_UNKNOWN;
        if (m_Desc.CPUAccessFlags & CPU_ACCESS_READ)
        {
            StaginHeapProps.Type = D3D12_HEAP_TYPE_READBACK;
            InitialState         = RESOURCE_STATE_COPY_DEST;
        }
        else if (m_Desc.CPUAccessFlags & CPU_ACCESS_WRITE)
        {
            StaginHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
            InitialState         = RESOURCE_STATE_GENERIC_READ;
        }
        else
            UNEXPECTED("Unexpected CPU access");

        SetState(InitialState);
        auto D3D12State = ResourceStateFlagsToD3D12ResourceStates(InitialState);

        StaginHeapProps.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        StaginHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        StaginHeapProps.CreationNodeMask     = 1;
        StaginHeapProps.VisibleNodeMask      = 1;

        UINT64 stagingBufferSize = 0;
        Uint32 NumSubresources   = Uint32{Desc.MipLevels} * (Desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D ? 1 : Uint32{Desc.DepthOrArraySize});
        m_StagingFootprints      = ALLOCATE(GetRawAllocator(), "Memory for staging footprints", D3D12_PLACED_SUBRESOURCE_FOOTPRINT, NumSubresources + 1);
        pd3d12Device->GetCopyableFootprints(&Desc, 0, NumSubresources, 0, m_StagingFootprints, nullptr, nullptr, &stagingBufferSize);
        m_StagingFootprints[NumSubresources] = D3D12_PLACED_SUBRESOURCE_FOOTPRINT{stagingBufferSize};

        D3D12_RESOURCE_DESC BufferDesc = {};

        BufferDesc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        BufferDesc.Alignment          = 0;
        BufferDesc.Width              = stagingBufferSize;
        BufferDesc.Height             = 1;
        BufferDesc.DepthOrArraySize   = 1;
        BufferDesc.MipLevels          = 1;
        BufferDesc.Format             = DXGI_FORMAT_UNKNOWN;
        BufferDesc.SampleDesc.Count   = 1;
        BufferDesc.SampleDesc.Quality = 0;
        BufferDesc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        BufferDesc.Flags              = D3D12_RESOURCE_FLAG_NONE;

        // Resources on D3D12_HEAP_TYPE_READBACK heaps do not support persistent map. Map() and Unmap() must be
        // called between CPU and GPU accesses to the same memory address on some system architectures, when the
        // page caching behavior is write-back. Map() and Unmap() invalidate and flush the last level CPU cache
        // on some ARM systems, to marshal data between the CPU and GPU through memory addresses with write-back behavior.
        // https://docs.microsoft.com/en-us/windows/desktop/api/d3d12/nf-d3d12-id3d12resource-map
        auto hr = pd3d12Device->CreateCommittedResource(&StaginHeapProps, D3D12_HEAP_FLAG_NONE, &BufferDesc, D3D12State,
                                                        nullptr, __uuidof(m_pd3d12Resource), reinterpret_cast<void**>(static_cast<ID3D12Resource**>(&m_pd3d12Resource)));
        if (FAILED(hr))
            LOG_ERROR_AND_THROW("Failed to create staging buffer");
    }
    else
    {
        UNEXPECTED("Unexpected usage");
    }
}


static TextureDesc InitTexDescFromD3D12Resource(ID3D12Resource* pTexture, const TextureDesc& SrcTexDesc)
{
    auto ResourceDesc = pTexture->GetDesc();

    TextureDesc TexDesc = SrcTexDesc;
    if (TexDesc.Format == TEX_FORMAT_UNKNOWN)
        TexDesc.Format = DXGI_FormatToTexFormat(ResourceDesc.Format);
    else
    {
        auto RefFormat = DXGI_FormatToTexFormat(ResourceDesc.Format);
        DEV_CHECK_ERR(RefFormat == TexDesc.Format, "The format specified by texture description (", GetTextureFormatAttribs(TexDesc.Format).Name, ")"
                                                                                                                                                  " does not match the D3D12 resource format (",
                      GetTextureFormatAttribs(RefFormat).Name, ")");
        (void)RefFormat;
    }

    TexDesc.Width     = static_cast<Uint32>(ResourceDesc.Width);
    TexDesc.Height    = Uint32{ResourceDesc.Height};
    TexDesc.ArraySize = Uint32{ResourceDesc.DepthOrArraySize};
    TexDesc.MipLevels = Uint32{ResourceDesc.MipLevels};
    switch (ResourceDesc.Dimension)
    {
        case D3D12_RESOURCE_DIMENSION_TEXTURE1D: TexDesc.Type = TexDesc.ArraySize == 1 ? RESOURCE_DIM_TEX_1D : RESOURCE_DIM_TEX_1D_ARRAY; break;
        case D3D12_RESOURCE_DIMENSION_TEXTURE2D: TexDesc.Type = TexDesc.ArraySize == 1 ? RESOURCE_DIM_TEX_2D : RESOURCE_DIM_TEX_2D_ARRAY; break;
        case D3D12_RESOURCE_DIMENSION_TEXTURE3D: TexDesc.Type = RESOURCE_DIM_TEX_3D; break;
    }

    TexDesc.SampleCount = ResourceDesc.SampleDesc.Count;

    TexDesc.Usage     = USAGE_DEFAULT;
    TexDesc.BindFlags = BIND_NONE;
    if ((ResourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0)
        TexDesc.BindFlags |= BIND_RENDER_TARGET;
    if ((ResourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0)
        TexDesc.BindFlags |= BIND_DEPTH_STENCIL;
    if ((ResourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0)
        TexDesc.BindFlags |= BIND_UNORDERED_ACCESS;
    if ((ResourceDesc.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0)
    {
        auto FormatAttribs = GetTextureFormatAttribs(TexDesc.Format);
        if (FormatAttribs.IsTypeless ||
            (FormatAttribs.ComponentType != COMPONENT_TYPE_DEPTH &&
             FormatAttribs.ComponentType != COMPONENT_TYPE_DEPTH_STENCIL))
        {
            TexDesc.BindFlags |= BIND_SHADER_RESOURCE;
        }
    }

    return TexDesc;
}

TextureD3D12Impl::TextureD3D12Impl(IReferenceCounters*        pRefCounters,
                                   FixedBlockMemoryAllocator& TexViewObjAllocator,
                                   RenderDeviceD3D12Impl*     pDeviceD3D12,
                                   const TextureDesc&         TexDesc,
                                   RESOURCE_STATE             InitialState,
                                   ID3D12Resource*            pTexture) :
    TTextureBase{pRefCounters, TexViewObjAllocator, pDeviceD3D12, InitTexDescFromD3D12Resource(pTexture, TexDesc)}
{
    m_pd3d12Resource = pTexture;
    SetState(InitialState);
}
IMPLEMENT_QUERY_INTERFACE(TextureD3D12Impl, IID_TextureD3D12, TTextureBase)

void TextureD3D12Impl::CreateViewInternal(const struct TextureViewDesc& ViewDesc, ITextureView** ppView, bool bIsDefaultView)
{
    VERIFY(ppView != nullptr, "View pointer address is null");
    if (!ppView) return;
    VERIFY(*ppView == nullptr, "Overwriting reference to existing object may cause memory leaks");

    *ppView = nullptr;

    try
    {
        auto* pDeviceD3D12Impl = ValidatedCast<RenderDeviceD3D12Impl>(GetDevice());
        auto& TexViewAllocator = pDeviceD3D12Impl->GetTexViewObjAllocator();
        VERIFY(&TexViewAllocator == &m_dbgTexViewObjAllocator, "Texture view allocator does not match allocator provided during texture initialization");

        auto UpdatedViewDesc = ViewDesc;
        CorrectTextureViewDesc(UpdatedViewDesc);

        DescriptorHeapAllocation ViewDescriptor;
        switch (ViewDesc.ViewType)
        {
            case TEXTURE_VIEW_SHADER_RESOURCE:
            {
                VERIFY(m_Desc.BindFlags & BIND_SHADER_RESOURCE, "BIND_SHADER_RESOURCE flag is not set");
                ViewDescriptor = pDeviceD3D12Impl->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                CreateSRV(UpdatedViewDesc, ViewDescriptor.GetCpuHandle());
            }
            break;

            case TEXTURE_VIEW_RENDER_TARGET:
            {
                VERIFY(m_Desc.BindFlags & BIND_RENDER_TARGET, "BIND_RENDER_TARGET flag is not set");
                ViewDescriptor = pDeviceD3D12Impl->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
                CreateRTV(UpdatedViewDesc, ViewDescriptor.GetCpuHandle());
            }
            break;

            case TEXTURE_VIEW_DEPTH_STENCIL:
            {
                VERIFY(m_Desc.BindFlags & BIND_DEPTH_STENCIL, "BIND_DEPTH_STENCIL is not set");
                ViewDescriptor = pDeviceD3D12Impl->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
                CreateDSV(UpdatedViewDesc, ViewDescriptor.GetCpuHandle());
            }
            break;

            case TEXTURE_VIEW_UNORDERED_ACCESS:
            {
                VERIFY(m_Desc.BindFlags & BIND_UNORDERED_ACCESS, "BIND_UNORDERED_ACCESS flag is not set");
                ViewDescriptor = pDeviceD3D12Impl->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                CreateUAV(UpdatedViewDesc, ViewDescriptor.GetCpuHandle());
            }
            break;

            default: UNEXPECTED("Unknown view type"); break;
        }

        DescriptorHeapAllocation TexArraySRVDescriptor, MipUAVDescriptors;
        if (UpdatedViewDesc.Flags & TEXTURE_VIEW_FLAG_ALLOW_MIP_MAP_GENERATION)
        {
            VERIFY_EXPR((m_Desc.MiscFlags & MISC_TEXTURE_FLAG_GENERATE_MIPS) != 0 && (m_Desc.Type == RESOURCE_DIM_TEX_2D || m_Desc.Type == RESOURCE_DIM_TEX_2D_ARRAY));

            {
                TexArraySRVDescriptor           = pDeviceD3D12Impl->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);
                TextureViewDesc TexArraySRVDesc = UpdatedViewDesc;
                // Create texture array SRV
                TexArraySRVDesc.TextureDim = RESOURCE_DIM_TEX_2D_ARRAY;
                TexArraySRVDesc.ViewType   = TEXTURE_VIEW_SHADER_RESOURCE;
                CreateSRV(TexArraySRVDesc, TexArraySRVDescriptor.GetCpuHandle());
            }

            MipUAVDescriptors = pDeviceD3D12Impl->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_Desc.MipLevels);
            for (Uint32 MipLevel = 0; MipLevel < m_Desc.MipLevels; ++MipLevel)
            {
                TextureViewDesc UAVDesc = UpdatedViewDesc;
                // Always create texture array UAV
                UAVDesc.TextureDim      = RESOURCE_DIM_TEX_2D_ARRAY;
                UAVDesc.ViewType        = TEXTURE_VIEW_UNORDERED_ACCESS;
                UAVDesc.MostDetailedMip = MipLevel;
                UAVDesc.NumMipLevels    = 1;
                if (UAVDesc.Format == TEX_FORMAT_RGBA8_UNORM_SRGB)
                    UAVDesc.Format = TEX_FORMAT_RGBA8_UNORM;
                else if (UAVDesc.Format == TEX_FORMAT_BGRA8_UNORM_SRGB)
                    UAVDesc.Format = TEX_FORMAT_BGRA8_UNORM;
                CreateUAV(UAVDesc, MipUAVDescriptors.GetCpuHandle(MipLevel));
            }
        }
        auto pViewD3D12 = NEW_RC_OBJ(TexViewAllocator, "TextureViewD3D12Impl instance", TextureViewD3D12Impl, bIsDefaultView ? this : nullptr)(GetDevice(), UpdatedViewDesc, this, std::move(ViewDescriptor), std::move(TexArraySRVDescriptor), std::move(MipUAVDescriptors), bIsDefaultView);
        VERIFY(pViewD3D12->GetDesc().ViewType == ViewDesc.ViewType, "Incorrect view type");

        if (bIsDefaultView)
            *ppView = pViewD3D12;
        else
            pViewD3D12->QueryInterface(IID_TextureView, reinterpret_cast<IObject**>(ppView));
    }
    catch (const std::runtime_error&)
    {
        const auto* ViewTypeName = GetTexViewTypeLiteralName(ViewDesc.ViewType);
        LOG_ERROR("Failed to create view \"", ViewDesc.Name ? ViewDesc.Name : "", "\" (", ViewTypeName, ") for texture \"", m_Desc.Name ? m_Desc.Name : "", "\"");
    }
}

TextureD3D12Impl::~TextureD3D12Impl()
{
    // D3D12 object can only be destroyed when it is no longer used by the GPU
    auto* pDeviceD3D12Impl = ValidatedCast<RenderDeviceD3D12Impl>(GetDevice());
    pDeviceD3D12Impl->SafeReleaseDeviceObject(std::move(m_pd3d12Resource), m_Desc.CommandQueueMask);
    if (m_StagingFootprints != nullptr)
    {
        FREE(GetRawAllocator(), m_StagingFootprints);
    }
}

void TextureD3D12Impl::CreateSRV(TextureViewDesc& SRVDesc, D3D12_CPU_DESCRIPTOR_HANDLE SRVHandle)
{
    VERIFY(SRVDesc.ViewType == TEXTURE_VIEW_SHADER_RESOURCE, "Incorrect view type: shader resource is expected");

    if (SRVDesc.Format == TEX_FORMAT_UNKNOWN)
    {
        SRVDesc.Format = m_Desc.Format;
    }
    D3D12_SHADER_RESOURCE_VIEW_DESC D3D12_SRVDesc;
    TextureViewDesc_to_D3D12_SRV_DESC(SRVDesc, D3D12_SRVDesc, m_Desc.SampleCount);

    auto* pDeviceD3D12 = static_cast<RenderDeviceD3D12Impl*>(GetDevice())->GetD3D12Device();
    pDeviceD3D12->CreateShaderResourceView(m_pd3d12Resource, &D3D12_SRVDesc, SRVHandle);
}

void TextureD3D12Impl::CreateRTV(TextureViewDesc& RTVDesc, D3D12_CPU_DESCRIPTOR_HANDLE RTVHandle)
{
    VERIFY(RTVDesc.ViewType == TEXTURE_VIEW_RENDER_TARGET, "Incorrect view type: render target is expected");

    if (RTVDesc.Format == TEX_FORMAT_UNKNOWN)
    {
        RTVDesc.Format = m_Desc.Format;
    }

    D3D12_RENDER_TARGET_VIEW_DESC D3D12_RTVDesc;
    TextureViewDesc_to_D3D12_RTV_DESC(RTVDesc, D3D12_RTVDesc, m_Desc.SampleCount);

    auto* pDeviceD3D12 = static_cast<RenderDeviceD3D12Impl*>(GetDevice())->GetD3D12Device();
    pDeviceD3D12->CreateRenderTargetView(m_pd3d12Resource, &D3D12_RTVDesc, RTVHandle);
}

void TextureD3D12Impl::CreateDSV(TextureViewDesc& DSVDesc, D3D12_CPU_DESCRIPTOR_HANDLE DSVHandle)
{
    VERIFY(DSVDesc.ViewType == TEXTURE_VIEW_DEPTH_STENCIL, "Incorrect view type: depth stencil is expected");

    if (DSVDesc.Format == TEX_FORMAT_UNKNOWN)
    {
        DSVDesc.Format = m_Desc.Format;
    }

    D3D12_DEPTH_STENCIL_VIEW_DESC D3D12_DSVDesc;
    TextureViewDesc_to_D3D12_DSV_DESC(DSVDesc, D3D12_DSVDesc, m_Desc.SampleCount);

    auto* pDeviceD3D12 = static_cast<RenderDeviceD3D12Impl*>(GetDevice())->GetD3D12Device();
    pDeviceD3D12->CreateDepthStencilView(m_pd3d12Resource, &D3D12_DSVDesc, DSVHandle);
}

void TextureD3D12Impl::CreateUAV(TextureViewDesc& UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE UAVHandle)
{
    VERIFY(UAVDesc.ViewType == TEXTURE_VIEW_UNORDERED_ACCESS, "Incorrect view type: unordered access is expected");

    if (UAVDesc.Format == TEX_FORMAT_UNKNOWN)
    {
        UAVDesc.Format = m_Desc.Format;
    }

    D3D12_UNORDERED_ACCESS_VIEW_DESC D3D12_UAVDesc;
    TextureViewDesc_to_D3D12_UAV_DESC(UAVDesc, D3D12_UAVDesc);

    auto* pDeviceD3D12 = static_cast<RenderDeviceD3D12Impl*>(GetDevice())->GetD3D12Device();
    pDeviceD3D12->CreateUnorderedAccessView(m_pd3d12Resource, nullptr, &D3D12_UAVDesc, UAVHandle);
}

void TextureD3D12Impl::SetD3D12ResourceState(D3D12_RESOURCE_STATES state)
{
    SetState(D3D12ResourceStatesToResourceStateFlags(state));
}

D3D12_RESOURCE_STATES TextureD3D12Impl::GetD3D12ResourceState() const
{
    return ResourceStateFlagsToD3D12ResourceStates(GetState());
}

} // namespace Diligent
