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

#include "ShaderResourceLayoutD3D12.hpp"
#include "ShaderResourceCacheD3D12.hpp"
#include "BufferD3D12Impl.hpp"
#include "BufferViewD3D12.h"
#include "TextureD3D12Impl.hpp"
#include "TextureViewD3D12Impl.hpp"
#include "SamplerD3D12Impl.hpp"
#include "ShaderD3D12Impl.hpp"
#include "RootSignature.hpp"
#include "PipelineStateD3D12Impl.hpp"
#include "ShaderResourceVariableBase.hpp"
#include "ShaderVariableD3DBase.hpp"

namespace Diligent
{

ShaderResourceLayoutD3D12::~ShaderResourceLayoutD3D12()
{
    for (Uint32 r = 0; r < GetTotalResourceCount(); ++r)
        GetResource(r).~D3D12Resource();
}

D3D12_DESCRIPTOR_RANGE_TYPE GetDescriptorRangeType(CachedResourceType ResType)
{
    class ResTypeToD3D12DescrRangeType
    {
    public:
        ResTypeToD3D12DescrRangeType()
        {
            // clang-format off
            m_Map[(size_t)CachedResourceType::CBV]     = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
            m_Map[(size_t)CachedResourceType::TexSRV]  = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            m_Map[(size_t)CachedResourceType::BufSRV]  = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            m_Map[(size_t)CachedResourceType::TexUAV]  = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
            m_Map[(size_t)CachedResourceType::BufUAV]  = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
            m_Map[(size_t)CachedResourceType::Sampler] = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
            // clang-format on
        }

        D3D12_DESCRIPTOR_RANGE_TYPE operator[](CachedResourceType ResType) const
        {
            auto Ind = static_cast<size_t>(ResType);
            VERIFY(Ind >= 0 && Ind < (size_t)CachedResourceType::NumTypes, "Unexpected resource type");
            return m_Map[Ind];
        }

    private:
        std::array<D3D12_DESCRIPTOR_RANGE_TYPE, static_cast<size_t>(CachedResourceType::NumTypes)> m_Map;
    };

    static const ResTypeToD3D12DescrRangeType ResTypeToDescrRangeTypeMap;
    return ResTypeToDescrRangeTypeMap[ResType];
}


void ShaderResourceLayoutD3D12::AllocateMemory(IMemoryAllocator&                                                  Allocator,
                                               const std::array<Uint32, SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES>& CbvSrvUavCount,
                                               const std::array<Uint32, SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES>& SamplerCount)
{
    m_CbvSrvUavOffsets[0] = 0;
    for (SHADER_RESOURCE_VARIABLE_TYPE VarType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC; VarType < SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES; VarType = static_cast<SHADER_RESOURCE_VARIABLE_TYPE>(VarType + 1))
    {
        VERIFY(m_CbvSrvUavOffsets[VarType] + CbvSrvUavCount[VarType] <= std::numeric_limits<Uint16>::max(), "Offset is not representable in 16 bits");
        m_CbvSrvUavOffsets[VarType + 1] = static_cast<Uint16>(m_CbvSrvUavOffsets[VarType] + CbvSrvUavCount[VarType]);
        VERIFY_EXPR(GetCbvSrvUavCount(VarType) == CbvSrvUavCount[VarType]);
    }

    m_SamplersOffsets[0] = m_CbvSrvUavOffsets[SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES];
    for (SHADER_RESOURCE_VARIABLE_TYPE VarType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC; VarType < SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES; VarType = static_cast<SHADER_RESOURCE_VARIABLE_TYPE>(VarType + 1))
    {
        VERIFY(m_SamplersOffsets[VarType] + SamplerCount[VarType] <= std::numeric_limits<Uint16>::max(), "Offset is not representable in 16 bits");
        m_SamplersOffsets[VarType + 1] = static_cast<Uint16>(m_SamplersOffsets[VarType] + SamplerCount[VarType]);
        VERIFY_EXPR(GetSamplerCount(VarType) == SamplerCount[VarType]);
    }

    size_t MemSize = GetTotalResourceCount() * sizeof(D3D12Resource);
    if (MemSize == 0)
        return;

    auto* pRawMem    = ALLOCATE_RAW(Allocator, "Raw memory buffer for shader resource layout resources", MemSize);
    m_ResourceBuffer = std::unique_ptr<void, STDDeleterRawMem<void>>(pRawMem, Allocator);
}


// http://diligentgraphics.com/diligent-engine/architecture/d3d12/shader-resource-layout#Initializing-Shader-Resource-Layouts-and-Root-Signature-in-a-Pipeline-State-Object
// http://diligentgraphics.com/diligent-engine/architecture/d3d12/shader-resource-cache#Initializing-Shader-Resource-Layouts-in-a-Pipeline-State
void ShaderResourceLayoutD3D12::Initialize(ID3D12Device*                               pd3d12Device,
                                           PIPELINE_TYPE                               PipelineType,
                                           const PipelineResourceLayoutDesc&           ResourceLayout,
                                           std::shared_ptr<const ShaderResourcesD3D12> pSrcResources,
                                           IMemoryAllocator&                           LayoutDataAllocator,
                                           const SHADER_RESOURCE_VARIABLE_TYPE* const  AllowedVarTypes,
                                           Uint32                                      NumAllowedTypes,
                                           ShaderResourceCacheD3D12*                   pResourceCache,
                                           RootSignature*                              pRootSig)
{
    m_pd3d12Device = pd3d12Device;
    m_pResources   = std::move(pSrcResources);

    VERIFY_EXPR((pResourceCache != nullptr) ^ (pRootSig != nullptr));

    const Uint32 AllowedTypeBits = GetAllowedTypeBits(AllowedVarTypes, NumAllowedTypes);

    std::array<Uint32, SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES> CbvSrvUavCount = {};
    std::array<Uint32, SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES> SamplerCount   = {};

    // Count number of resources to allocate all needed memory
    m_pResources->ProcessResources(
        [&](const D3DShaderResourceAttribs& CB, Uint32) //
        {
            auto VarType = m_pResources->FindVariableType(CB, ResourceLayout);
            if (IsAllowedType(VarType, AllowedTypeBits))
                ++CbvSrvUavCount[VarType];
        },
        [&](const D3DShaderResourceAttribs& Sam, Uint32) //
        {
            auto VarType = m_pResources->FindVariableType(Sam, ResourceLayout);
            if (IsAllowedType(VarType, AllowedTypeBits))
            {
                constexpr bool LogImtblSamplerArrayError = true;

                auto ImtblSamplerInd = m_pResources->FindImmutableSampler(Sam, ResourceLayout, LogImtblSamplerArrayError);
                // Skip immutable samplers
                if (ImtblSamplerInd < 0)
                    ++SamplerCount[VarType];
            }
        },
        [&](const D3DShaderResourceAttribs& TexSRV, Uint32) //
        {
            auto VarType = m_pResources->FindVariableType(TexSRV, ResourceLayout);
            if (IsAllowedType(VarType, AllowedTypeBits))
            {
                ++CbvSrvUavCount[VarType];
                if (TexSRV.IsCombinedWithSampler())
                {
                    const auto& SamplerAttribs = m_pResources->GetCombinedSampler(TexSRV);
                    auto        SamplerVarType = m_pResources->FindVariableType(SamplerAttribs, ResourceLayout);
                    DEV_CHECK_ERR(SamplerVarType == VarType,
                                  "The type (", GetShaderVariableTypeLiteralName(VarType), ") of texture SRV variable '", TexSRV.Name,
                                  "' is not consistent with the type (", GetShaderVariableTypeLiteralName(SamplerVarType),
                                  ") of the sampler '", SamplerAttribs.Name, "' that is assigned to it");
                    (void)SamplerVarType;
                }
            }
        },
        [&](const D3DShaderResourceAttribs& TexUAV, Uint32) //
        {
            auto VarType = m_pResources->FindVariableType(TexUAV, ResourceLayout);
            if (IsAllowedType(VarType, AllowedTypeBits))
                ++CbvSrvUavCount[VarType];
        },
        [&](const D3DShaderResourceAttribs& BufSRV, Uint32) //
        {
            auto VarType = m_pResources->FindVariableType(BufSRV, ResourceLayout);
            if (IsAllowedType(VarType, AllowedTypeBits))
                ++CbvSrvUavCount[VarType];
        },
        [&](const D3DShaderResourceAttribs& BufUAV, Uint32) //
        {
            auto VarType = m_pResources->FindVariableType(BufUAV, ResourceLayout);
            if (IsAllowedType(VarType, AllowedTypeBits))
                ++CbvSrvUavCount[VarType];
        } //
    );

    AllocateMemory(LayoutDataAllocator, CbvSrvUavCount, SamplerCount);

    std::array<Uint32, SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES> CurrCbvSrvUav = {};
    std::array<Uint32, SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES> CurrSampler   = {};

    Uint32 StaticResCacheTblSizes[4] = {0, 0, 0, 0};

    auto AddResource = [&](const D3DShaderResourceAttribs& Attribs,
                           CachedResourceType              ResType,
                           SHADER_RESOURCE_VARIABLE_TYPE   VarType,
                           Uint32                          SamplerId = D3D12Resource::InvalidSamplerId) //
    {
        Uint32 RootIndex = D3D12Resource::InvalidRootIndex;
        Uint32 Offset    = D3D12Resource::InvalidOffset;

        D3D12_DESCRIPTOR_RANGE_TYPE DescriptorRangeType = GetDescriptorRangeType(ResType);

        if (pRootSig)
        {
            pRootSig->AllocateResourceSlot(m_pResources->GetShaderType(), PipelineType, Attribs, VarType, DescriptorRangeType, RootIndex, Offset);
            VERIFY(RootIndex <= D3D12Resource::MaxRootIndex, "Root index excceeds allowed limit");
        }
        else
        {
            // If root signature is not provided - use artifial root signature to store
            // static shader resources:
            // SRVs at root index D3D12_DESCRIPTOR_RANGE_TYPE_SRV (0)
            // UAVs at root index D3D12_DESCRIPTOR_RANGE_TYPE_UAV (1)
            // CBVs at root index D3D12_DESCRIPTOR_RANGE_TYPE_CBV (2)
            // Samplers at root index D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER (3)

            // http://diligentgraphics.com/diligent-engine/architecture/d3d12/shader-resource-layout#Initializing-Special-Resource-Layout-for-Managing-Static-Shader-Resources

            VERIFY_EXPR(pResourceCache != nullptr);

            RootIndex = DescriptorRangeType;
            Offset    = Attribs.BindPoint;
            // Resources in the static resource cache are indexed by the bind point
            StaticResCacheTblSizes[RootIndex] = std::max(StaticResCacheTblSizes[RootIndex], Offset + Attribs.BindCount);
        }
        VERIFY(RootIndex != D3D12Resource::InvalidRootIndex, "Root index must be valid");
        VERIFY(Offset != D3D12Resource::InvalidOffset, "Offset must be valid");

        // Immutable samplers are never copied, and SamplerId == InvalidSamplerId
        auto& NewResource = (ResType == CachedResourceType::Sampler) ?
            GetSampler(VarType, CurrSampler[VarType]++) :
            GetSrvCbvUav(VarType, CurrCbvSrvUav[VarType]++);
        ::new (&NewResource) D3D12Resource(*this, Attribs, VarType, ResType, RootIndex, Offset, SamplerId);
    };


    m_pResources->ProcessResources(
        [&](const D3DShaderResourceAttribs& CB, Uint32) //
        {
            auto VarType = m_pResources->FindVariableType(CB, ResourceLayout);
            if (IsAllowedType(VarType, AllowedTypeBits))
                AddResource(CB, CachedResourceType::CBV, VarType);
        },
        [&](const D3DShaderResourceAttribs& Sam, Uint32) //
        {
            auto VarType = m_pResources->FindVariableType(Sam, ResourceLayout);
            if (IsAllowedType(VarType, AllowedTypeBits))
            {
                // The error (if any) have already been logged when counting the resources
                constexpr bool LogImtblSamplerArrayError = false;

                auto ImtblSamplerInd = m_pResources->FindImmutableSampler(Sam, ResourceLayout, LogImtblSamplerArrayError);
                if (ImtblSamplerInd >= 0)
                {
                    if (pRootSig != nullptr)
                        pRootSig->InitImmutableSampler(m_pResources->GetShaderType(), Sam.Name, m_pResources->GetCombinedSamplerSuffix(), Sam);
                }
                else
                {
                    AddResource(Sam, CachedResourceType::Sampler, VarType);
                }
            }
        },
        [&](const D3DShaderResourceAttribs& TexSRV, Uint32) //
        {
            auto VarType = m_pResources->FindVariableType(TexSRV, ResourceLayout);
            if (IsAllowedType(VarType, AllowedTypeBits))
            {
                static_assert(SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES == 3, "Unexpected number of shader variable types");
                VERIFY(CurrSampler[SHADER_RESOURCE_VARIABLE_TYPE_STATIC] + CurrSampler[SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE] + CurrSampler[SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC] == GetTotalSamplerCount(), "All samplers must be initialized before texture SRVs");

                Uint32 SamplerId = D3D12Resource::InvalidSamplerId;
                if (TexSRV.IsCombinedWithSampler())
                {
                    const auto& SamplerAttribs = m_pResources->GetCombinedSampler(TexSRV);
                    auto        SamplerVarType = m_pResources->FindVariableType(SamplerAttribs, ResourceLayout);
                    DEV_CHECK_ERR(SamplerVarType == VarType,
                                  "The type (", GetShaderVariableTypeLiteralName(VarType), ") of texture SRV variable '", TexSRV.Name,
                                  "' is not consistent with the type (", GetShaderVariableTypeLiteralName(SamplerVarType),
                                  ") of the sampler '", SamplerAttribs.Name, "' that is assigned to it");

                    // The error (if any) have already been logged when counting the resources
                    constexpr bool LogImtblSamplerArrayError = false;

                    auto ImtblSamplerInd = m_pResources->FindImmutableSampler(SamplerAttribs, ResourceLayout, LogImtblSamplerArrayError);
                    if (ImtblSamplerInd >= 0)
                    {
                    // Immutable samplers are never copied, and SamplerId == InvalidSamplerId
#ifdef DILIGENT_DEBUG
                        auto SamplerCount = GetTotalSamplerCount();
                        for (Uint32 s = 0; s < SamplerCount; ++s)
                        {
                            const auto& Sampler = GetSampler(s);
                            if (strcmp(Sampler.Attribs.Name, SamplerAttribs.Name) == 0)
                                LOG_ERROR("Immutable sampler '", Sampler.Attribs.Name, "' was found among resources. This seems to be a bug");
                        }
#endif
                    }
                    else
                    {
                        auto SamplerCount = GetTotalSamplerCount();
                        bool SamplerFound = false;
                        for (SamplerId = 0; SamplerId < SamplerCount; ++SamplerId)
                        {
                            const auto& Sampler = GetSampler(SamplerId);
                            SamplerFound        = strcmp(Sampler.Attribs.Name, SamplerAttribs.Name) == 0;
                            if (SamplerFound)
                                break;
                        }

                        if (!SamplerFound)
                        {
                            LOG_ERROR("Unable to find sampler '", SamplerAttribs.Name, "' assigned to texture SRV '", TexSRV.Name, "' in the list of already created resources. This seems to be a bug.");
                            SamplerId = D3D12Resource::InvalidSamplerId;
                        }
                        VERIFY(SamplerId <= D3D12Resource::MaxSamplerId, "Sampler index excceeds allowed limit");
                    }
                }
                AddResource(TexSRV, CachedResourceType::TexSRV, VarType, SamplerId);
            }
        },
        [&](const D3DShaderResourceAttribs& TexUAV, Uint32) //
        {
            auto VarType = m_pResources->FindVariableType(TexUAV, ResourceLayout);
            if (IsAllowedType(VarType, AllowedTypeBits))
                AddResource(TexUAV, CachedResourceType::TexUAV, VarType);
        },
        [&](const D3DShaderResourceAttribs& BufSRV, Uint32) //
        {
            auto VarType = m_pResources->FindVariableType(BufSRV, ResourceLayout);
            if (IsAllowedType(VarType, AllowedTypeBits))
                AddResource(BufSRV, CachedResourceType::BufSRV, VarType);
        },
        [&](const D3DShaderResourceAttribs& BufUAV, Uint32) //
        {
            auto VarType = m_pResources->FindVariableType(BufUAV, ResourceLayout);
            if (IsAllowedType(VarType, AllowedTypeBits))
                AddResource(BufUAV, CachedResourceType::BufUAV, VarType);
        } //
    );

#ifdef DILIGENT_DEBUG
    for (SHADER_RESOURCE_VARIABLE_TYPE VarType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC; VarType < SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES; VarType = static_cast<SHADER_RESOURCE_VARIABLE_TYPE>(VarType + 1))
    {
        VERIFY(CurrCbvSrvUav[VarType] == CbvSrvUavCount[VarType], "Not all Srv/Cbv/Uavs are initialized, which result in a crash when dtor is called");
        VERIFY(CurrSampler[VarType] == SamplerCount[VarType], "Not all Samplers are initialized, which result in a crash when dtor is called");
    }
#endif

    if (pResourceCache)
    {
        // Initialize resource cache to store static resources
        // http://diligentgraphics.com/diligent-engine/architecture/d3d12/shader-resource-cache#Initializing-the-Cache-for-Static-Shader-Resources
        // http://diligentgraphics.com/diligent-engine/architecture/d3d12/shader-resource-cache#Initializing-Shader-Objects
        VERIFY_EXPR(pRootSig == nullptr);
        pResourceCache->Initialize(GetRawAllocator(), _countof(StaticResCacheTblSizes), StaticResCacheTblSizes);
#ifdef DILIGENT_DEBUG
        pResourceCache->GetRootTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV).SetDebugAttribs(StaticResCacheTblSizes[D3D12_DESCRIPTOR_RANGE_TYPE_SRV], D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_pResources->GetShaderType());
        pResourceCache->GetRootTable(D3D12_DESCRIPTOR_RANGE_TYPE_UAV).SetDebugAttribs(StaticResCacheTblSizes[D3D12_DESCRIPTOR_RANGE_TYPE_UAV], D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_pResources->GetShaderType());
        pResourceCache->GetRootTable(D3D12_DESCRIPTOR_RANGE_TYPE_CBV).SetDebugAttribs(StaticResCacheTblSizes[D3D12_DESCRIPTOR_RANGE_TYPE_CBV], D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_pResources->GetShaderType());
        pResourceCache->GetRootTable(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER).SetDebugAttribs(StaticResCacheTblSizes[D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER], D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, m_pResources->GetShaderType());
#endif
    }
}


