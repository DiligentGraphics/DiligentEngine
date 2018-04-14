/*     Copyright 2015-2018 Egor Yusov
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

//#include "RenderDeviceGL.h"
//#include "TextureGL.h"
//#include "BufferGL.h"

#include "Errors.h"
#include "TestCreateObjFromNativeResVK.h"

using namespace Diligent;

void TestCreateObjFromNativeResVK::CreateTexture(Diligent::ITexture *pTexture)
{
//#if PLATFORM_WIN32 || PLATFORM_LINUX || PLATFORM_ANDROID
//    RefCntAutoPtr<IRenderDeviceGL> pDeviceGL(m_pDevice, IID_RenderDeviceGL);
//    const auto &SrcTexDesc = pTexture->GetDesc();
//    if (SrcTexDesc.Type == RESOURCE_DIM_TEX_CUBE_ARRAY)
//        return;
//
//    RefCntAutoPtr<ITextureGL> pTextureGL(pTexture, IID_TextureGL);
//    auto GLHandle = pTextureGL->GetGLTextureHandle();
//    RefCntAutoPtr<ITexture> pAttachedTexture;
//    auto TmpTexDesc = SrcTexDesc;
//    TmpTexDesc.Width = 0;
//    TmpTexDesc.Height = 0;
//    TmpTexDesc.MipLevels = 0;
//    TmpTexDesc.Format = TEX_FORMAT_UNKNOWN;
//    pDeviceGL->CreateTextureFromGLHandle(GLHandle, TmpTexDesc, &pAttachedTexture);
//    ++m_NumTexturesCreated;
//    
//    const auto &TestTexDesc = pAttachedTexture->GetDesc();
//    VERIFY_EXPR(TestTexDesc == SrcTexDesc);
//    RefCntAutoPtr<ITextureGL> pAttachedTextureGL(pAttachedTexture, IID_TextureGL);
//    VERIFY_EXPR(pAttachedTextureGL->GetGLTextureHandle() == GLHandle);
//    VERIFY_EXPR(pAttachedTextureGL->GetBindTarget() == pTextureGL->GetBindTarget());
//    VERIFY_EXPR( reinterpret_cast<size_t>(pAttachedTextureGL->GetNativeHandle()) == GLHandle);
//#endif
}

void TestCreateObjFromNativeResVK::CreateBuffer(Diligent::IBuffer *pBuffer)
{
//#if PLATFORM_WIN32 || PLATFORM_LINUX || PLATFORM_ANDROID
//    RefCntAutoPtr<IRenderDeviceGL> pDeviceGL(m_pDevice, IID_RenderDeviceGL);
//    const auto &SrcBuffDesc = pBuffer->GetDesc();
//    RefCntAutoPtr<IBufferGL> pBufferGL(pBuffer, IID_BufferGL);
//    auto GLBufferHandle = pBufferGL->GetGLBufferHandle();
//  
//    RefCntAutoPtr<IBuffer> pBufferFromNativeGLHandle;
//    pDeviceGL->CreateBufferFromGLHandle(GLBufferHandle, SrcBuffDesc, &pBufferFromNativeGLHandle);
//    ++m_NumBuffersCreated;
//    
//    const auto &TestBufferDesc = pBufferFromNativeGLHandle->GetDesc();
//    VERIFY_EXPR(TestBufferDesc == SrcBuffDesc);
//
//    RefCntAutoPtr<IBufferGL> pTestBufferGL(pBufferFromNativeGLHandle, IID_BufferGL);
//    VERIFY_EXPR(pTestBufferGL->GetGLBufferHandle() == GLBufferHandle);
//    VERIFY_EXPR( reinterpret_cast<size_t>(pTestBufferGL->GetNativeHandle()) == GLBufferHandle);
//#endif
}
