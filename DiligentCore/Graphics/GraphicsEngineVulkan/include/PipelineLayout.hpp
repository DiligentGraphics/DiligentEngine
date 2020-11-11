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
/// Declaration of Diligent::PipelineLayout class
#include <array>

#include "ShaderBase.hpp"
#include "ShaderResourceLayoutVk.hpp"
#include "VulkanUtilities/VulkanObjectWrappers.hpp"
#include "VulkanUtilities/VulkanLogicalDevice.hpp"
#include "VulkanUtilities/VulkanCommandBuffer.hpp"

namespace Diligent
{

class RenderDeviceVkImpl;
class DeviceContextVkImpl;
class ShaderResourceCacheVk;

/// Implementation of the Diligent::PipelineLayout class
class PipelineLayout
{
public:
    static VkDescriptorType GetVkDescriptorType(const SPIRVShaderResourceAttribs& Res);

    PipelineLayout();
    void Release(RenderDeviceVkImpl* pDeviceVkImpl, Uint64 CommandQueueMask);
    void Finalize(const VulkanUtilities::VulkanLogicalDevice& LogicalDevice);

    VkPipelineLayout GetVkPipelineLayout() const { return m_LayoutMgr.GetVkPipelineLayout(); }

    std::array<Uint32, 2> GetDescriptorSetSizes(Uint32& NumSets) const;

    void InitResourceCache(RenderDeviceVkImpl*    pDeviceVkImpl,
                           ShaderResourceCacheVk& ResourceCache,
                           IMemoryAllocator&      CacheMemAllocator,
                           const char*            DbgPipelineName) const;

    void AllocateResourceSlot(const SPIRVShaderResourceAttribs& ResAttribs,
                              SHADER_RESOURCE_VARIABLE_TYPE     VariableType,
                              VkSampler                         vkImmutableSampler,
                              SHADER_TYPE                       ShaderType,
                              Uint32&                           DescriptorSet,
                              Uint32&                           Binding,
                              Uint32&                           OffsetInCache,
                              std::vector<uint32_t>&            SPIRV);

    Uint32 GetTotalDescriptors(SHADER_RESOURCE_VARIABLE_TYPE VarType) const
    {
        VERIFY_EXPR(VarType >= 0 && VarType < SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES);
        return m_LayoutMgr.GetDescriptorSet(VarType).TotalDescriptors;
    }

    bool IsSameAs(const PipelineLayout& RS) const
    {
        return m_LayoutMgr == RS.m_LayoutMgr;
    }
    size_t GetHash() const
    {
        return m_LayoutMgr.GetHash();
    }

    VkDescriptorSetLayout GetDynamicDescriptorSetVkLayout() const
    {
        return m_LayoutMgr.GetDescriptorSet(SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC).VkLayout;
    }

    struct DescriptorSetBindInfo
    {
        std::vector<VkDescriptorSet> vkSets;
        std::vector<uint32_t>        DynamicOffsets;
        const ShaderResourceCacheVk* pResourceCache          = nullptr;
        VkPipelineBindPoint          BindPoint               = VK_PIPELINE_BIND_POINT_MAX_ENUM;
        Uint32                       SetCout                 = 0;
        Uint32                       DynamicOffsetCount      = 0;
        bool                         DynamicBuffersPresent   = false;
        bool                         DynamicDescriptorsBound = false;
#ifdef DILIGENT_DEBUG
        const PipelineLayout* pDbgPipelineLayout = nullptr;
#endif
        DescriptorSetBindInfo() :
            // clang-format off
            vkSets        (2),
            DynamicOffsets(64)
        // clang-format on
        {
        }

        void Reset()
        {
            pResourceCache          = nullptr;
            BindPoint               = VK_PIPELINE_BIND_POINT_MAX_ENUM;
            SetCout                 = 0;
            DynamicOffsetCount      = 0;
            DynamicBuffersPresent   = false;
            DynamicDescriptorsBound = false;

#ifdef DILIGENT_DEBUG
            // In release mode, do not clear vectors as this causes unnecessary work
            vkSets.clear();
            DynamicOffsets.clear();

            pDbgPipelineLayout = nullptr;
#endif
        }
    };

