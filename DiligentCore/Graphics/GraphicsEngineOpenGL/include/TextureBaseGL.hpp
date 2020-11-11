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
#include "TextureGL.h"
#include "TextureBase.hpp"
#include "RenderDevice.h"
#include "GLObjectWrapper.hpp"
#include "TextureViewGLImpl.hpp"
#include "AsyncWritableResource.hpp"
#include "RenderDeviceGLImpl.hpp"

namespace Diligent
{

class FixedBlockMemoryAllocator;

/// Base implementation of a texture object in OpenGL backend.
class TextureBaseGL : public TextureBase<ITextureGL, RenderDeviceGLImpl, TextureViewGLImpl, FixedBlockMemoryAllocator>, public AsyncWritableResource
{
public:
    using TTextureBase = TextureBase<ITextureGL, RenderDeviceGLImpl, TextureViewGLImpl, FixedBlockMemoryAllocator>;
    using ViewImplType = TextureViewGLImpl;

    TextureBaseGL(IReferenceCounters*        pRefCounters,
                  FixedBlockMemoryAllocator& TexViewObjAllocator,
                  RenderDeviceGLImpl*        pDeviceGL,
                  const TextureDesc&         TexDesc,
                  GLenum                     BindTarget,
                  const TextureData*         pInitData         = nullptr,
                  bool                       bIsDeviceInternal = false);

    TextureBaseGL(IReferenceCounters*        pRefCounters,
                  FixedBlockMemoryAllocator& TexViewObjAllocator,
                  RenderDeviceGLImpl*        pDeviceGL,
                  GLContextState&            GLState,
                  const TextureDesc&         TexDesc,
                  GLuint                     GLTextureHandle,
                  GLenum                     BindTarget,
                  bool                       bIsDeviceInternal);

    /// Initializes a dummy texture (dummy textures are used by the swap chain to
    /// proxy default framebuffer).
    TextureBaseGL(IReferenceCounters*        pRefCounters,
                  FixedBlockMemoryAllocator& TexViewObjAllocator,
                  RenderDeviceGLImpl*        pDeviceGL,
                  const TextureDesc&         TexDesc,
                  bool                       bIsDeviceInternal);

    ~TextureBaseGL();

    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override;

    const GLObjectWrappers::GLTextureObj& GetGLHandle() const { return m_GlTexture; }

    /// Implementation of ITextureGL::GetBindTarget().
    virtual GLenum DILIGENT_CALL_TYPE GetBindTarget() const override final { return m_BindTarget; }

    GLenum GetGLTexFormat() const { return m_GLTexFormat; }

    void TextureMemoryBarrier(Uint32 RequiredBarriers, class GLContextState& GLContextState);

    virtual void AttachToFramebuffer(const struct TextureViewDesc& ViewDesc, GLenum AttachmentPoint) = 0;

    void CopyData(DeviceContextGLImpl* pDeviceCtxGL,
                  TextureBaseGL*       pSrcTextureGL,
                  Uint32               SrcMipLevel,
                  Uint32               SrcSlice,
                  const Box*           pSrcBox,
                  Uint32               DstMipLevel,
                  Uint32               DstSlice,
                  Uint32               DstX,
                  Uint32               DstY,
                  Uint32               DstZ);

    /// Implementation of ITextureGL::GetGLTextureHandle().
    virtual GLuint DILIGENT_CALL_TYPE GetGLTextureHandle() override final { return GetGLHandle(); }

    /// Implementation of ITexture::GetNativeHandle() in OpenGL backend.
    virtual void* DILIGENT_CALL_TYPE GetNativeHandle() override final
    {
        return reinterpret_cast<void*>(static_cast<size_t>(GetGLTextureHandle()));
    }

    virtual void UpdateData(class GLContextState&    CtxState,
                            Uint32                   MipLevel,
                            Uint32                   Slice,
                            const Box&               DstBox,
                            const TextureSubResData& SubresData) = 0;

    static constexpr Uint32 PBOOffsetAlignment = 4;

    IBuffer* GetPBO()
    {
        return m_pPBO;
    }

protected:
    virtual void CreateViewInternal(const struct TextureViewDesc& ViewDesc,
                                    ITextureView**                ppView,
                                    bool                          bIsDefaultView) override;

    void SetDefaultGLParameters();

    GLObjectWrappers::GLTextureObj m_GlTexture;
    RefCntAutoPtr<IBuffer>         m_pPBO; // For staging textures
    const GLenum                   m_BindTarget;
    const GLenum                   m_GLTexFormat;
    //Uint32 m_uiMapTarget;
};

} // namespace Diligent
