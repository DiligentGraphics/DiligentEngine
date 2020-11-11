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

#include "Texture2DArray_OGL.hpp"
#include "RenderDeviceGLImpl.hpp"
#include "DeviceContextGLImpl.hpp"
#include "GLTypeConversions.hpp"
#include "GraphicsAccessories.hpp"
#include "BufferGLImpl.hpp"

namespace Diligent
{

Texture2DArray_OGL::Texture2DArray_OGL(IReferenceCounters*        pRefCounters,
                                       FixedBlockMemoryAllocator& TexViewObjAllocator,
                                       RenderDeviceGLImpl*        pDeviceGL,
                                       GLContextState&            GLState,
                                       const TextureDesc&         TexDesc,
                                       const TextureData*         pInitData /*= nullptr*/,
                                       bool                       bIsDeviceInternal /*= false*/) :
    // clang-format off
    TextureBaseGL
    {
        pRefCounters,
        TexViewObjAllocator,
        pDeviceGL,
        TexDesc,
        static_cast<GLenum>(TexDesc.SampleCount > 1 ? GL_TEXTURE_2D_MULTISAMPLE_ARRAY : GL_TEXTURE_2D_ARRAY),
        pInitData,
        bIsDeviceInternal
    }
// clang-format on
{
    if (TexDesc.Usage == USAGE_STAGING)
    {
        // We will use PBO initialized by TextureBaseGL
        return;
    }

    GLState.BindTexture(-1, m_BindTarget, m_GlTexture);

    if (m_Desc.SampleCount > 1)
    {
        //                                                              format          width         height          depth
        glTexStorage3DMultisample(m_BindTarget, m_Desc.SampleCount, m_GLTexFormat, m_Desc.Width, m_Desc.Height, m_Desc.ArraySize, GL_TRUE);
        // The last parameter specifies whether the image will use identical sample locations and the same number of
        // samples for all texels in the image, and the sample locations will not depend on the internal format or size
        // of the image.
        CHECK_GL_ERROR_AND_THROW("Failed to allocate storage for the 2D multisample texture array");
        // * An INVALID_ENUM error is generated if sizedinternalformat is not colorrenderable,
        //   depth - renderable, or stencil - renderable
        // * An INVALID_OPERATION error is generated if samples is greater than the maximum number of samples
        //   supported for this target and internalformat. The maximum number of samples supported can be
        //   determined by calling glGetInternalformativ with a pname of GL_SAMPLES

        SetDefaultGLParameters();
    }
    else
    {
        //                             levels             format          width         height          depth
        glTexStorage3D(m_BindTarget, m_Desc.MipLevels, m_GLTexFormat, m_Desc.Width, m_Desc.Height, m_Desc.ArraySize);
        CHECK_GL_ERROR_AND_THROW("Failed to allocate storage for the 2D texture array");
        // When target is GL_TEXTURE_2D_ARRAY, calling glTexStorage3D is equivalent to the following pseudo-code:
        //for (i = 0; i < levels; i++)
        //{
        //    glTexImage3D(target, i, internalformat, width, height, depth, 0, format, type, NULL);
        //    width = max(1, (width / 2));
        //    height = max(1, (height / 2));
        //}

        SetDefaultGLParameters();

        if (pInitData != nullptr && pInitData->pSubResources != nullptr)
        {
            if (m_Desc.MipLevels * m_Desc.ArraySize == pInitData->NumSubresources)
            {
                for (Uint32 Slice = 0; Slice < m_Desc.ArraySize; ++Slice)
                {
                    for (Uint32 Mip = 0; Mip < m_Desc.MipLevels; ++Mip)
                    {
                        Box DstBox{0, std::max(m_Desc.Width >> Mip, 1U),
                                   0, std::max(m_Desc.Height >> Mip, 1U)};
                        // UpdateData() is a virtual function. If we try to call it through vtbl from here,
                        // we will get into TextureBaseGL::UpdateData(), because instance of Texture2DArray_OGL
                        // is not fully constructed yet.
                        // To call the required function, we need to explicitly specify the class:
                        Texture2DArray_OGL::UpdateData(GLState, Mip, Slice, DstBox, pInitData->pSubResources[Slice * m_Desc.MipLevels + Mip]);
                    }
                }
            }
            else
            {
                UNEXPECTED("Incorrect number of subresources");
            }
        }
    }

    GLState.BindTexture(-1, m_BindTarget, GLObjectWrappers::GLTextureObj::Null());
}

Texture2DArray_OGL::Texture2DArray_OGL(IReferenceCounters*        pRefCounters,
                                       FixedBlockMemoryAllocator& TexViewObjAllocator,
                                       RenderDeviceGLImpl*        pDeviceGL,
                                       GLContextState&            GLState,
                                       const TextureDesc&         TexDesc,
                                       GLuint                     GLTextureHandle,
                                       GLuint                     GLBindTarget,
                                       bool                       bIsDeviceInternal) :
    // clang-format off
    TextureBaseGL
    {
        pRefCounters,
        TexViewObjAllocator,
        pDeviceGL,
        GLState,
        TexDesc,
        GLTextureHandle,
        GLBindTarget != 0 ?
            static_cast<GLenum>(GLBindTarget) :
            static_cast<GLenum>(TexDesc.SampleCount > 1 ? GL_TEXTURE_2D_MULTISAMPLE_ARRAY : GL_TEXTURE_2D_ARRAY),
        bIsDeviceInternal
    }
// clang-format on
{
}

Texture2DArray_OGL::~Texture2DArray_OGL()
{
}

void Texture2DArray_OGL::UpdateData(GLContextState&          ContextState,
                                    Uint32                   MipLevel,
                                    Uint32                   Slice,
                                    const Box&               DstBox,
                                    const TextureSubResData& SubresData)
{
    TextureBaseGL::UpdateData(ContextState, MipLevel, Slice, DstBox, SubresData);

    ContextState.BindTexture(-1, m_BindTarget, m_GlTexture);

    // Bind buffer if it is provided; copy from CPU memory otherwise
    GLuint UnpackBuffer = 0;
    if (SubresData.pSrcBuffer != nullptr)
    {
        auto* pBufferGL = ValidatedCast<BufferGLImpl>(SubresData.pSrcBuffer);
        UnpackBuffer    = pBufferGL->GetGLHandle();
    }

    // Transfers to OpenGL memory are called unpack operations
    // If there is a buffer bound to GL_PIXEL_UNPACK_BUFFER target, then all the pixel transfer
    // operations will be performed from this buffer.
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, UnpackBuffer);

