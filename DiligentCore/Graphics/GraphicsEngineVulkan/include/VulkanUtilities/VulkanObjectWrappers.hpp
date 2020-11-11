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

#include <memory>
#include "VulkanLogicalDevice.hpp"

namespace VulkanUtilities
{

// In 32-bit version, all Vulkan handles are typedefed as uint64_t, so we have to
// use another way to distinguish objects.
enum class VulkanHandleTypeId : uint32_t;

template <typename VulkanObjectType, VulkanHandleTypeId>
class VulkanObjectWrapper
{
public:
    using VkObjectType = VulkanObjectType;

    // clang-format off
    VulkanObjectWrapper() : 
        m_pLogicalDevice{nullptr       },
        m_VkObject      {VK_NULL_HANDLE}
    {}

    VulkanObjectWrapper(std::shared_ptr<const VulkanLogicalDevice> pLogicalDevice, VulkanObjectType&& vkObject) :
        m_pLogicalDevice{pLogicalDevice},
        m_VkObject      {vkObject      }
    {
        vkObject = VK_NULL_HANDLE;
    }
    // This constructor does not take ownership of the vulkan object
    explicit VulkanObjectWrapper(VulkanObjectType vkObject) :
        m_VkObject {vkObject}
    {
    }

    VulkanObjectWrapper             (const VulkanObjectWrapper&) = delete;
    VulkanObjectWrapper& operator = (const VulkanObjectWrapper&) = delete;

    VulkanObjectWrapper(VulkanObjectWrapper&& rhs)noexcept : 
        m_pLogicalDevice{std::move(rhs.m_pLogicalDevice)},
        m_VkObject      {rhs.m_VkObject                 }
    {
        rhs.m_VkObject = VK_NULL_HANDLE;
    }

    // clang-format on

    VulkanObjectWrapper& operator=(VulkanObjectWrapper&& rhs) noexcept
    {
        Release();
        m_pLogicalDevice = std::move(rhs.m_pLogicalDevice);
        m_VkObject       = rhs.m_VkObject;
        rhs.m_VkObject   = VK_NULL_HANDLE;
        return *this;
    }

    operator VulkanObjectType() const
    {
        return m_VkObject;
    }

    void Release()
    {
        // For externally managed objects, m_pLogicalDevice is null
        if (m_pLogicalDevice && m_VkObject != VK_NULL_HANDLE)
        {
            m_pLogicalDevice->ReleaseVulkanObject(std::move(*this));
        }
        m_VkObject = VK_NULL_HANDLE;
        m_pLogicalDevice.reset();
    }

    ~VulkanObjectWrapper()
    {
        Release();
    }

private:
    friend class VulkanLogicalDevice;

    std::shared_ptr<const VulkanLogicalDevice> m_pLogicalDevice;
    VulkanObjectType                           m_VkObject;
};

} // namespace VulkanUtilities
