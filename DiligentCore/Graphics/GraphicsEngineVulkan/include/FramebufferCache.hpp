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
/// Declaration of Diligent::FramebufferCache class

#include <unordered_map>
#include <mutex>
#include "VulkanUtilities/VulkanObjectWrappers.hpp"

namespace Diligent
{

class RenderDeviceVkImpl;
class FramebufferCache
{
public:
    FramebufferCache(RenderDeviceVkImpl& DeviceVKImpl) :
        m_DeviceVk{DeviceVKImpl}
    {}

    // clang-format off
    FramebufferCache             (const FramebufferCache&) = delete;
    FramebufferCache             (FramebufferCache&&)      = delete;
    FramebufferCache& operator = (const FramebufferCache&) = delete;
    FramebufferCache& operator = (FramebufferCache&&)      = delete;
    // clang-format on

    ~FramebufferCache();

    // This structure is used as the key to find framebuffer
    struct FramebufferCacheKey
    {
        // Default memeber initialization is intentionally omitted
        VkRenderPass Pass;
        Uint32       NumRenderTargets;
        VkImageView  DSV;
        VkImageView  RTVs[MAX_RENDER_TARGETS];
        Uint64       CommandQueueMask;

        bool   operator==(const FramebufferCacheKey& rhs) const;
        size_t GetHash() const;

    private:
        mutable size_t Hash = 0;
    };

    VkFramebuffer GetFramebuffer(const FramebufferCacheKey& Key, uint32_t width, uint32_t height, uint32_t layers);
    void          OnDestroyImageView(VkImageView ImgView);
    void          OnDestroyRenderPass(VkRenderPass Pass);

private:
    RenderDeviceVkImpl& m_DeviceVk;

    struct FramebufferCacheKeyHash
    {
        std::size_t operator()(const FramebufferCacheKey& Key) const
        {
            return Key.GetHash();
        }
    };

    std::mutex                                                                                            m_Mutex;
    std::unordered_map<FramebufferCacheKey, VulkanUtilities::FramebufferWrapper, FramebufferCacheKeyHash> m_Cache;

    std::unordered_multimap<VkImageView, FramebufferCacheKey>  m_ViewToKeyMap;
    std::unordered_multimap<VkRenderPass, FramebufferCacheKey> m_RenderPassToKeyMap;
};

} // namespace Diligent
