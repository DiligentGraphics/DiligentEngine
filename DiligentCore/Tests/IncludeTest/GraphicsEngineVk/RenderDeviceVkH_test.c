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

#include "DiligentCore/ThirdParty/Vulkan-Headers/include/vulkan/vulkan.h"
#include "DiligentCore/Graphics/GraphicsEngineVulkan/interface/RenderDeviceVk.h"

void TestRenderDeviceVkCInterface(IRenderDeviceVk* pDevice)
{
    VkDevice vkDevice = IRenderDeviceVk_GetVkDevice(pDevice);
    (void)vkDevice;

    VkPhysicalDevice vkPhysDevice = IRenderDeviceVk_GetVkPhysicalDevice(pDevice);
    (void)vkPhysDevice;

    VkInstance vkInst = IRenderDeviceVk_GetVkInstance(pDevice);
    (void)vkInst;

    Uint64 Fence = IRenderDeviceVk_GetNextFenceValue(pDevice, (Uint32)0);
    (void)Fence;

    Fence = IRenderDeviceVk_GetCompletedFenceValue(pDevice, (Uint32)0);

    bool IsSignaled = IRenderDeviceVk_IsFenceSignaled(pDevice, (Uint32)0, (Uint64)0);
    (void)IsSignaled;

    IRenderDeviceVk_CreateTextureFromVulkanImage(pDevice, (VkImage)NULL, (TextureDesc*)NULL, RESOURCE_STATE_SHADER_RESOURCE, (ITexture**)NULL);
    IRenderDeviceVk_CreateBufferFromVulkanResource(pDevice, (VkBuffer)NULL, (BufferDesc*)NULL, RESOURCE_STATE_CONSTANT_BUFFER, (IBuffer**)NULL);
}