void ShaderResourceLayoutD3D12::D3D12Resource::CacheCB(IDeviceObject*                      pBuffer,
                                                       ShaderResourceCacheD3D12::Resource& DstRes,
                                                       Uint32                              ArrayInd,
                                                       D3D12_CPU_DESCRIPTOR_HANDLE         ShdrVisibleHeapCPUDescriptorHandle,
                                                       Uint32&                             BoundDynamicCBsCounter) const
{
    // http://diligentgraphics.com/diligent-engine/architecture/d3d12/shader-resource-cache#Binding-Objects-to-Shader-Variables

    // We cannot use ValidatedCast<> here as the resource retrieved from the
    // resource mapping can be of wrong type
    RefCntAutoPtr<BufferD3D12Impl> pBuffD3D12(pBuffer, IID_BufferD3D12);
#ifdef DILIGENT_DEVELOPMENT
    VerifyConstantBufferBinding(Attribs, GetVariableType(), ArrayInd, pBuffer, pBuffD3D12.RawPtr(), DstRes.pObject.RawPtr(), ParentResLayout.GetShaderName());
#endif
    if (pBuffD3D12)
    {
        if (GetVariableType() != SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC && DstRes.pObject != nullptr)
        {
            // Do not update resource if one is already bound unless it is dynamic. This may be
            // dangerous as CopyDescriptorsSimple() may interfere with GPU reading the same descriptor.
            return;
        }

        DstRes.Type                = GetResType();
        DstRes.CPUDescriptorHandle = pBuffD3D12->GetCBVHandle();
        VERIFY(DstRes.CPUDescriptorHandle.ptr != 0 || pBuffD3D12->GetDesc().Usage == USAGE_DYNAMIC, "No relevant CBV CPU descriptor handle");

        if (ShdrVisibleHeapCPUDescriptorHandle.ptr != 0)
        {
            // Dynamic resources are assigned descriptor in the GPU-visible heap at every draw call, and
            // the descriptor is copied by the RootSignature when resources are committed
            VERIFY(DstRes.pObject == nullptr, "Static and mutable resource descriptors must be copied only once");

            ID3D12Device* pd3d12Device = ParentResLayout.m_pd3d12Device;
            pd3d12Device->CopyDescriptorsSimple(1, ShdrVisibleHeapCPUDescriptorHandle, DstRes.CPUDescriptorHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }

        if (DstRes.pObject != nullptr && DstRes.pObject.RawPtr<const BufferD3D12Impl>()->GetDesc().Usage == USAGE_DYNAMIC)
        {
            VERIFY(BoundDynamicCBsCounter > 0, "There is a dynamic CB bound in the resource cache, but the dynamic CB counter is zero");
            --BoundDynamicCBsCounter;
        }
        if (pBuffD3D12->GetDesc().Usage == USAGE_DYNAMIC)
            ++BoundDynamicCBsCounter;
        DstRes.pObject = std::move(pBuffD3D12);
    }
}


template <typename TResourceViewType>
struct ResourceViewTraits
{};

template <>
struct ResourceViewTraits<ITextureViewD3D12>
{
    static const INTERFACE_ID& IID;

    static bool VerifyView(ITextureViewD3D12* pViewD3D12, const D3DShaderResourceAttribs& Attribs, const char* ShaderName)
    {
        return true;
    }
};
const INTERFACE_ID& ResourceViewTraits<ITextureViewD3D12>::IID = IID_TextureViewD3D12;

template <>
struct ResourceViewTraits<IBufferViewD3D12>
{
    static const INTERFACE_ID& IID;

    static bool VerifyView(IBufferViewD3D12* pViewD3D12, const D3DShaderResourceAttribs& Attribs, const char* ShaderName)
    {
        return VerifyBufferViewModeD3D(pViewD3D12, Attribs, ShaderName);
    }
};
const INTERFACE_ID& ResourceViewTraits<IBufferViewD3D12>::IID = IID_BufferViewD3D12;

template <typename TResourceViewType,    ///< ResType of the view (ITextureViewD3D12 or IBufferViewD3D12)
          typename TViewTypeEnum,        ///< ResType of the expected view type enum (TEXTURE_VIEW_TYPE or BUFFER_VIEW_TYPE)
          typename TBindSamplerProcType> ///< ResType of the procedure to set sampler
void ShaderResourceLayoutD3D12::D3D12Resource::CacheResourceView(IDeviceObject*                      pView,
                                                                 ShaderResourceCacheD3D12::Resource& DstRes,
                                                                 Uint32                              ArrayIndex,
                                                                 D3D12_CPU_DESCRIPTOR_HANDLE         ShdrVisibleHeapCPUDescriptorHandle,
                                                                 TViewTypeEnum                       dbgExpectedViewType,
                                                                 TBindSamplerProcType                BindSamplerProc) const
{
    // We cannot use ValidatedCast<> here as the resource retrieved from the
    // resource mapping can be of wrong type
    RefCntAutoPtr<TResourceViewType> pViewD3D12{pView, ResourceViewTraits<TResourceViewType>::IID};
#ifdef DILIGENT_DEVELOPMENT
    VerifyResourceViewBinding(Attribs, GetVariableType(), ArrayIndex, pView, pViewD3D12.RawPtr(), {dbgExpectedViewType}, DstRes.pObject.RawPtr(), ParentResLayout.GetShaderName());
    ResourceViewTraits<TResourceViewType>::VerifyView(pViewD3D12, Attribs, ParentResLayout.GetShaderName());
#endif
    if (pViewD3D12)
    {
        if (GetVariableType() != SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC && DstRes.pObject != nullptr)
        {
            // Do not update resource if one is already bound unless it is dynamic. This may be
            // dangerous as CopyDescriptorsSimple() may interfere with GPU reading the same descriptor.
            return;
        }

        DstRes.Type                = GetResType();
        DstRes.CPUDescriptorHandle = pViewD3D12->GetCPUDescriptorHandle();
        VERIFY(DstRes.CPUDescriptorHandle.ptr != 0, "No relevant D3D12 view");

        if (ShdrVisibleHeapCPUDescriptorHandle.ptr != 0)
        {
            // Dynamic resources are assigned descriptor in the GPU-visible heap at every draw call, and
            // the descriptor is copied by the RootSignature when resources are committed
            VERIFY(DstRes.pObject == nullptr, "Static and mutable resource descriptors must be copied only once");

            ID3D12Device* pd3d12Device = ParentResLayout.m_pd3d12Device;
            pd3d12Device->CopyDescriptorsSimple(1, ShdrVisibleHeapCPUDescriptorHandle, DstRes.CPUDescriptorHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }

        BindSamplerProc(pViewD3D12);

        DstRes.pObject = std::move(pViewD3D12);
    }
}

void ShaderResourceLayoutD3D12::D3D12Resource::CacheSampler(IDeviceObject*                      pSampler,
                                                            ShaderResourceCacheD3D12::Resource& DstSam,
                                                            Uint32                              ArrayIndex,
                                                            D3D12_CPU_DESCRIPTOR_HANDLE         ShdrVisibleHeapCPUDescriptorHandle) const
{
    VERIFY(Attribs.IsValidBindPoint(), "Invalid bind point");
    VERIFY_EXPR(ArrayIndex < Attribs.BindCount);

    RefCntAutoPtr<ISamplerD3D12> pSamplerD3D12(pSampler, IID_SamplerD3D12);
    if (pSamplerD3D12)
    {
        if (GetVariableType() != SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC && DstSam.pObject != nullptr)
        {
            if (DstSam.pObject != pSampler)
            {
                auto VarTypeStr = GetShaderVariableTypeLiteralName(GetVariableType());
                LOG_ERROR_MESSAGE("Non-null sampler is already bound to ", VarTypeStr, " shader variable '", Attribs.GetPrintName(ArrayIndex),
                                  "' in shader '", ParentResLayout.GetShaderName(), "'. Attempting to bind another sampler is an error and will "
                                                                                    "be ignored. Use another shader resource binding instance or label the variable as dynamic.");
            }

            // Do not update resource if one is already bound unless it is dynamic. This may be
            // dangerous as CopyDescriptorsSimple() may interfere with GPU reading the same descriptor.
            return;
        }

        DstSam.Type = CachedResourceType::Sampler;

        DstSam.CPUDescriptorHandle = pSamplerD3D12->GetCPUDescriptorHandle();
        VERIFY(DstSam.CPUDescriptorHandle.ptr != 0, "No relevant D3D12 sampler descriptor handle");

        if (ShdrVisibleHeapCPUDescriptorHandle.ptr != 0)
        {
            // Dynamic resources are assigned descriptor in the GPU-visible heap at every draw call, and
            // the descriptor is copied by the RootSignature when resources are committed
            VERIFY(DstSam.pObject == nullptr, "Static and mutable resource descriptors must be copied only once");

            ID3D12Device* pd3d12Device = ParentResLayout.m_pd3d12Device;
            pd3d12Device->CopyDescriptorsSimple(1, ShdrVisibleHeapCPUDescriptorHandle, DstSam.CPUDescriptorHandle, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
        }

        DstSam.pObject = std::move(pSamplerD3D12);
    }
    else
    {
        LOG_ERROR_MESSAGE("Failed to bind object '", pSampler->GetDesc().Name, "' to variable '", Attribs.GetPrintName(ArrayIndex),
                          "' in shader '", ParentResLayout.GetShaderName(), "'. Incorect object type: sampler is expected.");
    }
}

const ShaderResourceLayoutD3D12::D3D12Resource& ShaderResourceLayoutD3D12::GetAssignedSampler(const D3D12Resource& TexSrv) const
{
    VERIFY(TexSrv.GetResType() == CachedResourceType::TexSRV, "Unexpected resource type: texture SRV is expected");
    VERIFY(TexSrv.ValidSamplerAssigned(), "Texture SRV has no associated sampler");
    const auto& SamInfo = GetSampler(TexSrv.SamplerId);
    VERIFY(SamInfo.GetVariableType() == TexSrv.GetVariableType(), "Inconsistent texture and sampler variable types");
    VERIFY(StreqSuff(SamInfo.Attribs.Name, TexSrv.Attribs.Name, m_pResources->GetCombinedSamplerSuffix()), "Sampler name '", SamInfo.Attribs.Name, "' does not match texture name '", TexSrv.Attribs.Name, '\'');
    return SamInfo;
}

ShaderResourceLayoutD3D12::D3D12Resource& ShaderResourceLayoutD3D12::GetAssignedSampler(const D3D12Resource& TexSrv)
{
    return const_cast<D3D12Resource&>(const_cast<const ShaderResourceLayoutD3D12*>(this)->GetAssignedSampler(TexSrv));
}


void ShaderResourceLayoutD3D12::D3D12Resource::BindResource(IDeviceObject*            pObj,
                                                            Uint32                    ArrayIndex,
                                                            ShaderResourceCacheD3D12& ResourceCache) const
{
    VERIFY_EXPR(ArrayIndex < Attribs.BindCount);

    const bool IsSampler          = GetResType() == CachedResourceType::Sampler;
    auto       DescriptorHeapType = IsSampler ? D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER : D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    auto&      DstRes             = ResourceCache.GetRootTable(RootIndex).GetResource(OffsetFromTableStart + ArrayIndex, DescriptorHeapType, ParentResLayout.m_pResources->GetShaderType());

    auto ShdrVisibleHeapCPUDescriptorHandle = IsSampler ?
        ResourceCache.GetShaderVisibleTableCPUDescriptorHandle<D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER>(RootIndex, OffsetFromTableStart + ArrayIndex) :
        ResourceCache.GetShaderVisibleTableCPUDescriptorHandle<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>(RootIndex, OffsetFromTableStart + ArrayIndex);

#ifdef DILIGENT_DEBUG
    {
        if (ResourceCache.DbgGetContentType() == ShaderResourceCacheD3D12::DbgCacheContentType::StaticShaderResources)
        {
            VERIFY(ShdrVisibleHeapCPUDescriptorHandle.ptr == 0, "Static shader resources of a shader should not be assigned shader visible descriptor space");
        }
        else if (ResourceCache.DbgGetContentType() == ShaderResourceCacheD3D12::DbgCacheContentType::SRBResources)
        {
            if (GetResType() == CachedResourceType::CBV && Attribs.BindCount == 1)
            {
                VERIFY(ShdrVisibleHeapCPUDescriptorHandle.ptr == 0, "Non-array constant buffers are bound as root views and should not be assigned shader visible descriptor space");
            }
            else
            {
                if (GetVariableType() == SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC)
                    VERIFY(ShdrVisibleHeapCPUDescriptorHandle.ptr == 0, "Dynamic resources of a shader resource binding should be assigned shader visible descriptor space at every draw call");
                else
                    VERIFY(ShdrVisibleHeapCPUDescriptorHandle.ptr != 0, "Non-dynamics resources of a shader resource binding must be assigned shader visible descriptor space");
            }
        }
        else
        {
            UNEXPECTED("Unknown content type");
        }
    }
#endif

    if (pObj)
    {
        switch (GetResType())
        {
            case CachedResourceType::CBV:
                CacheCB(pObj, DstRes, ArrayIndex, ShdrVisibleHeapCPUDescriptorHandle, ResourceCache.GetBoundDynamicCBsCounter());
                break;

            case CachedResourceType::TexSRV:
                CacheResourceView<ITextureViewD3D12>(
                    pObj, DstRes, ArrayIndex, ShdrVisibleHeapCPUDescriptorHandle, TEXTURE_VIEW_SHADER_RESOURCE,
                    [&](ITextureViewD3D12* pTexView) //
                    {
                        if (ValidSamplerAssigned())
                        {
                            auto& Sam = ParentResLayout.GetAssignedSampler(*this);
                            //VERIFY( !Sam.Attribs.IsImmutableSampler(), "Immutable samplers should never be assigned space in the cache" );
                            VERIFY_EXPR(Attribs.BindCount == Sam.Attribs.BindCount || Sam.Attribs.BindCount == 1);
                            auto SamplerArrInd = Sam.Attribs.BindCount > 1 ? ArrayIndex : 0;

                            auto ShdrVisibleSamplerHeapCPUDescriptorHandle = ResourceCache.GetShaderVisibleTableCPUDescriptorHandle<D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER>(Sam.RootIndex, Sam.OffsetFromTableStart + SamplerArrInd);

                            auto& DstSam = ResourceCache.GetRootTable(Sam.RootIndex).GetResource(Sam.OffsetFromTableStart + SamplerArrInd, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, ParentResLayout.m_pResources->GetShaderType());
#ifdef DILIGENT_DEBUG
                            {
                                if (ResourceCache.DbgGetContentType() == ShaderResourceCacheD3D12::DbgCacheContentType::StaticShaderResources)
                                {
                                    VERIFY(ShdrVisibleSamplerHeapCPUDescriptorHandle.ptr == 0, "Static shader resources of a shader should not be assigned shader visible descriptor space");
                                }
                                else if (ResourceCache.DbgGetContentType() == ShaderResourceCacheD3D12::DbgCacheContentType::SRBResources)
                                {
                                    if (GetVariableType() == SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC)
                                        VERIFY(ShdrVisibleSamplerHeapCPUDescriptorHandle.ptr == 0, "Dynamic resources of a shader resource binding should be assigned shader visible descriptor space at every draw call");
                                    else
                                        VERIFY(ShdrVisibleSamplerHeapCPUDescriptorHandle.ptr != 0 || pTexView == nullptr, "Non-dynamics resources of a shader resource binding must be assigned shader visible descriptor space");
                                }
                                else
                                {
                                    UNEXPECTED("Unknown content type");
                                }
                            }
#endif
                            auto pSampler = pTexView->GetSampler();
                            if (pSampler)
                            {
                                Sam.CacheSampler(pSampler, DstSam, SamplerArrInd, ShdrVisibleSamplerHeapCPUDescriptorHandle);
                            }
                            else
                            {
                                LOG_ERROR_MESSAGE("Failed to bind sampler to variable '", Sam.Attribs.Name, ". Sampler is not set in the texture view '", pTexView->GetDesc().Name, '\'');
                            }
                        }
                    });
                break;

            case CachedResourceType::TexUAV:
                CacheResourceView<ITextureViewD3D12>(pObj, DstRes, ArrayIndex, ShdrVisibleHeapCPUDescriptorHandle, TEXTURE_VIEW_UNORDERED_ACCESS, [](ITextureViewD3D12*) {});
                break;

            case CachedResourceType::BufSRV:
                CacheResourceView<IBufferViewD3D12>(pObj, DstRes, ArrayIndex, ShdrVisibleHeapCPUDescriptorHandle, BUFFER_VIEW_SHADER_RESOURCE, [](IBufferViewD3D12*) {});
                break;

            case CachedResourceType::BufUAV:
                CacheResourceView<IBufferViewD3D12>(pObj, DstRes, ArrayIndex, ShdrVisibleHeapCPUDescriptorHandle, BUFFER_VIEW_UNORDERED_ACCESS, [](IBufferViewD3D12*) {});
                break;

            case CachedResourceType::Sampler:
                DEV_CHECK_ERR(ParentResLayout.IsUsingSeparateSamplers(), "Samplers should not be set directly when using combined texture samplers");
                CacheSampler(pObj, DstRes, ArrayIndex, ShdrVisibleHeapCPUDescriptorHandle);
                break;

            default: UNEXPECTED("Unknown resource type ", static_cast<Int32>(GetResType()));
        }
    }
    else
    {
        if (DstRes.pObject != nullptr && GetVariableType() != SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC)
            LOG_ERROR_MESSAGE("Shader variable '", Attribs.Name, "' in shader '", ParentResLayout.GetShaderName(), "' is not dynamic but is being reset to null. This is an error and may cause unpredicted behavior. Use another shader resource binding instance or label the variable as dynamic if you need to bind another resource.");

        DstRes = ShaderResourceCacheD3D12::Resource{};
        if (ValidSamplerAssigned())
        {
            auto&                       Sam           = ParentResLayout.GetAssignedSampler(*this);
            D3D12_CPU_DESCRIPTOR_HANDLE NullHandle    = {0};
            auto                        SamplerArrInd = Sam.Attribs.BindCount > 1 ? ArrayIndex : 0;
            auto&                       DstSam        = ResourceCache.GetRootTable(Sam.RootIndex).GetResource(Sam.OffsetFromTableStart + SamplerArrInd, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, ParentResLayout.m_pResources->GetShaderType());
            if (DstSam.pObject != nullptr && Sam.GetVariableType() != SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC)
                LOG_ERROR_MESSAGE("Sampler variable '", Sam.Attribs.Name, "' in shader '", ParentResLayout.GetShaderName(), "' is not dynamic but is being reset to null. This is an error and may cause unpredicted behavior. Use another shader resource binding instance or label the variable as dynamic if you need to bind another sampler.");
            DstSam = ShaderResourceCacheD3D12::Resource{};
        }
    }
}

bool ShaderResourceLayoutD3D12::D3D12Resource::IsBound(Uint32 ArrayIndex, const ShaderResourceCacheD3D12& ResourceCache) const
{
    VERIFY_EXPR(ArrayIndex < Attribs.BindCount);

    if (RootIndex < ResourceCache.GetNumRootTables())
    {
        const auto& RootTable = ResourceCache.GetRootTable(RootIndex);
        if (OffsetFromTableStart + ArrayIndex < RootTable.GetSize())
        {
            const auto& CachedRes =
                RootTable.GetResource(OffsetFromTableStart + ArrayIndex,
                                      GetResType() == CachedResourceType::Sampler ? D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER : D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                                      ParentResLayout.m_pResources->GetShaderType());
            if (CachedRes.pObject != nullptr)
            {
                VERIFY(CachedRes.CPUDescriptorHandle.ptr != 0 || CachedRes.pObject.RawPtr<BufferD3D12Impl>()->GetDesc().Usage == USAGE_DYNAMIC, "No relevant descriptor handle");
                return true;
            }
        }
    }

    return false;
}


void ShaderResourceLayoutD3D12::CopyStaticResourceDesriptorHandles(const ShaderResourceCacheD3D12& SrcCache, const ShaderResourceLayoutD3D12& DstLayout, ShaderResourceCacheD3D12& DstCache) const
{
    // Static shader resources are stored as follows:
    // CBVs at root index D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
    // SRVs at root index D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
    // UAVs at root index D3D12_DESCRIPTOR_RANGE_TYPE_UAV, and
    // Samplers at root index D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER
    // Every resource is stored at offset that equals resource bind point

    auto CbvSrvUavCount = DstLayout.GetCbvSrvUavCount(SHADER_RESOURCE_VARIABLE_TYPE_STATIC);
    VERIFY(GetCbvSrvUavCount(SHADER_RESOURCE_VARIABLE_TYPE_STATIC) == CbvSrvUavCount, "Number of static resources in the source cache (", GetCbvSrvUavCount(SHADER_RESOURCE_VARIABLE_TYPE_STATIC), ") is not consistent with the number of static resources in destination cache (", CbvSrvUavCount, ")");
    auto& DstBoundDynamicCBsCounter = DstCache.GetBoundDynamicCBsCounter();
    for (Uint32 r = 0; r < CbvSrvUavCount; ++r)
    {
        // Get resource attributes
        const auto& res       = DstLayout.GetSrvCbvUav(SHADER_RESOURCE_VARIABLE_TYPE_STATIC, r);
        auto        RangeType = GetDescriptorRangeType(res.GetResType());
        for (Uint32 ArrInd = 0; ArrInd < res.Attribs.BindCount; ++ArrInd)
        {
            auto BindPoint = res.Attribs.BindPoint + ArrInd;
            // Source resource in the static resource cache is in the root table at index RangeType, at offset BindPoint
            // D3D12_DESCRIPTOR_RANGE_TYPE_SRV = 0,
            // D3D12_DESCRIPTOR_RANGE_TYPE_UAV = 1
            // D3D12_DESCRIPTOR_RANGE_TYPE_CBV = 2
            const auto& SrcRes = SrcCache.GetRootTable(RangeType).GetResource(BindPoint, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_pResources->GetShaderType());
            if (!SrcRes.pObject)
                LOG_ERROR_MESSAGE("No resource is assigned to static shader variable '", res.Attribs.GetPrintName(ArrInd), "' in shader '", GetShaderName(), "'.");
            // Destination resource is at the root index and offset defined by the resource layout
            auto& DstRes = DstCache.GetRootTable(res.RootIndex).GetResource(res.OffsetFromTableStart + ArrInd, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_pResources->GetShaderType());

            if (DstRes.pObject != SrcRes.pObject)
            {
                VERIFY(DstRes.pObject == nullptr, "Static resource has already been initialized, and the resource to be assigned from the shader does not match previously assigned resource");

                if (SrcRes.Type == CachedResourceType::CBV)
                {
                    if (DstRes.pObject && DstRes.pObject.RawPtr<const BufferD3D12Impl>()->GetDesc().Usage == USAGE_DYNAMIC)
                    {
                        VERIFY_EXPR(DstBoundDynamicCBsCounter > 0);
                        --DstBoundDynamicCBsCounter;
                    }
                    if (SrcRes.pObject && SrcRes.pObject.RawPtr<const BufferD3D12Impl>()->GetDesc().Usage == USAGE_DYNAMIC)
                    {
                        ++DstBoundDynamicCBsCounter;
                    }
                }

                DstRes.pObject             = SrcRes.pObject;
                DstRes.Type                = SrcRes.Type;
                DstRes.CPUDescriptorHandle = SrcRes.CPUDescriptorHandle;

                auto ShdrVisibleHeapCPUDescriptorHandle = DstCache.GetShaderVisibleTableCPUDescriptorHandle<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>(res.RootIndex, res.OffsetFromTableStart + ArrInd);
                VERIFY_EXPR(ShdrVisibleHeapCPUDescriptorHandle.ptr != 0 || DstRes.Type == CachedResourceType::CBV);
                // Root views are not assigned space in the GPU-visible descriptor heap allocation
                if (ShdrVisibleHeapCPUDescriptorHandle.ptr != 0)
                {
                    m_pd3d12Device->CopyDescriptorsSimple(1, ShdrVisibleHeapCPUDescriptorHandle, SrcRes.CPUDescriptorHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                }
            }
            else
            {
                VERIFY_EXPR(DstRes.pObject == SrcRes.pObject);
                VERIFY_EXPR(DstRes.Type == SrcRes.Type);
                VERIFY_EXPR(DstRes.CPUDescriptorHandle.ptr == SrcRes.CPUDescriptorHandle.ptr);
            }
        }

        if (res.ValidSamplerAssigned())
        {
            const auto& SamInfo = DstLayout.GetAssignedSampler(res);

            //VERIFY(!SamInfo.Attribs.IsImmutableSampler(), "Immutable samplers should never be assigned space in the cache");

            VERIFY(SamInfo.Attribs.IsValidBindPoint(), "Sampler bind point must be valid");
            VERIFY_EXPR(SamInfo.Attribs.BindCount == res.Attribs.BindCount || SamInfo.Attribs.BindCount == 1);
        }
    }

    auto SamplerCount = DstLayout.GetSamplerCount(SHADER_RESOURCE_VARIABLE_TYPE_STATIC);
    VERIFY(GetSamplerCount(SHADER_RESOURCE_VARIABLE_TYPE_STATIC) == SamplerCount, "Number of static-type samplers in the source cache (", GetSamplerCount(SHADER_RESOURCE_VARIABLE_TYPE_STATIC), ") is not consistent with the number of static-type samplers in destination cache (", SamplerCount, ")");
    for (Uint32 s = 0; s < SamplerCount; ++s)
    {
        const auto& SamInfo = DstLayout.GetSampler(SHADER_RESOURCE_VARIABLE_TYPE_STATIC, s);
        for (Uint32 ArrInd = 0; ArrInd < SamInfo.Attribs.BindCount; ++ArrInd)
        {
            auto BindPoint = SamInfo.Attribs.BindPoint + ArrInd;
            // Source sampler in the static resource cache is in the root table at index 3
            // (D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER = 3), at offset BindPoint
            const auto& SrcSampler = SrcCache.GetRootTable(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER).GetResource(BindPoint, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, m_pResources->GetShaderType());
            if (!SrcSampler.pObject)
                LOG_ERROR_MESSAGE("No sampler assigned to static shader variable '", SamInfo.Attribs.GetPrintName(ArrInd), "' in shader '", GetShaderName(), "'.");
            auto& DstSampler = DstCache.GetRootTable(SamInfo.RootIndex).GetResource(SamInfo.OffsetFromTableStart + ArrInd, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, m_pResources->GetShaderType());

            if (DstSampler.pObject != SrcSampler.pObject)
            {
                VERIFY(DstSampler.pObject == nullptr, "Static-type sampler has already been initialized, and the sampler to be assigned from the shader does not match previously assigned resource");

                DstSampler.pObject             = SrcSampler.pObject;
                DstSampler.Type                = SrcSampler.Type;
                DstSampler.CPUDescriptorHandle = SrcSampler.CPUDescriptorHandle;

                auto ShdrVisibleSamplerHeapCPUDescriptorHandle = DstCache.GetShaderVisibleTableCPUDescriptorHandle<D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER>(SamInfo.RootIndex, SamInfo.OffsetFromTableStart + ArrInd);
                VERIFY_EXPR(ShdrVisibleSamplerHeapCPUDescriptorHandle.ptr != 0);
                if (ShdrVisibleSamplerHeapCPUDescriptorHandle.ptr != 0)
                {
                    m_pd3d12Device->CopyDescriptorsSimple(1, ShdrVisibleSamplerHeapCPUDescriptorHandle, SrcSampler.CPUDescriptorHandle, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
                }
            }
            else
            {
                VERIFY_EXPR(DstSampler.pObject == SrcSampler.pObject);
                VERIFY_EXPR(DstSampler.Type == SrcSampler.Type);
                VERIFY_EXPR(DstSampler.CPUDescriptorHandle.ptr == SrcSampler.CPUDescriptorHandle.ptr);
            }
        }
    }
}


#ifdef DILIGENT_DEVELOPMENT
bool ShaderResourceLayoutD3D12::dvpVerifyBindings(const ShaderResourceCacheD3D12& ResourceCache) const
{
    bool BindingsOK = true;
    for (SHADER_RESOURCE_VARIABLE_TYPE VarType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC; VarType < SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES; VarType = static_cast<SHADER_RESOURCE_VARIABLE_TYPE>(VarType + 1))
    {
        for (Uint32 r = 0; r < GetCbvSrvUavCount(VarType); ++r)
        {
            const auto& res = GetSrvCbvUav(VarType, r);
            VERIFY(res.GetVariableType() == VarType, "Unexpected variable type");

            for (Uint32 ArrInd = 0; ArrInd < res.Attribs.BindCount; ++ArrInd)
            {
                const auto& CachedRes = ResourceCache.GetRootTable(res.RootIndex).GetResource(res.OffsetFromTableStart + ArrInd, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_pResources->GetShaderType());
                if (CachedRes.pObject)
                    VERIFY(CachedRes.Type == res.GetResType(), "Inconsistent cached resource types");
                else
                    VERIFY(CachedRes.Type == CachedResourceType::Unknown, "Unexpected cached resource types");

                if (!CachedRes.pObject ||
                    // Dynamic buffers do not have CPU descriptor handle as they do not keep D3D12 buffer, and space is allocated from the GPU ring buffer
                    CachedRes.CPUDescriptorHandle.ptr == 0 && !(CachedRes.Type == CachedResourceType::CBV && CachedRes.pObject.RawPtr<const BufferD3D12Impl>()->GetDesc().Usage == USAGE_DYNAMIC))
                {
                    LOG_ERROR_MESSAGE("No resource is bound to ", GetShaderVariableTypeLiteralName(res.GetVariableType()), " variable '", res.Attribs.GetPrintName(ArrInd), "' in shader '", GetShaderName(), "'");
                    BindingsOK = false;
                }

                if (res.Attribs.BindCount > 1 && res.ValidSamplerAssigned())
                {
                    // Verify that if single sampler is used for all texture array elements, all samplers set in the resource views are consistent
                    const auto& SamInfo = GetAssignedSampler(res);
                    if (SamInfo.Attribs.BindCount == 1)
                    {
                        const auto& CachedSampler = ResourceCache.GetRootTable(SamInfo.RootIndex).GetResource(SamInfo.OffsetFromTableStart, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, m_pResources->GetShaderType());
                        // Conversion must always succeed as the type is verified when resource is bound to the variable
                        if (const auto* pTexView = CachedRes.pObject.RawPtr<const TextureViewD3D12Impl>())
                        {
                            const auto* pSampler = pTexView->GetSampler();
                            if (pSampler != nullptr && CachedSampler.pObject != nullptr && CachedSampler.pObject != pSampler)
                            {
                                LOG_ERROR_MESSAGE("All elements of texture array '", res.Attribs.Name, "' in shader '", GetShaderName(), "' share the same sampler. However, the sampler set in view for element ", ArrInd, " does not match bound sampler. This may cause incorrect behavior on GL platform.");
                            }
                        }
                    }
                }

#    ifdef DILIGENT_DEBUG
                {
                    const auto ShdrVisibleHeapCPUDescriptorHandle = ResourceCache.GetShaderVisibleTableCPUDescriptorHandle<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>(res.RootIndex, res.OffsetFromTableStart + ArrInd);
                    if (ResourceCache.DbgGetContentType() == ShaderResourceCacheD3D12::DbgCacheContentType::StaticShaderResources)
                    {
                        VERIFY(ShdrVisibleHeapCPUDescriptorHandle.ptr == 0, "Static shader resources of a shader should not be assigned shader visible descriptor space");
                    }
                    else if (ResourceCache.DbgGetContentType() == ShaderResourceCacheD3D12::DbgCacheContentType::SRBResources)
                    {
                        if (res.GetResType() == CachedResourceType::CBV && res.Attribs.BindCount == 1)
                        {
                            VERIFY(ShdrVisibleHeapCPUDescriptorHandle.ptr == 0, "Non-array constant buffers are bound as root views and should not be assigned shader visible descriptor space");
                        }
                        else
                        {
                            if (res.GetVariableType() == SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC)
                                VERIFY(ShdrVisibleHeapCPUDescriptorHandle.ptr == 0, "Dynamic resources of a shader resource binding should be assigned shader visible descriptor space at every draw call");
                            else
                                VERIFY(ShdrVisibleHeapCPUDescriptorHandle.ptr != 0, "Non-dynamics resources of a shader resource binding must be assigned shader visible descriptor space");
                        }
                    }
                    else
                    {
                        UNEXPECTED("Unknown content type");
                    }
                }
#    endif
            }

            if (res.ValidSamplerAssigned())
            {
                VERIFY(res.GetResType() == CachedResourceType::TexSRV, "Sampler can only be assigned to a texture SRV");
                const auto& SamInfo = GetAssignedSampler(res);
                //VERIFY(!SamInfo.Attribs.IsImmutableSampler(), "Immutable samplers should never be assigned space in the cache" );
                VERIFY(SamInfo.Attribs.IsValidBindPoint(), "Sampler bind point must be valid");

                for (Uint32 ArrInd = 0; ArrInd < SamInfo.Attribs.BindCount; ++ArrInd)
                {
                    const auto& CachedSampler = ResourceCache.GetRootTable(SamInfo.RootIndex).GetResource(SamInfo.OffsetFromTableStart + ArrInd, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, m_pResources->GetShaderType());
                    if (CachedSampler.pObject)
                        VERIFY(CachedSampler.Type == CachedResourceType::Sampler, "Incorrect cached sampler type");
                    else
                        VERIFY(CachedSampler.Type == CachedResourceType::Unknown, "Unexpected cached sampler type");
                    if (!CachedSampler.pObject || CachedSampler.CPUDescriptorHandle.ptr == 0)
                    {
                        LOG_ERROR_MESSAGE("No sampler is assigned to texture variable '", res.Attribs.GetPrintName(ArrInd), "' in shader '", GetShaderName(), "'");
                        BindingsOK = false;
                    }

#    ifdef DILIGENT_DEBUG
                    {
                        const auto ShdrVisibleHeapCPUDescriptorHandle = ResourceCache.GetShaderVisibleTableCPUDescriptorHandle<D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER>(SamInfo.RootIndex, SamInfo.OffsetFromTableStart + ArrInd);
                        if (ResourceCache.DbgGetContentType() == ShaderResourceCacheD3D12::DbgCacheContentType::StaticShaderResources)
                        {
                            VERIFY(ShdrVisibleHeapCPUDescriptorHandle.ptr == 0, "Static shader resources of a shader should not be assigned shader visible descriptor space");
                        }
                        else if (ResourceCache.DbgGetContentType() == ShaderResourceCacheD3D12::DbgCacheContentType::SRBResources)
                        {
                            if (SamInfo.GetVariableType() == SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC)
                                VERIFY(ShdrVisibleHeapCPUDescriptorHandle.ptr == 0, "Dynamic resources of a shader resource binding should be assigned shader visible descriptor space at every draw call");
                            else
                                VERIFY(ShdrVisibleHeapCPUDescriptorHandle.ptr != 0, "Non-dynamics resources of a shader resource binding must be assigned shader visible descriptor space");
                        }
                        else
                        {
                            UNEXPECTED("Unknown content type");
                        }
                    }
#    endif
                }
            }
        }

        for (Uint32 s = 0; s < GetSamplerCount(VarType); ++s)
        {
            const auto& sam = GetSampler(VarType, s);
            VERIFY(sam.GetVariableType() == VarType, "Unexpected sampler variable type");

            for (Uint32 ArrInd = 0; ArrInd < sam.Attribs.BindCount; ++ArrInd)
            {
                const auto& CachedSampler = ResourceCache.GetRootTable(sam.RootIndex).GetResource(sam.OffsetFromTableStart + ArrInd, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, m_pResources->GetShaderType());
                if (CachedSampler.pObject)
                    VERIFY(CachedSampler.Type == CachedResourceType::Sampler, "Incorrect cached sampler type");
                else
                    VERIFY(CachedSampler.Type == CachedResourceType::Unknown, "Unexpected cached sampler type");
                if (!CachedSampler.pObject || CachedSampler.CPUDescriptorHandle.ptr == 0)
                {
                    LOG_ERROR_MESSAGE("No sampler is bound to sampler variable '", sam.Attribs.GetPrintName(ArrInd), "' in shader '", GetShaderName(), "'");
                    BindingsOK = false;
                }
            }
        }
    }

    return BindingsOK;
}
#endif

} // namespace Diligent
