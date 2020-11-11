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

#pragma once

/// \file
/// Declaration of Diligent::PipelineStateD3D11Impl class

#include "PipelineStateD3D11.h"
#include "RenderDeviceD3D11.h"
#include "PipelineStateBase.hpp"
#include "ShaderResourceLayoutD3D11.hpp"
#include "SRBMemoryAllocator.hpp"
#include "RenderDeviceD3D11Impl.hpp"
#include "ShaderD3D11Impl.hpp"

namespace Diligent
{

class FixedBlockMemoryAllocator;

/// Pipeline state object implementation in Direct3D11 backend.
class PipelineStateD3D11Impl final : public PipelineStateBase<IPipelineStateD3D11, RenderDeviceD3D11Impl>
{
public:
    using TPipelineStateBase = PipelineStateBase<IPipelineStateD3D11, RenderDeviceD3D11Impl>;

    PipelineStateD3D11Impl(IReferenceCounters*                    pRefCounters,
                           class RenderDeviceD3D11Impl*           pDeviceD3D11,
                           const GraphicsPipelineStateCreateInfo& CreateInfo);
    PipelineStateD3D11Impl(IReferenceCounters*                   pRefCounters,
                           class RenderDeviceD3D11Impl*          pDeviceD3D11,
                           const ComputePipelineStateCreateInfo& CreateInfo);
    ~PipelineStateD3D11Impl();

    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override final;

    /// Implementation of IPipelineState::BindStaticResources() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE BindStaticResources(Uint32            ShaderFlags,
                                                        IResourceMapping* pResourceMapping,
                                                        Uint32            Flags) override final;

    /// Implementation of IPipelineState::GetStaticVariableCount() in Direct3D11 backend.
    virtual Uint32 DILIGENT_CALL_TYPE GetStaticVariableCount(SHADER_TYPE ShaderType) const override final;

    /// Implementation of IPipelineState::GetStaticVariableByName() in Direct3D11 backend.
    virtual IShaderResourceVariable* DILIGENT_CALL_TYPE GetStaticVariableByName(SHADER_TYPE ShaderType,
                                                                                const Char* Name) override final;

    /// Implementation of IPipelineState::GetStaticVariableByIndex() in Direct3D11 backend.
    virtual IShaderResourceVariable* DILIGENT_CALL_TYPE GetStaticVariableByIndex(SHADER_TYPE ShaderType,
                                                                                 Uint32      Index) override final;

    /// Implementation of IPipelineState::CreateShaderResourceBinding() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CreateShaderResourceBinding(IShaderResourceBinding** ppShaderResourceBinding,
                                                                bool                     InitStaticResources) override final;

    /// Implementation of IPipelineState::IsCompatibleWith() in Direct3D11 backend.
    virtual bool DILIGENT_CALL_TYPE IsCompatibleWith(const IPipelineState* pPSO) const override final;


    /// Implementation of IPipelineStateD3D11::GetD3D11BlendState() method.
    virtual ID3D11BlendState* DILIGENT_CALL_TYPE GetD3D11BlendState() override final;

    /// Implementation of IPipelineStateD3D11::GetD3D11RasterizerState() method.
    virtual ID3D11RasterizerState* DILIGENT_CALL_TYPE GetD3D11RasterizerState() override final;

    /// Implementation of IPipelineStateD3D11::GetD3D11DepthStencilState() method.
    virtual ID3D11DepthStencilState* DILIGENT_CALL_TYPE GetD3D11DepthStencilState() override final;

    /// Implementation of IPipelineStateD3D11::GetD3D11InputLayout() method.
    virtual ID3D11InputLayout* DILIGENT_CALL_TYPE GetD3D11InputLayout() override final;

    /// Implementation of IPipelineStateD3D11::GetD3D11VertexShader() method.
    virtual ID3D11VertexShader* DILIGENT_CALL_TYPE GetD3D11VertexShader() override final;

    /// Implementation of IPipelineStateD3D11::GetD3D11PixelShader() method.
    virtual ID3D11PixelShader* DILIGENT_CALL_TYPE GetD3D11PixelShader() override final;

    /// Implementation of IPipelineStateD3D11::GetD3D11GeometryShader() method.
    virtual ID3D11GeometryShader* DILIGENT_CALL_TYPE GetD3D11GeometryShader() override final;

    /// Implementation of IPipelineStateD3D11::GetD3D11DomainShader() method.
    virtual ID3D11DomainShader* DILIGENT_CALL_TYPE GetD3D11DomainShader() override final;

    /// Implementation of IPipelineStateD3D11::GetD3D11HullShader() method.
    virtual ID3D11HullShader* DILIGENT_CALL_TYPE GetD3D11HullShader() override final;

    /// Implementation of IPipelineStateD3D11::GetD3D11ComputeShader() method.
    virtual ID3D11ComputeShader* DILIGENT_CALL_TYPE GetD3D11ComputeShader() override final;


    SRBMemoryAllocator& GetSRBMemoryAllocator()
    {
        return m_SRBMemAllocator;
    }

    const ShaderResourceLayoutD3D11& GetStaticResourceLayout(Uint32 s) const
    {
        VERIFY_EXPR(s < GetNumShaderStages());
        return m_pStaticResourceLayouts[s];
    }

    ShaderResourceCacheD3D11& GetStaticResourceCache(Uint32 s)
    {
        VERIFY_EXPR(s < GetNumShaderStages());
        return m_pStaticResourceCaches[s];
    }

    const ShaderD3D11Impl* GetShaderByType(SHADER_TYPE ShaderType) const;
    const ShaderD3D11Impl* GetShader(Uint32 Index) const;

    void SetImmutableSamplers(ShaderResourceCacheD3D11& ResourceCache, Uint32 ShaderInd) const;

