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
/// Declaration of Diligent::PipelineStateVkImpl class

#include <array>

#include "RenderDeviceVk.h"
#include "PipelineStateVk.h"
#include "PipelineStateBase.hpp"
#include "PipelineLayout.hpp"
#include "ShaderResourceLayoutVk.hpp"
#include "ShaderVariableVk.hpp"
#include "FixedBlockMemoryAllocator.hpp"
#include "SRBMemoryAllocator.hpp"
#include "VulkanUtilities/VulkanObjectWrappers.hpp"
#include "VulkanUtilities/VulkanCommandBuffer.hpp"
#include "PipelineLayout.hpp"
#include "RenderDeviceVkImpl.hpp"

namespace Diligent
{

class FixedBlockMemoryAllocator;
class ShaderVariableManagerVk;

/// Pipeline state object implementation in Vulkan backend.
class PipelineStateVkImpl final : public PipelineStateBase<IPipelineStateVk, RenderDeviceVkImpl>
{
public:
    using TPipelineStateBase = PipelineStateBase<IPipelineStateVk, RenderDeviceVkImpl>;

    PipelineStateVkImpl(IReferenceCounters* pRefCounters, RenderDeviceVkImpl* pDeviceVk, const GraphicsPipelineStateCreateInfo& CreateInfo);
    PipelineStateVkImpl(IReferenceCounters* pRefCounters, RenderDeviceVkImpl* pDeviceVk, const ComputePipelineStateCreateInfo& CreateInfo);
    ~PipelineStateVkImpl();

    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override final;

    /// Implementation of IPipelineState::CreateShaderResourceBinding() in Vulkan backend.
    virtual void DILIGENT_CALL_TYPE CreateShaderResourceBinding(IShaderResourceBinding** ppShaderResourceBinding, bool InitStaticResources) override final;

    /// Implementation of IPipelineState::IsCompatibleWith() in Vulkan backend.
    virtual bool DILIGENT_CALL_TYPE IsCompatibleWith(const IPipelineState* pPSO) const override final;

    /// Implementation of IPipelineStateVk::GetRenderPass().
    virtual IRenderPassVk* DILIGENT_CALL_TYPE GetRenderPass() const override final { return m_pRenderPass.RawPtr<IRenderPassVk>(); }

    /// Implementation of IPipelineStateVk::GetVkPipeline().
    virtual VkPipeline DILIGENT_CALL_TYPE GetVkPipeline() const override final { return m_Pipeline; }

    /// Implementation of IPipelineState::BindStaticResources() in Vulkan backend.
    virtual void DILIGENT_CALL_TYPE BindStaticResources(Uint32 ShaderFlags, IResourceMapping* pResourceMapping, Uint32 Flags) override final;

    /// Implementation of IPipelineState::GetStaticVariableCount() in Vulkan backend.
    virtual Uint32 DILIGENT_CALL_TYPE GetStaticVariableCount(SHADER_TYPE ShaderType) const override final;

    /// Implementation of IPipelineState::GetStaticVariableByName() in Vulkan backend.
    virtual IShaderResourceVariable* DILIGENT_CALL_TYPE GetStaticVariableByName(SHADER_TYPE ShaderType, const Char* Name) override final;

    /// Implementation of IPipelineState::GetStaticVariableByIndex() in Vulkan backend.
    virtual IShaderResourceVariable* DILIGENT_CALL_TYPE GetStaticVariableByIndex(SHADER_TYPE ShaderType, Uint32 Index) override final;

    void CommitAndTransitionShaderResources(IShaderResourceBinding*                pShaderResourceBinding,
                                            DeviceContextVkImpl*                   pCtxVkImpl,
                                            bool                                   CommitResources,
                                            RESOURCE_STATE_TRANSITION_MODE         StateTransitionMode,
                                            PipelineLayout::DescriptorSetBindInfo* pDescrSetBindInfo) const;

