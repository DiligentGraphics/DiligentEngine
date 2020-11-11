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

#include "TextureBaseGL.hpp"
#include "RenderDeviceGLImpl.hpp"
#include "GLTypeConversions.hpp"
#include "TextureViewGLImpl.hpp"
#include "GLContextState.hpp"
#include "DeviceContextGLImpl.hpp"
#include "EngineMemory.h"
#include "GraphicsAccessories.hpp"
#include "Align.hpp"

namespace Diligent
{

TextureBaseGL::TextureBaseGL(IReferenceCounters*        pRefCounters,
                             FixedBlockMemoryAllocator& TexViewObjAllocator,
                             RenderDeviceGLImpl*        pDeviceGL,
                             const TextureDesc&         TexDesc,
                             GLenum                     BindTarget,
                             const TextureData*         pInitData /*= nullptr*/,
                             bool                       bIsDeviceInternal /*= false*/) :
    // clang-format off
    TTextureBase
    {
        pRefCounters,
        TexViewObjAllocator,
        pDeviceGL,
        TexDesc,
        bIsDeviceInternal
    },
    m_GlTexture     {TexDesc.Usage != USAGE_STAGING},
    m_BindTarget    {BindTarget },
    m_GLTexFormat   {TexFormatToGLInternalTexFormat(m_Desc.Format, m_Desc.BindFlags)}
    //m_uiMapTarget(0)
// clang-format on
{
    VERIFY(m_GLTexFormat != 0, "Unsupported texture format");
    if (TexDesc.Usage == USAGE_IMMUTABLE && pInitData == nullptr)
        LOG_ERROR_AND_THROW("Immutable textures must be initialized with data at creation time");

    if (TexDesc.Usage == USAGE_STAGING)
    {
        BufferDesc  StagingBufferDesc;
        std::string StagingBuffName = "Internal staging buffer of texture '";
        StagingBuffName += m_Desc.Name;
        StagingBuffName += '\'';
        StagingBufferDesc.Name = StagingBuffName.c_str();

        StagingBufferDesc.uiSizeInBytes  = GetStagingTextureSubresourceOffset(m_Desc, m_Desc.ArraySize, 0, PBOOffsetAlignment);
        StagingBufferDesc.Usage          = USAGE_STAGING;
        StagingBufferDesc.CPUAccessFlags = TexDesc.CPUAccessFlags;

        pDeviceGL->CreateBuffer(StagingBufferDesc, nullptr, &m_pPBO);
        VERIFY_EXPR(m_pPBO);
    }
}

static GLenum GetTextureInternalFormat(GLContextState& GLState, GLenum BindTarget, const GLObjectWrappers::GLTextureObj& GLTex, TEXTURE_FORMAT TexFmtFromDesc)
{
    GLState.BindTexture(-1, BindTarget, GLTex);

    GLenum QueryBindTarget = BindTarget;
    if (BindTarget == GL_TEXTURE_CUBE_MAP || BindTarget == GL_TEXTURE_CUBE_MAP_ARRAY)
        QueryBindTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X;

    GLint GlFormat = 0;
#if GL_TEXTURE_INTERNAL_FORMAT
    glGetTexLevelParameteriv(QueryBindTarget, 0, GL_TEXTURE_INTERNAL_FORMAT, &GlFormat);
    if (glGetError() == GL_NO_ERROR && GlFormat != 0)
    {
        VERIFY(TexFmtFromDesc == TEX_FORMAT_UNKNOWN || static_cast<GLenum>(GlFormat) == TexFormatToGLInternalTexFormat(TexFmtFromDesc), "Texture format does not match the format specified by the texture description");
    }
    else
    {
        if (TexFmtFromDesc != TEX_FORMAT_UNKNOWN)
        {
            GlFormat = TexFormatToGLInternalTexFormat(TexFmtFromDesc);
        }
        else
        {
            LOG_WARNING_MESSAGE("Unable to query internal texture format while the format specified by texture description is TEX_FORMAT_UNKNOWN.");
        }
    }
#else
    if (TexFmtFromDesc != TEX_FORMAT_UNKNOWN)
    {
        GlFormat = TexFormatToGLInternalTexFormat(TexFmtFromDesc);
    }
    else
    {
        LOG_WARNING_MESSAGE("Texture format query is not supported while the format specified by texture description is TEX_FORMAT_UNKNOWN.");
    }
#endif

    GLState.BindTexture(-1, BindTarget, GLObjectWrappers::GLTextureObj::Null());

    return GlFormat;
}

static TextureDesc GetTextureDescFromGLHandle(GLContextState& GLState, TextureDesc TexDesc, GLuint GLHandle, GLenum BindTarget)
{
    VERIFY(BindTarget != GL_TEXTURE_CUBE_MAP_ARRAY, "Cubemap arrays are not currently supported");

    GLObjectWrappers::GLTextureObj TmpGLTexWrapper(true, GLObjectWrappers::GLTextureCreateReleaseHelper(GLHandle));
    GLState.BindTexture(-1, BindTarget, TmpGLTexWrapper);

    GLenum QueryBindTarget = BindTarget;
    if (BindTarget == GL_TEXTURE_CUBE_MAP)
        QueryBindTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X;


#if GL_TEXTURE_WIDTH
    GLint TexWidth = 0;
    glGetTexLevelParameteriv(QueryBindTarget, 0, GL_TEXTURE_WIDTH, &TexWidth);
    if (glGetError() == GL_NO_ERROR && TexWidth > 0)
    {
        if (TexDesc.Width != 0 && TexDesc.Width != static_cast<Uint32>(TexWidth))
        {
            LOG_WARNING_MESSAGE("The width (", TexDesc.Width, ") of texture '", TexDesc.Name,
                                "' specified by TextureDesc struct does not match the actual width (", TexWidth, ")");
        }
        TexDesc.Width = static_cast<Uint32>(TexWidth);
    }
    else
    {
        if (TexDesc.Width == 0)
        {
            LOG_WARNING_MESSAGE("Unable to query the width of texture '", TexDesc.Name,
                                "' while the Width member of TextureDesc struct is 0.");
        }
    }
#else
    if (TexDesc.Width == 0)
    {
        LOG_WARNING_MESSAGE("Texture width query is not supported while the Width member of TextureDesc struct of texture '",
                            TexDesc.Name, "' is 0.");
    }
#endif

    if (TexDesc.Type >= RESOURCE_DIM_TEX_2D)
    {
#if GL_TEXTURE_HEIGHT
        GLint TexHeight = 0;
        glGetTexLevelParameteriv(QueryBindTarget, 0, GL_TEXTURE_HEIGHT, &TexHeight);
        if (glGetError() == GL_NO_ERROR && TexHeight > 0)
        {
            if (TexDesc.Height != 0 && TexDesc.Height != static_cast<Uint32>(TexHeight))
            {
                LOG_WARNING_MESSAGE("The height (", TexDesc.Height, ") of texture '", TexDesc.Name,
                                    "' specified by TextureDesc struct does not match the actual height (", TexHeight, ")");
            }
            TexDesc.Height = static_cast<Uint32>(TexHeight);
        }
        else
        {
            if (TexDesc.Height == 0)
            {
                LOG_WARNING_MESSAGE("Unable to query the height of texture '", TexDesc.Name,
                                    "' while the Height member of TextureDesc struct is 0.");
            }
        }
#else
        if (TexDesc.Height == 0)
        {
            LOG_WARNING_MESSAGE("Texture height query is not supported while the Height member of TextureDesc struct of texture '",
                                TexDesc.Name, "' is 0.");
        }
#endif
    }
    else
    {
        TexDesc.Height = 1;
    }

    if (TexDesc.Type == RESOURCE_DIM_TEX_3D)
    {
#if GL_TEXTURE_DEPTH
        GLint TexDepth = 0;
        glGetTexLevelParameteriv(QueryBindTarget, 0, GL_TEXTURE_DEPTH, &TexDepth);
        if (glGetError() == GL_NO_ERROR && TexDepth > 0)
        {
            if (TexDesc.Depth != 0 && TexDesc.Depth != static_cast<Uint32>(TexDepth))
            {
                LOG_WARNING_MESSAGE("The depth (", TexDesc.Depth, ") of texture '", TexDesc.Name,
                                    "' specified by TextureDesc struct does not match the actual depth (", TexDepth, ")");
            }
            TexDesc.Depth = static_cast<Uint32>(TexDepth);
        }
        else
        {
            if (TexDesc.Depth == 0)
            {
                LOG_WARNING_MESSAGE("Unable to query the depth of texture '", TexDesc.Name,
                                    "' while the Depth member of TextureDesc struct is 0.");
            }
        }
#else
        if (TexDesc.Depth == 0)
        {
            LOG_WARNING_MESSAGE("Texture depth query is not supported while the Depth member of TextureDesc struct of texture '",
                                TexDesc.Name, "' is 0.");
        }
#endif
    }

    if (TexDesc.Type == RESOURCE_DIM_TEX_1D || TexDesc.Type == RESOURCE_DIM_TEX_2D)
        TexDesc.ArraySize = 1; // TexDesc.Depth also

#if GL_TEXTURE_INTERNAL_FORMAT
    GLint GlFormat = 0;
    glGetTexLevelParameteriv(QueryBindTarget, 0, GL_TEXTURE_INTERNAL_FORMAT, &GlFormat);
    if (glGetError() == GL_NO_ERROR && GlFormat != 0)
    {
        if (TexDesc.Format != TEX_FORMAT_UNKNOWN && static_cast<GLenum>(GlFormat) != TexFormatToGLInternalTexFormat(TexDesc.Format))
        {
            LOG_WARNING_MESSAGE("The format (", GetTextureFormatAttribs(TexDesc.Format).Name, ") of texture '", TexDesc.Name,
                                "' specified by TextureDesc struct does not match GL texture internal format (", GlFormat, ")");
        }

        TexDesc.Format = GLInternalTexFormatToTexFormat(GlFormat);
    }
    else
    {
        if (TexDesc.Format == TEX_FORMAT_UNKNOWN)
        {
            LOG_WARNING_MESSAGE("Unable to query the format of texture '", TexDesc.Name,
                                "' while the Format member of TextureDesc struct is TEX_FORMAT_UNKNOWN.");
        }
    }
#else
    if (TexDesc.Format == TEX_FORMAT_UNKNOWN)
    {
        LOG_WARNING_MESSAGE("Texture format query is not supported while the Format member of TextureDesc struct of texture '",
                            TexDesc.Name, "' is TEX_FORMAT_UNKNOWN.");
    }
#endif

    // GL_TEXTURE_IMMUTABLE_LEVELS is only supported in GL4.3+ and GLES3.1+
    GLint MipLevels = 0;
    glGetTexParameteriv(BindTarget, GL_TEXTURE_IMMUTABLE_LEVELS, &MipLevels);
    if (glGetError() == GL_NO_ERROR && MipLevels > 0)
    {
        if (TexDesc.MipLevels != 0 && TexDesc.MipLevels != static_cast<Uint32>(MipLevels))
        {
            LOG_WARNING_MESSAGE("The number of mip levels (", TexDesc.MipLevels, ") of texture '", TexDesc.Name,
                                "' specified by TextureDesc struct does not match the actual number of mip levels (", MipLevels, ")");
        }

        TexDesc.MipLevels = static_cast<Uint32>(MipLevels);
    }
    else
    {
        if (TexDesc.MipLevels == 0)
        {
            LOG_WARNING_MESSAGE("Unable to query the mip level count of texture '", TexDesc.Name,
                                "' while the MipLevels member of TextureDesc struct is 0.");
        }
    }

    GLState.BindTexture(-1, BindTarget, GLObjectWrappers::GLTextureObj::Null());
    return TexDesc;
}

TextureBaseGL::TextureBaseGL(IReferenceCounters*        pRefCounters,
                             FixedBlockMemoryAllocator& TexViewObjAllocator,
                             RenderDeviceGLImpl*        pDeviceGL,
                             GLContextState&            GLState,
                             const TextureDesc&         TexDesc,
                             GLuint                     GLTextureHandle,
                             GLenum                     BindTarget,
                             bool                       bIsDeviceInternal /* = false*/) :
    // clang-format off
    TTextureBase
    {
        pRefCounters,
        TexViewObjAllocator,
        pDeviceGL,
        GetTextureDescFromGLHandle(GLState, TexDesc, GLTextureHandle, BindTarget),
        bIsDeviceInternal
    },
    // Create texture object wrapper, but use external texture handle
    m_GlTexture     {true, GLObjectWrappers::GLTextureCreateReleaseHelper(GLTextureHandle)},
    m_BindTarget    {BindTarget},
    m_GLTexFormat   {GetTextureInternalFormat(GLState, BindTarget, m_GlTexture, TexDesc.Format)}
// clang-format on
{
}

TextureBaseGL::TextureBaseGL(IReferenceCounters*        pRefCounters,
                             FixedBlockMemoryAllocator& TexViewObjAllocator,
                             RenderDeviceGLImpl*        pDeviceGL,
                             const TextureDesc&         TexDesc,
                             bool                       bIsDeviceInternal) :
    // clang-format off
    TTextureBase
    {
        pRefCounters,
        TexViewObjAllocator,
        pDeviceGL,
        TexDesc,
        bIsDeviceInternal
    },
    m_GlTexture  {false},
    m_BindTarget {0    },
    m_GLTexFormat{0    }
// clang-format on
{
}

TextureBaseGL::~TextureBaseGL()
{
    // Release all FBOs that contain current texture
    // NOTE: we cannot check if BIND_RENDER_TARGET
    // flag is set, because CopyData() can bind
    // texture as render target even when no flag
    // is set
    static_cast<RenderDeviceGLImpl*>(GetDevice())->OnReleaseTexture(this);
}

IMPLEMENT_QUERY_INTERFACE(TextureBaseGL, IID_TextureGL, TTextureBase)


void TextureBaseGL::CreateViewInternal(const struct TextureViewDesc& OrigViewDesc, ITextureView** ppView, bool bIsDefaultView)
{
    VERIFY(ppView != nullptr, "Null pointer provided");
    if (!ppView) return;
    VERIFY(*ppView == nullptr, "Overwriting reference to existing object may cause memory leaks");

    *ppView = nullptr;

    try
    {
        auto ViewDesc = OrigViewDesc;
        CorrectTextureViewDesc(ViewDesc);

        auto* pDeviceGLImpl    = GetDevice();
        auto& TexViewAllocator = pDeviceGLImpl->GetTexViewObjAllocator();
        VERIFY(&TexViewAllocator == &m_dbgTexViewObjAllocator, "Texture view allocator does not match allocator provided during texture initialization");

        // http://www.opengl.org/wiki/Texture_Storage#Texture_views

        GLenum GLViewFormat = TexFormatToGLInternalTexFormat(ViewDesc.Format, m_Desc.BindFlags);
        VERIFY(GLViewFormat != 0, "Unsupported texture format");

        TextureViewGLImpl* pViewOGL = nullptr;
        if (ViewDesc.ViewType == TEXTURE_VIEW_SHADER_RESOURCE)
        {
            // clang-format off
            bool bIsFullTextureView =
                ViewDesc.TextureDim      == m_Desc.Type &&
                ViewDesc.Format          == GetDefaultTextureViewFormat(m_Desc.Format, ViewDesc.ViewType, m_Desc.BindFlags) &&
                ViewDesc.MostDetailedMip == 0 &&
                ViewDesc.NumMipLevels    == m_Desc.MipLevels &&
                ViewDesc.FirstArraySlice == 0 &&
                ViewDesc.NumArraySlices  == m_Desc.ArraySize;
            // clang-format on

            pViewOGL = NEW_RC_OBJ(TexViewAllocator, "TextureViewGLImpl instance", TextureViewGLImpl, bIsDefaultView ? this : nullptr)(
                pDeviceGLImpl, ViewDesc, this,
                !bIsFullTextureView, // Create OpenGL texture view object if view
                                     // does not address the whole texture
                bIsDefaultView);
            if (!bIsFullTextureView)
            {
                GLenum GLViewTarget = 0;
                GLuint NumLayers    = ViewDesc.NumArraySlices;
                switch (ViewDesc.TextureDim)
                {
                    case RESOURCE_DIM_TEX_1D:
                        GLViewTarget            = GL_TEXTURE_1D;
                        ViewDesc.NumArraySlices = NumLayers = 1;
                        break;

                    case RESOURCE_DIM_TEX_1D_ARRAY:
                        GLViewTarget = GL_TEXTURE_1D_ARRAY;
                        break;

                    case RESOURCE_DIM_TEX_2D:
                        GLViewTarget            = m_Desc.SampleCount > 1 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
                        ViewDesc.NumArraySlices = NumLayers = 1;
                        break;

                    case RESOURCE_DIM_TEX_2D_ARRAY:
                        GLViewTarget = m_Desc.SampleCount > 1 ? GL_TEXTURE_2D_MULTISAMPLE_ARRAY : GL_TEXTURE_2D_ARRAY;
                        break;

                    case RESOURCE_DIM_TEX_3D:
                    {
                        GLViewTarget = GL_TEXTURE_3D;
                        // If target is GL_TEXTURE_3D, NumLayers must equal 1.
                        Uint32 MipDepth = std::max(m_Desc.Depth >> ViewDesc.MostDetailedMip, 1U);
                        if (ViewDesc.FirstDepthSlice != 0 || ViewDesc.NumDepthSlices != MipDepth)
                        {
                            LOG_ERROR("3D texture view '", (ViewDesc.Name ? ViewDesc.Name : ""), "' (most detailed mip: ", ViewDesc.MostDetailedMip,
                                      "; mip levels: ", ViewDesc.NumMipLevels, "; first slice: ", ViewDesc.FirstDepthSlice,
                                      "; num depth slices: ", ViewDesc.NumDepthSlices, ") of texture '", m_Desc.Name, "' does not references"
                                                                                                                      " all depth slices. 3D texture views in OpenGL must address all depth slices.");
                            ViewDesc.NumDepthSlices  = MipDepth;
                            ViewDesc.FirstDepthSlice = 0;
                        }
                        NumLayers = 1;
                        break;
                    }

                    case RESOURCE_DIM_TEX_CUBE:
                        GLViewTarget = GL_TEXTURE_CUBE_MAP;
                        break;

                    case RESOURCE_DIM_TEX_CUBE_ARRAY:
                        GLViewTarget = GL_TEXTURE_CUBE_MAP_ARRAY;
                        break;

                    default: UNEXPECTED("Unsupported texture view type");
                }

                glTextureView(pViewOGL->GetHandle(), GLViewTarget, m_GlTexture, GLViewFormat, ViewDesc.MostDetailedMip, ViewDesc.NumMipLevels, ViewDesc.FirstArraySlice, NumLayers);
                CHECK_GL_ERROR_AND_THROW("Failed to create texture view");
                pViewOGL->SetBindTarget(GLViewTarget);
            }
        }
        else if (ViewDesc.ViewType == TEXTURE_VIEW_UNORDERED_ACCESS)
        {
            // clang-format off
            VERIFY(ViewDesc.NumArraySlices == 1 ||
                   m_Desc.Type == RESOURCE_DIM_TEX_3D && ViewDesc.NumDepthSlices == std::max(m_Desc.Depth >> ViewDesc.MostDetailedMip, 1U) ||
                   ViewDesc.NumArraySlices == m_Desc.ArraySize,
                   "Only single array/depth slice or the whole texture can be bound as UAV in OpenGL.");
            // clang-format on
            VERIFY(ViewDesc.AccessFlags != 0, "At least one access flag must be specified");
            pViewOGL = NEW_RC_OBJ(TexViewAllocator, "TextureViewGLImpl instance", TextureViewGLImpl, bIsDefaultView ? this : nullptr)(
                pDeviceGLImpl, ViewDesc, this,
                false, // Do NOT create texture view OpenGL object
                bIsDefaultView);
        }
        else if (ViewDesc.ViewType == TEXTURE_VIEW_RENDER_TARGET)
        {
            VERIFY(ViewDesc.NumMipLevels == 1, "Only single mip level can be bound as RTV");
            pViewOGL = NEW_RC_OBJ(TexViewAllocator, "TextureViewGLImpl instance", TextureViewGLImpl, bIsDefaultView ? this : nullptr)(
                pDeviceGLImpl, ViewDesc, this,
                false, // Do NOT create texture view OpenGL object
                bIsDefaultView);
        }
        else if (ViewDesc.ViewType == TEXTURE_VIEW_DEPTH_STENCIL)
        {
            VERIFY(ViewDesc.NumMipLevels == 1, "Only single mip level can be bound as DSV");
            pViewOGL = NEW_RC_OBJ(TexViewAllocator, "TextureViewGLImpl instance", TextureViewGLImpl, bIsDefaultView ? this : nullptr)(
                pDeviceGLImpl, ViewDesc, this,
                false, // Do NOT create texture view OpenGL object
                bIsDefaultView);
        }

        if (bIsDefaultView)
            *ppView = pViewOGL;
        else
        {
            if (pViewOGL)
            {
                pViewOGL->QueryInterface(IID_TextureView, reinterpret_cast<IObject**>(ppView));
            }
        }
    }
    catch (const std::runtime_error&)
    {
        const auto* ViewTypeName = GetTexViewTypeLiteralName(OrigViewDesc.ViewType);
        LOG_ERROR("Failed to create view '", (OrigViewDesc.Name ? OrigViewDesc.Name : ""), "' (", ViewTypeName, ") for texture '", (m_Desc.Name ? m_Desc.Name : ""), "'");
    }
}


void TextureBaseGL::UpdateData(GLContextState& CtxState, Uint32 MipLevel, Uint32 Slice, const Box& DstBox, const TextureSubResData& SubresData)
{
    // GL_TEXTURE_UPDATE_BARRIER_BIT:
    //      Writes to a texture via glTex( Sub )Image*, glCopyTex( Sub )Image*, glClearTex*Image,
    //      glCompressedTex( Sub )Image*, and reads via glTexImage() after the barrier will reflect
    //      data written by shaders prior to the barrier. Additionally, texture writes from these
    //      commands issued after the barrier will not execute until all shader writes initiated prior
    //      to the barrier complete
    TextureMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT, CtxState);
}

