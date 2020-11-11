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
#include "PipelineStateD3D12Impl.hpp"
#include "ShaderD3D12Impl.hpp"
#include "D3D12TypeConversions.hpp"
#include "RenderDeviceD3D12Impl.hpp"
#include "DXGITypeConversions.hpp"
#include "ShaderResourceBindingD3D12Impl.hpp"
#include "CommandContext.hpp"
#include "EngineMemory.h"
#include "StringTools.hpp"
#include "ShaderVariableD3D12.hpp"

namespace Diligent
{
namespace
{
#ifdef _MSC_VER
#    pragma warning(push)
#    pragma warning(disable : 4324) //  warning C4324: structure was padded due to alignment specifier
#endif

template <typename InnerStructType, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE SubObjType>
struct alignas(void*) PSS_SubObject
{
    const D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type{SubObjType};
    InnerStructType                           Obj{};

    PSS_SubObject() {}

    PSS_SubObject& operator=(const InnerStructType& obj)
    {
        Obj = obj;
        return *this;
    }

    InnerStructType* operator->() { return &Obj; }
    InnerStructType* operator&() { return &Obj; }
    InnerStructType& operator*() { return Obj; }
};

#ifdef _MSC_VER
#    pragma warning(pop)
#endif

} // namespace

class PrimitiveTopology_To_D3D12_PRIMITIVE_TOPOLOGY_TYPE
{
public:
    PrimitiveTopology_To_D3D12_PRIMITIVE_TOPOLOGY_TYPE()
    {
        // clang-format off
        m_Map[PRIMITIVE_TOPOLOGY_UNDEFINED]      = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
        m_Map[PRIMITIVE_TOPOLOGY_TRIANGLE_LIST]  = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        m_Map[PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP] = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        m_Map[PRIMITIVE_TOPOLOGY_POINT_LIST]     = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
        m_Map[PRIMITIVE_TOPOLOGY_LINE_LIST]      = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        m_Map[PRIMITIVE_TOPOLOGY_LINE_STRIP]     = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        // clang-format on
        for (int t = static_cast<int>(PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST); t < static_cast<int>(PRIMITIVE_TOPOLOGY_NUM_TOPOLOGIES); ++t)
            m_Map[t] = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
    }

