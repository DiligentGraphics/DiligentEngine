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
#include <atlbase.h>

#include "BufferD3D11Impl.hpp"
#include "RenderDeviceD3D11Impl.hpp"
#include "DeviceContextD3D11Impl.hpp"
#include "D3D11TypeConversions.hpp"
#include "BufferViewD3D11Impl.hpp"
#include "GraphicsAccessories.hpp"
#include "EngineMemory.h"

namespace Diligent
{

BufferD3D11Impl::BufferD3D11Impl(IReferenceCounters*        pRefCounters,
                                 FixedBlockMemoryAllocator& BuffViewObjMemAllocator,
                                 RenderDeviceD3D11Impl*     pRenderDeviceD3D11,
                                 const BufferDesc&          BuffDesc,
                                 const BufferData*          pBuffData /*= nullptr*/) :
    // clang-format off
    TBufferBase
    {
        pRefCounters,
        BuffViewObjMemAllocator,
        pRenderDeviceD3D11,
        BuffDesc,
        false
    }
// clang-format on
{
    ValidateBufferInitData(BuffDesc, pBuffData);

    if (m_Desc.Usage == USAGE_UNIFIED)
    {
        LOG_ERROR_AND_THROW("Unified resources are not supported in Direct3D11");
    }

    if (m_Desc.Usage == USAGE_IMMUTABLE)
        VERIFY(pBuffData != nullptr && pBuffData->pData != nullptr, "Initial data must not be null for immutable buffers");

    if (m_Desc.BindFlags & BIND_UNIFORM_BUFFER)
    {
        Uint32 AlignmentMask = 15;
        m_Desc.uiSizeInBytes = (m_Desc.uiSizeInBytes + AlignmentMask) & (~AlignmentMask);
    }

    D3D11_BUFFER_DESC D3D11BuffDesc;
    D3D11BuffDesc.BindFlags = BindFlagsToD3D11BindFlags(m_Desc.BindFlags);
    D3D11BuffDesc.ByteWidth = m_Desc.uiSizeInBytes;
    D3D11BuffDesc.MiscFlags = 0;
    if (m_Desc.BindFlags & BIND_INDIRECT_DRAW_ARGS)
    {
        D3D11BuffDesc.MiscFlags |= D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
    }
    D3D11BuffDesc.Usage = UsageToD3D11Usage(m_Desc.Usage);

    // Size of each element in the buffer structure (in bytes) when the buffer represents a structured buffer, or
    // the size of the format that is used for views of the buffer
    D3D11BuffDesc.StructureByteStride = m_Desc.ElementByteStride;
    if ((m_Desc.BindFlags & BIND_UNORDERED_ACCESS) || (m_Desc.BindFlags & BIND_SHADER_RESOURCE))
    {
        if (m_Desc.Mode == BUFFER_MODE_STRUCTURED)
        {
            D3D11BuffDesc.MiscFlags |= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
            VERIFY(D3D11BuffDesc.StructureByteStride != 0, "StructureByteStride cannot be zero for a structured buffer");
        }
        else if (m_Desc.Mode == BUFFER_MODE_FORMATTED)
        {
            VERIFY(D3D11BuffDesc.StructureByteStride != 0, "StructureByteStride cannot not be zero for a formatted buffer");
        }
        else if (m_Desc.Mode == BUFFER_MODE_RAW)
        {
            D3D11BuffDesc.MiscFlags |= D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
        }
        else
        {
            UNEXPECTED("Unexpected buffer mode");
        }
    }

    D3D11BuffDesc.CPUAccessFlags = CPUAccessFlagsToD3D11CPUAccessFlags(m_Desc.CPUAccessFlags);

    D3D11_SUBRESOURCE_DATA InitData;
    InitData.pSysMem          = pBuffData ? pBuffData->pData : nullptr;
    InitData.SysMemPitch      = pBuffData ? pBuffData->DataSize : 0;
    InitData.SysMemSlicePitch = 0;

    auto* pDeviceD3D11 = pRenderDeviceD3D11->GetD3D11Device();
    CHECK_D3D_RESULT_THROW(pDeviceD3D11->CreateBuffer(&D3D11BuffDesc, InitData.pSysMem ? &InitData : nullptr, &m_pd3d11Buffer),
                           "Failed to create the Direct3D11 buffer");

    if (*m_Desc.Name != 0)
    {
        auto hr = m_pd3d11Buffer->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(m_Desc.Name)), m_Desc.Name);
        DEV_CHECK_ERR(SUCCEEDED(hr), "Failed to set buffer name");
    }

    SetState(RESOURCE_STATE_UNDEFINED);
}

