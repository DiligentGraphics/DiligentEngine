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
#include "FramebufferCache.hpp"
#include "HashUtils.hpp"
#include "RenderDeviceVkImpl.hpp"

namespace Diligent
{


bool FramebufferCache::FramebufferCacheKey::operator==(const FramebufferCacheKey& rhs) const
{
    // clang-format off
    if (GetHash()        != rhs.GetHash()        ||
        Pass             != rhs.Pass             ||
        NumRenderTargets != rhs.NumRenderTargets ||
        DSV              != rhs.DSV              ||
        CommandQueueMask != rhs.CommandQueueMask)
    {
        return false;
    }
    // clang-format on

    for (Uint32 rt = 0; rt < NumRenderTargets; ++rt)
        if (RTVs[rt] != rhs.RTVs[rt])
            return false;

    return true;
}

size_t FramebufferCache::FramebufferCacheKey::GetHash() const
{
    if (Hash == 0)
    {
        Hash = ComputeHash(Pass, NumRenderTargets, DSV, CommandQueueMask);
        for (Uint32 rt = 0; rt < NumRenderTargets; ++rt)
            HashCombine(Hash, RTVs[rt]);
    }
    return Hash;
}

VkFramebuffer FramebufferCache::GetFramebuffer(const FramebufferCacheKey& Key, uint32_t width, uint32_t height, uint32_t layers)
{
    std::lock_guard<std::mutex> Lock{m_Mutex};
    auto                        it = m_Cache.find(Key);
    if (it != m_Cache.end())
    {
        return it->second;
    }
    else
    {
        VkFramebufferCreateInfo FramebufferCI = {};

        FramebufferCI.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        FramebufferCI.pNext           = nullptr;
        FramebufferCI.flags           = 0; // reserved for future use
        FramebufferCI.renderPass      = Key.Pass;
        FramebufferCI.attachmentCount = (Key.DSV != VK_NULL_HANDLE ? 1 : 0) + Key.NumRenderTargets;
        VkImageView Attachments[1 + MAX_RENDER_TARGETS];
        uint32_t    attachment = 0;
        if (Key.DSV != VK_NULL_HANDLE)
            Attachments[attachment++] = Key.DSV;
        for (Uint32 rt = 0; rt < Key.NumRenderTargets; ++rt)
            Attachments[attachment++] = Key.RTVs[rt];
        VERIFY_EXPR(attachment == FramebufferCI.attachmentCount);
        FramebufferCI.pAttachments = Attachments;
        FramebufferCI.width        = width;
        FramebufferCI.height       = height;
        FramebufferCI.layers       = layers;
        auto          Framebuffer  = m_DeviceVk.GetLogicalDevice().CreateFramebuffer(FramebufferCI);
        VkFramebuffer fb           = Framebuffer;

        auto new_it = m_Cache.insert(std::make_pair(Key, std::move(Framebuffer)));
        VERIFY(new_it.second, "New framebuffer must be inserted into the map");
        (void)new_it;

        m_RenderPassToKeyMap.emplace(Key.Pass, Key);
        if (Key.DSV != VK_NULL_HANDLE)
            m_ViewToKeyMap.emplace(Key.DSV, Key);
        for (Uint32 rt = 0; rt < Key.NumRenderTargets; ++rt)
            if (Key.RTVs[rt] != VK_NULL_HANDLE)
                m_ViewToKeyMap.emplace(Key.RTVs[rt], Key);

        return fb;
    }
}

FramebufferCache::~FramebufferCache()
{
    VERIFY(m_Cache.empty(), "All framebuffers must be released");
    VERIFY(m_ViewToKeyMap.empty(), "All image views must be released and the cache must be notified");
    VERIFY(m_RenderPassToKeyMap.empty(), "All render passes must be released and the cache must be notified");
}

void FramebufferCache::OnDestroyImageView(VkImageView ImgView)
{
    // TODO: when a render pass is released, we need to also destroy
    // all entries in the m_ViewToKeyMap that refer to all keys with
    // that render pass

    std::lock_guard<std::mutex> Lock{m_Mutex};

    auto equal_range = m_ViewToKeyMap.equal_range(ImgView);
    for (auto it = equal_range.first; it != equal_range.second; ++it)
    {
        auto fb_it = m_Cache.find(it->second);
        // Multiple image views may be associated with the same key.
        // The framebuffer is deleted whenever any of the image views is deleted
        if (fb_it != m_Cache.end())
        {
            m_DeviceVk.SafeReleaseDeviceObject(std::move(fb_it->second), it->second.CommandQueueMask);
            m_Cache.erase(fb_it);
        }
    }
    m_ViewToKeyMap.erase(equal_range.first, equal_range.second);
}

void FramebufferCache::OnDestroyRenderPass(VkRenderPass Pass)
{
    // TODO: when an image view is released, we need to also destroy
    // all entries in the m_RenderPassToKeyMap that refer to the keys
    // with the same image view
    std::lock_guard<std::mutex> Lock{m_Mutex};

    auto equal_range = m_RenderPassToKeyMap.equal_range(Pass);
    for (auto it = equal_range.first; it != equal_range.second; ++it)
    {
        auto fb_it = m_Cache.find(it->second);
        // Multiple image views may be associated with the same key.
        // The framebuffer is deleted whenever any of the image views or render pass is destroyed
        if (fb_it != m_Cache.end())
        {
            m_DeviceVk.SafeReleaseDeviceObject(std::move(fb_it->second), it->second.CommandQueueMask);
            m_Cache.erase(fb_it);
        }
    }
    m_RenderPassToKeyMap.erase(equal_range.first, equal_range.second);
}

} // namespace Diligent
