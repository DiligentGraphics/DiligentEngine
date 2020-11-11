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

#include "TextureCube_OGL.hpp"
#include "RenderDeviceGLImpl.hpp"
#include "DeviceContextGLImpl.hpp"
#include "GLTypeConversions.hpp"
#include "GraphicsAccessories.hpp"
#include "BufferGLImpl.hpp"

namespace Diligent
{

TextureCube_OGL::TextureCube_OGL(IReferenceCounters*        pRefCounters,
                                 FixedBlockMemoryAllocator& TexViewObjAllocator,
                                 class RenderDeviceGLImpl*  pDeviceGL,
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
        GL_TEXTURE_CUBE_MAP,
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

    VERIFY(m_Desc.SampleCount == 1, "Multisampled cubemap textures are not supported");

    GLState.BindTexture(-1, m_BindTarget, m_GlTexture);

    VERIFY(m_Desc.ArraySize == 6, "Cubemap texture is expected to have 6 slices");
    //                             levels             format          width         height
    glTexStorage2D(m_BindTarget, m_Desc.MipLevels, m_GLTexFormat, m_Desc.Width, m_Desc.Height);
    CHECK_GL_ERROR_AND_THROW("Failed to allocate storage for the Cubemap texture");
    //When target is GL_TEXTURE_CUBE_MAP, glTexStorage2D is equivalent to:
    //
    //for (i = 0; i < levels; i++) {
    //    for (face in (+X, -X, +Y, -Y, +Z, -Z)) {
    //        glTexImage2D(face, i, internalformat, width, height, 0, format, type, NULL);
    //    }
    //    width = max(1, (width / 2));
    //    height = max(1, (height / 2));
    //}
    SetDefaultGLParameters();

    if (pInitData != nullptr && pInitData->pSubResources != nullptr)
    {
        const auto ExpectedSubresources = m_Desc.MipLevels * 6;
        if (m_Desc.MipLevels * 6 == pInitData->NumSubresources)
        {
            for (Uint32 Face = 0; Face < 6; ++Face)
            {
                for (Uint32 Mip = 0; Mip < m_Desc.MipLevels; ++Mip)
                {
                    Box DstBox{0, std::max(m_Desc.Width >> Mip, 1U),
                               0, std::max(m_Desc.Height >> Mip, 1U)};
                    // UpdateData() is a virtual function. If we try to call it through vtbl from here,
                    // we will get into TextureBaseGL::UpdateData(), because instance of TextureCube_OGL
                    // is not fully constructed yet.
                    // To call the required function, we need to explicitly specify the class:
                    TextureCube_OGL::UpdateData(GLState, Mip, Face, DstBox, pInitData->pSubResources[Face * m_Desc.MipLevels + Mip]);
                }
            }
        }
        else
        {
            UNEXPECTED("Incorrect number of subresources. ", pInitData->NumSubresources, " while ", ExpectedSubresources, " is expected");
            (void)ExpectedSubresources;
        }
    }

    GLState.BindTexture(-1, m_BindTarget, GLObjectWrappers::GLTextureObj::Null());
}

TextureCube_OGL::TextureCube_OGL(IReferenceCounters*        pRefCounters,
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
        static_cast<GLenum>(GLBindTarget != 0 ? GLBindTarget : GL_TEXTURE_CUBE_MAP),
        bIsDeviceInternal
    }
// clang-format on
{
}

TextureCube_OGL::~TextureCube_OGL()
{
}

// Move static member initialization out of function to avoid
// potential issues in multithreaded code
// clang-format off
static constexpr GLenum CubeMapFaces[6] = 
{
    GL_TEXTURE_CUBE_MAP_POSITIVE_X, 
    GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 
    GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 
    GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
};
// clang-format on

void TextureCube_OGL::UpdateData(GLContextState&          ContextState,
                                 Uint32                   MipLevel,
                                 Uint32                   Slice,
                                 const Box&               DstBox,
                                 const TextureSubResData& SubresData)
{
    TextureBaseGL::UpdateData(ContextState, MipLevel, Slice, DstBox, SubresData);

    // Texture must be bound as GL_TEXTURE_CUBE_MAP, but glTexSubImage2D()
    // then takes one of GL_TEXTURE_CUBE_MAP_POSITIVE_X ... GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
    ContextState.BindTexture(-1, m_BindTarget, m_GlTexture);

    auto CubeMapFaceBindTarget = CubeMapFaces[Slice];

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
        // clang-format off
        VERIFY((DstBox.MinX % 4) == 0 && (DstBox.MinY % 4) == 0 &&
               ((DstBox.MaxX % 4) == 0 || DstBox.MaxX == MipWidth) && 
               ((DstBox.MaxY % 4) == 0 || DstBox.MaxY == MipHeight), 
               "Compressed texture update region must be 4 pixel-aligned" );
        // clang-format on
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

        // Texture must be bound as GL_TEXTURE_CUBE_MAP, but glCompressedTexSubImage2D()
        // takes one of GL_TEXTURE_CUBE_MAP_POSITIVE_X ... GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
        auto UpdateRegionWidth  = DstBox.MaxX - DstBox.MinX;
        auto UpdateRegionHeight = DstBox.MaxY - DstBox.MinY;
        UpdateRegionWidth       = std::min(UpdateRegionWidth, MipWidth - DstBox.MinX);
        UpdateRegionHeight      = std::min(UpdateRegionHeight, MipHeight - DstBox.MinY);
        glCompressedTexSubImage2D(CubeMapFaceBindTarget, MipLevel,
                                  DstBox.MinX,
                                  DstBox.MinY,
                                  UpdateRegionWidth,
                                  UpdateRegionHeight,
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
                                  // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glCompressedTexSubImage2D.xhtml
                                  SubresData.pSrcBuffer != nullptr ? reinterpret_cast<void*>(static_cast<size_t>(SubresData.SrcOffset)) : SubresData.pData);
    }
    else
    {
        const auto& TexFmtInfo = GetTextureFormatAttribs(m_Desc.Format);
        const auto  PixelSize  = Uint32{TexFmtInfo.NumComponents} * Uint32{TexFmtInfo.ComponentSize};
        VERIFY((SubresData.Stride % PixelSize) == 0, "Data stride is not multiple of pixel size");
        glPixelStorei(GL_UNPACK_ROW_LENGTH, SubresData.Stride / PixelSize);
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
        glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

        // Texture must be bound as GL_TEXTURE_CUBE_MAP, but glTexSubImage2D()
        // takes one of GL_TEXTURE_CUBE_MAP_POSITIVE_X ... GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
        glTexSubImage2D(CubeMapFaceBindTarget, MipLevel,
                        DstBox.MinX,
                        DstBox.MinY,
                        DstBox.MaxX - DstBox.MinX,
                        DstBox.MaxY - DstBox.MinY,
                        TransferAttribs.PixelFormat, TransferAttribs.DataType,
                        // If a non-zero named buffer object is bound to the GL_PIXEL_UNPACK_BUFFER target, 'data' is treated
                        // as a byte offset into the buffer object's data store.
                        // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexSubImage2D.xhtml
                        SubresData.pSrcBuffer != nullptr ? reinterpret_cast<void*>(static_cast<size_t>(SubresData.SrcOffset)) : SubresData.pData);
    }
    CHECK_GL_ERROR("Failed to update subimage data");

