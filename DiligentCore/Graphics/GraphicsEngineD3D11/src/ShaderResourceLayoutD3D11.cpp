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

#include <d3dcompiler.h>

#include "ShaderResourceLayoutD3D11.hpp"
#include "ShaderResourceCacheD3D11.hpp"
#include "BufferD3D11Impl.hpp"
#include "BufferViewD3D11Impl.hpp"
#include "TextureBaseD3D11.hpp"
#include "TextureViewD3D11.h"
#include "SamplerD3D11Impl.hpp"
#include "ShaderD3D11Impl.hpp"
#include "ShaderResourceVariableBase.hpp"

namespace Diligent
{


ShaderResourceLayoutD3D11::~ShaderResourceLayoutD3D11()
{
    // clang-format off
    HandleResources(
        [&](ConstBuffBindInfo& cb) 
        {
            cb.~ConstBuffBindInfo();
        },

        [&](TexSRVBindInfo& ts)
        {
            ts.~TexSRVBindInfo();
        },

        [&](TexUAVBindInfo& uav)
        {
            uav.~TexUAVBindInfo();
        },

        [&](BuffSRVBindInfo& srv)
        {
            srv.~BuffSRVBindInfo();
        },

        [&](BuffUAVBindInfo& uav)
        {
            uav.~BuffUAVBindInfo();
        },

        [&](SamplerBindInfo& sam)
        {
            sam.~SamplerBindInfo();
        }
    );
    // clang-format on
}


size_t ShaderResourceLayoutD3D11::GetRequiredMemorySize(const ShaderResourcesD3D11&          SrcResources,
                                                        const PipelineResourceLayoutDesc&    ResourceLayout,
                                                        const SHADER_RESOURCE_VARIABLE_TYPE* AllowedVarTypes,
                                                        Uint32                               NumAllowedTypes) noexcept
{
    // Skip immutable samplers as they are initialized directly in the resource cache by the PSO
    constexpr bool CountImtblSamplers = false;
    auto           ResCounters        = SrcResources.CountResources(ResourceLayout, AllowedVarTypes, NumAllowedTypes, CountImtblSamplers);
    // clang-format off
    auto MemSize = ResCounters.NumCBs      * sizeof(ConstBuffBindInfo) +
                   ResCounters.NumTexSRVs  * sizeof(TexSRVBindInfo)    +
                   ResCounters.NumTexUAVs  * sizeof(TexUAVBindInfo)    +
                   ResCounters.NumBufSRVs  * sizeof(BuffSRVBindInfo)   + 
                   ResCounters.NumBufUAVs  * sizeof(BuffUAVBindInfo)   +
                   ResCounters.NumSamplers * sizeof(SamplerBindInfo);
    // clang-format on
    return MemSize;
}


void ShaderResourceLayoutD3D11::Initialize(std::shared_ptr<const ShaderResourcesD3D11> pSrcResources,
                                           const PipelineResourceLayoutDesc&           ResourceLayout,
                                           const SHADER_RESOURCE_VARIABLE_TYPE*        VarTypes,
                                           Uint32                                      NumVarTypes,
                                           IMemoryAllocator&                           ResCacheDataAllocator,
                                           IMemoryAllocator&                           ResLayoutDataAllocator)
{
    m_pResources = std::move(pSrcResources);

    // http://diligentgraphics.com/diligent-engine/architecture/d3d11/shader-resource-layout#Shader-Resource-Layout-Initialization

    const auto AllowedTypeBits = GetAllowedTypeBits(VarTypes, NumVarTypes);

    // Count total number of resources of allowed types
    // Skip immutable samplers as they are initialized directly in the resource cache by the PSO
    constexpr bool CountImtblSamplers = false;
    auto           ResCounters        = m_pResources->CountResources(ResourceLayout, VarTypes, NumVarTypes, CountImtblSamplers);

    // Initialize offsets
    size_t CurrentOffset = 0;

    auto AdvanceOffset = [&CurrentOffset](size_t NumBytes) //
    {
        constexpr size_t MaxOffset = std::numeric_limits<OffsetType>::max();
        VERIFY(CurrentOffset <= MaxOffset, "Current offser (", CurrentOffset, ") exceeds max allowed value (", MaxOffset, ")");
        auto Offset = static_cast<OffsetType>(CurrentOffset);
        CurrentOffset += NumBytes;
        return Offset;
    };

    // clang-format off
    auto CBOffset    = AdvanceOffset(ResCounters.NumCBs      * sizeof(ConstBuffBindInfo));  (void)CBOffset; // To suppress warning
    m_TexSRVsOffset  = AdvanceOffset(ResCounters.NumTexSRVs  * sizeof(TexSRVBindInfo)   );
    m_TexUAVsOffset  = AdvanceOffset(ResCounters.NumTexUAVs  * sizeof(TexUAVBindInfo)   );
    m_BuffSRVsOffset = AdvanceOffset(ResCounters.NumBufSRVs  * sizeof(BuffSRVBindInfo)  );
    m_BuffUAVsOffset = AdvanceOffset(ResCounters.NumBufUAVs  * sizeof(BuffUAVBindInfo)  );
    m_SamplerOffset  = AdvanceOffset(ResCounters.NumSamplers * sizeof(SamplerBindInfo)  );
    m_MemorySize     = AdvanceOffset(0);
    // clang-format on

    VERIFY_EXPR(m_MemorySize == GetRequiredMemorySize(*m_pResources, ResourceLayout, VarTypes, NumVarTypes));

    if (m_MemorySize)
    {
        auto* pRawMem    = ALLOCATE_RAW(ResLayoutDataAllocator, "Raw memory buffer for shader resource layout resources", m_MemorySize);
        m_ResourceBuffer = std::unique_ptr<void, STDDeleterRawMem<void>>(pRawMem, ResLayoutDataAllocator);
    }

    // clang-format off
    VERIFY_EXPR(ResCounters.NumCBs     == GetNumCBs()     );
    VERIFY_EXPR(ResCounters.NumTexSRVs == GetNumTexSRVs() );
    VERIFY_EXPR(ResCounters.NumTexUAVs == GetNumTexUAVs() );
    VERIFY_EXPR(ResCounters.NumBufSRVs == GetNumBufSRVs() );
    VERIFY_EXPR(ResCounters.NumBufUAVs == GetNumBufUAVs() );
    VERIFY_EXPR(ResCounters.NumSamplers== GetNumSamplers());
    // clang-format on

    // Current resource index for every resource type
    Uint32 cb     = 0;
    Uint32 texSrv = 0;
    Uint32 texUav = 0;
    Uint32 bufSrv = 0;
    Uint32 bufUav = 0;
    Uint32 sam    = 0;

    Uint32 NumCBSlots      = 0;
    Uint32 NumSRVSlots     = 0;
    Uint32 NumSamplerSlots = 0;
    Uint32 NumUAVSlots     = 0;
    m_pResources->ProcessResources(
        [&](const D3DShaderResourceAttribs& CB, Uint32) //
        {
            auto VarType = m_pResources->FindVariableType(CB, ResourceLayout);
            if (IsAllowedType(VarType, AllowedTypeBits))
            {
                // Initialize current CB in place, increment CB counter
                new (&GetResource<ConstBuffBindInfo>(cb++)) ConstBuffBindInfo(CB, *this, VarType);
                NumCBSlots = std::max(NumCBSlots, Uint32{CB.BindPoint} + Uint32{CB.BindCount});
            }
        },

        [&](const D3DShaderResourceAttribs& Sampler, Uint32) //
        {
            auto VarType = m_pResources->FindVariableType(Sampler, ResourceLayout);
            if (IsAllowedType(VarType, AllowedTypeBits))
            {
                // Constructor of PipelineStateD3D11Impl initializes immutable samplers and will log the error, if any
                constexpr bool LogImtblSamplerArrayError = false;
                auto           ImtblSamplerInd           = m_pResources->FindImmutableSampler(Sampler, ResourceLayout, LogImtblSamplerArrayError);
                if (ImtblSamplerInd >= 0)
                {
                    // Skip immutble samplers as they are initialized directly in the resource cache by the PSO
                    return;
                }
                // Initialize current sampler in place, increment sampler counter
                new (&GetResource<SamplerBindInfo>(sam++)) SamplerBindInfo(Sampler, *this, VarType);
                NumSamplerSlots = std::max(NumSamplerSlots, Uint32{Sampler.BindPoint} + Uint32{Sampler.BindCount});
            }
        },

        [&](const D3DShaderResourceAttribs& TexSRV, Uint32) //
        {
            auto VarType = m_pResources->FindVariableType(TexSRV, ResourceLayout);
            if (!IsAllowedType(VarType, AllowedTypeBits))
                return;

            auto NumSamplers = GetNumSamplers();
            VERIFY(sam == NumSamplers, "All samplers must be initialized before texture SRVs");

            Uint32 AssignedSamplerIndex = TexSRVBindInfo::InvalidSamplerIndex;
            if (TexSRV.IsCombinedWithSampler())
            {
                const auto& AssignedSamplerAttribs = m_pResources->GetCombinedSampler(TexSRV);
                auto        AssignedSamplerType    = m_pResources->FindVariableType(AssignedSamplerAttribs, ResourceLayout);
                VERIFY(AssignedSamplerType == VarType,
                       "The type (", GetShaderVariableTypeLiteralName(VarType), ") of texture SRV variable '", TexSRV.Name,
                       "' is not consistent with the type (", GetShaderVariableTypeLiteralName(AssignedSamplerType),
                       ") of the sampler '", AssignedSamplerAttribs.Name,
                       "' that is assigned to it. This should never happen as when combined texture samplers are used, "
                       "the type of the sampler is derived from the type of the texture it is assigned to SRV.");

                bool SamplerFound = false;
                for (AssignedSamplerIndex = 0; AssignedSamplerIndex < NumSamplers; ++AssignedSamplerIndex)
                {
                    const auto& Sampler = GetResource<SamplerBindInfo>(AssignedSamplerIndex);
                    SamplerFound        = strcmp(Sampler.m_Attribs.Name, AssignedSamplerAttribs.Name) == 0;
                    if (SamplerFound)
                        break; // Otherwise AssignedSamplerIndex will be incremented
                }

                if (!SamplerFound)
                {
                    AssignedSamplerIndex = TexSRVBindInfo::InvalidSamplerIndex;
#ifdef DILIGENT_DEBUG
                    // Shader error will be logged by the PipelineStateD3D11Impl
                    constexpr bool LogImtblSamplerArrayError = false;
                    if (m_pResources->FindImmutableSampler(AssignedSamplerAttribs, ResourceLayout, LogImtblSamplerArrayError) < 0)
                    {
                        UNEXPECTED("Unable to find non-immutable sampler assigned to texture SRV '", TexSRV.Name, "'.");
                    }
#endif
                }
                else
                {
#ifdef DILIGENT_DEBUG
                    // Shader error will be logged by the PipelineStateD3D11Impl
                    constexpr bool LogImtblSamplerArrayError = false;
                    if (m_pResources->FindImmutableSampler(AssignedSamplerAttribs, ResourceLayout, LogImtblSamplerArrayError) >= 0)
                    {
                        UNEXPECTED("Immutable sampler '", AssignedSamplerAttribs.Name, "' is assigned to texture SRV '", TexSRV.Name, "'.");
                    }
#endif
                }
            }

            // Initialize tex SRV in place, increment counter of tex SRVs
            new (&GetResource<TexSRVBindInfo>(texSrv++)) TexSRVBindInfo(TexSRV, AssignedSamplerIndex, *this, VarType);
            NumSRVSlots = std::max(NumSRVSlots, Uint32{TexSRV.BindPoint} + Uint32{TexSRV.BindCount});
        },

        [&](const D3DShaderResourceAttribs& TexUAV, Uint32) //
        {
            auto VarType = m_pResources->FindVariableType(TexUAV, ResourceLayout);
            if (IsAllowedType(VarType, AllowedTypeBits))
            {
                // Initialize tex UAV in place, increment counter of tex UAVs
                new (&GetResource<TexUAVBindInfo>(texUav++)) TexUAVBindInfo(TexUAV, *this, VarType);
                NumUAVSlots = std::max(NumUAVSlots, Uint32{TexUAV.BindPoint} + Uint32{TexUAV.BindCount});
            }
        },

        [&](const D3DShaderResourceAttribs& BuffSRV, Uint32) //
        {
            auto VarType = m_pResources->FindVariableType(BuffSRV, ResourceLayout);
            if (IsAllowedType(VarType, AllowedTypeBits))
            {
                // Initialize buff SRV in place, increment counter of buff SRVs
                new (&GetResource<BuffSRVBindInfo>(bufSrv++)) BuffSRVBindInfo(BuffSRV, *this, VarType);
                NumSRVSlots = std::max(NumSRVSlots, Uint32{BuffSRV.BindPoint} + Uint32{BuffSRV.BindCount});
            }
        },

        [&](const D3DShaderResourceAttribs& BuffUAV, Uint32) //
        {
            auto VarType = m_pResources->FindVariableType(BuffUAV, ResourceLayout);
            if (IsAllowedType(VarType, AllowedTypeBits))
            {
                // Initialize buff UAV in place, increment counter of buff UAVs
                new (&GetResource<BuffUAVBindInfo>(bufUav++)) BuffUAVBindInfo(BuffUAV, *this, VarType);
                NumUAVSlots = std::max(NumUAVSlots, Uint32{BuffUAV.BindPoint} + Uint32{BuffUAV.BindCount});
            }
        });

    // clang-format off
    VERIFY(cb     == GetNumCBs(),      "Not all CBs are initialized which will cause a crash when dtor is called");
    VERIFY(texSrv == GetNumTexSRVs(),  "Not all Tex SRVs are initialized which will cause a crash when dtor is called");
    VERIFY(texUav == GetNumTexUAVs(),  "Not all Tex UAVs are initialized which will cause a crash when dtor is called");
    VERIFY(bufSrv == GetNumBufSRVs(),  "Not all Buf SRVs are initialized which will cause a crash when dtor is called");
    VERIFY(bufUav == GetNumBufUAVs(),  "Not all Buf UAVs are initialized which will cause a crash when dtor is called");
    VERIFY(sam    == GetNumSamplers(), "Not all samplers are initialized which will cause a crash when dtor is called");
    // clang-format on

    // Shader resource cache in the SRB is initialized by the constructor of ShaderResourceBindingD3D11Impl to
    // hold all variable types. The corresponding layout in the SRB is initialized to keep mutable and dynamic
    // variables only
    // http://diligentgraphics.com/diligent-engine/architecture/d3d11/shader-resource-cache#Shader-Resource-Cache-Initialization
    if (!m_ResourceCache.IsInitialized())
    {
        // NOTE that here we are using max bind points required to cache only the shader variables of allowed types!
        m_ResourceCache.Initialize(NumCBSlots, NumSRVSlots, NumSamplerSlots, NumUAVSlots, ResCacheDataAllocator);
    }
}

void ShaderResourceLayoutD3D11::CopyResources(ShaderResourceCacheD3D11& DstCache) const
{
    // clang-format off
    VERIFY( DstCache.GetCBCount()      >= m_ResourceCache.GetCBCount(),      "Dst cache is not large enough to contain all CBs" );
    VERIFY( DstCache.GetSRVCount()     >= m_ResourceCache.GetSRVCount(),     "Dst cache is not large enough to contain all SRVs" );
    VERIFY( DstCache.GetSamplerCount() >= m_ResourceCache.GetSamplerCount(), "Dst cache is not large enough to contain all samplers" );
    VERIFY( DstCache.GetUAVCount()     >= m_ResourceCache.GetUAVCount(),     "Dst cache is not large enough to contain all UAVs" );
    // clang-format on

    ShaderResourceCacheD3D11::CachedCB*       CachedCBs          = nullptr;
    ID3D11Buffer**                            d3d11CBs           = nullptr;
    ShaderResourceCacheD3D11::CachedResource* CachedSRVResources = nullptr;
    ID3D11ShaderResourceView**                d3d11SRVs          = nullptr;
    ShaderResourceCacheD3D11::CachedSampler*  CachedSamplers     = nullptr;
    ID3D11SamplerState**                      d3d11Samplers      = nullptr;
    ShaderResourceCacheD3D11::CachedResource* CachedUAVResources = nullptr;
    ID3D11UnorderedAccessView**               d3d11UAVs          = nullptr;
    // clang-format off
    m_ResourceCache.GetCBArrays     (CachedCBs,          d3d11CBs);
    m_ResourceCache.GetSRVArrays    (CachedSRVResources, d3d11SRVs);
    m_ResourceCache.GetSamplerArrays(CachedSamplers,     d3d11Samplers);
    m_ResourceCache.GetUAVArrays    (CachedUAVResources, d3d11UAVs);
    // clang-format on


    ShaderResourceCacheD3D11::CachedCB*       DstCBs           = nullptr;
    ID3D11Buffer**                            DstD3D11CBs      = nullptr;
    ShaderResourceCacheD3D11::CachedResource* DstSRVResources  = nullptr;
    ID3D11ShaderResourceView**                DstD3D11SRVs     = nullptr;
    ShaderResourceCacheD3D11::CachedSampler*  DstSamplers      = nullptr;
    ID3D11SamplerState**                      DstD3D11Samplers = nullptr;
    ShaderResourceCacheD3D11::CachedResource* DstUAVResources  = nullptr;
    ID3D11UnorderedAccessView**               DstD3D11UAVs     = nullptr;
    // clang-format off
    DstCache.GetCBArrays     (DstCBs,          DstD3D11CBs);
    DstCache.GetSRVArrays    (DstSRVResources, DstD3D11SRVs);
    DstCache.GetSamplerArrays(DstSamplers,     DstD3D11Samplers);
    DstCache.GetUAVArrays    (DstUAVResources, DstD3D11UAVs);
    // clang-format on

    HandleConstResources(
        [&](const ConstBuffBindInfo& cb) //
        {
            for (auto CBSlot = cb.m_Attribs.BindPoint; CBSlot < cb.m_Attribs.BindPoint + cb.m_Attribs.BindCount; ++CBSlot)
            {
                VERIFY_EXPR(CBSlot < m_ResourceCache.GetCBCount() && CBSlot < DstCache.GetCBCount());
                DstCBs[CBSlot]      = CachedCBs[CBSlot];
                DstD3D11CBs[CBSlot] = d3d11CBs[CBSlot];
            }
        },

        [&](const TexSRVBindInfo& ts) //
        {
            for (auto SRVSlot = ts.m_Attribs.BindPoint; SRVSlot < ts.m_Attribs.BindPoint + ts.m_Attribs.BindCount; ++SRVSlot)
            {
                VERIFY_EXPR(SRVSlot < m_ResourceCache.GetSRVCount() && SRVSlot < DstCache.GetSRVCount());
                DstSRVResources[SRVSlot] = CachedSRVResources[SRVSlot];
                DstD3D11SRVs[SRVSlot]    = d3d11SRVs[SRVSlot];
            }
        },

        [&](const TexUAVBindInfo& uav) //
        {
            for (auto UAVSlot = uav.m_Attribs.BindPoint; UAVSlot < uav.m_Attribs.BindPoint + uav.m_Attribs.BindCount; ++UAVSlot)
            {
                VERIFY_EXPR(UAVSlot < m_ResourceCache.GetUAVCount() && UAVSlot < DstCache.GetUAVCount());
                DstUAVResources[UAVSlot] = CachedUAVResources[UAVSlot];
                DstD3D11UAVs[UAVSlot]    = d3d11UAVs[UAVSlot];
            }
        },

        [&](const BuffSRVBindInfo& srv) //
        {
            for (auto SRVSlot = srv.m_Attribs.BindPoint; SRVSlot < srv.m_Attribs.BindPoint + srv.m_Attribs.BindCount; ++SRVSlot)
            {
                VERIFY_EXPR(SRVSlot < m_ResourceCache.GetSRVCount() && SRVSlot < DstCache.GetSRVCount());
                DstSRVResources[SRVSlot] = CachedSRVResources[SRVSlot];
                DstD3D11SRVs[SRVSlot]    = d3d11SRVs[SRVSlot];
            }
        },

        [&](const BuffUAVBindInfo& uav) //
        {
            for (auto UAVSlot = uav.m_Attribs.BindPoint; UAVSlot < uav.m_Attribs.BindPoint + uav.m_Attribs.BindCount; ++UAVSlot)
            {
                VERIFY_EXPR(UAVSlot < m_ResourceCache.GetUAVCount() && UAVSlot < DstCache.GetUAVCount());
                DstUAVResources[UAVSlot] = CachedUAVResources[UAVSlot];
                DstD3D11UAVs[UAVSlot]    = d3d11UAVs[UAVSlot];
            }
        },

        [&](const SamplerBindInfo& sam) //
        {
            //VERIFY(!sam.IsImmutableSampler, "Variables are not created for immutable samplers");
            for (auto SamSlot = sam.m_Attribs.BindPoint; SamSlot < sam.m_Attribs.BindPoint + sam.m_Attribs.BindCount; ++SamSlot)
            {
                VERIFY_EXPR(SamSlot < m_ResourceCache.GetSamplerCount() && SamSlot < DstCache.GetSamplerCount());
                DstSamplers[SamSlot]      = CachedSamplers[SamSlot];
                DstD3D11Samplers[SamSlot] = d3d11Samplers[SamSlot];
            }
        });
}

void ShaderResourceLayoutD3D11::ConstBuffBindInfo::BindResource(IDeviceObject* pBuffer,
                                                                Uint32         ArrayIndex)
{
    DEV_CHECK_ERR(ArrayIndex < m_Attribs.BindCount, "Array index (", ArrayIndex, ") is out of range for variable '", m_Attribs.Name, "'. Max allowed index: ", m_Attribs.BindCount - 1);

    // We cannot use ValidatedCast<> here as the resource retrieved from the
    // resource mapping can be of wrong type
    RefCntAutoPtr<BufferD3D11Impl> pBuffD3D11Impl{pBuffer, IID_BufferD3D11};
#ifdef DILIGENT_DEVELOPMENT
    {
        auto& CachedCB = m_ParentResLayout.m_ResourceCache.GetCB(m_Attribs.BindPoint + ArrayIndex);
        VerifyConstantBufferBinding(m_Attribs, GetType(), ArrayIndex, pBuffer, pBuffD3D11Impl.RawPtr(), CachedCB.pBuff.RawPtr(), m_ParentResLayout.GetShaderName());
    }
#endif
    m_ParentResLayout.m_ResourceCache.SetCB(m_Attribs.BindPoint + ArrayIndex, std::move(pBuffD3D11Impl));
}


void ShaderResourceLayoutD3D11::TexSRVBindInfo::BindResource(IDeviceObject* pView,
                                                             Uint32         ArrayIndex)
{
    DEV_CHECK_ERR(ArrayIndex < m_Attribs.BindCount, "Array index (", ArrayIndex, ") is out of range for variable '", m_Attribs.Name, "'. Max allowed index: ", m_Attribs.BindCount - 1);
    auto& ResourceCache = m_ParentResLayout.m_ResourceCache;

    // We cannot use ValidatedCast<> here as the resource retrieved from the
    // resource mapping can be of wrong type
    RefCntAutoPtr<TextureViewD3D11Impl> pViewD3D11{pView, IID_TextureViewD3D11};
#ifdef DILIGENT_DEVELOPMENT
    {
        auto& CachedSRV = ResourceCache.GetSRV(m_Attribs.BindPoint + ArrayIndex);
        VerifyResourceViewBinding(m_Attribs, GetType(), ArrayIndex, pView, pViewD3D11.RawPtr(), {TEXTURE_VIEW_SHADER_RESOURCE},
                                  CachedSRV.pView.RawPtr(), m_ParentResLayout.GetShaderName());
    }
#endif

    if (ValidSamplerAssigned())
    {
        auto& Sampler = m_ParentResLayout.GetResource<SamplerBindInfo>(SamplerIndex);
        //VERIFY(!Sampler.IsImmutableSampler, "Immutable samplers are not assigned to texture SRVs as they are initialized directly in the shader resource cache");
        VERIFY_EXPR(Sampler.m_Attribs.BindCount == m_Attribs.BindCount || Sampler.m_Attribs.BindCount == 1);
        auto SamplerBindPoint = Sampler.m_Attribs.BindPoint + (Sampler.m_Attribs.BindCount != 1 ? ArrayIndex : 0);

        SamplerD3D11Impl* pSamplerD3D11Impl = nullptr;
        if (pViewD3D11)
        {
            pSamplerD3D11Impl = ValidatedCast<SamplerD3D11Impl>(pViewD3D11->GetSampler());
#ifdef DILIGENT_DEVELOPMENT
            if (pSamplerD3D11Impl == nullptr)
            {
                if (Sampler.m_Attribs.BindCount > 1)
                    LOG_ERROR_MESSAGE("Failed to bind sampler to variable '", Sampler.m_Attribs.Name, "[", ArrayIndex, "]'. Sampler is not set in the texture view '", pViewD3D11->GetDesc().Name, "'");
                else
                    LOG_ERROR_MESSAGE("Failed to bind sampler to variable '", Sampler.m_Attribs.Name, "'. Sampler is not set in the texture view '", pViewD3D11->GetDesc().Name, "'");
            }
#endif
        }
#ifdef DILIGENT_DEVELOPMENT
        if (Sampler.GetType() != SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC)
        {
            auto& CachedSampler = ResourceCache.GetSampler(SamplerBindPoint);
            if (CachedSampler.pSampler != nullptr && CachedSampler.pSampler != pSamplerD3D11Impl)
            {
                auto VarTypeStr = GetShaderVariableTypeLiteralName(GetType());
                LOG_ERROR_MESSAGE("Non-null sampler is already bound to ", VarTypeStr, " shader variable '", Sampler.m_Attribs.GetPrintName(ArrayIndex), "' in shader '", m_ParentResLayout.GetShaderName(), "'. Attempting to bind another sampler or null is an error and may cause unpredicted behavior. Use another shader resource binding instance or label the variable as dynamic.");
            }
        }
#endif
        ResourceCache.SetSampler(SamplerBindPoint, pSamplerD3D11Impl);
    }

    ResourceCache.SetTexSRV(m_Attribs.BindPoint + ArrayIndex, std::move(pViewD3D11));
}

void ShaderResourceLayoutD3D11::SamplerBindInfo::BindResource(IDeviceObject* pSampler,
                                                              Uint32         ArrayIndex)
{
    DEV_CHECK_ERR(ArrayIndex < m_Attribs.BindCount,
                  "Array index (", ArrayIndex, ") is out of range for variable '", m_Attribs.Name,
                  "'. Max allowed index: ", m_Attribs.BindCount - 1);
    auto& ResourceCache = m_ParentResLayout.m_ResourceCache;
    //VERIFY(!IsImmutableSampler, "Cannot bind sampler to an immutable sampler");

    // We cannot use ValidatedCast<> here as the resource retrieved from the
    // resource mapping can be of wrong type
    RefCntAutoPtr<SamplerD3D11Impl> pSamplerD3D11{pSampler, IID_SamplerD3D11};

#ifdef DILIGENT_DEVELOPMENT
    if (pSampler && !pSamplerD3D11)
    {
        LOG_ERROR_MESSAGE("Failed to bind object '", pSampler->GetDesc().Name, "' to variable '", m_Attribs.GetPrintName(ArrayIndex),
                          "' in shader '", m_ParentResLayout.GetShaderName(), "'. Incorect object type: sampler is expected.");
    }

    if (m_Attribs.IsCombinedWithTexSRV())
    {
        auto* TexSRVName = m_ParentResLayout.m_pResources->GetCombinedTextureSRV(m_Attribs).Name;
        LOG_WARNING_MESSAGE("Texture sampler sampler '", m_Attribs.Name, "' is assigned to texture SRV '",
                            TexSRVName, "' and should not be accessed directly. The sampler is initialized when texture SRV is set to '",
                            TexSRVName, "' variable.");
    }

    if (GetType() != SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC)
    {
        auto& CachedSampler = ResourceCache.GetSampler(m_Attribs.BindPoint + ArrayIndex);
        if (CachedSampler.pSampler != nullptr && CachedSampler.pSampler != pSamplerD3D11)
        {
            auto VarTypeStr = GetShaderVariableTypeLiteralName(GetType());
            LOG_ERROR_MESSAGE("Non-null sampler is already bound to ", VarTypeStr, " shader variable '",
                              m_Attribs.GetPrintName(ArrayIndex), "' in shader '", m_ParentResLayout.GetShaderName(),
                              "'. Attempting to bind another sampler or null is an error and may cause unpredicted behavior. "
                              "Use another shader resource binding instance or label the variable as dynamic.");
        }
    }
#endif

    ResourceCache.SetSampler(m_Attribs.BindPoint + ArrayIndex, std::move(pSamplerD3D11));
}

void ShaderResourceLayoutD3D11::BuffSRVBindInfo::BindResource(IDeviceObject* pView,
                                                              Uint32         ArrayIndex)
{
    DEV_CHECK_ERR(ArrayIndex < m_Attribs.BindCount, "Array index (", ArrayIndex, ") is out of range for variable '",
                  m_Attribs.Name, "'. Max allowed index: ", m_Attribs.BindCount - 1);
    auto& ResourceCache = m_ParentResLayout.m_ResourceCache;

    // We cannot use ValidatedCast<> here as the resource retrieved from the
    // resource mapping can be of wrong type
    RefCntAutoPtr<BufferViewD3D11Impl> pViewD3D11{pView, IID_BufferViewD3D11};
#ifdef DILIGENT_DEVELOPMENT
    {
        auto& CachedSRV = ResourceCache.GetSRV(m_Attribs.BindPoint + ArrayIndex);
        VerifyResourceViewBinding(m_Attribs, GetType(), ArrayIndex, pView, pViewD3D11.RawPtr(), {BUFFER_VIEW_SHADER_RESOURCE},
                                  CachedSRV.pView.RawPtr(), m_ParentResLayout.GetShaderName());
        VerifyBufferViewModeD3D(pViewD3D11.RawPtr(), m_Attribs, m_ParentResLayout.GetShaderName());
    }
#endif
    ResourceCache.SetBufSRV(m_Attribs.BindPoint + ArrayIndex, std::move(pViewD3D11));
}


void ShaderResourceLayoutD3D11::TexUAVBindInfo::BindResource(IDeviceObject* pView,
                                                             Uint32         ArrayIndex)
{
    DEV_CHECK_ERR(ArrayIndex < m_Attribs.BindCount, "Array index (", ArrayIndex, ") is out of range for variable '",
                  m_Attribs.Name, "'. Max allowed index: ", m_Attribs.BindCount - 1);
    auto& ResourceCache = m_ParentResLayout.m_ResourceCache;

    // We cannot use ValidatedCast<> here as the resource retrieved from the
    // resource mapping can be of wrong type
    RefCntAutoPtr<TextureViewD3D11Impl> pViewD3D11{pView, IID_TextureViewD3D11};
#ifdef DILIGENT_DEVELOPMENT
    {
        auto& CachedUAV = ResourceCache.GetUAV(m_Attribs.BindPoint + ArrayIndex);
        VerifyResourceViewBinding(m_Attribs, GetType(), ArrayIndex, pView, pViewD3D11.RawPtr(), {TEXTURE_VIEW_UNORDERED_ACCESS},
                                  CachedUAV.pView.RawPtr(), m_ParentResLayout.GetShaderName());
    }
#endif
    ResourceCache.SetTexUAV(m_Attribs.BindPoint + ArrayIndex, std::move(pViewD3D11));
}


void ShaderResourceLayoutD3D11::BuffUAVBindInfo::BindResource(IDeviceObject* pView,
                                                              Uint32         ArrayIndex)
{
    DEV_CHECK_ERR(ArrayIndex < m_Attribs.BindCount, "Array index (", ArrayIndex, ") is out of range for variable '",
                  m_Attribs.Name, "'. Max allowed index: ", m_Attribs.BindCount - 1);
    auto& ResourceCache = m_ParentResLayout.m_ResourceCache;

    // We cannot use ValidatedCast<> here as the resource retrieved from the
    // resource mapping can be of wrong type
    RefCntAutoPtr<BufferViewD3D11Impl> pViewD3D11{pView, IID_BufferViewD3D11};
#ifdef DILIGENT_DEVELOPMENT
    {
        auto& CachedUAV = ResourceCache.GetUAV(m_Attribs.BindPoint + ArrayIndex);
        VerifyResourceViewBinding(m_Attribs, GetType(), ArrayIndex, pView, pViewD3D11.RawPtr(), {BUFFER_VIEW_UNORDERED_ACCESS},
                                  CachedUAV.pView.RawPtr(), m_ParentResLayout.GetShaderName());
        VerifyBufferViewModeD3D(pViewD3D11.RawPtr(), m_Attribs, m_ParentResLayout.GetShaderName());
    }
#endif
    ResourceCache.SetBufUAV(m_Attribs.BindPoint + ArrayIndex, std::move(pViewD3D11));
}



// Helper template class that facilitates binding CBs, SRVs, and UAVs
class BindResourceHelper
{
public:
    BindResourceHelper(IResourceMapping& RM, Uint32 Fl) :
        ResourceMapping{RM},
        Flags{Fl}
    {
    }