    D3D12_PRIMITIVE_TOPOLOGY_TYPE operator[](PRIMITIVE_TOPOLOGY Topology) const
    {
        return m_Map[static_cast<int>(Topology)];
    }

private:
    std::array<D3D12_PRIMITIVE_TOPOLOGY_TYPE, PRIMITIVE_TOPOLOGY_NUM_TOPOLOGIES> m_Map;
};

template <typename PSOCreateInfoType>
void PipelineStateD3D12Impl::InitInternalObjects(const PSOCreateInfoType&                   CreateInfo,
                                                 std::vector<D3D12PipelineShaderStageInfo>& ShaderStages)
{
    m_ResourceLayoutIndex.fill(-1);

    ExtractShaders<ShaderD3D12Impl>(CreateInfo, ShaderStages);

    LinearAllocator MemPool{GetRawAllocator()};

    const auto NumShaderStages = GetNumShaderStages();
    VERIFY_EXPR(NumShaderStages > 0 && NumShaderStages == ShaderStages.size());

    MemPool.AddSpace<ShaderResourceCacheD3D12>(NumShaderStages);
    MemPool.AddSpace<ShaderResourceLayoutD3D12>(NumShaderStages * 2);
    MemPool.AddSpace<ShaderVariableManagerD3D12>(NumShaderStages);

    ReserveSpaceForPipelineDesc(CreateInfo, MemPool);

    MemPool.Reserve();

    m_pStaticResourceCaches = MemPool.ConstructArray<ShaderResourceCacheD3D12>(NumShaderStages, ShaderResourceCacheD3D12::DbgCacheContentType::StaticShaderResources);

    // The memory is now owned by PipelineStateD3D12Impl and will be freed by Destruct().
    auto* Ptr = MemPool.ReleaseOwnership();
    VERIFY_EXPR(Ptr == m_pStaticResourceCaches);
    (void)Ptr;

    m_pShaderResourceLayouts = MemPool.ConstructArray<ShaderResourceLayoutD3D12>(NumShaderStages * 2, std::ref(*this));

    m_pStaticVarManagers = MemPool.Allocate<ShaderVariableManagerD3D12>(NumShaderStages);
    for (Uint32 s = 0; s < NumShaderStages; ++s)
        new (m_pStaticVarManagers + s) ShaderVariableManagerD3D12{*this, GetStaticShaderResCache(s)};

    InitializePipelineDesc(CreateInfo, MemPool);

    m_RootSig.AllocateImmutableSamplers(CreateInfo.PSODesc.ResourceLayout);

    // It is important to construct all objects before initializing them because if an exception is thrown,
    // destructors will be called for all objects

    InitResourceLayouts(CreateInfo, ShaderStages);
}


PipelineStateD3D12Impl::PipelineStateD3D12Impl(IReferenceCounters*                    pRefCounters,
                                               RenderDeviceD3D12Impl*                 pDeviceD3D12,
                                               const GraphicsPipelineStateCreateInfo& CreateInfo) :
    TPipelineStateBase{pRefCounters, pDeviceD3D12, CreateInfo.PSODesc},
    m_SRBMemAllocator{GetRawAllocator()}
{
    try
    {
        std::vector<D3D12PipelineShaderStageInfo> ShaderStages;

        InitInternalObjects(CreateInfo, ShaderStages);

        auto pd3d12Device = pDeviceD3D12->GetD3D12Device();
        if (m_Desc.PipelineType == PIPELINE_TYPE_GRAPHICS)
        {
            const auto& GraphicsPipeline = GetGraphicsPipelineDesc();

            D3D12_GRAPHICS_PIPELINE_STATE_DESC d3d12PSODesc = {};

            for (const auto& Stage : ShaderStages)
            {
                auto* pShaderD3D12 = Stage.pShader;
                auto  ShaderType   = pShaderD3D12->GetDesc().ShaderType;
                VERIFY_EXPR(ShaderType == Stage.Type);

                D3D12_SHADER_BYTECODE* pd3d12ShaderBytecode = nullptr;
                switch (ShaderType)
                {
                        // clang-format off
                case SHADER_TYPE_VERTEX:   pd3d12ShaderBytecode = &d3d12PSODesc.VS; break;
                case SHADER_TYPE_PIXEL:    pd3d12ShaderBytecode = &d3d12PSODesc.PS; break;
                case SHADER_TYPE_GEOMETRY: pd3d12ShaderBytecode = &d3d12PSODesc.GS; break;
                case SHADER_TYPE_HULL:     pd3d12ShaderBytecode = &d3d12PSODesc.HS; break;
                case SHADER_TYPE_DOMAIN:   pd3d12ShaderBytecode = &d3d12PSODesc.DS; break;
                    // clang-format on
                    default: UNEXPECTED("Unexpected shader type");
                }
                auto* pByteCode = pShaderD3D12->GetShaderByteCode();

                pd3d12ShaderBytecode->pShaderBytecode = pByteCode->GetBufferPointer();
                pd3d12ShaderBytecode->BytecodeLength  = pByteCode->GetBufferSize();
            }

            d3d12PSODesc.pRootSignature = m_RootSig.GetD3D12RootSignature();

            memset(&d3d12PSODesc.StreamOutput, 0, sizeof(d3d12PSODesc.StreamOutput));

            BlendStateDesc_To_D3D12_BLEND_DESC(GraphicsPipeline.BlendDesc, d3d12PSODesc.BlendState);
            // The sample mask for the blend state.
            d3d12PSODesc.SampleMask = GraphicsPipeline.SampleMask;

            RasterizerStateDesc_To_D3D12_RASTERIZER_DESC(GraphicsPipeline.RasterizerDesc, d3d12PSODesc.RasterizerState);
            DepthStencilStateDesc_To_D3D12_DEPTH_STENCIL_DESC(GraphicsPipeline.DepthStencilDesc, d3d12PSODesc.DepthStencilState);

            std::vector<D3D12_INPUT_ELEMENT_DESC, STDAllocatorRawMem<D3D12_INPUT_ELEMENT_DESC>> d312InputElements(STD_ALLOCATOR_RAW_MEM(D3D12_INPUT_ELEMENT_DESC, GetRawAllocator(), "Allocator for vector<D3D12_INPUT_ELEMENT_DESC>"));

            const auto& InputLayout = GetGraphicsPipelineDesc().InputLayout;
            if (InputLayout.NumElements > 0)
            {
                LayoutElements_To_D3D12_INPUT_ELEMENT_DESCs(InputLayout, d312InputElements);
                d3d12PSODesc.InputLayout.NumElements        = static_cast<UINT>(d312InputElements.size());
                d3d12PSODesc.InputLayout.pInputElementDescs = d312InputElements.data();
            }
            else
            {
                d3d12PSODesc.InputLayout.NumElements        = 0;
                d3d12PSODesc.InputLayout.pInputElementDescs = nullptr;
            }

            d3d12PSODesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
            static const PrimitiveTopology_To_D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimTopologyToD3D12TopologyType;
            d3d12PSODesc.PrimitiveTopologyType = PrimTopologyToD3D12TopologyType[GraphicsPipeline.PrimitiveTopology];

            d3d12PSODesc.NumRenderTargets = GraphicsPipeline.NumRenderTargets;
            for (Uint32 rt = 0; rt < GraphicsPipeline.NumRenderTargets; ++rt)
                d3d12PSODesc.RTVFormats[rt] = TexFormatToDXGI_Format(GraphicsPipeline.RTVFormats[rt]);
            for (Uint32 rt = GraphicsPipeline.NumRenderTargets; rt < _countof(d3d12PSODesc.RTVFormats); ++rt)
                d3d12PSODesc.RTVFormats[rt] = DXGI_FORMAT_UNKNOWN;
            d3d12PSODesc.DSVFormat = TexFormatToDXGI_Format(GraphicsPipeline.DSVFormat);

            d3d12PSODesc.SampleDesc.Count   = GraphicsPipeline.SmplDesc.Count;
            d3d12PSODesc.SampleDesc.Quality = GraphicsPipeline.SmplDesc.Quality;

            // For single GPU operation, set this to zero. If there are multiple GPU nodes,
            // set bits to identify the nodes (the device's physical adapters) for which the
            // graphics pipeline state is to apply. Each bit in the mask corresponds to a single node.
            d3d12PSODesc.NodeMask = 0;

            d3d12PSODesc.CachedPSO.pCachedBlob           = nullptr;
            d3d12PSODesc.CachedPSO.CachedBlobSizeInBytes = 0;

            // The only valid bit is D3D12_PIPELINE_STATE_FLAG_TOOL_DEBUG, which can only be set on WARP devices.
            d3d12PSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

            HRESULT hr = pd3d12Device->CreateGraphicsPipelineState(&d3d12PSODesc, __uuidof(ID3D12PipelineState), reinterpret_cast<void**>(static_cast<ID3D12PipelineState**>(&m_pd3d12PSO)));
            if (FAILED(hr))
                LOG_ERROR_AND_THROW("Failed to create pipeline state");
        }

#ifdef D3D12_H_HAS_MESH_SHADER
        else if (m_Desc.PipelineType == PIPELINE_TYPE_MESH)
        {
            const auto& GraphicsPipeline = GetGraphicsPipelineDesc();

            struct MESH_SHADER_PIPELINE_STATE_DESC
            {
                PSS_SubObject<D3D12_PIPELINE_STATE_FLAGS, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_FLAGS>            Flags;
                PSS_SubObject<UINT, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_NODE_MASK>                              NodeMask;
                PSS_SubObject<ID3D12RootSignature*, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE>         pRootSignature;
                PSS_SubObject<D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS>                    PS;
                PSS_SubObject<D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS>                    AS;
                PSS_SubObject<D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS>                    MS;
                PSS_SubObject<D3D12_BLEND_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND>                      BlendState;
                PSS_SubObject<D3D12_DEPTH_STENCIL_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL>      DepthStencilState;
                PSS_SubObject<D3D12_RASTERIZER_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER>            RasterizerState;
                PSS_SubObject<DXGI_SAMPLE_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC>                SampleDesc;
                PSS_SubObject<UINT, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK>                            SampleMask;
                PSS_SubObject<DXGI_FORMAT, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT>            DSVFormat;
                PSS_SubObject<D3D12_RT_FORMAT_ARRAY, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS> RTVFormatArray;
                PSS_SubObject<D3D12_CACHED_PIPELINE_STATE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CACHED_PSO>      CachedPSO;
            };
            MESH_SHADER_PIPELINE_STATE_DESC d3d12PSODesc = {};

            for (const auto& Stage : ShaderStages)
            {
                auto* pShaderD3D12 = Stage.pShader;
                auto  ShaderType   = pShaderD3D12->GetDesc().ShaderType;
                VERIFY_EXPR(ShaderType == Stage.Type);

                D3D12_SHADER_BYTECODE* pd3d12ShaderBytecode = nullptr;
                switch (ShaderType)
                {
                        // clang-format off
                case SHADER_TYPE_AMPLIFICATION: pd3d12ShaderBytecode = &d3d12PSODesc.AS; break;
                case SHADER_TYPE_MESH:          pd3d12ShaderBytecode = &d3d12PSODesc.MS; break;
                case SHADER_TYPE_PIXEL:         pd3d12ShaderBytecode = &d3d12PSODesc.PS; break;
                    // clang-format on
                    default: UNEXPECTED("Unexpected shader type");
                }
                auto* pByteCode = pShaderD3D12->GetShaderByteCode();

                pd3d12ShaderBytecode->pShaderBytecode = pByteCode->GetBufferPointer();
                pd3d12ShaderBytecode->BytecodeLength  = pByteCode->GetBufferSize();
            }

            d3d12PSODesc.pRootSignature = m_RootSig.GetD3D12RootSignature();

            BlendStateDesc_To_D3D12_BLEND_DESC(GraphicsPipeline.BlendDesc, *d3d12PSODesc.BlendState);
            d3d12PSODesc.SampleMask = GraphicsPipeline.SampleMask;

            RasterizerStateDesc_To_D3D12_RASTERIZER_DESC(GraphicsPipeline.RasterizerDesc, *d3d12PSODesc.RasterizerState);
            DepthStencilStateDesc_To_D3D12_DEPTH_STENCIL_DESC(GraphicsPipeline.DepthStencilDesc, *d3d12PSODesc.DepthStencilState);

            d3d12PSODesc.RTVFormatArray->NumRenderTargets = GraphicsPipeline.NumRenderTargets;
            for (Uint32 rt = 0; rt < GraphicsPipeline.NumRenderTargets; ++rt)
                d3d12PSODesc.RTVFormatArray->RTFormats[rt] = TexFormatToDXGI_Format(GraphicsPipeline.RTVFormats[rt]);
            for (Uint32 rt = GraphicsPipeline.NumRenderTargets; rt < _countof(d3d12PSODesc.RTVFormatArray->RTFormats); ++rt)
                d3d12PSODesc.RTVFormatArray->RTFormats[rt] = DXGI_FORMAT_UNKNOWN;
            d3d12PSODesc.DSVFormat = TexFormatToDXGI_Format(GraphicsPipeline.DSVFormat);

            d3d12PSODesc.SampleDesc->Count   = GraphicsPipeline.SmplDesc.Count;
            d3d12PSODesc.SampleDesc->Quality = GraphicsPipeline.SmplDesc.Quality;

            // For single GPU operation, set this to zero. If there are multiple GPU nodes,
            // set bits to identify the nodes (the device's physical adapters) for which the
            // graphics pipeline state is to apply. Each bit in the mask corresponds to a single node.
            d3d12PSODesc.NodeMask = 0;

            d3d12PSODesc.CachedPSO->pCachedBlob           = nullptr;
            d3d12PSODesc.CachedPSO->CachedBlobSizeInBytes = 0;

            // The only valid bit is D3D12_PIPELINE_STATE_FLAG_TOOL_DEBUG, which can only be set on WARP devices.
            d3d12PSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

            D3D12_PIPELINE_STATE_STREAM_DESC streamDesc;
            streamDesc.SizeInBytes                   = sizeof(d3d12PSODesc);
            streamDesc.pPipelineStateSubobjectStream = &d3d12PSODesc;

            auto* device2 = pDeviceD3D12->GetD3D12Device2();

            CHECK_D3D_RESULT_THROW(device2->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&m_pd3d12PSO)), "Failed to create pipeline state");
        }
#endif // D3D12_H_HAS_MESH_SHADER
        else
        {
            LOG_ERROR_AND_THROW("Unsupported pipeline type");
        }