static BufferDesc BuffDescFromD3D11Buffer(ID3D11Buffer* pd3d11Buffer, BufferDesc BuffDesc)
{
    D3D11_BUFFER_DESC D3D11BuffDesc;
    pd3d11Buffer->GetDesc(&D3D11BuffDesc);

    VERIFY(BuffDesc.uiSizeInBytes == 0 || BuffDesc.uiSizeInBytes == D3D11BuffDesc.ByteWidth,
           "Buffer size specified by the BufferDesc (", BuffDesc.uiSizeInBytes,
           ") does not match d3d11 buffer size (", D3D11BuffDesc.ByteWidth, ")");
    BuffDesc.uiSizeInBytes = Uint32{D3D11BuffDesc.ByteWidth};

    auto BindFlags = D3D11BindFlagsToBindFlags(D3D11BuffDesc.BindFlags);
    if (D3D11BuffDesc.MiscFlags & D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS)
        BindFlags |= BIND_INDIRECT_DRAW_ARGS;
    VERIFY(BuffDesc.BindFlags == 0 || BuffDesc.BindFlags == BindFlags,
           "Bind flags specified by the BufferDesc (", BuffDesc.BindFlags,
           ") do not match bind flags recovered from d3d11 buffer desc (", BindFlags, ")");
    BuffDesc.BindFlags = BindFlags;

    auto Usage = D3D11UsageToUsage(D3D11BuffDesc.Usage);
    VERIFY(BuffDesc.Usage == 0 || BuffDesc.Usage == Usage,
           "Usage specified by the BufferDesc (", BuffDesc.Usage,
           ") do not match buffer usage recovered from d3d11 buffer desc (", Usage, ")");
    BuffDesc.Usage = Usage;

    auto CPUAccessFlags = D3D11CPUAccessFlagsToCPUAccessFlags(D3D11BuffDesc.CPUAccessFlags);
    VERIFY(BuffDesc.CPUAccessFlags == 0 || BuffDesc.CPUAccessFlags == CPUAccessFlags,
           "CPU access flags specified by the BufferDesc (", BuffDesc.CPUAccessFlags,
           ") do not match CPU access flags recovered from d3d11 buffer desc (", CPUAccessFlags, ")");
    BuffDesc.CPUAccessFlags = CPUAccessFlags;

    if ((BuffDesc.BindFlags & BIND_UNORDERED_ACCESS) || (BuffDesc.BindFlags & BIND_SHADER_RESOURCE))
    {
        if (D3D11BuffDesc.StructureByteStride != 0)
        {
            VERIFY(BuffDesc.ElementByteStride == 0 || BuffDesc.ElementByteStride == D3D11BuffDesc.StructureByteStride,
                   "Element byte stride specified by the BufferDesc (", BuffDesc.ElementByteStride,
                   ") does not match the structured byte stride recovered from d3d11 buffer desc (", D3D11BuffDesc.StructureByteStride, ")");
            BuffDesc.ElementByteStride = Uint32{D3D11BuffDesc.StructureByteStride};
        }
        if (D3D11BuffDesc.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED)
        {
            VERIFY(BuffDesc.Mode == BUFFER_MODE_UNDEFINED || BuffDesc.Mode == BUFFER_MODE_STRUCTURED,
                   "Inconsistent buffer mode: BUFFER_MODE_STRUCTURED or BUFFER_MODE_UNDEFINED is expected");
            BuffDesc.Mode = BUFFER_MODE_STRUCTURED;
        }
        else if (D3D11BuffDesc.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS)
        {
            VERIFY(BuffDesc.Mode == BUFFER_MODE_UNDEFINED || BuffDesc.Mode == BUFFER_MODE_RAW,
                   "Inconsistent buffer mode: BUFFER_MODE_RAW or BUFFER_MODE_UNDEFINED is expected");
            BuffDesc.Mode = BUFFER_MODE_RAW;
        }
        else
        {
            if (BuffDesc.ElementByteStride != 0)
            {
                VERIFY(BuffDesc.Mode == BUFFER_MODE_UNDEFINED || BuffDesc.Mode == BUFFER_MODE_FORMATTED,
                       "Inconsistent buffer mode: BUFFER_MODE_FORMATTED or BUFFER_MODE_UNDEFINED is expected");
                BuffDesc.Mode = BUFFER_MODE_FORMATTED;
            }
            else
                BuffDesc.Mode = BUFFER_MODE_UNDEFINED;
        }
    }

    return BuffDesc;
}
BufferD3D11Impl::BufferD3D11Impl(IReferenceCounters*          pRefCounters,
                                 FixedBlockMemoryAllocator&   BuffViewObjMemAllocator,
                                 class RenderDeviceD3D11Impl* pDeviceD3D11,
                                 const BufferDesc&            BuffDesc,
                                 RESOURCE_STATE               InitialState,
                                 ID3D11Buffer*                pd3d11Buffer) :
    // clang-format off
    TBufferBase
    {
        pRefCounters,
        BuffViewObjMemAllocator,
        pDeviceD3D11,
        BuffDescFromD3D11Buffer(pd3d11Buffer, BuffDesc),
        false
    }