    // Prepares Vulkan descriptor sets for binding. Actual binding
    // may not be possible until draw command time because dynamic offsets are
    // set by the same Vulkan command. If there are no dynamic descriptors, this
    // function also binds descriptor sets rightaway.
    void PrepareDescriptorSets(DeviceContextVkImpl*         pCtxVkImpl,
                               bool                         IsCompute,
                               const ShaderResourceCacheVk& ResourceCache,
                               DescriptorSetBindInfo&       BindInfo,
                               VkDescriptorSet              VkDynamicDescrSet) const;

    // Computes dynamic offsets and binds descriptor sets
    __forceinline void BindDescriptorSetsWithDynamicOffsets(VulkanUtilities::VulkanCommandBuffer& CmdBuffer,
                                                            Uint32                                CtxId,
                                                            DeviceContextVkImpl*                  pCtxVkImpl,
                                                            DescriptorSetBindInfo&                BindInfo) const;

private:
    class DescriptorSetLayoutManager
    {
    public:
        struct DescriptorSetLayout
        {
            DescriptorSetLayout() noexcept {}
            // clang-format off
            DescriptorSetLayout             (DescriptorSetLayout&&)      = default;
            DescriptorSetLayout             (const DescriptorSetLayout&) = delete;
            DescriptorSetLayout& operator = (const DescriptorSetLayout&) = delete;
            DescriptorSetLayout& operator = (DescriptorSetLayout&&)      = delete;
            // clang-format on

            uint32_t                                    TotalDescriptors      = 0;
            int8_t                                      SetIndex              = -1;
            uint8_t                                     NumDynamicDescriptors = 0; // Total number of uniform and storage buffers, counting all array elements
            uint16_t                                    NumLayoutBindings     = 0;
            VkDescriptorSetLayoutBinding*               pBindings             = nullptr;
            VulkanUtilities::DescriptorSetLayoutWrapper VkLayout;

            ~DescriptorSetLayout();
            void AddBinding(const VkDescriptorSetLayoutBinding& Binding, IMemoryAllocator& MemAllocator);
            void Finalize(const VulkanUtilities::VulkanLogicalDevice& LogicalDevice, IMemoryAllocator& MemAllocator, VkDescriptorSetLayoutBinding* pNewBindings);
            void Release(RenderDeviceVkImpl* pRenderDeviceVk, IMemoryAllocator& MemAllocator, Uint64 CommandQueueMask);

            bool   operator==(const DescriptorSetLayout& rhs) const;
            bool   operator!=(const DescriptorSetLayout& rhs) const { return !(*this == rhs); }
            size_t GetHash() const;

        private:
            void          ReserveMemory(Uint32 NumBindings, IMemoryAllocator& MemAllocator);
            static size_t GetMemorySize(Uint32 NumBindings);
        };

        DescriptorSetLayoutManager(IMemoryAllocator& MemAllocator);
        ~DescriptorSetLayoutManager();

        // clang-format off
        DescriptorSetLayoutManager            (const DescriptorSetLayoutManager&) = delete;
        DescriptorSetLayoutManager& operator= (const DescriptorSetLayoutManager&) = delete;
        DescriptorSetLayoutManager            (DescriptorSetLayoutManager&&)      = delete;
        DescriptorSetLayoutManager& operator= (DescriptorSetLayoutManager&&)      = delete;
        // clang-format on

        void Finalize(const VulkanUtilities::VulkanLogicalDevice& LogicalDevice);
        void Release(RenderDeviceVkImpl* pRenderDeviceVk, Uint64 CommandQueueMask);

        DescriptorSetLayout&       GetDescriptorSet(SHADER_RESOURCE_VARIABLE_TYPE VarType) { return m_DescriptorSetLayouts[VarType == SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC ? 1 : 0]; }
        const DescriptorSetLayout& GetDescriptorSet(SHADER_RESOURCE_VARIABLE_TYPE VarType) const { return m_DescriptorSetLayouts[VarType == SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC ? 1 : 0]; }

        bool             operator==(const DescriptorSetLayoutManager& rhs) const;
        bool             operator!=(const DescriptorSetLayoutManager& rhs) const { return !(*this == rhs); }
        size_t           GetHash() const;
        VkPipelineLayout GetVkPipelineLayout() const { return m_VkPipelineLayout; }