    if (UnpackBuffer != 0)
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    ContextState.BindTexture(-1, m_BindTarget, GLObjectWrappers::GLTextureObj::Null());
}

void TextureCube_OGL::AttachToFramebuffer(const TextureViewDesc& ViewDesc, GLenum AttachmentPoint)
{
    if (ViewDesc.NumArraySlices == m_Desc.ArraySize)
    {
        glFramebufferTexture(GL_DRAW_FRAMEBUFFER, AttachmentPoint, m_GlTexture, ViewDesc.MostDetailedMip);
        CHECK_GL_ERROR("Failed to attach texture cube to draw framebuffer");
        glFramebufferTexture(GL_READ_FRAMEBUFFER, AttachmentPoint, m_GlTexture, ViewDesc.MostDetailedMip);
        CHECK_GL_ERROR("Failed to attach texture cube to read framebuffer");
    }
    else if (ViewDesc.NumArraySlices == 1)
    {
        // Texture name must either be zero or the name of an existing 3D texture, 1D or 2D array texture,
        // cube map array texture, or multisample array texture.

        auto CubeMapFaceBindTarget = CubeMapFaces[ViewDesc.FirstArraySlice];
        // For glFramebufferTexture2D, if texture is not zero, textarget must be one of GL_TEXTURE_2D, GL_TEXTURE_RECTANGLE,
        // GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
        // GL_TEXTURE_CUBE_MAP_NEGATIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
        // or GL_TEXTURE_2D_MULTISAMPLE.
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, AttachmentPoint, CubeMapFaceBindTarget, m_GlTexture, ViewDesc.MostDetailedMip);
        CHECK_GL_ERROR("Failed to attach texture cube face to draw framebuffer");
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, AttachmentPoint, CubeMapFaceBindTarget, m_GlTexture, ViewDesc.MostDetailedMip);
        CHECK_GL_ERROR("Failed to attach texture cube face to read framebuffer");
    }
    else
    {
        UNEXPECTED("Only one slice or the entire cubemap can be attached to a framebuffer");
    }
}

} // namespace Diligent
