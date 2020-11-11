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

#include "BaseInterfacesGL.h"
#include "TextureViewGL.h"
#include "TextureViewBase.hpp"
#include "RenderDevice.h"
#include "GLObjectWrapper.hpp"
#include "RenderDeviceGLImpl.hpp"

namespace Diligent
{

class FixedBlockMemoryAllocator;

/// Texture view implementation in OpenGL backend.
class TextureViewGLImpl final : public TextureViewBase<ITextureViewGL, RenderDeviceGLImpl>
{
public:
    using TTextureViewBase = TextureViewBase<ITextureViewGL, RenderDeviceGLImpl>;

    TextureViewGLImpl(IReferenceCounters*           pRefCounters,
                      RenderDeviceGLImpl*           pDevice,
                      const struct TextureViewDesc& ViewDesc,
                      class TextureBaseGL*          pTexture,
                      bool                          bCreateGLViewTex,
                      bool                          bIsDefaultView);
    ~TextureViewGLImpl();

    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override final;

    const GLObjectWrappers::GLTextureObj& GetHandle();

    GLenum GetBindTarget();
    void   SetBindTarget(GLenum ViewTexBindTarget) { m_ViewTexBindTarget = ViewTexBindTarget; }

protected:
    GLObjectWrappers::GLTextureObj m_ViewTexGLHandle;
    GLenum                         m_ViewTexBindTarget;
};

} // namespace Diligent
