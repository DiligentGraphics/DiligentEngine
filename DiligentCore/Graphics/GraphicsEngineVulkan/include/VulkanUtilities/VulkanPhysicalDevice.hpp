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
#include <vector>
#include "VulkanInstance.hpp"

namespace VulkanUtilities
{

class VulkanPhysicalDevice
{
public:
    struct ExtensionFeatures
    {
        VkPhysicalDeviceMeshShaderFeaturesNV         MeshShader        = {};
        VkPhysicalDevice16BitStorageFeaturesKHR      Storage16Bit      = {};
        VkPhysicalDevice8BitStorageFeaturesKHR       Storage8Bit       = {};
        VkPhysicalDeviceShaderFloat16Int8FeaturesKHR ShaderFloat16Int8 = {};
    };

public:
    // clang-format off
    VulkanPhysicalDevice             (const VulkanPhysicalDevice&) = delete;
    VulkanPhysicalDevice             (VulkanPhysicalDevice&&)      = delete;
    VulkanPhysicalDevice& operator = (const VulkanPhysicalDevice&) = delete;
    VulkanPhysicalDevice& operator = (VulkanPhysicalDevice&&)      = delete;
    // clang-format on

    static std::unique_ptr<VulkanPhysicalDevice> Create(VkPhysicalDevice      vkDevice,
                                                        const VulkanInstance& Instance);

    // clang-format off
    uint32_t         FindQueueFamily     (VkQueueFlags QueueFlags)                           const;
    VkPhysicalDevice GetVkDeviceHandle   ()                                                  const { return m_VkDevice; }
    bool             IsExtensionSupported(const char* ExtensionName)                         const;
    bool             CheckPresentSupport (uint32_t queueFamilyIndex, VkSurfaceKHR VkSurface) const;
    // clang-format on

    static constexpr uint32_t InvalidMemoryTypeIndex = ~uint32_t{0};

    uint32_t GetMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties) const;

    const VkPhysicalDeviceProperties&       GetProperties() const { return m_Properties; }
    const VkPhysicalDeviceFeatures&         GetFeatures() const { return m_Features; }
    const ExtensionFeatures&                GetExtFeatures() const { return m_ExtFeatures; }
    const VkPhysicalDeviceMemoryProperties& GetMemoryProperties() const { return m_MemoryProperties; }
    VkFormatProperties                      GetPhysicalDeviceFormatProperties(VkFormat imageFormat) const;

private:
    VulkanPhysicalDevice(VkPhysicalDevice      vkDevice,
                         const VulkanInstance& Instance);

    const VkPhysicalDevice               m_VkDevice;
    VkPhysicalDeviceProperties           m_Properties       = {};
    VkPhysicalDeviceFeatures             m_Features         = {};
    VkPhysicalDeviceMemoryProperties     m_MemoryProperties = {};
    ExtensionFeatures                    m_ExtFeatures      = {};
    std::vector<VkQueueFamilyProperties> m_QueueFamilyProperties;
    std::vector<VkExtensionProperties>   m_SupportedExtensions;
};

} // namespace VulkanUtilities
