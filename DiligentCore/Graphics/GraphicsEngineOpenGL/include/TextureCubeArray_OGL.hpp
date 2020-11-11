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

#include "TextureBaseGL.hpp"

namespace Diligent
{

/// Cube array texture implementation in OpenGL backend.
class TextureCubeArray_OGL final : public TextureBaseGL
{
public:
    TextureCubeArray_OGL(IReferenceCounters*        pRefCounters,
                         FixedBlockMemoryAllocator& TexViewObjAllocator,
                         class RenderDeviceGLImpl*  pDeviceGL,
                         class GLContextState&      GLState,
                         const TextureDesc&         TexDesc,
                         const TextureData*         pInitData         = nullptr,
                         bool                       bIsDeviceInternal = false);

    TextureCubeArray_OGL(IReferenceCounters*        pRefCounters,
                         FixedBlockMemoryAllocator& TexViewObjAllocator,
                         class RenderDeviceGLImpl*  pDeviceGL,
                         class GLContextState&      GLState,
                         const TextureDesc&         TexDesc,
                         GLuint                     GLTextureHandle,
                         GLuint                     GLBindTarget,
                         bool                       bIsDeviceInternal = false);
    ~TextureCubeArray_OGL();

    /// Implementation of TextureBaseGL::UpdateData() for cube texture array.
    virtual void UpdateData(class GLContextState&    CtxState,
                            Uint32                   MipLevel,
                            Uint32                   Slice,
                            const Box&               DstBox,
                            const TextureSubResData& SubresData) override final;

    /// Implementation of TextureBaseGL::AttachToFramebuffer() for cube texture array.
    virtual void AttachToFramebuffer(const struct TextureViewDesc& ViewDesc,
                                     GLenum                        AttachmentPoint) override final;
};

} // namespace Diligent
