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

#if PLATFORM_WIN32

#    ifndef GLEW_STATIC
#        define GLEW_STATIC // Must be defined to use static version of glew
#    endif
#    include "GL/glew.h"
#elif PLATFORM_LINUX

#    ifndef GLEW_STATIC
#        define GLEW_STATIC // Must be defined to use static version of glew
#    endif
#    ifndef GLEW_NO_GLU
#        define GLEW_NO_GLU
#    endif

#    include "GL/glew.h"

// Undefine beautiful defines from GL/glx.h -> X11/Xlib.h
#    ifdef Bool
#        undef Bool
#    endif
#    ifdef True
#        undef True
#    endif
#    ifdef False
#        undef False
#    endif
#    ifdef Status
#        undef Status
#    endif
#    ifdef Success
#        undef Success
#    endif

#elif PLATFORM_MACOS

#    ifndef GLEW_STATIC
#        define GLEW_STATIC // Must be defined to use static version of glew
#    endif
#    ifndef GLEW_NO_GLU
#        define GLEW_NO_GLU
#    endif

#    include "GL/glew.h"

#elif PLATFORM_ANDROID

#    include <GLES3/gl3.h>
#    include <GLES3/gl3ext.h>

#elif PLATFORM_IOS

#    include <OpenGLES/ES3/gl.h>

#else
#    error Unsupported platform
#endif

#include "RenderDeviceGL.h"
#include "TextureGL.h"
#include "BufferGL.h"
#include "GraphicsAccessories.hpp"

#include "GL/CreateObjFromNativeResGL.hpp"

#include "gtest/gtest.h"

namespace Diligent
{

namespace Testing
{

void TestCreateObjFromNativeResGL::CreateTexture(Diligent::ITexture* pTexture)
{
#if PLATFORM_WIN32 || PLATFORM_LINUX || PLATFORM_ANDROID
    RefCntAutoPtr<IRenderDeviceGL> pDeviceGL(m_pDevice, IID_RenderDeviceGL);
    RefCntAutoPtr<ITextureGL>      pTextureGL(pTexture, IID_TextureGL);
    ASSERT_NE(pDeviceGL, nullptr);
    ASSERT_NE(pTextureGL, nullptr);

    const auto& SrcTexDesc = pTexture->GetDesc();
    if (SrcTexDesc.Type == RESOURCE_DIM_TEX_CUBE_ARRAY)
        return;

    auto GLHandle = pTextureGL->GetGLTextureHandle();
    ASSERT_NE(GLHandle, (GLuint)0);

    auto TmpTexDesc = SrcTexDesc;

    TmpTexDesc.Width     = 0;
    TmpTexDesc.Height    = 0;
    TmpTexDesc.MipLevels = 0;
    TmpTexDesc.Format    = TEX_FORMAT_UNKNOWN;

    RefCntAutoPtr<ITexture> pAttachedTexture;
    pDeviceGL->CreateTextureFromGLHandle(GLHandle, 0, TmpTexDesc, RESOURCE_STATE_UNKNOWN, &pAttachedTexture);
    ASSERT_NE(pAttachedTexture, nullptr);

    auto TestTexDesc = pAttachedTexture->GetDesc();
    if (m_pDevice->GetTextureFormatInfo(SrcTexDesc.Format).IsTypeless)
        TestTexDesc.Format = SrcTexDesc.Format;

    EXPECT_EQ(TestTexDesc, SrcTexDesc) << "Src tex desc:  " << GetObjectDescString(SrcTexDesc)
                                       << "\nTest tex desc: " << GetObjectDescString(TestTexDesc);

    RefCntAutoPtr<ITextureGL> pAttachedTextureGL(pAttachedTexture, IID_TextureGL);
    ASSERT_NE(pAttachedTextureGL, nullptr);
    EXPECT_EQ(pAttachedTextureGL->GetGLTextureHandle(), GLHandle);
    EXPECT_EQ(pAttachedTextureGL->GetBindTarget(), pTextureGL->GetBindTarget());
    EXPECT_EQ(reinterpret_cast<size_t>(pAttachedTextureGL->GetNativeHandle()), GLHandle);
#endif
}

void TestCreateObjFromNativeResGL::CreateBuffer(Diligent::IBuffer* pBuffer)
{
#if PLATFORM_WIN32 || PLATFORM_LINUX || PLATFORM_ANDROID
    RefCntAutoPtr<IRenderDeviceGL> pDeviceGL(m_pDevice, IID_RenderDeviceGL);
    RefCntAutoPtr<IBufferGL>       pBufferGL(pBuffer, IID_BufferGL);
    ASSERT_NE(pDeviceGL, nullptr);
    ASSERT_NE(pBufferGL, nullptr);

    const auto& SrcBuffDesc    = pBuffer->GetDesc();
    auto        GLBufferHandle = pBufferGL->GetGLBufferHandle();
    ASSERT_NE(GLBufferHandle, (GLuint)0);

    RefCntAutoPtr<IBuffer> pBufferFromNativeGLHandle;
    pDeviceGL->CreateBufferFromGLHandle(GLBufferHandle, SrcBuffDesc, RESOURCE_STATE_UNKNOWN, &pBufferFromNativeGLHandle);
    ASSERT_NE(pBufferFromNativeGLHandle, nullptr);

    const auto& TestBufferDesc = pBufferFromNativeGLHandle->GetDesc();
    EXPECT_EQ(TestBufferDesc, SrcBuffDesc) << "Src buff desc:  " << GetObjectDescString(SrcBuffDesc)
                                           << "\nTest buff desc: " << GetObjectDescString(TestBufferDesc);

    RefCntAutoPtr<IBufferGL> pTestBufferGL(pBufferFromNativeGLHandle, IID_BufferGL);
    ASSERT_NE(pTestBufferGL, nullptr);
    EXPECT_EQ(pTestBufferGL->GetGLBufferHandle(), GLBufferHandle);
    EXPECT_EQ(reinterpret_cast<size_t>(pTestBufferGL->GetNativeHandle()), GLBufferHandle);
#endif
}

} // namespace Testing

} // namespace Diligent