    template <typename ResourceType>
    void Bind(ResourceType& Res)
    {
        if ((Flags & (1 << Res.GetType())) == 0)
            return;

        for (Uint16 elem = 0; elem < Res.m_Attribs.BindCount; ++elem)
        {
            if ((Flags & BIND_SHADER_RESOURCES_KEEP_EXISTING) && Res.IsBound(elem))
                continue;

            const auto*                  VarName = Res.m_Attribs.Name;
            RefCntAutoPtr<IDeviceObject> pRes;
            ResourceMapping.GetResource(VarName, &pRes, elem);
            if (pRes)
            {
                //  Call non-virtual function
                Res.BindResource(pRes, elem);
            }
            else
            {
                if ((Flags & BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED) && !Res.IsBound(elem))
                {
                    LOG_ERROR_MESSAGE("Unable to bind resource to shader variable '", VarName,
                                      "': resource is not found in the resource mapping");
                }
            }
        }
    }

private:
    IResourceMapping& ResourceMapping;
    const Uint32      Flags;
};

void ShaderResourceLayoutD3D11::BindResources(IResourceMapping*               pResourceMapping,
                                              Uint32                          Flags,
                                              const ShaderResourceCacheD3D11& dbgResourceCache)
{
    VERIFY(&dbgResourceCache == &m_ResourceCache, "Resource cache does not match the cache provided at initialization");

    if (pResourceMapping == nullptr)
    {
        LOG_ERROR_MESSAGE("Failed to bind resources in shader '", GetShaderName(), "': resource mapping is null");
        return;
    }

    if ((Flags & BIND_SHADER_RESOURCES_UPDATE_ALL) == 0)
        Flags |= BIND_SHADER_RESOURCES_UPDATE_ALL;

    BindResourceHelper BindResHelper(*pResourceMapping, Flags);

    // clang-format off
    HandleResources(
        [&](ConstBuffBindInfo& cb)
        {
            BindResHelper.Bind(cb);
        },

        [&](TexSRVBindInfo& ts)
        {
            BindResHelper.Bind(ts);
        },

        [&](TexUAVBindInfo& uav)
        {
            BindResHelper.Bind(uav);
        },

        [&](BuffSRVBindInfo& srv)
        {
            BindResHelper.Bind(srv);
        },

        [&](BuffUAVBindInfo& uav)
        {
            BindResHelper.Bind(uav);
        },

        [&](SamplerBindInfo& sam)
        {
            if (!m_pResources->IsUsingCombinedTextureSamplers())
                BindResHelper.Bind(sam);
        }
    );
    // clang-format on
}

template <typename ResourceType>
IShaderResourceVariable* ShaderResourceLayoutD3D11::GetResourceByName(const Char* Name)
{
    auto NumResources = GetNumResources<ResourceType>();
    for (Uint32 res = 0; res < NumResources; ++res)
    {
        auto& Resource = GetResource<ResourceType>(res);
        if (strcmp(Resource.m_Attribs.Name, Name) == 0)
            return &Resource;
    }

    return nullptr;
}

IShaderResourceVariable* ShaderResourceLayoutD3D11::GetShaderVariable(const Char* Name)
{
    if (auto* pCB = GetResourceByName<ConstBuffBindInfo>(Name))
        return pCB;

    if (auto* pTexSRV = GetResourceByName<TexSRVBindInfo>(Name))
        return pTexSRV;

    if (auto* pTexUAV = GetResourceByName<TexUAVBindInfo>(Name))
        return pTexUAV;

    if (auto* pBuffSRV = GetResourceByName<BuffSRVBindInfo>(Name))
        return pBuffSRV;

    if (auto* pBuffUAV = GetResourceByName<BuffUAVBindInfo>(Name))
        return pBuffUAV;

    if (!m_pResources->IsUsingCombinedTextureSamplers())
    {
        // Immutable samplers are never created in the resource layout
        if (auto* pSampler = GetResourceByName<SamplerBindInfo>(Name))
            return pSampler;
    }

    return nullptr;
}

class ShaderVariableIndexLocator
{
public:
    ShaderVariableIndexLocator(const ShaderResourceLayoutD3D11& _Layout, const ShaderResourceLayoutD3D11::ShaderVariableD3D11Base& Variable) :
        // clang-format off
        Layout   {_Layout},
        VarOffset(reinterpret_cast<const Uint8*>(&Variable) - reinterpret_cast<const Uint8*>(_Layout.m_ResourceBuffer.get()))
    // clang-format on
    {}