    __forceinline void BindDescriptorSetsWithDynamicOffsets(VulkanUtilities::VulkanCommandBuffer&  CmdBuffer,
                                                            Uint32                                 CtxId,
                                                            DeviceContextVkImpl*                   pCtxVkImpl,
                                                            PipelineLayout::DescriptorSetBindInfo& BindInfo)
    {
        m_PipelineLayout.BindDescriptorSetsWithDynamicOffsets(CmdBuffer, CtxId, pCtxVkImpl, BindInfo);
    }

    const PipelineLayout& GetPipelineLayout() const { return m_PipelineLayout; }

    const ShaderResourceLayoutVk& GetShaderResLayout(Uint32 ShaderInd) const
    {
        VERIFY_EXPR(ShaderInd < GetNumShaderStages());
        return m_ShaderResourceLayouts[ShaderInd];
    }

    SRBMemoryAllocator& GetSRBMemoryAllocator()
    {
        return m_SRBMemAllocator;
    }

    static RenderPassDesc GetImplicitRenderPassDesc(Uint32                                                        NumRenderTargets,
                                                    const TEXTURE_FORMAT                                          RTVFormats[],
                                                    TEXTURE_FORMAT                                                DSVFormat,
                                                    Uint8                                                         SampleCount,
                                                    std::array<RenderPassAttachmentDesc, MAX_RENDER_TARGETS + 1>& Attachments,
                                                    std::array<AttachmentReference, MAX_RENDER_TARGETS + 1>&      AttachmentReferences,
                                                    SubpassDesc&                                                  SubpassDesc);


    void InitializeStaticSRBResources(ShaderResourceCacheVk& ResourceCache) const;

private:
    using TShaderStages = ShaderResourceLayoutVk::TShaderStages;

    template <typename PSOCreateInfoType>
    void InitInternalObjects(const PSOCreateInfoType&                           CreateInfo,
                             std::vector<VkPipelineShaderStageCreateInfo>&      vkShaderStages,
                             std::vector<VulkanUtilities::ShaderModuleWrapper>& ShaderModules);

    void InitResourceLayouts(const PipelineStateCreateInfo& CreateInfo,
                             TShaderStages&                 ShaderStages);

    void Destruct();

    const ShaderResourceLayoutVk& GetStaticShaderResLayout(Uint32 ShaderInd) const
    {
        VERIFY_EXPR(ShaderInd < GetNumShaderStages());
        return m_ShaderResourceLayouts[GetNumShaderStages() + ShaderInd];
    }

    const ShaderResourceCacheVk& GetStaticResCache(Uint32 ShaderInd) const
    {
        VERIFY_EXPR(ShaderInd < GetNumShaderStages());
        return m_StaticResCaches[ShaderInd];
    }

    const ShaderVariableManagerVk& GetStaticVarMgr(Uint32 ShaderInd) const
    {
        VERIFY_EXPR(ShaderInd < GetNumShaderStages());
        return m_StaticVarsMgrs[ShaderInd];
    }

    ShaderResourceLayoutVk*  m_ShaderResourceLayouts = nullptr; // [m_NumShaderStages * 2]
    ShaderResourceCacheVk*   m_StaticResCaches       = nullptr; // [m_NumShaderStages]
    ShaderVariableManagerVk* m_StaticVarsMgrs        = nullptr; // [m_NumShaderStages]

    // SRB memory allocator must be declared before m_pDefaultShaderResBinding
    SRBMemoryAllocator m_SRBMemAllocator;

    VulkanUtilities::PipelineWrapper m_Pipeline;
    PipelineLayout                   m_PipelineLayout;

    // Resource layout index in m_ShaderResourceLayouts array for every shader stage,
    // indexed by the shader type pipeline index (returned by GetShaderTypePipelineIndex)
    std::array<Int8, MAX_SHADERS_IN_PIPELINE> m_ResourceLayoutIndex = {-1, -1, -1, -1, -1};

    bool m_HasStaticResources    = false;
    bool m_HasNonStaticResources = false;
};

} // namespace Diligent