        if (*m_Desc.Name != 0)
        {
            m_pd3d12PSO->SetName(WidenString(m_Desc.Name).c_str());
            String RootSignatureDesc("Root signature for PSO '");
            RootSignatureDesc.append(m_Desc.Name);
            RootSignatureDesc.push_back('\'');
            m_RootSig.GetD3D12RootSignature()->SetName(WidenString(RootSignatureDesc).c_str());
        }
    }
    catch (...)
    {
        Destruct();
        throw;
    }
}

PipelineStateD3D12Impl::PipelineStateD3D12Impl(IReferenceCounters*                   pRefCounters,
                                               RenderDeviceD3D12Impl*                pDeviceD3D12,
                                               const ComputePipelineStateCreateInfo& CreateInfo) :
    TPipelineStateBase{pRefCounters, pDeviceD3D12, CreateInfo.PSODesc},
    m_SRBMemAllocator{GetRawAllocator()}
{
    try
    {
        std::vector<D3D12PipelineShaderStageInfo> ShaderStages;

        InitInternalObjects(CreateInfo, ShaderStages);

        auto pd3d12Device = pDeviceD3D12->GetD3D12Device();

        D3D12_COMPUTE_PIPELINE_STATE_DESC d3d12PSODesc = {};

        VERIFY_EXPR(ShaderStages[0].Type == SHADER_TYPE_COMPUTE);
        auto* pByteCode                 = ShaderStages[0].pShader->GetShaderByteCode();
        d3d12PSODesc.CS.pShaderBytecode = pByteCode->GetBufferPointer();
        d3d12PSODesc.CS.BytecodeLength  = pByteCode->GetBufferSize();

        // For single GPU operation, set this to zero. If there are multiple GPU nodes,
        // set bits to identify the nodes (the device's physical adapters) for which the
        // graphics pipeline state is to apply. Each bit in the mask corresponds to a single node.
        d3d12PSODesc.NodeMask = 0;

        d3d12PSODesc.CachedPSO.pCachedBlob           = nullptr;
        d3d12PSODesc.CachedPSO.CachedBlobSizeInBytes = 0;

        // The only valid bit is D3D12_PIPELINE_STATE_FLAG_TOOL_DEBUG, which can only be set on WARP devices.
        d3d12PSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

        d3d12PSODesc.pRootSignature = m_RootSig.GetD3D12RootSignature();

        HRESULT hr = pd3d12Device->CreateComputePipelineState(&d3d12PSODesc, __uuidof(ID3D12PipelineState), reinterpret_cast<void**>(static_cast<ID3D12PipelineState**>(&m_pd3d12PSO)));
        if (FAILED(hr))
            LOG_ERROR_AND_THROW("Failed to create pipeline state");

        if (*m_Desc.Name != 0)
        {
            m_pd3d12PSO->SetName(WidenString(m_Desc.Name).c_str());
            String RootSignatureDesc("Root signature for PSO '");
            RootSignatureDesc.append(m_Desc.Name);
            RootSignatureDesc.push_back('\'');
            m_RootSig.GetD3D12RootSignature()->SetName(WidenString(RootSignatureDesc).c_str());
        }
    }
    catch (...)
    {
        Destruct();
        throw;
    }
}

