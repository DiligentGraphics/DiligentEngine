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
 *  In no event and under no legal theory, whether in tort (including neVkigence), 
 *  contract, or otherwise, unless required by applicable law (such as deliberate 
 *  and grossly neVkigent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental, 
 *  or consequential damages of any character arising as a result of this License or 
 *  out of the use or inability to use the software (including but not limited to damages 
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and 
 *  all other commercial damages or losses), even if such Contributor has been advised 
 *  of the possibility of such damages.
 */

#include "RenderDeviceMtl.h"
#include "TextureMtl.h"
#include "BufferMtl.h"
#include "GraphicsAccessories.hpp"

#include "Metal/CreateObjFromNativeResMtl.hpp"

#include "gtest/gtest.h"

namespace Diligent
{

namespace Testing
{

void TestCreateObjFromNativeResMtl::CreateTexture(Diligent::ITexture* pTexture)
{
#if 0
    RefCntAutoPtr<IRenderDeviceVk> pDeviceVk(m_pDevice, IID_RenderDeviceVk);
    RefCntAutoPtr<ITextureVk>      pTextureVk(pTexture, IID_TextureVk);
    ASSERT_NE(pDeviceVk, nullptr);
    ASSERT_NE(pTextureVk, nullptr);

    const auto& SrcTexDesc = pTexture->GetDesc();
    if (SrcTexDesc.Type == RESOURCE_DIM_TEX_CUBE_ARRAY)
        return;

    auto VkHandle = pTextureVk->GetVkImage();
    ASSERT_NE(VkHandle, (VkImage)VK_NULL_HANDLE);

    RefCntAutoPtr<ITexture> pAttachedTexture;
    pDeviceVk->CreateTextureFromVulkanImage(VkHandle, SrcTexDesc, RESOURCE_STATE_UNKNOWN, &pAttachedTexture);
    ASSERT_NE(pAttachedTexture, nullptr);

    const auto& TestTexDesc = pAttachedTexture->GetDesc();
    EXPECT_EQ(TestTexDesc, SrcTexDesc) << "Src tex desc:  " << GetObjectDescString(SrcTexDesc)
                                       << "\nTest tex desc: " << GetObjectDescString(TestTexDesc);

    RefCntAutoPtr<ITextureVk> pAttachedTextureVk(pAttachedTexture, IID_TextureVk);
    ASSERT_NE(pAttachedTextureVk, nullptr);
    EXPECT_EQ(pAttachedTextureVk->GetVkImage(), VkHandle);
    EXPECT_EQ(reinterpret_cast<VkImage>(pAttachedTextureVk->GetNativeHandle()), VkHandle);
#endif
}

void TestCreateObjFromNativeResMtl::CreateBuffer(Diligent::IBuffer* pBuffer)
{
#if 0
    RefCntAutoPtr<IRenderDeviceVk> pDeviceVk(m_pDevice, IID_RenderDeviceVk);
    RefCntAutoPtr<IBufferVk>       pBufferVk(pBuffer, IID_BufferVk);
    ASSERT_NE(pDeviceVk, nullptr);
    ASSERT_NE(pBufferVk, nullptr);

    auto VkBufferHandle = pBufferVk->GetVkBuffer();
    ASSERT_NE(VkBufferHandle, (VkBuffer)VK_NULL_HANDLE);

    const auto& SrcBuffDesc = pBuffer->GetDesc();

    RefCntAutoPtr<IBuffer> pBufferFromNativeVkHandle;
    pDeviceVk->CreateBufferFromVulkanResource(VkBufferHandle, SrcBuffDesc, RESOURCE_STATE_UNKNOWN, &pBufferFromNativeVkHandle);
    ASSERT_NE(pBufferFromNativeVkHandle, nullptr);

    const auto& TestBufferDesc = pBufferFromNativeVkHandle->GetDesc();
    EXPECT_EQ(TestBufferDesc, SrcBuffDesc) << "Src buff desc:  " << GetObjectDescString(SrcBuffDesc)
                                           << "\nTest buff desc: " << GetObjectDescString(TestBufferDesc);

    RefCntAutoPtr<IBufferVk> pTestBufferVk(pBufferFromNativeVkHandle, IID_BufferVk);
    ASSERT_NE(pTestBufferVk, nullptr);
    EXPECT_EQ(pTestBufferVk->GetVkBuffer(), VkBufferHandle);
    EXPECT_EQ(reinterpret_cast<VkBuffer>(pTestBufferVk->GetNativeHandle()), VkBufferHandle);
#endif
}

} // namespace Testing

} // namespace Diligent
