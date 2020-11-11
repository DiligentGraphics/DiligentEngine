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

#include "TextureViewGLImpl.hpp"
#include "RenderDeviceGLImpl.hpp"
#include "TextureBaseGL.hpp"
#include "DeviceContextGLImpl.hpp"

namespace Diligent
{

TextureViewGLImpl::TextureViewGLImpl(IReferenceCounters*    pRefCounters,
                                     RenderDeviceGLImpl*    pDevice,
                                     const TextureViewDesc& ViewDesc,
                                     TextureBaseGL*         pTexture,
                                     bool                   bCreateGLViewTex,
                                     bool                   bIsDefaultView) :
    // clang-format off
    TTextureViewBase
    {
        pRefCounters,
        pDevice,
        ViewDesc,
        pTexture,
        bIsDefaultView
    },
    m_ViewTexGLHandle   {bCreateGLViewTex},
    m_ViewTexBindTarget {0               }
// clang-format on
{
}

TextureViewGLImpl::~TextureViewGLImpl()
{
}

IMPLEMENT_QUERY_INTERFACE(TextureViewGLImpl, IID_TextureViewGL, TTextureViewBase)

const GLObjectWrappers::GLTextureObj& TextureViewGLImpl::GetHandle()
{
    if (m_ViewTexGLHandle)
        return m_ViewTexGLHandle;
    else
    {
        auto* pTexture = GetTexture();
        CHECK_DYNAMIC_TYPE(TextureBaseGL, pTexture);
        return static_cast<TextureBaseGL*>(pTexture)->GetGLHandle();
    }
}

GLenum TextureViewGLImpl::GetBindTarget()
{
    if (m_ViewTexGLHandle)
        return m_ViewTexBindTarget;
    else
    {
        auto* pTexture = GetTexture();
        CHECK_DYNAMIC_TYPE(TextureBaseGL, pTexture);
        return static_cast<TextureBaseGL*>(pTexture)->GetBindTarget();
    }
}

} // namespace Diligent
