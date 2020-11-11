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
#include <array>
#include "PipelineStateD3D11Impl.hpp"
#include "RenderDeviceD3D11Impl.hpp"
#include "ShaderResourceBindingD3D11Impl.hpp"
#include "EngineMemory.h"

namespace Diligent
{

template <typename PSOCreateInfoType>
void PipelineStateD3D11Impl::InitInternalObjects(const PSOCreateInfoType& CreateInfo)
{
    m_ResourceLayoutIndex.fill(-1);

    std::vector<std::pair<SHADER_TYPE, ShaderD3D11Impl*>> ShaderStages;
    ExtractShaders<ShaderD3D11Impl>(CreateInfo, ShaderStages);

    const auto NumShaderStages = GetNumShaderStages();
    VERIFY_EXPR(NumShaderStages > 0 && NumShaderStages == ShaderStages.size());

    LinearAllocator MemPool{GetRawAllocator()};

    MemPool.AddSpace<ShaderResourceCacheD3D11>(NumShaderStages);
    MemPool.AddSpace<ShaderResourceLayoutD3D11>(NumShaderStages);

    ReserveSpaceForPipelineDesc(CreateInfo, MemPool);

    MemPool.Reserve();

    m_pStaticResourceCaches = MemPool.ConstructArray<ShaderResourceCacheD3D11>(NumShaderStages);

    // The memory is now owned by PipelineStateD3D11Impl and will be freed by Destruct().
    auto* Ptr = MemPool.ReleaseOwnership();
    VERIFY_EXPR(Ptr == m_pStaticResourceCaches);
    (void)Ptr;

    m_pStaticResourceLayouts = MemPool.Allocate<ShaderResourceLayoutD3D11>(NumShaderStages);
    for (Uint32 i = 0; i < NumShaderStages; ++i)
        new (m_pStaticResourceLayouts + i) ShaderResourceLayoutD3D11{*this, m_pStaticResourceCaches[i]}; // noexcept

    InitializePipelineDesc(CreateInfo, MemPool);

    // It is important to construct all objects before initializing them because if an exception is thrown,
    // destructors will be called for all objects

    InitResourceLayouts(CreateInfo, ShaderStages);
}


PipelineStateD3D11Impl::PipelineStateD3D11Impl(IReferenceCounters*                    pRefCounters,
                                               RenderDeviceD3D11Impl*                 pRenderDeviceD3D11,
                                               const GraphicsPipelineStateCreateInfo& CreateInfo) :
    // clang-format off
    TPipelineStateBase
    {
        pRefCounters,
        pRenderDeviceD3D11,
        CreateInfo
    },
    m_SRBMemAllocator{GetRawAllocator()},
    m_ImmutableSamplers (STD_ALLOCATOR_RAW_MEM(ImmutableSamplerInfo, GetRawAllocator(), "Allocator for vector<ImmutableSamplerInfo>"))
// clang-format on
{
    try
    {
        InitInternalObjects(CreateInfo);

        auto& GraphicsPipeline = GetGraphicsPipelineDesc();

#define INIT_SHADER(ShortName, ExpectedType)                                                                                                                                         \
    do                                                                                                                                                                               \
    {                                                                                                                                                                                \
        auto* pShader  = ValidatedCast<ShaderD3D11Impl>(CreateInfo.p##ShortName);                                                                                                    \
        m_p##ShortName = pShader;                                                                                                                                                    \
        if (m_p##ShortName && m_p##ShortName->GetDesc().ShaderType != ExpectedType)                                                                                                  \
        {                                                                                                                                                                            \
            LOG_ERROR_AND_THROW(GetShaderTypeLiteralName(ExpectedType), " shader is expeceted while ", GetShaderTypeLiteralName(m_p##ShortName->GetDesc().ShaderType), " provided"); \
        }                                                                                                                                                                            \
        if (pShader != nullptr)                                                                                                                                                      \
            HashCombine(m_ShaderResourceLayoutHash, pShader->GetD3D11Resources()->GetHash());                                                                                        \
    } while (false)

        INIT_SHADER(VS, SHADER_TYPE_VERTEX);
        INIT_SHADER(PS, SHADER_TYPE_PIXEL);
        INIT_SHADER(GS, SHADER_TYPE_GEOMETRY);
        INIT_SHADER(DS, SHADER_TYPE_DOMAIN);
        INIT_SHADER(HS, SHADER_TYPE_HULL);
#undef INIT_SHADER

        if (m_pVS == nullptr)
        {
            LOG_ERROR_AND_THROW("Vertex shader is null");
        }

        auto* pDeviceD3D11 = pRenderDeviceD3D11->GetD3D11Device();

        D3D11_BLEND_DESC D3D11BSDesc = {};
        BlendStateDesc_To_D3D11_BLEND_DESC(GraphicsPipeline.BlendDesc, D3D11BSDesc);
        CHECK_D3D_RESULT_THROW(pDeviceD3D11->CreateBlendState(&D3D11BSDesc, &m_pd3d11BlendState),
                               "Failed to create D3D11 blend state object");

        D3D11_RASTERIZER_DESC D3D11RSDesc = {};
        RasterizerStateDesc_To_D3D11_RASTERIZER_DESC(GraphicsPipeline.RasterizerDesc, D3D11RSDesc);
        CHECK_D3D_RESULT_THROW(pDeviceD3D11->CreateRasterizerState(&D3D11RSDesc, &m_pd3d11RasterizerState),
                               "Failed to create D3D11 rasterizer state");

        D3D11_DEPTH_STENCIL_DESC D3D11DSSDesc = {};
        DepthStencilStateDesc_To_D3D11_DEPTH_STENCIL_DESC(GraphicsPipeline.DepthStencilDesc, D3D11DSSDesc);
        CHECK_D3D_RESULT_THROW(pDeviceD3D11->CreateDepthStencilState(&D3D11DSSDesc, &m_pd3d11DepthStencilState),
                               "Failed to create D3D11 depth stencil state");

        // Create input layout
        const auto& InputLayout = GraphicsPipeline.InputLayout;
        if (InputLayout.NumElements > 0)
        {
            std::vector<D3D11_INPUT_ELEMENT_DESC, STDAllocatorRawMem<D3D11_INPUT_ELEMENT_DESC>> d311InputElements(STD_ALLOCATOR_RAW_MEM(D3D11_INPUT_ELEMENT_DESC, GetRawAllocator(), "Allocator for vector<D3D11_INPUT_ELEMENT_DESC>"));
            LayoutElements_To_D3D11_INPUT_ELEMENT_DESCs(InputLayout, d311InputElements);

            ID3DBlob* pVSByteCode = m_pVS.RawPtr<ShaderD3D11Impl>()->GetBytecode();
            if (!pVSByteCode)
                LOG_ERROR_AND_THROW("Vertex Shader byte code does not exist");

            CHECK_D3D_RESULT_THROW(pDeviceD3D11->CreateInputLayout(d311InputElements.data(), static_cast<UINT>(d311InputElements.size()), pVSByteCode->GetBufferPointer(), pVSByteCode->GetBufferSize(), &m_pd3d11InputLayout),
                                   "Failed to create the Direct3D11 input layout");
        }
    }
    catch (...)
    {
        Destruct();
        throw;
    }
}

PipelineStateD3D11Impl::PipelineStateD3D11Impl(IReferenceCounters*                   pRefCounters,
                                               RenderDeviceD3D11Impl*                pRenderDeviceD3D11,
                                               const ComputePipelineStateCreateInfo& CreateInfo) :
    // clang-format off
    TPipelineStateBase
    {
        pRefCounters,
        pRenderDeviceD3D11,
        CreateInfo
    },
    m_SRBMemAllocator{GetRawAllocator()},
    m_ImmutableSamplers(STD_ALLOCATOR_RAW_MEM(ImmutableSamplerInfo, GetRawAllocator(), "Allocator for vector<ImmutableSamplerInfo>"))
// clang-format on
{
    try
    {
        InitInternalObjects(CreateInfo);

        m_pCS = ValidatedCast<ShaderD3D11Impl>(CreateInfo.pCS);
        if (m_pCS == nullptr)
        {
            LOG_ERROR_AND_THROW("Compute shader is null");
        }

        if (m_pCS->GetDesc().ShaderType != SHADER_TYPE_COMPUTE)
        {
            LOG_ERROR_AND_THROW(GetShaderTypeLiteralName(SHADER_TYPE_COMPUTE), " shader is expeceted while ", GetShaderTypeLiteralName(m_pCS->GetDesc().ShaderType), " provided");
        }
        m_ShaderResourceLayoutHash = m_pCS->GetD3D11Resources()->GetHash();
    }
    catch (...)
    {
        Destruct();
        throw;
    }
}

PipelineStateD3D11Impl::~PipelineStateD3D11Impl()
{
    Destruct();
}

void PipelineStateD3D11Impl::Destruct()
{
    if (m_pStaticResourceLayouts != nullptr)
    {
        for (Uint32 l = 0; l < GetNumShaderStages(); ++l)
        {
            m_pStaticResourceLayouts[l].~ShaderResourceLayoutD3D11();
        }
    }

    if (m_pStaticResourceCaches != nullptr)
    {
        for (Uint32 s = 0; s < GetNumShaderStages(); ++s)
        {
            m_pStaticResourceCaches[s].Destroy(GetRawAllocator());
            m_pStaticResourceCaches[s].~ShaderResourceCacheD3D11();
        }
    }

    // All subobjects are allocated in contiguous chunks of memory.
    if (auto* pRawMem = m_pStaticResourceCaches)
        GetRawAllocator().Free(pRawMem);
}

IMPLEMENT_QUERY_INTERFACE(PipelineStateD3D11Impl, IID_PipelineStateD3D11, TPipelineStateBase)


void PipelineStateD3D11Impl::InitResourceLayouts(const PipelineStateCreateInfo&                               CreateInfo,
                                                 const std::vector<std::pair<SHADER_TYPE, ShaderD3D11Impl*>>& ShaderStages)
{
    const auto& ResourceLayout = m_Desc.ResourceLayout;

#ifdef DILIGENT_DEVELOPMENT
    {
        const ShaderResources* pResources[MAX_SHADERS_IN_PIPELINE] = {};
        for (Uint32 s = 0; s < ShaderStages.size(); ++s)
        {
            auto* pShader = ShaderStages[s].second;
            pResources[s] = &(*pShader->GetD3D11Resources());
        }
        ShaderResources::DvpVerifyResourceLayout(ResourceLayout, pResources, GetNumShaderStages(),
                                                 (CreateInfo.Flags & PSO_CREATE_FLAG_IGNORE_MISSING_VARIABLES) == 0,
                                                 (CreateInfo.Flags & PSO_CREATE_FLAG_IGNORE_MISSING_IMMUTABLE_SAMPLERS) == 0);
    }
#endif

    decltype(m_ImmutableSamplers) ImmutableSamplers{STD_ALLOCATOR_RAW_MEM(ImmutableSamplerInfo, GetRawAllocator(), "Allocator for vector<ImmutableSamplerInfo>")};

    std::array<size_t, MAX_SHADERS_IN_PIPELINE> ShaderResLayoutDataSizes = {};
    std::array<size_t, MAX_SHADERS_IN_PIPELINE> ShaderResCacheDataSizes  = {};
    for (Uint32 s = 0; s < ShaderStages.size(); ++s)
    {
        const auto* pShader    = ShaderStages[s].second;
        const auto& ShaderDesc = pShader->GetDesc();
        const auto& Resources  = *pShader->GetD3D11Resources();
        VERIFY_EXPR(ShaderDesc.ShaderType == Resources.GetShaderType());

        // The cache will be initialized by the resource layout

        // Shader resource layout will only contain dynamic and mutable variables
        const SHADER_RESOURCE_VARIABLE_TYPE StaticVarTypes[] = {SHADER_RESOURCE_VARIABLE_TYPE_STATIC};
        m_pStaticResourceLayouts[s].Initialize(
            pShader->GetD3D11Resources(),
            m_Desc.ResourceLayout,
            StaticVarTypes,
            _countof(StaticVarTypes),
            GetRawAllocator(),
            GetRawAllocator() //
        );

        // Initialize immutable samplers
        for (Uint32 sam = 0; sam < Resources.GetNumSamplers(); ++sam)
        {
            const auto&    SamplerAttribs            = Resources.GetSampler(sam);
            constexpr bool LogImtblSamplerArrayError = true;
            auto           SrcImtblSamplerInd        = Resources.FindImmutableSampler(SamplerAttribs, ResourceLayout, LogImtblSamplerArrayError);
            if (SrcImtblSamplerInd >= 0)
            {
                const auto& SrcImtblSamplerInfo = ResourceLayout.ImmutableSamplers[SrcImtblSamplerInd];

                RefCntAutoPtr<ISampler> pImbtlSampler;
                GetDevice()->CreateSampler(SrcImtblSamplerInfo.Desc, &pImbtlSampler);
                ImmutableSamplers.emplace_back(SamplerAttribs, std::move(pImbtlSampler));
            }
        }
        m_ImmutableSamplerOffsets[s + 1] = static_cast<Uint16>(ImmutableSamplers.size());

        if (m_Desc.SRBAllocationGranularity > 1)
        {
            const SHADER_RESOURCE_VARIABLE_TYPE SRBVarTypes[] = //
                {
                    SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE,
                    SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC //
                };
            ShaderResLayoutDataSizes[s] = ShaderResourceLayoutD3D11::GetRequiredMemorySize(Resources, ResourceLayout, SRBVarTypes, _countof(SRBVarTypes));
            ShaderResCacheDataSizes[s]  = ShaderResourceCacheD3D11::GetRequriedMemorySize(Resources);
        }

        auto ShaderInd                   = GetShaderTypePipelineIndex(ShaderDesc.ShaderType, m_Desc.PipelineType);
        m_ResourceLayoutIndex[ShaderInd] = static_cast<Int8>(s);
    }

    if (m_Desc.SRBAllocationGranularity > 1)
    {
        m_SRBMemAllocator.Initialize(m_Desc.SRBAllocationGranularity, GetNumShaderStages(), ShaderResLayoutDataSizes.data(), GetNumShaderStages(), ShaderResCacheDataSizes.data());
    }

    m_ImmutableSamplers.reserve(ImmutableSamplers.size());
    for (auto& ImtblSam : ImmutableSamplers)
        m_ImmutableSamplers.emplace_back(std::move(ImtblSam));

    for (Uint32 s = 0; s < GetNumShaderStages(); ++s)
    {
        // Initialize immutable samplers in the static resource cache to avoid warning messages
        SetImmutableSamplers(m_pStaticResourceCaches[s], s);
    }
}

ID3D11BlendState* PipelineStateD3D11Impl::GetD3D11BlendState()
{
    return m_pd3d11BlendState;
}

ID3D11RasterizerState* PipelineStateD3D11Impl::GetD3D11RasterizerState()
{
    return m_pd3d11RasterizerState;
}

ID3D11DepthStencilState* PipelineStateD3D11Impl::GetD3D11DepthStencilState()
{
    return m_pd3d11DepthStencilState;
}

ID3D11InputLayout* PipelineStateD3D11Impl::GetD3D11InputLayout()
{
    return m_pd3d11InputLayout;
}

void PipelineStateD3D11Impl::CreateShaderResourceBinding(IShaderResourceBinding** ppShaderResourceBinding, bool InitStaticResources)
{
    auto& SRBAllocator      = GetDevice()->GetSRBAllocator();
    auto  pShaderResBinding = NEW_RC_OBJ(SRBAllocator, "ShaderResourceBindingD3D11Impl instance", ShaderResourceBindingD3D11Impl)(this, false);
    if (InitStaticResources)
        pShaderResBinding->InitializeStaticResources(nullptr);
    pShaderResBinding->QueryInterface(IID_ShaderResourceBinding, reinterpret_cast<IObject**>(static_cast<IShaderResourceBinding**>(ppShaderResourceBinding)));
}

bool PipelineStateD3D11Impl::IsCompatibleWith(const IPipelineState* pPSO) const
{
    VERIFY_EXPR(pPSO != nullptr);

    if (pPSO == this)
        return true;

    const PipelineStateD3D11Impl* pPSOD3D11 = ValidatedCast<const PipelineStateD3D11Impl>(pPSO);
    if (m_ShaderResourceLayoutHash != pPSOD3D11->m_ShaderResourceLayoutHash)
        return false;

    if (GetNumShaderStages() != pPSOD3D11->GetNumShaderStages())
        return false;

    for (Uint32 s = 0; s < GetNumShaderStages(); ++s)
    {
        auto* pShader0 = GetShader(s);
        auto* pShader1 = pPSOD3D11->GetShader(s);
        if (pShader0->GetDesc().ShaderType != pShader1->GetDesc().ShaderType)
            return false;
        const auto& Res0 = *pShader0->GetD3D11Resources();
        const auto& Res1 = *pShader1->GetD3D11Resources();
        if (!Res0.IsCompatibleWith(Res1))
            return false;
    }

    return true;
}

ID3D11VertexShader* PipelineStateD3D11Impl::GetD3D11VertexShader()
{
    if (!m_pVS) return nullptr;
    auto* pVSD3D11 = m_pVS.RawPtr<ShaderD3D11Impl>();
    return static_cast<ID3D11VertexShader*>(pVSD3D11->GetD3D11Shader());
}

ID3D11PixelShader* PipelineStateD3D11Impl::GetD3D11PixelShader()
{
    if (!m_pPS) return nullptr;
    auto* pPSD3D11 = m_pPS.RawPtr<ShaderD3D11Impl>();
    return static_cast<ID3D11PixelShader*>(pPSD3D11->GetD3D11Shader());
}

ID3D11GeometryShader* PipelineStateD3D11Impl::GetD3D11GeometryShader()
{
    if (!m_pGS) return nullptr;
    auto* pGSD3D11 = m_pGS.RawPtr<ShaderD3D11Impl>();
    return static_cast<ID3D11GeometryShader*>(pGSD3D11->GetD3D11Shader());
}

ID3D11DomainShader* PipelineStateD3D11Impl::GetD3D11DomainShader()
{
    if (!m_pDS) return nullptr;
    auto* pDSD3D11 = m_pDS.RawPtr<ShaderD3D11Impl>();
    return static_cast<ID3D11DomainShader*>(pDSD3D11->GetD3D11Shader());
}

ID3D11HullShader* PipelineStateD3D11Impl::GetD3D11HullShader()
{
    if (!m_pHS) return nullptr;
    auto* pHSD3D11 = m_pHS.RawPtr<ShaderD3D11Impl>();
    return static_cast<ID3D11HullShader*>(pHSD3D11->GetD3D11Shader());
}

ID3D11ComputeShader* PipelineStateD3D11Impl::GetD3D11ComputeShader()
{
    if (!m_pCS) return nullptr;
    auto* pCSD3D11 = m_pCS.RawPtr<ShaderD3D11Impl>();
    return static_cast<ID3D11ComputeShader*>(pCSD3D11->GetD3D11Shader());
}


void PipelineStateD3D11Impl::BindStaticResources(Uint32 ShaderFlags, IResourceMapping* pResourceMapping, Uint32 Flags)
{
    for (Uint32 s = 0; s < GetNumShaderStages(); ++s)
    {
        auto& StaticResLayout = m_pStaticResourceLayouts[s];
        if ((ShaderFlags & StaticResLayout.GetShaderType()) != 0)
            StaticResLayout.BindResources(pResourceMapping, Flags, m_pStaticResourceCaches[s]);
    }
}

Uint32 PipelineStateD3D11Impl::GetStaticVariableCount(SHADER_TYPE ShaderType) const
{
    const auto LayoutInd = GetStaticVariableCountHelper(ShaderType, m_ResourceLayoutIndex);
    if (LayoutInd < 0)
        return 0;

    VERIFY_EXPR(static_cast<Uint32>(LayoutInd) <= GetNumShaderStages());
    return m_pStaticResourceLayouts[LayoutInd].GetTotalResourceCount();
}

IShaderResourceVariable* PipelineStateD3D11Impl::GetStaticVariableByName(SHADER_TYPE ShaderType, const Char* Name)
{
    const auto LayoutInd = GetStaticVariableByNameHelper(ShaderType, Name, m_ResourceLayoutIndex);
    if (LayoutInd < 0)
        return nullptr;

    VERIFY_EXPR(static_cast<Uint32>(LayoutInd) <= GetNumShaderStages());
    return m_pStaticResourceLayouts[LayoutInd].GetShaderVariable(Name);
}

IShaderResourceVariable* PipelineStateD3D11Impl::GetStaticVariableByIndex(SHADER_TYPE ShaderType, Uint32 Index)
{
    const auto LayoutInd = GetStaticVariableByIndexHelper(ShaderType, Index, m_ResourceLayoutIndex);
    if (LayoutInd < 0)
        return nullptr;

    VERIFY_EXPR(static_cast<Uint32>(LayoutInd) <= GetNumShaderStages());
    return m_pStaticResourceLayouts[LayoutInd].GetShaderVariable(Index);
}

void PipelineStateD3D11Impl::SetImmutableSamplers(ShaderResourceCacheD3D11& ResourceCache, Uint32 ShaderInd) const
{
    auto NumCachedSamplers = ResourceCache.GetSamplerCount();
    for (Uint32 s = m_ImmutableSamplerOffsets[ShaderInd]; s < m_ImmutableSamplerOffsets[ShaderInd + 1]; ++s)
    {
        auto&       SamplerInfo       = m_ImmutableSamplers[s];
        const auto& SamAttribs        = SamplerInfo.Attribs;
        auto*       pSamplerD3D11Impl = SamplerInfo.pSampler.RawPtr<SamplerD3D11Impl>();
        // Limiting EndBindPoint is required when initializing immutable samplers in a Shader's static cache
        auto EndBindPoint = std::min(static_cast<Uint32>(SamAttribs.BindPoint) + SamAttribs.BindCount, NumCachedSamplers);
        for (Uint32 BindPoint = SamAttribs.BindPoint; BindPoint < EndBindPoint; ++BindPoint)
            ResourceCache.SetSampler(BindPoint, pSamplerD3D11Impl);
    }
}

const ShaderD3D11Impl* PipelineStateD3D11Impl::GetShaderByType(SHADER_TYPE ShaderType) const
{
    switch (ShaderType)
    {
        // clang-format off
        case SHADER_TYPE_VERTEX:   return m_pVS;
        case SHADER_TYPE_PIXEL:    return m_pPS;
        case SHADER_TYPE_GEOMETRY: return m_pGS;
        case SHADER_TYPE_HULL:     return m_pHS;
        case SHADER_TYPE_DOMAIN:   return m_pDS;
        case SHADER_TYPE_COMPUTE:  return m_pCS;
        default:                   UNEXPECTED("unsupported shader type"); return nullptr;
            // clang-format on
    }
}

const ShaderD3D11Impl* PipelineStateD3D11Impl::GetShader(Uint32 Index) const
{
    if (Index < GetNumShaderStages())
        return GetShaderByType(GetShaderStageType(Index));

    UNEXPECTED("Shader index is out of range");
    return nullptr;
}

} // namespace Diligent
