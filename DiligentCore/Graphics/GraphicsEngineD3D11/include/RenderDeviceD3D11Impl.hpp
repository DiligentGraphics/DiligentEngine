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
/// Declaration of Diligent::RenderDeviceD3D11Impl class

#include "RenderDeviceD3D11.h"
#include "RenderDeviceD3DBase.hpp"
#include "DeviceContextD3D11.h"

namespace Diligent
{

/// Render device implementation in Direct3D11 backend.
class RenderDeviceD3D11Impl final : public RenderDeviceD3DBase<IRenderDeviceD3D11>
{
public:
    using TRenderDeviceBase = RenderDeviceD3DBase<IRenderDeviceD3D11>;

    RenderDeviceD3D11Impl(IReferenceCounters*          pRefCounters,
                          IMemoryAllocator&            RawMemAllocator,
                          IEngineFactory*              pEngineFactory,
                          const EngineD3D11CreateInfo& EngineAttribs,
                          ID3D11Device*                pd3d11Device,
                          Uint32                       NumDeferredContexts) noexcept(false);
    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override final;

    /// Implementation of IRenderDevice::CreateBuffer() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CreateBuffer(const BufferDesc& BuffDesc,
                                                 const BufferData* pBuffData,
                                                 IBuffer**         ppBuffer) override final;

    /// Implementation of IRenderDevice::CreateShader() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CreateShader(const ShaderCreateInfo& ShaderCI,
                                                 IShader**               ppShader) override final;

    /// Implementation of IRenderDevice::CreateTexture() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CreateTexture(const TextureDesc& TexDesc,
                                                  const TextureData* pData,
                                                  ITexture**         ppTexture) override final;

    /// Implementation of IRenderDevice::CreateSampler() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CreateSampler(const SamplerDesc& SamplerDesc,
                                                  ISampler**         ppSampler) override final;

    /// Implementation of IRenderDevice::CreateGraphicsPipelineState() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CreateGraphicsPipelineState(const GraphicsPipelineStateCreateInfo& PSOCreateInfo,
                                                                IPipelineState**                       ppPipelineState) override final;

    /// Implementation of IRenderDevice::CreateComputePipelineState() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CreateComputePipelineState(const ComputePipelineStateCreateInfo& PSOCreateInfo,
                                                               IPipelineState**                      ppPipelineState) override final;

    /// Implementation of IRenderDevice::CreateFence() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CreateFence(const FenceDesc& Desc,
                                                IFence**         ppFence) override final;

    /// Implementation of IRenderDevice::CreateQuery() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CreateQuery(const QueryDesc& Desc,
                                                IQuery**         ppQuery) override final;

    /// Implementation of IRenderDevice::CreateRenderPass() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CreateRenderPass(const RenderPassDesc& Desc,
                                                     IRenderPass**         ppRenderPass) override final;

    /// Implementation of IRenderDevice::CreateFramebuffer() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CreateFramebuffer(const FramebufferDesc& Desc,
                                                      IFramebuffer**         ppFramebuffer) override final;

    /// Implementation of IRenderDeviceD3D11::GetD3D11Device() in Direct3D11 backend.
    ID3D11Device* DILIGENT_CALL_TYPE GetD3D11Device() override final { return m_pd3d11Device; }

    /// Implementation of IRenderDeviceD3D11::CreateBufferFromD3DResource() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CreateBufferFromD3DResource(ID3D11Buffer* pd3d11Buffer, const BufferDesc& BuffDesc, RESOURCE_STATE InitialState, IBuffer** ppBuffer) override final;

    /// Implementation of IRenderDeviceD3D11::CreateTextureFromD3DResource() for 1D textures in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CreateTexture1DFromD3DResource(ID3D11Texture1D* pd3d11Texture,
                                                                   RESOURCE_STATE   InitialState,
                                                                   ITexture**       ppTexture) override final;

    /// Implementation of IRenderDeviceD3D11::CreateTextureFromD3DResource() for 2D textures in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CreateTexture2DFromD3DResource(ID3D11Texture2D* pd3d11Texture,
                                                                   RESOURCE_STATE   InitialState,
                                                                   ITexture**       ppTexture) override final;

    /// Implementation of IRenderDeviceD3D11::CreateTextureFromD3DResource() for 3D textures in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CreateTexture3DFromD3DResource(ID3D11Texture3D* pd3d11Texture,
                                                                   RESOURCE_STATE   InitialState,
                                                                   ITexture**       ppTexture) override final;

    /// Implementation of IRenderDevice::ReleaseStaleResources() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE ReleaseStaleResources(bool ForceRelease = false) override final {}

    /// Implementation of IRenderDevice::IdleGPU() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE IdleGPU() override final;

    size_t GetCommandQueueCount() const { return 1; }
    Uint64 GetCommandQueueMask() const { return Uint64{1}; }

private:
    template <typename PSOCreateInfoType>
    void CreatePipelineState(const PSOCreateInfoType& PSOCreateInfo, IPipelineState** ppPipelineState);

    virtual void TestTextureFormat(TEXTURE_FORMAT TexFormat) override final;

    EngineD3D11CreateInfo m_EngineAttribs;

    /// D3D11 device
    CComPtr<ID3D11Device> m_pd3d11Device;
};

} // namespace Diligent