PipelineStateD3D12Impl::~PipelineStateD3D12Impl()
{
    Destruct();
}

void PipelineStateD3D12Impl::Destruct()
{
    auto& ShaderResLayoutAllocator = GetRawAllocator();
    for (Uint32 s = 0; s < GetNumShaderStages(); ++s)
    {
        if (m_pStaticVarManagers != nullptr)
        {
            m_pStaticVarManagers[s].Destroy(GetRawAllocator());
            m_pStaticVarManagers[s].~ShaderVariableManagerD3D12();
        }

        if (m_pShaderResourceLayouts != nullptr)
        {
            m_pShaderResourceLayouts[s].~ShaderResourceLayoutD3D12();
            m_pShaderResourceLayouts[GetNumShaderStages() + s].~ShaderResourceLayoutD3D12();
        }

        if (m_pStaticResourceCaches != nullptr)
        {
            m_pStaticResourceCaches[s].~ShaderResourceCacheD3D12();
        }
    }
    // All internal objects are allocated in contiguous chunks of memory.
    if (auto* pRawMem = m_pStaticResourceCaches)
    {
        ShaderResLayoutAllocator.Free(pRawMem);
    }

    if (m_pd3d12PSO)
    {
        // D3D12 object can only be destroyed when it is no longer used by the GPU
        m_pDevice->SafeReleaseDeviceObject(std::move(m_pd3d12PSO), m_Desc.CommandQueueMask);
    }
}

