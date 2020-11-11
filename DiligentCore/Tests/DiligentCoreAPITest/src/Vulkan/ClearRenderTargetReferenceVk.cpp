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

#include "Vulkan/TestingEnvironmentVk.hpp"
#include "Vulkan/TestingSwapChainVk.hpp"

#include "DeviceContextVk.h"

#include "volk/volk.h"

namespace Diligent
{

namespace Testing
{

void ClearRenderTargetReferenceVk(ISwapChain* pSwapChain, const float ClearColor[])
{
    auto* pEnv     = TestingEnvironmentVk::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    VkResult res = VK_SUCCESS;
    (void)res;

    auto* pTestingSwapChainVk = ValidatedCast<TestingSwapChainVk>(pSwapChain);

    VkCommandBuffer vkCmdBuffer = pEnv->AllocateCommandBuffer();

    pTestingSwapChainVk->TransitionRenderTarget(vkCmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    VkClearColorValue vkClearColor = {};

    vkClearColor.float32[0] = ClearColor[0];
    vkClearColor.float32[1] = ClearColor[1];
    vkClearColor.float32[2] = ClearColor[2];
    vkClearColor.float32[3] = ClearColor[3];

    VkImageSubresourceRange SubResRange = {};

    SubResRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    SubResRange.layerCount = 1;
    SubResRange.levelCount = 1;
    vkCmdClearColorImage(vkCmdBuffer, pTestingSwapChainVk->GetVkRenderTargetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &vkClearColor, 1, &SubResRange);

    res = vkEndCommandBuffer(vkCmdBuffer);
    VERIFY(res >= 0, "Failed to end command buffer");

    RefCntAutoPtr<IDeviceContextVk> pContextVk{pContext, IID_DeviceContextVk};

    auto* pQeueVk = pContextVk->LockCommandQueue();
    auto  vkQueue = pQeueVk->GetVkQueue();

    VkSubmitInfo SubmitInfo       = {};
    SubmitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.pCommandBuffers    = &vkCmdBuffer;
    SubmitInfo.commandBufferCount = 1;
    vkQueueSubmit(vkQueue, 1, &SubmitInfo, VK_NULL_HANDLE);

    pContextVk->UnlockCommandQueue();
}

} // namespace Testing

} // namespace Diligent