    template <typename ResourceType>
    bool TryResource(ShaderResourceLayoutD3D11::OffsetType NextResourceTypeOffset)
    {
#ifdef DILIGENT_DEBUG
        {
            VERIFY(Layout.GetResourceOffset<ResourceType>() >= dbgPreviousResourceOffset, "Resource types are processed out of order!");
            dbgPreviousResourceOffset = Layout.GetResourceOffset<ResourceType>();
            VERIFY_EXPR(NextResourceTypeOffset >= Layout.GetResourceOffset<ResourceType>());
        }
#endif
        if (VarOffset < NextResourceTypeOffset)
        {
            auto RelativeOffset = VarOffset - Layout.GetResourceOffset<ResourceType>();
            DEV_CHECK_ERR(RelativeOffset % sizeof(ResourceType) == 0, "Offset is not multiple of resource type (", sizeof(ResourceType), ")");
            Index += static_cast<Uint32>(RelativeOffset / sizeof(ResourceType));
            return true;
        }
        else
        {
            Index += Layout.GetNumResources<ResourceType>();
            return false;
        }
    }

    Uint32 GetIndex() const { return Index; }

private:
    const ShaderResourceLayoutD3D11& Layout;
    const size_t                     VarOffset;
    Uint32                           Index = 0;
#ifdef DILIGENT_DEBUG
    Uint32 dbgPreviousResourceOffset = 0;
#endif
};

Uint32 ShaderResourceLayoutD3D11::GetVariableIndex(const ShaderVariableD3D11Base& Variable) const
{
    if (!m_ResourceBuffer)
    {
        LOG_ERROR("This shader resource layout does not have resources");
        return static_cast<Uint32>(-1);
    }

    ShaderVariableIndexLocator IdxLocator(*this, Variable);
    if (IdxLocator.TryResource<ConstBuffBindInfo>(m_TexSRVsOffset))
        return IdxLocator.GetIndex();

    if (IdxLocator.TryResource<TexSRVBindInfo>(m_TexUAVsOffset))
        return IdxLocator.GetIndex();

    if (IdxLocator.TryResource<TexUAVBindInfo>(m_BuffSRVsOffset))
        return IdxLocator.GetIndex();

    if (IdxLocator.TryResource<BuffSRVBindInfo>(m_BuffUAVsOffset))
        return IdxLocator.GetIndex();

    if (IdxLocator.TryResource<BuffUAVBindInfo>(m_SamplerOffset))
        return IdxLocator.GetIndex();

    if (!m_pResources->IsUsingCombinedTextureSamplers())
    {
        if (IdxLocator.TryResource<SamplerBindInfo>(m_MemorySize))
            return IdxLocator.GetIndex();
    }

    LOG_ERROR("Failed to get variable index. The variable ", &Variable, " does not belong to this shader resource layout");
    return static_cast<Uint32>(-1);
}

class ShaderVariableLocator
{
public:
    ShaderVariableLocator(ShaderResourceLayoutD3D11& _Layout, Uint32 _Index) :
        // clang-format off
        Layout{_Layout},
        Index {_Index }
    // clang-format on
    {
    }