IMPLEMENT_QUERY_INTERFACE(PipelineStateD3D12Impl, IID_PipelineStateD3D12, TPipelineStateBase)


void PipelineStateD3D12Impl::InitResourceLayouts(const PipelineStateCreateInfo&             CreateInfo,
                                                 std::vector<D3D12PipelineShaderStageInfo>& ShaderStages)
{
    auto        pd3d12Device   = GetDevice()->GetD3D12Device();
    const auto& ResourceLayout = m_Desc.ResourceLayout;

#ifdef DILIGENT_DEVELOPMENT
    {
        const ShaderResources* pResources[MAX_SHADERS_IN_PIPELINE] = {};
        for (size_t s = 0; s < ShaderStages.size(); ++s)
        {
            const auto* pShader = ShaderStages[s].pShader;
            pResources[s]       = &(*pShader->GetShaderResources());
        }
        ShaderResources::DvpVerifyResourceLayout(ResourceLayout, pResources, GetNumShaderStages(),
                                                 (CreateInfo.Flags & PSO_CREATE_FLAG_IGNORE_MISSING_VARIABLES) == 0,
                                                 (CreateInfo.Flags & PSO_CREATE_FLAG_IGNORE_MISSING_IMMUTABLE_SAMPLERS) == 0);
    }
#endif

    for (size_t s = 0; s < ShaderStages.size(); ++s)
    {
        auto* pShaderD3D12 = ShaderStages[s].pShader;
        auto  ShaderType   = pShaderD3D12->GetDesc().ShaderType;
        auto  ShaderInd    = GetShaderTypePipelineIndex(ShaderType, m_Desc.PipelineType);

        m_ResourceLayoutIndex[ShaderInd] = static_cast<Int8>(s);

        m_pShaderResourceLayouts[s].Initialize(
            pd3d12Device,
            m_Desc.PipelineType,
            ResourceLayout,
            pShaderD3D12->GetShaderResources(),
            GetRawAllocator(),
            nullptr,
            0,
            nullptr,
            &m_RootSig //
        );

        const SHADER_RESOURCE_VARIABLE_TYPE StaticVarType[] = {SHADER_RESOURCE_VARIABLE_TYPE_STATIC};
        m_pShaderResourceLayouts[GetNumShaderStages() + s].Initialize(
            pd3d12Device,
            m_Desc.PipelineType,
            ResourceLayout,
            pShaderD3D12->GetShaderResources(),
            GetRawAllocator(),
            StaticVarType,
            _countof(StaticVarType),
            m_pStaticResourceCaches + s,
            nullptr //
        );

        m_pStaticVarManagers[s].Initialize(
            GetStaticShaderResLayout(static_cast<Uint32>(s)),
            GetRawAllocator(),
            nullptr,
            0 //
        );
    }
    m_RootSig.Finalize(pd3d12Device);

    if (m_Desc.SRBAllocationGranularity > 1)
    {
        std::array<size_t, MAX_SHADERS_IN_PIPELINE> ShaderVarMgrDataSizes = {};
        for (Uint32 s = 0; s < GetNumShaderStages(); ++s)
        {
            std::array<SHADER_RESOURCE_VARIABLE_TYPE, 2> AllowedVarTypes =
                {
                    SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE,
                    SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC //
                };
            Uint32 NumVariablesUnused = 0;

            ShaderVarMgrDataSizes[s] = ShaderVariableManagerD3D12::GetRequiredMemorySize(m_pShaderResourceLayouts[s], AllowedVarTypes.data(), static_cast<Uint32>(AllowedVarTypes.size()), NumVariablesUnused);
        }

        auto CacheMemorySize = m_RootSig.GetResourceCacheRequiredMemSize();
        m_SRBMemAllocator.Initialize(m_Desc.SRBAllocationGranularity, GetNumShaderStages(), ShaderVarMgrDataSizes.data(), 1, &CacheMemorySize);
    }

    m_ShaderResourceLayoutHash = m_RootSig.GetHash();
}