// clang-format on
{
    m_pd3d11Buffer = pd3d11Buffer;
    SetState(InitialState);
}

BufferD3D11Impl::~BufferD3D11Impl()
{
}

IMPLEMENT_QUERY_INTERFACE(BufferD3D11Impl, IID_BufferD3D11, TBufferBase)


void BufferD3D11Impl::CreateViewInternal(const BufferViewDesc& OrigViewDesc, IBufferView** ppView, bool bIsDefaultView)
{
    VERIFY(ppView != nullptr, "Null pointer provided");
    if (!ppView) return;
    VERIFY(*ppView == nullptr, "Overwriting reference to existing object may cause memory leaks");

    *ppView = nullptr;

    try
    {
        auto* pDeviceD3D11Impl  = ValidatedCast<RenderDeviceD3D11Impl>(GetDevice());
        auto& BuffViewAllocator = pDeviceD3D11Impl->GetBuffViewObjAllocator();
        VERIFY(&BuffViewAllocator == &m_dbgBuffViewAllocator, "Buff view allocator does not match allocator provided at buffer initialization");

        BufferViewDesc ViewDesc = OrigViewDesc;
        if (ViewDesc.ViewType == BUFFER_VIEW_UNORDERED_ACCESS)
        {
            CComPtr<ID3D11UnorderedAccessView> pUAV;
            CreateUAV(ViewDesc, &pUAV);
            *ppView = NEW_RC_OBJ(BuffViewAllocator, "BufferViewD3D11Impl instance", BufferViewD3D11Impl, bIsDefaultView ? this : nullptr)(pDeviceD3D11Impl, ViewDesc, this, pUAV, bIsDefaultView);
        }
        else if (ViewDesc.ViewType == BUFFER_VIEW_SHADER_RESOURCE)
        {
            CComPtr<ID3D11ShaderResourceView> pSRV;
            CreateSRV(ViewDesc, &pSRV);
            *ppView = NEW_RC_OBJ(BuffViewAllocator, "BufferViewD3D11Impl instance", BufferViewD3D11Impl, bIsDefaultView ? this : nullptr)(pDeviceD3D11Impl, ViewDesc, this, pSRV, bIsDefaultView);
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

void BufferD3D11Impl::CreateUAV(BufferViewDesc& UAVDesc, ID3D11UnorderedAccessView** ppD3D11UAV)
{
    CorrectBufferViewDesc(UAVDesc);

    D3D11_UNORDERED_ACCESS_VIEW_DESC D3D11_UAVDesc;
    BufferViewDesc_to_D3D11_UAV_DESC(m_Desc, UAVDesc, D3D11_UAVDesc);

    auto* pDeviceD3D11 = static_cast<RenderDeviceD3D11Impl*>(GetDevice())->GetD3D11Device();
    CHECK_D3D_RESULT_THROW(pDeviceD3D11->CreateUnorderedAccessView(m_pd3d11Buffer, &D3D11_UAVDesc, ppD3D11UAV),
                           "Failed to create D3D11 unordered access view");
}

void BufferD3D11Impl::CreateSRV(struct BufferViewDesc& SRVDesc, ID3D11ShaderResourceView** ppD3D11SRV)
{
    CorrectBufferViewDesc(SRVDesc);

    D3D11_SHADER_RESOURCE_VIEW_DESC D3D11_SRVDesc;
    BufferViewDesc_to_D3D11_SRV_DESC(m_Desc, SRVDesc, D3D11_SRVDesc);

    auto* pDeviceD3D11 = static_cast<RenderDeviceD3D11Impl*>(GetDevice())->GetD3D11Device();
    CHECK_D3D_RESULT_THROW(pDeviceD3D11->CreateShaderResourceView(m_pd3d11Buffer, &D3D11_SRVDesc, ppD3D11SRV),
                           "Failed to create D3D11 shader resource view");
}

} // namespace Diligent