    const auto& TransferAttribs = GetNativePixelTransferAttribs(m_Desc.Format);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    if (TransferAttribs.IsCompressed)
    {
        auto MipWidth  = std::max(m_Desc.Width >> MipLevel, 1U);
        auto MipHeight = std::max(m_Desc.Height >> MipLevel, 1U);
        VERIFY((DstBox.MinX % 4) == 0 && (DstBox.MinY % 4) == 0 &&
                   ((DstBox.MaxX % 4) == 0 || DstBox.MaxX == MipWidth) &&
                   ((DstBox.MaxY % 4) == 0 || DstBox.MaxY == MipHeight),
               "Compressed texture update region must be 4 pixel-aligned");
#ifdef DILIGENT_DEBUG
        {
            const auto& FmtAttribs      = GetTextureFormatAttribs(m_Desc.Format);
            auto        BlockBytesInRow = ((DstBox.MaxX - DstBox.MinX + 3) / 4) * Uint32{FmtAttribs.ComponentSize};
            VERIFY(SubresData.Stride == BlockBytesInRow,
                   "Compressed data stride (", SubresData.Stride, " must match the size of a row of compressed blocks (", BlockBytesInRow, ")");
        }
#endif
        //glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        //glPixelStorei(GL_UNPACK_COMPRESSED_BLOCK_WIDTH, 0);
        auto UpdateRegionWidth  = DstBox.MaxX - DstBox.MinX;
        auto UpdateRegionHeight = DstBox.MaxY - DstBox.MinY;
        UpdateRegionWidth       = std::min(UpdateRegionWidth, MipWidth - DstBox.MinX);
        UpdateRegionHeight      = std::min(UpdateRegionHeight, MipHeight - DstBox.MinY);
        glCompressedTexSubImage3D(m_BindTarget, MipLevel,
                                  DstBox.MinX,
                                  DstBox.MinY,
                                  Slice,
                                  UpdateRegionWidth,
                                  UpdateRegionHeight,
                                  1,
                                  // The format must be the same compressed-texture format previously
                                  // specified by glTexStorage2D() (thank you OpenGL for another useless
                                  // parameter that is nothing but the source of confusion), otherwise
                                  // INVALID_OPERATION error is generated.
                                  m_GLTexFormat,
                                  // An INVALID_VALUE error is generated if imageSize is not consistent with
                                  // the format, dimensions, and contents of the compressed image( too little or
                                  // too much data ),
                                  ((DstBox.MaxY - DstBox.MinY + 3) / 4) * SubresData.Stride,
                                  // If a non-zero named buffer object is bound to the GL_PIXEL_UNPACK_BUFFER target, 'data' is treated
                                  // as a byte offset into the buffer object's data store.
                                  // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glCompressedTexSubImage3D.xhtml
                                  SubresData.pSrcBuffer != nullptr ? reinterpret_cast<void*>(static_cast<size_t>(SubresData.SrcOffset)) : SubresData.pData);
    }
    else
    {
        const auto TexFmtInfo = GetTextureFormatAttribs(m_Desc.Format);
        const auto PixelSize  = Uint32{TexFmtInfo.NumComponents} * Uint32{TexFmtInfo.ComponentSize};
        VERIFY((SubresData.Stride % PixelSize) == 0, "Data stride is not multiple of pixel size");
        glPixelStorei(GL_UNPACK_ROW_LENGTH, SubresData.Stride / PixelSize);
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
        glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

        glTexSubImage3D(m_BindTarget, MipLevel,
                        DstBox.MinX,
                        DstBox.MinY,
                        Slice,
                        DstBox.MaxX - DstBox.MinX,
                        DstBox.MaxY - DstBox.MinY,
                        1,
                        TransferAttribs.PixelFormat, TransferAttribs.DataType,
                        // If a non-zero named buffer object is bound to the GL_PIXEL_UNPACK_BUFFER target, 'data' is treated
                        // as a byte offset into the buffer object's data store.
                        // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexSubImage3D.xhtml
                        SubresData.pSrcBuffer != nullptr ? reinterpret_cast<void*>(static_cast<size_t>(SubresData.SrcOffset)) : SubresData.pData);
    }
    CHECK_GL_ERROR("Failed to update subimage data");