void PipelineStateD3D12Impl::CreateShaderResourceBinding(IShaderResourceBinding** ppShaderResourceBinding, bool InitStaticResources)
{
    auto& SRBAllocator     = m_pDevice->GetSRBAllocator();
    auto  pResBindingD3D12 = NEW_RC_OBJ(SRBAllocator, "ShaderResourceBindingD3D12Impl instance", ShaderResourceBindingD3D12Impl)(this, false);
    if (InitStaticResources)
        pResBindingD3D12->InitializeStaticResources(nullptr);
    pResBindingD3D12->QueryInterface(IID_ShaderResourceBinding, reinterpret_cast<IObject**>(ppShaderResourceBinding));
}

bool PipelineStateD3D12Impl::IsCompatibleWith(const IPipelineState* pPSO) const
{
    VERIFY_EXPR(pPSO != nullptr);

    if (pPSO == this)
        return true;

    const PipelineStateD3D12Impl* pPSOD3D12 = ValidatedCast<const PipelineStateD3D12Impl>(pPSO);
    if (m_ShaderResourceLayoutHash != pPSOD3D12->m_ShaderResourceLayoutHash)
        return false;

    auto IsSameRootSignature = m_RootSig.IsSameAs(pPSOD3D12->m_RootSig);

#ifdef DILIGENT_DEBUG
    {
        bool IsCompatibleShaders = true;
        if (GetNumShaderStages() != pPSOD3D12->GetNumShaderStages())
            IsCompatibleShaders = false;

        if (IsCompatibleShaders)
        {
            for (Uint32 s = 0; s < GetNumShaderStages(); ++s)
            {
                if (GetShaderStageType(s) != pPSOD3D12->GetShaderStageType(s))
                {
                    IsCompatibleShaders = false;
                    break;
                }

                const auto& Res0 = GetShaderResLayout(s).GetResources();
                const auto& Res1 = pPSOD3D12->GetShaderResLayout(s).GetResources();
                if (!Res0.IsCompatibleWith(Res1))
                {
                    IsCompatibleShaders = false;
                    break;
                }
            }
        }

        if (IsCompatibleShaders)
            VERIFY(IsSameRootSignature, "Compatible shaders must have same root signatures");
    }
#endif

    return IsSameRootSignature;
}

