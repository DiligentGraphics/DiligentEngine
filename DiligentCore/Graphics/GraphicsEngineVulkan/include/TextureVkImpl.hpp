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
/// Declaration of Diligent::TextureVkImpl class

#include "TextureVk.h"
#include "RenderDeviceVk.h"
#include "TextureBase.hpp"
#include "TextureViewVkImpl.hpp"
#include "VulkanUtilities/VulkanMemoryManager.hpp"
#include "RenderDeviceVkImpl.hpp"

namespace Diligent
{

class FixedBlockMemoryAllocator;

/// Texture object implementation in Vulkan backend.
class TextureVkImpl final : public TextureBase<ITextureVk, RenderDeviceVkImpl, TextureViewVkImpl, FixedBlockMemoryAllocator>
{
public:
    using TTextureBase = TextureBase<ITextureVk, RenderDeviceVkImpl, TextureViewVkImpl, FixedBlockMemoryAllocator>;
    using ViewImplType = TextureViewVkImpl;

    // Creates a new Vk resource
    TextureVkImpl(IReferenceCounters*        pRefCounters,
                  FixedBlockMemoryAllocator& TexViewObjAllocator,
                  RenderDeviceVkImpl*        pDeviceVk,
                  const TextureDesc&         TexDesc,
                  const TextureData*         pInitData = nullptr);

    // Attaches to an existing Vk resource
    TextureVkImpl(IReferenceCounters*        pRefCounters,
                  FixedBlockMemoryAllocator& TexViewObjAllocator,
                  class RenderDeviceVkImpl*  pDeviceVk,
                  const TextureDesc&         TexDesc,
                  RESOURCE_STATE             InitialState,
                  VkImage                    VkImageHandle);

    ~TextureVkImpl();

    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override final;

    /// Implementation of ITextureVk::GetVkImage().
    virtual VkImage DILIGENT_CALL_TYPE GetVkImage() const override final { return m_VulkanImage; }

    /// Implementation of ITexture::GetNativeHandle() in Vulkan backend.
    virtual void* DILIGENT_CALL_TYPE GetNativeHandle() override final
    {
        auto vkImage = GetVkImage();
        return reinterpret_cast<void*>(vkImage);
    }

    /// Implementation of ITextureVk::SetLayout().
    virtual void DILIGENT_CALL_TYPE SetLayout(VkImageLayout Layout) override final;

    /// Implementation of ITextureVk::GetLayout().
    virtual VkImageLayout DILIGENT_CALL_TYPE GetLayout() const override final;

    VkBuffer GetVkStagingBuffer() const
    {
        return m_StagingBuffer;
    }

    uint8_t* GetStagingDataCPUAddress() const
    {
        auto* StagingDataCPUAddress = reinterpret_cast<uint8_t*>(m_MemoryAllocation.Page->GetCPUMemory());
        VERIFY_EXPR(StagingDataCPUAddress != nullptr);
        StagingDataCPUAddress += m_StagingDataAlignedOffset;
        return StagingDataCPUAddress;
    }

    void InvalidateStagingRange(VkDeviceSize Offset, VkDeviceSize Size);

    // Buffer offset must be a multiple of 4 (18.4)
    static constexpr Uint32 StagingBufferOffsetAlignment = 4;

protected:
    void CreateViewInternal(const struct TextureViewDesc& ViewDesc, ITextureView** ppView, bool bIsDefaultView) override;
    //void PrepareVkInitData(const TextureData &InitData, Uint32 NumSubresources, std::vector<Vk_SUBRESOURCE_DATA> &VkInitData);

    bool CheckCSBasedMipGenerationSupport(VkFormat vkFmt) const;

    VulkanUtilities::ImageViewWrapper CreateImageView(TextureViewDesc& ViewDesc);

    VulkanUtilities::ImageWrapper           m_VulkanImage;
    VulkanUtilities::BufferWrapper          m_StagingBuffer;
    VulkanUtilities::VulkanMemoryAllocation m_MemoryAllocation;
    VkDeviceSize                            m_StagingDataAlignedOffset;
    bool                                    m_bCSBasedMipGenerationSupported = false;
};

} // namespace Diligent