//void TextureBaseGL::UpdateData(Uint32 Offset, Uint32 Size, const void* pData)
//{
//    CTexture::UpdateData(Offset, Size, pData);
//
//    glBindTexture(GL_ARRAY_Texture, m_GlTexture);
//    glTextureSubData(GL_ARRAY_Texture, Offset, Size, pData);
//    glBindTexture(GL_ARRAY_Texture, 0);
//}
//

void TextureBaseGL::CopyData(DeviceContextGLImpl* pDeviceCtxGL,
                             TextureBaseGL*       pSrcTextureGL,
                             Uint32               SrcMipLevel,
                             Uint32               SrcSlice,
                             const Box*           pSrcBox,
                             Uint32               DstMipLevel,
                             Uint32               DstSlice,
                             Uint32               DstX,
                             Uint32               DstY,
                             Uint32               DstZ)
{
    const auto& SrcTexDesc = pSrcTextureGL->GetDesc();

    Box SrcBox;
    if (pSrcBox == nullptr)
    {
        SrcBox.MaxX = std::max(SrcTexDesc.Width >> SrcMipLevel, 1u);
        if (SrcTexDesc.Type == RESOURCE_DIM_TEX_1D ||
            SrcTexDesc.Type == RESOURCE_DIM_TEX_1D_ARRAY)
            SrcBox.MaxY = 1;
        else
            SrcBox.MaxY = std::max(SrcTexDesc.Height >> SrcMipLevel, 1u);

        if (SrcTexDesc.Type == RESOURCE_DIM_TEX_3D)
            SrcBox.MaxZ = std::max(SrcTexDesc.Depth >> SrcMipLevel, 1u);
        else
            SrcBox.MaxZ = 1;
        pSrcBox = &SrcBox;
    }

#if GL_ARB_copy_image
    const bool IsDefaultBackBuffer = GetGLHandle() == 0;
    // We can't use glCopyImageSubData with the proxy texture of a default framebuffer
    // because we don't have the texture handle. Resort to quad rendering in this case.
    if (glCopyImageSubData && !IsDefaultBackBuffer)
    {
        GLint SrcSliceY = (SrcTexDesc.Type == RESOURCE_DIM_TEX_1D_ARRAY) ? SrcSlice : 0;
        GLint SrcSliceZ = (SrcTexDesc.Type == RESOURCE_DIM_TEX_2D_ARRAY) ? SrcSlice : 0;
        GLint DstSliceY = (m_Desc.Type == RESOURCE_DIM_TEX_1D_ARRAY) ? DstSlice : 0;
        GLint DstSliceZ = (m_Desc.Type == RESOURCE_DIM_TEX_2D_ARRAY) ? DstSlice : 0;
        glCopyImageSubData(
            pSrcTextureGL->GetGLHandle(),
            pSrcTextureGL->GetBindTarget(),
            SrcMipLevel,
            pSrcBox->MinX,
            pSrcBox->MinY + SrcSliceY,
            pSrcBox->MinZ + SrcSliceZ, // Slice must be zero for 3D texture
            GetGLHandle(),
            GetBindTarget(),
            DstMipLevel,
            DstX,
            DstY + DstSliceY,
            DstZ + DstSliceZ, // Slice must be zero for 3D texture
            pSrcBox->MaxX - pSrcBox->MinX,
            pSrcBox->MaxY - pSrcBox->MinY,
            pSrcBox->MaxZ - pSrcBox->MinZ);
        CHECK_GL_ERROR("glCopyImageSubData() failed");
    }
    else
#endif
    {
        const auto& FmtAttribs = GetDevice()->GetTextureFormatInfoExt(m_Desc.Format);
        if ((FmtAttribs.BindFlags & BIND_RENDER_TARGET) == 0)
        {
            LOG_ERROR_MESSAGE("Unable to perform copy operation because ", FmtAttribs.Name, " is not a color renderable format");
            return;
        }

        auto* pRenderDeviceGL = ValidatedCast<RenderDeviceGLImpl>(GetDevice());
#ifdef DILIGENT_DEBUG
        {
            auto& TexViewObjAllocator = pRenderDeviceGL->GetTexViewObjAllocator();
            VERIFY(&TexViewObjAllocator == &m_dbgTexViewObjAllocator, "Texture view allocator does not match allocator provided during texture initialization");
        }
#endif
        auto& TexRegionRender = *pRenderDeviceGL->m_pTexRegionRender;
        TexRegionRender.SetStates(pDeviceCtxGL);

        // Create temporary SRV for the entire source texture
        TextureViewDesc SRVDesc;
        SRVDesc.TextureDim = SrcTexDesc.Type;
        SRVDesc.ViewType   = TEXTURE_VIEW_SHADER_RESOURCE;
        CorrectTextureViewDesc(SRVDesc);
        // Note: texture view allocates memory for the copy of the name
        // If the name is empty, memory should not be allocated
        // We have to provide allocator even though it will never be used
        TextureViewGLImpl SRV(GetReferenceCounters(), GetDevice(), SRVDesc, pSrcTextureGL,
                              false, // Do NOT create texture view OpenGL object
                              true   // The view, like default view, should not
                                     // keep strong reference to the texture
        );

        for (Uint32 DepthSlice = 0; DepthSlice < pSrcBox->MaxZ - pSrcBox->MinZ; ++DepthSlice)
        {
            // Create temporary RTV for the target subresource
            TextureViewDesc RTVDesc;
            RTVDesc.TextureDim      = m_Desc.Type;
            RTVDesc.ViewType        = TEXTURE_VIEW_RENDER_TARGET;
            RTVDesc.FirstArraySlice = DepthSlice + DstSlice;
            RTVDesc.MostDetailedMip = DstMipLevel;
            RTVDesc.NumArraySlices  = 1;
            CorrectTextureViewDesc(RTVDesc);
            // Note: texture view allocates memory for the copy of the name
            // If the name is empty, memory should not be allocated
            // We have to provide allocator even though it will never be used
            TextureViewGLImpl RTV(GetReferenceCounters(), GetDevice(), RTVDesc, this,
                                  false, // Do NOT create texture view OpenGL object
                                  true   // The view, like default view, should not
                                         // keep strong reference to the texture
            );

            ITextureView* pRTVs[] = {&RTV};
            pDeviceCtxGL->SetRenderTargets(_countof(pRTVs), pRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

            // No need to set up the viewport as SetRenderTargets() does that

            TexRegionRender.Render(pDeviceCtxGL,
                                   &SRV,
                                   SrcTexDesc.Type,
                                   SrcTexDesc.Format,
                                   static_cast<Int32>(pSrcBox->MinX) - static_cast<Int32>(DstX),
                                   static_cast<Int32>(pSrcBox->MinY) - static_cast<Int32>(DstY),
                                   SrcSlice + pSrcBox->MinZ + DepthSlice,
                                   SrcMipLevel);
        }

        TexRegionRender.RestoreStates(pDeviceCtxGL);
    }
}


void TextureBaseGL::TextureMemoryBarrier(Uint32 RequiredBarriers, GLContextState& GLContextState)
{
#if GL_ARB_shader_image_load_store
#    ifdef DILIGENT_DEBUG
    {
        // clang-format off
        constexpr Uint32 TextureBarriers =
            GL_TEXTURE_FETCH_BARRIER_BIT       |
            GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
            GL_PIXEL_BUFFER_BARRIER_BIT        |
            GL_TEXTURE_UPDATE_BARRIER_BIT      |
            GL_FRAMEBUFFER_BARRIER_BIT;
        // clang-format on
        VERIFY((RequiredBarriers & TextureBarriers) != 0, "At least one texture memory barrier flag should be set");
        VERIFY((RequiredBarriers & ~TextureBarriers) == 0, "Inappropriate texture memory barrier flag");
    }
#    endif

    GLContextState.EnsureMemoryBarrier(RequiredBarriers, this);
#endif
}

void TextureBaseGL::SetDefaultGLParameters()
{
#ifdef DILIGENT_DEBUG
    {
        GLint BoundTex;
        GLint TextureBinding = 0;
        switch (m_BindTarget)
        {
                // clang-format off
            case GL_TEXTURE_1D:                     TextureBinding = GL_TEXTURE_BINDING_1D;                   break;
            case GL_TEXTURE_1D_ARRAY:               TextureBinding = GL_TEXTURE_BINDING_1D_ARRAY;             break;
            case GL_TEXTURE_2D:                     TextureBinding = GL_TEXTURE_BINDING_2D;                   break;
            case GL_TEXTURE_2D_ARRAY:               TextureBinding = GL_TEXTURE_BINDING_2D_ARRAY;             break;
            case GL_TEXTURE_2D_MULTISAMPLE:         TextureBinding = GL_TEXTURE_BINDING_2D_MULTISAMPLE;       break;
            case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:   TextureBinding = GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY; break;
            case GL_TEXTURE_3D:                     TextureBinding = GL_TEXTURE_BINDING_3D;                   break;
            case GL_TEXTURE_CUBE_MAP:               TextureBinding = GL_TEXTURE_BINDING_CUBE_MAP;             break;
            case GL_TEXTURE_CUBE_MAP_ARRAY:         TextureBinding = GL_TEXTURE_BINDING_CUBE_MAP_ARRAY;       break;
            default: UNEXPECTED("Unknown bind target");
                // clang-format on
        }
        glGetIntegerv(TextureBinding, &BoundTex);
        CHECK_GL_ERROR("Failed to set GL_TEXTURE_MIN_FILTER texture parameter");
        VERIFY(static_cast<GLuint>(BoundTex) == m_GlTexture, "Current texture is not bound to GL context");
    }
#endif

    if (m_BindTarget != GL_TEXTURE_2D_MULTISAMPLE &&
        m_BindTarget != GL_TEXTURE_2D_MULTISAMPLE_ARRAY)
    {
        // Note that texture bound to image unit must be complete.
        // That means that if an integer texture is being bound, its
        // GL_TEXTURE_MIN_FILTER and GL_TEXTURE_MAG_FILTER must be NEAREST,
        // otherwise it will be incomplete

        // The default value of GL_TEXTURE_MIN_FILTER is GL_NEAREST_MIPMAP_LINEAR
        // Reset it to GL_NEAREST to avoid incompletness issues with integer textures
        glTexParameteri(m_BindTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        CHECK_GL_ERROR("Failed to set GL_TEXTURE_MIN_FILTER texture parameter");

        // The default value of GL_TEXTURE_MAG_FILTER is GL_LINEAR
        glTexParameteri(m_BindTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        CHECK_GL_ERROR("Failed to set GL_TEXTURE_MAG_FILTER texture parameter");
    }
}

} // namespace Diligent