        void AllocateResourceSlot(const SPIRVShaderResourceAttribs& ResAttribs,
                                  SHADER_RESOURCE_VARIABLE_TYPE     VariableType,
                                  VkSampler                         vkImmutableSampler,
                                  SHADER_TYPE                       ShaderType,
                                  Uint32&                           DescriptorSet,
                                  Uint32&                           Binding,
                                  Uint32&                           OffsetInCache);

    private:
        IMemoryAllocator&                                                                           m_MemAllocator;
        VulkanUtilities::PipelineLayoutWrapper                                                      m_VkPipelineLayout;
        std::array<DescriptorSetLayout, 2>                                                          m_DescriptorSetLayouts;
        std::vector<VkDescriptorSetLayoutBinding, STDAllocatorRawMem<VkDescriptorSetLayoutBinding>> m_LayoutBindings;
        uint8_t                                                                                     m_ActiveSets = 0;
    };

    IMemoryAllocator&          m_MemAllocator;
    DescriptorSetLayoutManager m_LayoutMgr;
};


__forceinline void PipelineLayout::BindDescriptorSetsWithDynamicOffsets(VulkanUtilities::VulkanCommandBuffer& CmdBuffer,
                                                                        Uint32                                CtxId,
                                                                        DeviceContextVkImpl*                  pCtxVkImpl,
                                                                        DescriptorSetBindInfo&                BindInfo) const
{
    VERIFY(BindInfo.pDbgPipelineLayout != nullptr, "Pipeline layout is not initialized, which most likely means that CommitShaderResources() has never been called");
    VERIFY(BindInfo.pDbgPipelineLayout->IsSameAs(*this), "Inconsistent pipeline layout");
    VERIFY(BindInfo.DynamicOffsetCount > 0, "This function should only be called for pipelines that contain dynamic descriptors");

    VERIFY_EXPR(BindInfo.pResourceCache != nullptr);
#ifdef DILIGENT_DEBUG
    Uint32 TotalDynamicDescriptors = 0;
    for (SHADER_RESOURCE_VARIABLE_TYPE VarType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE; VarType <= SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC; VarType = static_cast<SHADER_RESOURCE_VARIABLE_TYPE>(VarType + 1))
    {
        const auto& Set = m_LayoutMgr.GetDescriptorSet(VarType);
        TotalDynamicDescriptors += Set.NumDynamicDescriptors;
    }
    VERIFY(BindInfo.DynamicOffsetCount == TotalDynamicDescriptors, "Incosistent dynamic buffer size");
    VERIFY_EXPR(BindInfo.DynamicOffsets.size() >= BindInfo.DynamicOffsetCount);
#endif

    auto NumOffsetsWritten = BindInfo.pResourceCache->GetDynamicBufferOffsets(CtxId, pCtxVkImpl, BindInfo.DynamicOffsets);
    VERIFY_EXPR(NumOffsetsWritten == BindInfo.DynamicOffsetCount);
    (void)NumOffsetsWritten;

    // Note that there is one global dynamic buffer from which all dynamic resources are suballocated in Vulkan back-end,
    // and this buffer is not resizable, so the buffer handle can never change.

    // vkCmdBindDescriptorSets causes the sets numbered [firstSet .. firstSet+descriptorSetCount-1] to use the
    // bindings stored in pDescriptorSets[0 .. descriptorSetCount-1] for subsequent rendering commands
    // (either compute or graphics, according to the pipelineBindPoint). Any bindings that were previously
    // applied via these sets are no longer valid (13.2.5)
    CmdBuffer.BindDescriptorSets(BindInfo.BindPoint,
                                 m_LayoutMgr.GetVkPipelineLayout(),
                                 0, // First set
                                 BindInfo.SetCout,
                                 BindInfo.vkSets.data(), // BindInfo.vkSets is never empty
                                 // dynamicOffsetCount must equal the total number of dynamic descriptors in the sets being bound (13.2.5)
                                 BindInfo.DynamicOffsetCount,
                                 BindInfo.DynamicOffsets.data());

    BindInfo.DynamicDescriptorsBound = true;
}

} // namespace Diligent