ShaderResourceCacheD3D12* PipelineStateD3D12Impl::CommitAndTransitionShaderResources(class DeviceContextD3D12Impl*        pDeviceCtx,
                                                                                     class CommandContext&                CmdCtx,
                                                                                     CommitAndTransitionResourcesAttribs& Attrib) const
{
#ifdef DILIGENT_DEVELOPMENT
    if (Attrib.pShaderResourceBinding == nullptr && ContainsShaderResources())
    {
        LOG_ERROR_MESSAGE("Pipeline state '", m_Desc.Name, "' requires shader resource binding object to ",
                          (Attrib.CommitResources ? "commit" : "transition"), " resources, but none is provided.");
    }
#endif

    auto* pResBindingD3D12Impl = ValidatedCast<ShaderResourceBindingD3D12Impl>(Attrib.pShaderResourceBinding);
    if (pResBindingD3D12Impl == nullptr)
    {
        if (Attrib.CommitResources)
        {
            if (m_Desc.IsComputePipeline())
                CmdCtx.AsComputeContext().SetRootSignature(GetD3D12RootSignature());
            else
                CmdCtx.AsGraphicsContext().SetRootSignature(GetD3D12RootSignature());
        }
        return nullptr;
    }

#ifdef DILIGENT_DEVELOPMENT
    if (IsIncompatibleWith(pResBindingD3D12Impl->GetPipelineState()))
    {
        LOG_ERROR_MESSAGE("Shader resource binding is incompatible with the pipeline state '", m_Desc.Name, "'. Operation will be ignored.");
        return nullptr;
    }


    if ((m_RootSig.GetTotalSrvCbvUavSlots(SHADER_RESOURCE_VARIABLE_TYPE_STATIC) != 0 ||
         m_RootSig.GetTotalRootViews(SHADER_RESOURCE_VARIABLE_TYPE_STATIC) != 0) &&
        !pResBindingD3D12Impl->StaticResourcesInitialized())
    {
        LOG_ERROR_MESSAGE("Static resources have not been initialized in the shader resource binding object being committed for PSO '", m_Desc.Name, "'. Please call IShaderResourceBinding::InitializeStaticResources().");
    }

    pResBindingD3D12Impl->dvpVerifyResourceBindings(this);
#endif

    auto& ResourceCache = pResBindingD3D12Impl->GetResourceCache();
    if (Attrib.CommitResources)
    {
        if (m_Desc.IsComputePipeline())
            CmdCtx.AsComputeContext().SetRootSignature(GetD3D12RootSignature());
        else
            CmdCtx.AsGraphicsContext().SetRootSignature(GetD3D12RootSignature());

        if (Attrib.TransitionResources)
        {
            (m_RootSig.*m_RootSig.TransitionAndCommitDescriptorHandles)(m_pDevice, ResourceCache, CmdCtx, m_Desc.IsComputePipeline(), Attrib.ValidateStates);
        }
        else
        {
            (m_RootSig.*m_RootSig.CommitDescriptorHandles)(m_pDevice, ResourceCache, CmdCtx, m_Desc.IsComputePipeline(), Attrib.ValidateStates);
        }
    }
    else
    {
        VERIFY(Attrib.TransitionResources, "Resources should be transitioned or committed or both");
        m_RootSig.TransitionResources(ResourceCache, CmdCtx);
    }

    // Process only non-dynamic buffers at this point. Dynamic buffers will be handled by the Draw/Dispatch command.
    m_RootSig.CommitRootViews(ResourceCache,
                              CmdCtx,
                              m_Desc.IsComputePipeline(),
                              Attrib.CtxId,
                              pDeviceCtx,
                              Attrib.CommitResources, // CommitViews
                              false,                  // ProcessDynamicBuffers
                              true,                   // ProcessNonDynamicBuffers
                              Attrib.TransitionResources,
                              Attrib.ValidateStates);

    return &ResourceCache;
}