private:
    template <typename PSOCreateInfoType>
    void InitInternalObjects(const PSOCreateInfoType& CreateInfo);

    void InitResourceLayouts(const PipelineStateCreateInfo&                               CreateInfo,
                             const std::vector<std::pair<SHADER_TYPE, ShaderD3D11Impl*>>& ShaderStages);

    void Destruct();

    CComPtr<ID3D11BlendState>        m_pd3d11BlendState;
    CComPtr<ID3D11RasterizerState>   m_pd3d11RasterizerState;
    CComPtr<ID3D11DepthStencilState> m_pd3d11DepthStencilState;
    CComPtr<ID3D11InputLayout>       m_pd3d11InputLayout;

    RefCntAutoPtr<ShaderD3D11Impl> m_pVS;
    RefCntAutoPtr<ShaderD3D11Impl> m_pPS;
    RefCntAutoPtr<ShaderD3D11Impl> m_pGS;
    RefCntAutoPtr<ShaderD3D11Impl> m_pDS;
    RefCntAutoPtr<ShaderD3D11Impl> m_pHS;
    RefCntAutoPtr<ShaderD3D11Impl> m_pCS;

    // The caches are indexed by the shader order in the PSO, not shader index
    ShaderResourceCacheD3D11*  m_pStaticResourceCaches  = nullptr; // [m_NumShaderStages]
    ShaderResourceLayoutD3D11* m_pStaticResourceLayouts = nullptr; // [m_NumShaderStages]

    // SRB memory allocator must be defined before the default shader res binding
    SRBMemoryAllocator m_SRBMemAllocator;

    // Resource layout index in m_pStaticResourceLayouts array for every shader stage,
    // indexed by the shader type pipeline index (returned by GetShaderTypePipelineIndex)
    std::array<Int8, MAX_SHADERS_IN_PIPELINE> m_ResourceLayoutIndex = {-1, -1, -1, -1, -1};

    std::array<Uint16, MAX_SHADERS_IN_PIPELINE + 1> m_ImmutableSamplerOffsets = {};
    struct ImmutableSamplerInfo
    {
        const D3DShaderResourceAttribs& Attribs;
        RefCntAutoPtr<ISampler>         pSampler;
        ImmutableSamplerInfo(const D3DShaderResourceAttribs& _Attribs,
                             RefCntAutoPtr<ISampler>         _pSampler) :
            // clang-format off
            Attribs  {_Attribs},
            pSampler {std::move(_pSampler)}
        // clang-format on
        {}
    };
    std::vector<ImmutableSamplerInfo, STDAllocatorRawMem<ImmutableSamplerInfo>> m_ImmutableSamplers;
};

} // namespace Diligent