    template <typename ResourceType>
    IShaderResourceVariable* TryResource()
    {
#ifdef DILIGENT_DEBUG
        {
            VERIFY(Layout.GetResourceOffset<ResourceType>() >= dbgPreviousResourceOffset, "Resource types are processed out of order!");
            dbgPreviousResourceOffset = Layout.GetResourceOffset<ResourceType>();
        }
#endif
        auto NumResources = Layout.GetNumResources<ResourceType>();
        if (Index < NumResources)
            return &Layout.GetResource<ResourceType>(Index);
        else
        {
            Index -= NumResources;
            return nullptr;
        }
    }

private:
    ShaderResourceLayoutD3D11& Layout;
    Uint32                     Index = 0;
#ifdef DILIGENT_DEBUG
    Uint32 dbgPreviousResourceOffset = 0;
#endif
};

IShaderResourceVariable* ShaderResourceLayoutD3D11::GetShaderVariable(Uint32 Index)
{
    ShaderVariableLocator VarLocator(*this, Index);

    if (auto* pCB = VarLocator.TryResource<ConstBuffBindInfo>())
        return pCB;

    if (auto* pTexSRV = VarLocator.TryResource<TexSRVBindInfo>())
        return pTexSRV;

    if (auto* pTexUAV = VarLocator.TryResource<TexUAVBindInfo>())
        return pTexUAV;

    if (auto* pBuffSRV = VarLocator.TryResource<BuffSRVBindInfo>())
        return pBuffSRV;

    if (auto* pBuffUAV = VarLocator.TryResource<BuffUAVBindInfo>())
        return pBuffUAV;

    if (!m_pResources->IsUsingCombinedTextureSamplers())
    {
        if (auto* pSampler = VarLocator.TryResource<SamplerBindInfo>())
            return pSampler;
    }

    auto TotalResCount = GetTotalResourceCount();
    LOG_ERROR(Index, " is not a valid variable index. Total resource count: ", TotalResCount);
    return nullptr;
}


#ifdef DILIGENT_DEVELOPMENT
bool ShaderResourceLayoutD3D11::dvpVerifyBindings() const
{

#    define LOG_MISSING_BINDING(VarType, Attrs, BindPt)                                                                                                                   \
        do                                                                                                                                                                \
        {                                                                                                                                                                 \
            if (Attrs.BindCount == 1)                                                                                                                                     \
                LOG_ERROR_MESSAGE("No resource is bound to ", VarType, " variable '", Attrs.Name, "' in shader '", GetShaderName(), "'");                                 \
            else                                                                                                                                                          \
                LOG_ERROR_MESSAGE("No resource is bound to ", VarType, " variable '", Attrs.Name, "[", BindPt - Attrs.BindPoint, "]' in shader '", GetShaderName(), "'"); \
        } while (false)

    m_ResourceCache.dbgVerifyCacheConsistency();

    bool BindingsOK = true;
    HandleConstResources(
        [&](const ConstBuffBindInfo& cb) //
        {
            for (Uint32 BindPoint = cb.m_Attribs.BindPoint; BindPoint < Uint32{cb.m_Attribs.BindPoint} + cb.m_Attribs.BindCount; ++BindPoint)
            {
                if (!m_ResourceCache.IsCBBound(BindPoint))
                {
                    LOG_MISSING_BINDING("constant buffer", cb.m_Attribs, BindPoint);
                    BindingsOK = false;
                }
            }
        },

        [&](const TexSRVBindInfo& ts) //
        {
            for (Uint32 BindPoint = ts.m_Attribs.BindPoint; BindPoint < Uint32{ts.m_Attribs.BindPoint} + ts.m_Attribs.BindCount; ++BindPoint)
            {
                if (!m_ResourceCache.IsSRVBound(BindPoint, true))
                {
                    LOG_MISSING_BINDING("texture", ts.m_Attribs, BindPoint);
                    BindingsOK = false;
                }

                if (ts.ValidSamplerAssigned())
                {
                    const auto& Sampler = GetConstResource<SamplerBindInfo>(ts.SamplerIndex);
                    VERIFY_EXPR(Sampler.m_Attribs.BindCount == ts.m_Attribs.BindCount || Sampler.m_Attribs.BindCount == 1);

                    // Verify that if single sampler is used for all texture array elements, all samplers set in the resource views are consistent
                    if (ts.m_Attribs.BindCount > 1 && Sampler.m_Attribs.BindCount == 1)
                    {
                        ShaderResourceCacheD3D11::CachedSampler* pCachedSamplers       = nullptr;
                        ID3D11SamplerState**                     ppCachedD3D11Samplers = nullptr;
                        m_ResourceCache.GetSamplerArrays(pCachedSamplers, ppCachedD3D11Samplers);
                        VERIFY_EXPR(Sampler.m_Attribs.BindPoint < m_ResourceCache.GetSamplerCount());
                        const auto& CachedSampler = pCachedSamplers[Sampler.m_Attribs.BindPoint];

                        ShaderResourceCacheD3D11::CachedResource* pCachedResources       = nullptr;
                        ID3D11ShaderResourceView**                ppCachedD3D11Resources = nullptr;
                        m_ResourceCache.GetSRVArrays(pCachedResources, ppCachedD3D11Resources);
                        VERIFY_EXPR(BindPoint < m_ResourceCache.GetSRVCount());
                        auto& CachedResource = pCachedResources[BindPoint];
                        if (CachedResource.pView)
                        {
                            auto* pTexView = CachedResource.pView.RawPtr<ITextureView>();
                            auto* pSampler = pTexView->GetSampler();
                            if (pSampler != nullptr && pSampler != CachedSampler.pSampler.RawPtr())
                            {
                                LOG_ERROR_MESSAGE("All elements of texture array '", ts.m_Attribs.Name, "' in shader '", GetShaderName(), "' share the same sampler. However, the sampler set in view for element ", BindPoint - ts.m_Attribs.BindPoint, " does not match bound sampler. This may cause incorrect behavior on GL platform.");
                            }
                        }
                    }
                }
            }
        },

        [&](const TexUAVBindInfo& uav) //
        {
            for (Uint32 BindPoint = uav.m_Attribs.BindPoint; BindPoint < Uint32{uav.m_Attribs.BindPoint} + uav.m_Attribs.BindCount; ++BindPoint)
            {
                if (!m_ResourceCache.IsUAVBound(BindPoint, true))
                {
                    LOG_MISSING_BINDING("texture UAV", uav.m_Attribs, BindPoint);
                    BindingsOK = false;
                }
            }
        },

        [&](const BuffSRVBindInfo& buf) //
        {
            for (Uint32 BindPoint = buf.m_Attribs.BindPoint; BindPoint < Uint32{buf.m_Attribs.BindPoint} + buf.m_Attribs.BindCount; ++BindPoint)
            {
                if (!m_ResourceCache.IsSRVBound(BindPoint, false))
                {
                    LOG_MISSING_BINDING("buffer", buf.m_Attribs, BindPoint);
                    BindingsOK = false;
                }
            }
        },

        [&](const BuffUAVBindInfo& uav) //
        {
            for (Uint32 BindPoint = uav.m_Attribs.BindPoint; BindPoint < Uint32{uav.m_Attribs.BindPoint} + uav.m_Attribs.BindCount; ++BindPoint)
            {
                if (!m_ResourceCache.IsUAVBound(BindPoint, false))
                {
                    LOG_MISSING_BINDING("buffer UAV", uav.m_Attribs, BindPoint);
                    BindingsOK = false;
                }
            }
        },

        [&](const SamplerBindInfo& sam) //
        {
            for (Uint32 BindPoint = sam.m_Attribs.BindPoint; BindPoint < Uint32{sam.m_Attribs.BindPoint} + sam.m_Attribs.BindCount; ++BindPoint)
            {
                if (!m_ResourceCache.IsSamplerBound(BindPoint))
                {
                    LOG_MISSING_BINDING("sampler", sam.m_Attribs, BindPoint);
                    BindingsOK = false;
                }
            }
        } // clang-format off
    ); // clang-format on
#    undef LOG_MISSING_BINDING

    return BindingsOK;
}

#endif
} // namespace Diligent
