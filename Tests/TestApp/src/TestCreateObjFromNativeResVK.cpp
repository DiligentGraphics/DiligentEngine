/*     Copyright 2015-2019 Egor Yusov
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF ANY PROPRIETARY RIGHTS.
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

#include "pch.h"

#if VULKAN_SUPPORTED
#include "vulkan.h"
#endif

#include "RenderDeviceVk.h"
#include "TextureVk.h"
#include "BufferVk.h"

#include "Errors.h"
#include "TestCreateObjFromNativeResVK.h"

using namespace Diligent;

void TestCreateObjFromNativeResVK::CreateTexture(Diligent::ITexture *pTexture)
{
#if VULKAN_SUPPORTED
    RefCntAutoPtr<IRenderDeviceVk> pDeviceVk(m_pDevice, IID_RenderDeviceVk);
    const auto& SrcTexDesc = pTexture->GetDesc();
    if (SrcTexDesc.Type == RESOURCE_DIM_TEX_CUBE_ARRAY)
        return;

    RefCntAutoPtr<ITextureVk> pTextureVk(pTexture, IID_TextureVk);
    auto VkHandle = pTextureVk->GetVkImage();
    RefCntAutoPtr<ITexture> pAttachedTexture;
    pDeviceVk->CreateTextureFromVulkanImage(VkHandle, SrcTexDesc, RESOURCE_STATE_UNKNOWN, &pAttachedTexture);
    ++m_NumTexturesCreated;
    
    const auto& TestTexDesc = pAttachedTexture->GetDesc();
    VERIFY_EXPR(TestTexDesc == SrcTexDesc);
    RefCntAutoPtr<ITextureVk> pAttachedTextureVk(pAttachedTexture, IID_TextureVk);
    VERIFY_EXPR(pAttachedTextureVk->GetVkImage() == VkHandle);
    VERIFY_EXPR( reinterpret_cast<VkImage>(pAttachedTextureVk->GetNativeHandle()) == VkHandle);
#endif
}

void TestCreateObjFromNativeResVK::CreateBuffer(Diligent::IBuffer *pBuffer)
{
#if VULKAN_SUPPORTED
    RefCntAutoPtr<IRenderDeviceVk> pDeviceVk(m_pDevice, IID_RenderDeviceVk);
    const auto &SrcBuffDesc = pBuffer->GetDesc();
    RefCntAutoPtr<IBufferVk> pBufferVk(pBuffer, IID_BufferVk);
    auto VkBufferHandle = pBufferVk->GetVkBuffer();
  
    RefCntAutoPtr<IBuffer> pBufferFromNativeVkHandle;
    pDeviceVk->CreateBufferFromVulkanResource(VkBufferHandle, SrcBuffDesc, RESOURCE_STATE_UNKNOWN, &pBufferFromNativeVkHandle);
    ++m_NumBuffersCreated;
    
    const auto &TestBufferDesc = pBufferFromNativeVkHandle->GetDesc();
    VERIFY_EXPR(TestBufferDesc == SrcBuffDesc);

    RefCntAutoPtr<IBufferVk> pTestBufferVk(pBufferFromNativeVkHandle, IID_BufferVk);
    VERIFY_EXPR(pTestBufferVk->GetVkBuffer() == VkBufferHandle);
    VERIFY_EXPR( reinterpret_cast<VkBuffer>(pTestBufferVk->GetNativeHandle()) == VkBufferHandle);
#endif
}