    if (UnpackBuffer != 0)
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    ContextState.BindTexture(-1, m_BindTarget, GLObjectWrappers::GLTextureObj::Null());
}

void Texture2DArray_OGL::AttachToFramebuffer(const TextureViewDesc& ViewDesc, GLenum AttachmentPoint)
{
    if (ViewDesc.NumArraySlices == m_Desc.ArraySize)
    {
        glFramebufferTexture(GL_DRAW_FRAMEBUFFER, AttachmentPoint, m_GlTexture, ViewDesc.MostDetailedMip);
        CHECK_GL_ERROR("Failed to attach texture 2D array to draw framebuffer");
        glFramebufferTexture(GL_READ_FRAMEBUFFER, AttachmentPoint, m_GlTexture, ViewDesc.MostDetailedMip);
        CHECK_GL_ERROR("Failed to attach texture 2D array to read framebuffer");
    }
    else if (ViewDesc.NumArraySlices == 1)
    {
        // Texture name must either be zero or the name of an existing 3D texture, 1D or 2D array texture,
        // cube map array texture, or multisample array texture.
        glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, AttachmentPoint, m_GlTexture, ViewDesc.MostDetailedMip, ViewDesc.FirstArraySlice);
        CHECK_GL_ERROR("Failed to attach texture 2D array to draw framebuffer");
        glFramebufferTextureLayer(GL_READ_FRAMEBUFFER, AttachmentPoint, m_GlTexture, ViewDesc.MostDetailedMip, ViewDesc.FirstArraySlice);
        CHECK_GL_ERROR("Failed to attach texture 2D array to read framebuffer");
    }
    else
    {
        UNEXPECTED("Only one slice or the entire texture array can be attached to a framebuffer");
    }
}

} // namespace Diligent