bool PipelineStateD3D12Impl::ContainsShaderResources() const
{
    for (auto VarType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC; VarType < SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES; VarType = static_cast<SHADER_RESOURCE_VARIABLE_TYPE>(VarType + 1))
    {
        if (m_RootSig.GetTotalSrvCbvUavSlots(VarType) != 0 ||
            m_RootSig.GetTotalRootViews(VarType) != 0)
            return true;
    }
    return false;
}

void PipelineStateD3D12Impl::BindStaticResources(Uint32 ShaderFlags, IResourceMapping* pResourceMapping, Uint32 Flags)
{
    for (Uint32 s = 0; s < GetNumShaderStages(); ++s)
    {
        auto ShaderType = GetStaticShaderResLayout(s).GetShaderType();
        if ((ShaderFlags & ShaderType) != 0)
            m_pStaticVarManagers[s].BindResources(pResourceMapping, Flags);
    }
}

Uint32 PipelineStateD3D12Impl::GetStaticVariableCount(SHADER_TYPE ShaderType) const
{
    const auto LayoutInd = GetStaticVariableCountHelper(ShaderType, m_ResourceLayoutIndex);
    if (LayoutInd < 0)
        return 0;

    VERIFY_EXPR(static_cast<Uint32>(LayoutInd) < GetNumShaderStages());
    return m_pStaticVarManagers[LayoutInd].GetVariableCount();
}

IShaderResourceVariable* PipelineStateD3D12Impl::GetStaticVariableByName(SHADER_TYPE ShaderType, const Char* Name)
{
    const auto LayoutInd = GetStaticVariableByNameHelper(ShaderType, Name, m_ResourceLayoutIndex);
    if (LayoutInd < 0)
        return nullptr;

    VERIFY_EXPR(static_cast<Uint32>(LayoutInd) < GetNumShaderStages());
    return m_pStaticVarManagers[LayoutInd].GetVariable(Name);
}

IShaderResourceVariable* PipelineStateD3D12Impl::GetStaticVariableByIndex(SHADER_TYPE ShaderType, Uint32 Index)
{
    const auto LayoutInd = GetStaticVariableByIndexHelper(ShaderType, Index, m_ResourceLayoutIndex);
    if (LayoutInd < 0)
        return nullptr;

    VERIFY_EXPR(static_cast<Uint32>(LayoutInd) < GetNumShaderStages());
    return m_pStaticVarManagers[LayoutInd].GetVariable(Index);
}

} // namespace Diligent
