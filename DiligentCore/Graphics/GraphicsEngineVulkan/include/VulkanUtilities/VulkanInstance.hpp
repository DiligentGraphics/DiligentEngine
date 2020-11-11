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

#include <vector>
#include <memory>
#include "VulkanHeaders.h"

namespace VulkanUtilities
{

class VulkanInstance : public std::enable_shared_from_this<VulkanInstance>
{
public:
    // clang-format off
    VulkanInstance             (const VulkanInstance&)  = delete;
    VulkanInstance             (      VulkanInstance&&) = delete;
    VulkanInstance& operator = (const VulkanInstance&)  = delete;
    VulkanInstance& operator = (      VulkanInstance&&) = delete;
    // clang-format on

    static std::shared_ptr<VulkanInstance> Create(bool                   EnableValidation,
                                                  uint32_t               GlobalExtensionCount,
                                                  const char* const*     ppGlobalExtensionNames,
                                                  VkAllocationCallbacks* pVkAllocator);
    ~VulkanInstance();

    std::shared_ptr<VulkanInstance> GetSharedPtr()
    {
        return shared_from_this();
    }

    std::shared_ptr<const VulkanInstance> GetSharedPtr() const
    {
        return shared_from_this();
    }

    // clang-format off
    bool IsLayerAvailable    (const char* LayerName)    const;
    bool IsExtensionAvailable(const char* ExtensionName)const;
    bool IsExtensionEnabled  (const char* ExtensionName)const;

    VkPhysicalDevice SelectPhysicalDevice(uint32_t AdapterId)const;

    VkAllocationCallbacks* GetVkAllocator()const{return m_pVkAllocator;}
    VkInstance             GetVkInstance() const{return m_VkInstance;  }
    // clang-format on

private:
    VulkanInstance(bool                   EnableValidation,
                   uint32_t               GlobalExtensionCount,
                   const char* const*     ppGlobalExtensionNames,
                   VkAllocationCallbacks* pVkAllocator);

    bool                         m_DebugUtilsEnabled = false;
    VkAllocationCallbacks* const m_pVkAllocator;
    VkInstance                   m_VkInstance = VK_NULL_HANDLE;

    std::vector<VkLayerProperties>     m_Layers;
    std::vector<VkExtensionProperties> m_Extensions;
    std::vector<const char*>           m_EnabledExtensions;
    std::vector<VkPhysicalDevice>      m_PhysicalDevices;
};

} // namespace VulkanUtilities
