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

#include "GLContextState.hpp"
#include "TextureBaseGL.hpp"
#include "SamplerGLImpl.hpp"
#include "AsyncWritableResource.hpp"
#include "GLTypeConversions.hpp"
#include "BufferViewGLImpl.hpp"
#include "RenderDeviceGLImpl.hpp"

using namespace GLObjectWrappers;

namespace Diligent
{

GLContextState::GLContextState(RenderDeviceGLImpl* pDeviceGL)
{
    const DeviceCaps& DeviceCaps       = pDeviceGL->GetDeviceCaps();
    m_Caps.bFillModeSelectionSupported = DeviceCaps.Features.WireframeFill;

    {
        m_Caps.m_iMaxCombinedTexUnits = 0;
        glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &m_Caps.m_iMaxCombinedTexUnits);
        CHECK_GL_ERROR("Failed to get max combined tex image units count");
        VERIFY_EXPR(m_Caps.m_iMaxCombinedTexUnits > 0);

        m_Caps.m_iMaxDrawBuffers = 0;
        glGetIntegerv(GL_MAX_DRAW_BUFFERS, &m_Caps.m_iMaxDrawBuffers);
        CHECK_GL_ERROR("Failed to get max draw buffers count");
        VERIFY_EXPR(m_Caps.m_iMaxDrawBuffers > 0);

        glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &m_Caps.m_iMaxUniformBufferBindings);
        CHECK_GL_ERROR("Failed to get uniform buffers count");
        VERIFY_EXPR(m_Caps.m_iMaxUniformBufferBindings > 0);
    }

    m_BoundTextures.reserve(m_Caps.m_iMaxCombinedTexUnits);
    m_BoundSamplers.reserve(32);
    m_BoundImages.reserve(32);
    m_BoundUniformBuffers.reserve(m_Caps.m_iMaxUniformBufferBindings);
    m_BoundStorageBlocks.reserve(16);

    Invalidate();

    m_CurrentGLContext = pDeviceGL->m_GLContext.GetCurrentNativeGLContext();
}

void GLContextState::Invalidate()
{
#if !PLATFORM_ANDROID
    // On Android this results in OpenGL error, so we will not
    // clear the barriers. All the required barriers will be
    // executed next frame when needed
    if (m_PendingMemoryBarriers != 0)
        EnsureMemoryBarrier(m_PendingMemoryBarriers);
    m_PendingMemoryBarriers = 0;
#endif

    // Unity messes up at least VAO left in the context,
    // so unbid what we bound
    glUseProgram(0);
    glBindProgramPipeline(0);
    glBindVertexArray(0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    DEV_CHECK_GL_ERROR("Failed to reset GL context state");

    m_GLProgId     = -1;
    m_GLPipelineId = -1;
    m_VAOId        = -1;
    m_FBOId        = -1;

    m_BoundTextures.clear();
    m_BoundSamplers.clear();
    m_BoundImages.clear();
    m_BoundUniformBuffers.clear();
    m_BoundStorageBlocks.clear();

    m_DSState = DepthStencilGLState();
    m_RSState = RasterizerGLState();

    for (Uint32 rt = 0; rt < _countof(m_ColorWriteMasks); ++rt)
        m_ColorWriteMasks[rt] = 0xFF;

    m_bIndependentWriteMasks = EnableStateHelper();

    m_iActiveTexture   = -1;
    m_NumPatchVertices = -1;
}

template <typename ObjectType>
bool UpdateBoundObject(UniqueIdentifier& CurrentObjectID, const ObjectType& NewObject, GLuint& NewGLHandle)
{
    NewGLHandle = static_cast<GLuint>(NewObject);
    // Only ask for the ID if the object handle is non-zero
    // to avoid ID generation for null objects
    UniqueIdentifier NewObjectID = (NewGLHandle != 0) ? NewObject.GetUniqueID() : 0;

    // It is unsafe to use GL handle to keep track of bound textures
    // When a texture is released, GL is free to reuse its handle for
    // the new created textures
    if (CurrentObjectID != NewObjectID)
    {
        CurrentObjectID = NewObjectID;
        return true;
    }
    return false;
}

void GLContextState::SetProgram(const GLProgramObj& GLProgram)
{
    GLuint GLProgHandle = 0;
    if (UpdateBoundObject(m_GLProgId, GLProgram, GLProgHandle))
    {
        glUseProgram(GLProgHandle);
        DEV_CHECK_GL_ERROR("Failed to set GL program");
    }
}

void GLContextState::SetPipeline(const GLPipelineObj& GLPipeline)
{
    GLuint GLPipelineHandle = 0;
    if (UpdateBoundObject(m_GLPipelineId, GLPipeline, GLPipelineHandle))
    {
        glBindProgramPipeline(GLPipelineHandle);
        DEV_CHECK_GL_ERROR("Failed to bind program pipeline");
    }
}

void GLContextState::BindVAO(const GLVertexArrayObj& VAO)
{
    GLuint VAOHandle = 0;
    if (UpdateBoundObject(m_VAOId, VAO, VAOHandle))
    {
        glBindVertexArray(VAOHandle);
        DEV_CHECK_GL_ERROR("Failed to set VAO");
    }
}

void GLContextState::BindFBO(const GLFrameBufferObj& FBO)
{
    GLuint FBOHandle = 0;
    if (UpdateBoundObject(m_FBOId, FBO, FBOHandle))
    {
        // Even though the write mask only applies to writes to a framebuffer, the mask state is NOT
        // Framebuffer state. So it is NOT part of a Framebuffer Object or the Default Framebuffer.
        // Binding a new framebuffer will NOT affect the mask.

        // NOTE: if attachment image is a NON-immutable format texture and the selected
        // level is NOT level_base, the texture MUST BE MIPMAP COMPLETE
        // If image is part of a cubemap texture, the texture must also be mipmap cube complete.
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, FBOHandle);
        DEV_CHECK_GL_ERROR("Failed to bind FBO as draw framebuffer");
        glBindFramebuffer(GL_READ_FRAMEBUFFER, FBOHandle);
        DEV_CHECK_GL_ERROR("Failed to bind FBO as read framebuffer");
    }
}

template <class ObjectType>
bool UpdateBoundObjectsArr(std::vector<UniqueIdentifier>& BoundObjectIDs, Uint32 Index, const ObjectType& NewObject, GLuint& NewGLHandle)
{
    if (Index >= BoundObjectIDs.size())
        BoundObjectIDs.resize(Index + 1, -1);

    return UpdateBoundObject(BoundObjectIDs[Index], NewObject, NewGLHandle);
}

void GLContextState::SetActiveTexture(Int32 Index)
{
    if (Index < 0)
    {
        Index += m_Caps.m_iMaxCombinedTexUnits;
    }
    VERIFY(0 <= Index && Index < m_Caps.m_iMaxCombinedTexUnits, "Texture unit is out of range");

    if (m_iActiveTexture != Index)
    {
        glActiveTexture(GL_TEXTURE0 + Index);
        DEV_CHECK_GL_ERROR("Failed to activate texture slot ", Index);
        m_iActiveTexture = Index;
    }
}

void GLContextState::BindTexture(Int32 Index, GLenum BindTarget, const GLObjectWrappers::GLTextureObj& Tex)
{
    if (Index < 0)
    {
        Index += m_Caps.m_iMaxCombinedTexUnits;
    }
    VERIFY(0 <= Index && Index < m_Caps.m_iMaxCombinedTexUnits, "Texture unit is out of range");

    // Always update active texture unit
    SetActiveTexture(Index);

    GLuint GLTexHandle = 0;
    if (UpdateBoundObjectsArr(m_BoundTextures, Index, Tex, GLTexHandle))
    {
        glBindTexture(BindTarget, GLTexHandle);
        DEV_CHECK_GL_ERROR("Failed to bind texture to slot ", Index);
    }
}

void GLContextState::BindSampler(Uint32 Index, const GLObjectWrappers::GLSamplerObj& GLSampler)
{
    GLuint GLSamplerHandle = 0;
    if (UpdateBoundObjectsArr(m_BoundSamplers, Index, GLSampler, GLSamplerHandle))
    {
        glBindSampler(Index, GLSamplerHandle);
        DEV_CHECK_GL_ERROR("Failed to bind sampler to slot ", Index);
    }
}

void GLContextState::BindImage(Uint32             Index,
                               TextureViewGLImpl* pTexView,
                               GLint              MipLevel,
                               GLboolean          IsLayered,
                               GLint              Layer,
                               GLenum             Access,
                               GLenum             Format)
{
#if GL_ARB_shader_image_load_store
    BoundImageInfo NewImageInfo //
        {
            pTexView->GetUniqueID(),
            pTexView->GetHandle(),
            MipLevel,
            IsLayered,
            Layer,
            Access,
            Format //
        };
    if (Index >= m_BoundImages.size())
        m_BoundImages.resize(Index + 1);
    if (!(m_BoundImages[Index] == NewImageInfo))
    {
        m_BoundImages[Index] = NewImageInfo;
        glBindImageTexture(Index, NewImageInfo.GLHandle, MipLevel, IsLayered, Layer, Access, Format);
        DEV_CHECK_GL_ERROR("glBindImageTexture() failed");
    }
#else
    UNSUPPORTED("GL_ARB_shader_image_load_store is not supported");
#endif
}

void GLContextState::BindImage(Uint32 Index, BufferViewGLImpl* pBuffView, GLenum Access, GLenum Format)
{
#if GL_ARB_shader_image_load_store
    BoundImageInfo NewImageInfo //
        {
            pBuffView->GetUniqueID(),
            pBuffView->GetTexBufferHandle(),
            0,
            GL_FALSE,
            0,
            Access,
            Format //
        };
    if (Index >= m_BoundImages.size())
        m_BoundImages.resize(Index + 1);
    if (!(m_BoundImages[Index] == NewImageInfo))
    {
        m_BoundImages[Index] = NewImageInfo;
        glBindImageTexture(Index, NewImageInfo.GLHandle, 0, GL_FALSE, 0, Access, Format);
        DEV_CHECK_GL_ERROR("glBindImageTexture() failed");
    }
#else
    UNSUPPORTED("GL_ARB_shader_image_load_store is not supported");
#endif
}

void GLContextState::GetBoundImage(Uint32     Index,
                                   GLuint&    ImgHandle,
                                   GLint&     MipLevel,
                                   GLboolean& IsLayered,
                                   GLint&     Layer,
                                   GLenum&    Access,
                                   GLenum&    Format) const
{
    if (Index < m_BoundImages.size())
    {
        const auto& BoundImg = m_BoundImages[Index];

        ImgHandle = BoundImg.GLHandle;
        MipLevel  = BoundImg.MipLevel;
        IsLayered = BoundImg.IsLayered;
        Layer     = BoundImg.Layer;
        Access    = BoundImg.Access;
        Format    = BoundImg.Format;
    }
    else
    {
        ImgHandle = 0;
        MipLevel  = 0;
        IsLayered = GL_FALSE;
        Layer     = 0;
        Access    = GL_READ_ONLY;
        Format    = GL_R8;
    }
}

void GLContextState::BindUniformBuffer(Int32 Index, const GLObjectWrappers::GLBufferObj& Buff)
{
    VERIFY(0 <= Index && Index < m_Caps.m_iMaxUniformBufferBindings, "Uniform buffer index is out of range");

    GLuint GLBufferHandle = Buff;
    if (UpdateBoundObjectsArr(m_BoundUniformBuffers, Index, Buff, GLBufferHandle))
    {
        // In addition to binding buffer to the indexed buffer binding target, glBindBufferBase also binds
        // buffer to the generic buffer binding point specified by target.
        glBindBufferBase(GL_UNIFORM_BUFFER, Index, GLBufferHandle);
        DEV_CHECK_GL_ERROR("Failed to bind uniform buffer to slot ", Index);
    }
}

void GLContextState::BindStorageBlock(Int32 Index, const GLObjectWrappers::GLBufferObj& Buff, GLintptr Offset, GLsizeiptr Size)
{
#if GL_ARB_shader_storage_buffer_object
    BoundSSBOInfo NewSSBOInfo{Buff.GetUniqueID(), Offset, Size};
    if (Index >= static_cast<Int32>(m_BoundStorageBlocks.size()))
        m_BoundStorageBlocks.resize(Index + 1);

    if (!(m_BoundStorageBlocks[Index] == NewSSBOInfo))
    {
        m_BoundStorageBlocks[Index] = NewSSBOInfo;
        GLuint GLBufferHandle       = Buff;
        // In addition to binding buffer to the indexed buffer binding target, glBindBufferRange also binds
        // buffer to the generic buffer binding point specified by target.
        glBindBufferRange(GL_SHADER_STORAGE_BUFFER, Index, GLBufferHandle, Offset, Size);
        DEV_CHECK_GL_ERROR("Failed to bind shader storage block to slot ", Index);
    }
#else
    UNSUPPORTED("GL_ARB_shader_image_load_store is not supported");
#endif
}

void GLContextState::BindBuffer(GLenum BindTarget, const GLObjectWrappers::GLBufferObj& Buff, bool ResetVAO)
{
    // Binding ARRAY_BUFFER or ELEMENT_ARRAY_BUFFER affects currently bound VAO
    if (ResetVAO && (BindTarget == GL_ARRAY_BUFFER || BindTarget == GL_ELEMENT_ARRAY_BUFFER))
    {
        BindVAO(GLVertexArrayObj::Null());
    }

    // Note that glBindBufferBase, glBindBufferRange etc. also bind the
    // buffer to the generic buffer binding point specified by target.

    // GL_UNIFORM_BUFFER, GL_ATOMIC_COUNTER_BUFFER and GL_SHADER_STORAGE_BUFFER buffer binding points do not directly affect
    // uniform buffer, atomic counter buffer or shader storage buffer state, respectively. glBindBufferBase or glBindBufferRange
    // must be used to bind a buffer to an indexed uniform buffer, atomic counter buffer or shader storage buffer binding point.
    glBindBuffer(BindTarget, Buff);
    DEV_CHECK_GL_ERROR("Failed to bind buffer ", static_cast<GLint>(Buff), " to target ", BindTarget);
}

void GLContextState::EnsureMemoryBarrier(Uint32 RequiredBarriers, AsyncWritableResource* pRes /* = nullptr */)
{
#if GL_ARB_shader_image_load_store
    // Every resource tracks its own pending memory barriers.
    // Device context also tracks which barriers have not been executed
    // When a resource with pending memory barrier flag is bound to the context,
    // the context checks if the same flag is set in its own pending barriers.
    // Thus a memory barrier is only executed if some resource required that barrier
    // and it has not been executed yet. This is almost optimal strategy, but slightly
    // imperfect as the following scenario shows:

    // Draw 1: Barriers_A |= BARRIER_FLAG, Barrier_Ctx |= BARRIER_FLAG
    // Draw 2: Barriers_B |= BARRIER_FLAG, Barrier_Ctx |= BARRIER_FLAG
    // Draw 3: Bind B, execute BARRIER: Barriers_B = 0, Barrier_Ctx = 0 (Barriers_A == BARRIER_FLAG)
    // Draw 4: Barriers_B |= BARRIER_FLAG, Barrier_Ctx |= BARRIER_FLAG
    // Draw 5: Bind A, execute BARRIER, Barriers_A = 0, Barrier_Ctx = 0 (Barriers_B == BARRIER_FLAG)

    // In the last draw call, barrier for resource A has already been executed when resource B was
    // bound to the pipeline. Since Resource A has not been bound since then, its flag has not been
    // cleared.
    // This situation does not seem to be a problem though since a barier cannot be executed
    // twice in any situation

    Uint32 ResourcePendingBarriers = 0;
    if (pRes)
    {
        // If resource is specified, only set up memory barriers
        // that are required by the resource
        ResourcePendingBarriers = pRes->GetPendingMemortBarriers();
        RequiredBarriers &= ResourcePendingBarriers;
    }

    // Leave only pending barriers
    RequiredBarriers &= m_PendingMemoryBarriers;
    if (RequiredBarriers)
    {
        glMemoryBarrier(RequiredBarriers);
        DEV_CHECK_GL_ERROR("glMemoryBarrier() failed");
        m_PendingMemoryBarriers &= ~RequiredBarriers;
    }

    // Leave only these barriers that are still pending
    if (pRes)
        pRes->ResetPendingMemoryBarriers(m_PendingMemoryBarriers & ResourcePendingBarriers);
#else
    UNSUPPORTED("GL_ARB_shader_image_load_store is not supported");
#endif
}

void GLContextState::SetPendingMemoryBarriers(Uint32 PendingBarriers)
{
    m_PendingMemoryBarriers |= PendingBarriers;
}

void GLContextState::EnableDepthTest(bool bEnable)
{
    if (m_DSState.m_DepthEnableState != bEnable)
    {
        if (bEnable)
        {
            glEnable(GL_DEPTH_TEST);
            DEV_CHECK_GL_ERROR("Failed to enable detph test");
        }
        else
        {
            glDisable(GL_DEPTH_TEST);
            DEV_CHECK_GL_ERROR("Failed to disable detph test");
        }
        m_DSState.m_DepthEnableState = bEnable;
    }
}

void GLContextState::EnableDepthWrites(bool bEnable)
{
    if (m_DSState.m_DepthWritesEnableState != bEnable)
    {
        // If mask is non-zero, the depth buffer is enabled for writing; otherwise, it is disabled.
        glDepthMask(bEnable ? 1 : 0);
        DEV_CHECK_GL_ERROR("Failed to enale/disable depth writes");
        m_DSState.m_DepthWritesEnableState = bEnable;
    }
}

void GLContextState::SetDepthFunc(COMPARISON_FUNCTION CmpFunc)
{
    if (m_DSState.m_DepthCmpFunc != CmpFunc)
    {
        auto GlCmpFunc = CompareFuncToGLCompareFunc(CmpFunc);
        glDepthFunc(GlCmpFunc);
        DEV_CHECK_GL_ERROR("Failed to set GL comparison function");
        m_DSState.m_DepthCmpFunc = CmpFunc;
    }
}

void GLContextState::EnableStencilTest(bool bEnable)
{
    if (m_DSState.m_StencilTestEnableState != bEnable)
    {
        if (bEnable)
        {
            glEnable(GL_STENCIL_TEST);
            DEV_CHECK_GL_ERROR("Failed to enable stencil test");
        }
        else
        {
            glDisable(GL_STENCIL_TEST);
            DEV_CHECK_GL_ERROR("Failed to disable stencil test");
        }
        m_DSState.m_StencilTestEnableState = bEnable;
    }
}

void GLContextState::SetStencilWriteMask(Uint8 StencilWriteMask)
{
    if (m_DSState.m_StencilWriteMask != StencilWriteMask)
    {
        glStencilMask(StencilWriteMask);
        m_DSState.m_StencilWriteMask = StencilWriteMask;
    }
}

void GLContextState::SetStencilRef(GLenum Face, Int32 Ref)
{
    auto& FaceStencilOp = m_DSState.m_StencilOpState[Face == GL_FRONT ? 0 : 1];
    auto  GlStencilFunc = CompareFuncToGLCompareFunc(FaceStencilOp.Func);
    glStencilFuncSeparate(Face, GlStencilFunc, Ref, FaceStencilOp.Mask);
    DEV_CHECK_GL_ERROR("Failed to set stencil function");
}

void GLContextState::SetStencilFunc(GLenum Face, COMPARISON_FUNCTION Func, Int32 Ref, Uint32 Mask)
{
    auto& FaceStencilOp = m_DSState.m_StencilOpState[Face == GL_FRONT ? 0 : 1];
    if (FaceStencilOp.Func != Func ||
        FaceStencilOp.Ref != Ref ||
        FaceStencilOp.Mask != Mask)
    {
        FaceStencilOp.Func = Func;
        FaceStencilOp.Ref  = Ref;
        FaceStencilOp.Mask = Mask;

        SetStencilRef(Face, Ref);
    }
}

void GLContextState::SetStencilOp(GLenum Face, STENCIL_OP StencilFailOp, STENCIL_OP StencilDepthFailOp, STENCIL_OP StencilPassOp)
{
    auto& FaceStencilOp = m_DSState.m_StencilOpState[Face == GL_FRONT ? 0 : 1];
    if (FaceStencilOp.StencilFailOp != StencilFailOp ||
        FaceStencilOp.StencilDepthFailOp != StencilDepthFailOp ||
        FaceStencilOp.StencilPassOp != StencilPassOp)
    {
        auto glsfail = StencilOp2GlStencilOp(StencilFailOp);
        auto dpfail  = StencilOp2GlStencilOp(StencilDepthFailOp);
        auto dppass  = StencilOp2GlStencilOp(StencilPassOp);

        glStencilOpSeparate(Face, glsfail, dpfail, dppass);
        DEV_CHECK_GL_ERROR("Failed to set stencil operation");

        FaceStencilOp.StencilFailOp      = StencilFailOp;
        FaceStencilOp.StencilDepthFailOp = StencilDepthFailOp;
        FaceStencilOp.StencilPassOp      = StencilPassOp;
    }
}

void GLContextState::SetFillMode(FILL_MODE FillMode)
{
    if (m_Caps.bFillModeSelectionSupported)
    {
        if (m_RSState.FillMode != FillMode)
        {
            if (glPolygonMode != nullptr)
            {
                auto PolygonMode = FillMode == FILL_MODE_WIREFRAME ? GL_LINE : GL_FILL;
                glPolygonMode(GL_FRONT_AND_BACK, PolygonMode);
                DEV_CHECK_GL_ERROR("Failed to set polygon mode");
            }
            else
            {
                if (FillMode != FILL_MODE_SOLID)
                    LOG_ERROR("This API/device only supports solid fill mode");
            }

            m_RSState.FillMode = FillMode;
        }
    }
    else
    {
        if (FillMode == FILL_MODE_WIREFRAME)
            LOG_WARNING_MESSAGE("Wireframe fill mode is not supported on this device\n");
    }
}

void GLContextState::SetCullMode(CULL_MODE CullMode)
{
    if (m_RSState.CullMode != CullMode)
    {
        if (CullMode == CULL_MODE_NONE)
        {
            glDisable(GL_CULL_FACE);
            DEV_CHECK_GL_ERROR("Failed to disable face culling");
        }
        else
        {
            VERIFY(CullMode == CULL_MODE_FRONT || CullMode == CULL_MODE_BACK, "Unexpected cull mode");
            glEnable(GL_CULL_FACE);
            DEV_CHECK_GL_ERROR("Failed to enable face culling");
            auto CullFace = CullMode == CULL_MODE_BACK ? GL_BACK : GL_FRONT;
            glCullFace(CullFace);
            DEV_CHECK_GL_ERROR("Failed to set cull face");
        }

        m_RSState.CullMode = CullMode;
    }
}

void GLContextState::SetFrontFace(bool FrontCounterClockwise)
{
    if (m_RSState.FrontCounterClockwise != FrontCounterClockwise)
    {
        auto FrontFace = FrontCounterClockwise ? GL_CCW : GL_CW;
        glFrontFace(FrontFace);
        DEV_CHECK_GL_ERROR("Failed to set front face");
        m_RSState.FrontCounterClockwise = FrontCounterClockwise;
    }
}

void GLContextState::SetDepthBias(float fDepthBias, float fSlopeScaledDepthBias)
{
    if (m_RSState.fDepthBias != fDepthBias ||
        m_RSState.fSlopeScaledDepthBias != fSlopeScaledDepthBias)
    {
        if (fDepthBias != 0 || fSlopeScaledDepthBias != 0)
        {
            glEnable(GL_POLYGON_OFFSET_FILL);
            DEV_CHECK_GL_ERROR("Failed to enable polygon offset fill");
        }
        else
        {
            glDisable(GL_POLYGON_OFFSET_FILL);
            DEV_CHECK_GL_ERROR("Failed to disable polygon offset fill");
        }

        glPolygonOffset(fSlopeScaledDepthBias, fDepthBias);
        DEV_CHECK_GL_ERROR("Failed to set polygon offset");

        m_RSState.fDepthBias            = fDepthBias;
        m_RSState.fSlopeScaledDepthBias = fSlopeScaledDepthBias;
    }
}

void GLContextState::SetDepthClamp(bool bEnableDepthClamp)
{
    if (m_RSState.DepthClampEnable != bEnableDepthClamp)
    {
        if (bEnableDepthClamp)
        {
            // Note that enabling depth clamping in GL is the same as
            // disabling clipping in Direct3D.
            // https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_rasterizer_desc
            // https://www.khronos.org/opengl/wiki/GLAPI/glEnable
#pragma warning(push)
#pragma warning(disable : 4127)
            if (GL_DEPTH_CLAMP)
            {
                glEnable(GL_DEPTH_CLAMP);
                DEV_CHECK_GL_ERROR("Failed to enable depth clamp");
            }
        }
        else
        {
            if (GL_DEPTH_CLAMP)
            {
                glDisable(GL_DEPTH_CLAMP);
                DEV_CHECK_GL_ERROR("Failed to disable depth clamp");
            }
            else
            {
                LOG_WARNING_MESSAGE("Disabling depth clamp is not supported");
            }
#pragma warning(pop)
        }
        m_RSState.DepthClampEnable = bEnableDepthClamp;
    }
}

void GLContextState::EnableScissorTest(bool bEnableScissorTest)
{
    if (m_RSState.ScissorTestEnable != bEnableScissorTest)
    {
        if (bEnableScissorTest)
        {
            glEnable(GL_SCISSOR_TEST);
            DEV_CHECK_GL_ERROR("Failed to enable scissor test");
        }
        else
        {
            glDisable(GL_SCISSOR_TEST);
            DEV_CHECK_GL_ERROR("Failed to disable scissor clamp");
        }

        m_RSState.ScissorTestEnable = bEnableScissorTest;
    }
}

void GLContextState::SetBlendFactors(const float* BlendFactors)
{
    glBlendColor(BlendFactors[0], BlendFactors[1], BlendFactors[2], BlendFactors[3]);
    DEV_CHECK_GL_ERROR("Failed to set blend color");
}

void GLContextState::SetBlendState(const BlendStateDesc& BSDsc, Uint32 SampleMask)
{
    if (SampleMask != 0xFFFFFFFF)
        LOG_ERROR_MESSAGE("Sample mask is not currently implemented in GL backend");

    bool bEnableBlend = false;
    if (BSDsc.IndependentBlendEnable)
    {
        for (int i = 0; i < static_cast<int>(MAX_RENDER_TARGETS); ++i)
        {
            const auto& RT = BSDsc.RenderTargets[i];
            if (RT.BlendEnable)
                bEnableBlend = true;

            if (i < m_Caps.m_iMaxDrawBuffers)
            {
                SetColorWriteMask(i, RT.RenderTargetWriteMask, True);
            }
            else
            {
                VERIFY(RT.RenderTargetWriteMask == RenderTargetBlendDesc().RenderTargetWriteMask, "Render target write mask is specified for buffer ", i, " but this device only supports ", m_Caps.m_iMaxDrawBuffers, " draw buffers");
            }
        }
    }
    else
    {
        const auto& RT0 = BSDsc.RenderTargets[0];
        bEnableBlend    = RT0.BlendEnable;
        SetColorWriteMask(0, RT0.RenderTargetWriteMask, False);
    }

    if (bEnableBlend)
    {
        //  Sets the blend enable flag for ALL color buffers.
        glEnable(GL_BLEND);
        DEV_CHECK_GL_ERROR("Failed to enable alpha blending");

        if (BSDsc.AlphaToCoverageEnable)
        {
            glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
            DEV_CHECK_GL_ERROR("Failed to enable alpha to coverage");
        }
        else
        {
            glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
            DEV_CHECK_GL_ERROR("Failed to disable alpha to coverage");
        }

        if (BSDsc.IndependentBlendEnable)
        {
            for (int i = 0; i < static_cast<int>(MAX_RENDER_TARGETS); ++i)
            {
                const auto& RT = BSDsc.RenderTargets[i];

                if (i >= m_Caps.m_iMaxDrawBuffers)
                {
                    if (RT.BlendEnable)
                        LOG_ERROR_MESSAGE("Blend is enabled for render target ", i, " but this device only supports ", m_Caps.m_iMaxDrawBuffers, " draw buffers");
                    continue;
                }

                if (RT.BlendEnable)
                {
                    glEnablei(GL_BLEND, i);
                    DEV_CHECK_GL_ERROR("Failed to enable alpha blending");

                    auto srcFactorRGB   = BlendFactor2GLBlend(RT.SrcBlend);
                    auto dstFactorRGB   = BlendFactor2GLBlend(RT.DestBlend);
                    auto srcFactorAlpha = BlendFactor2GLBlend(RT.SrcBlendAlpha);
                    auto dstFactorAlpha = BlendFactor2GLBlend(RT.DestBlendAlpha);
                    glBlendFuncSeparatei(i, srcFactorRGB, dstFactorRGB, srcFactorAlpha, dstFactorAlpha);
                    DEV_CHECK_GL_ERROR("Failed to set separate blending factors");
                    auto modeRGB   = BlendOperation2GLBlendOp(RT.BlendOp);
                    auto modeAlpha = BlendOperation2GLBlendOp(RT.BlendOpAlpha);
                    glBlendEquationSeparatei(i, modeRGB, modeAlpha);
                    DEV_CHECK_GL_ERROR("Failed to set separate blending equations");
                }
                else
                {
                    glDisablei(GL_BLEND, i);
                    DEV_CHECK_GL_ERROR("Failed to disable alpha blending");
                }
            }
        }
        else
        {
            const auto& RT0            = BSDsc.RenderTargets[0];
            auto        srcFactorRGB   = BlendFactor2GLBlend(RT0.SrcBlend);
            auto        dstFactorRGB   = BlendFactor2GLBlend(RT0.DestBlend);
            auto        srcFactorAlpha = BlendFactor2GLBlend(RT0.SrcBlendAlpha);
            auto        dstFactorAlpha = BlendFactor2GLBlend(RT0.DestBlendAlpha);
            glBlendFuncSeparate(srcFactorRGB, dstFactorRGB, srcFactorAlpha, dstFactorAlpha);
            DEV_CHECK_GL_ERROR("Failed to set blending factors");

            auto modeRGB   = BlendOperation2GLBlendOp(RT0.BlendOp);
            auto modeAlpha = BlendOperation2GLBlendOp(RT0.BlendOpAlpha);
            glBlendEquationSeparate(modeRGB, modeAlpha);
            DEV_CHECK_GL_ERROR("Failed to set blending equations");
        }
    }
    else
    {
        //  Sets the blend disable flag for ALL color buffers.
        glDisable(GL_BLEND);
        DEV_CHECK_GL_ERROR("Failed to disable alpha blending");
    }
}

void GLContextState::SetColorWriteMask(Uint32 RTIndex, Uint32 WriteMask, Bool bIsIndependent)
{
    // Even though the write mask only applies to writes to a framebuffer, the mask state is NOT
    // Framebuffer state. So it is NOT part of a Framebuffer Object or the Default Framebuffer.
    // Binding a new framebuffer will NOT affect the mask.

    if (!bIsIndependent)
        RTIndex = 0;

    if (m_ColorWriteMasks[RTIndex] != WriteMask ||
        m_bIndependentWriteMasks != bIsIndependent)
    {
        if (bIsIndependent)
        {
            // Note that glColorMaski() does not set color mask for the framebuffer
            // attachment point RTIndex. Rather it sets the mask for what was set
            // by the glDrawBuffers() function for the i-th output
            glColorMaski(RTIndex,
                         (WriteMask & COLOR_MASK_RED) ? GL_TRUE : GL_FALSE,
                         (WriteMask & COLOR_MASK_GREEN) ? GL_TRUE : GL_FALSE,
                         (WriteMask & COLOR_MASK_BLUE) ? GL_TRUE : GL_FALSE,
                         (WriteMask & COLOR_MASK_ALPHA) ? GL_TRUE : GL_FALSE);
            DEV_CHECK_GL_ERROR("Failed to set GL color mask");

            m_ColorWriteMasks[RTIndex] = WriteMask;
        }
        else
        {
            // glColorMask() sets the mask for ALL draw buffers
            glColorMask(
                (WriteMask & COLOR_MASK_RED) ? GL_TRUE : GL_FALSE,
                (WriteMask & COLOR_MASK_GREEN) ? GL_TRUE : GL_FALSE,
                (WriteMask & COLOR_MASK_BLUE) ? GL_TRUE : GL_FALSE,
                (WriteMask & COLOR_MASK_ALPHA) ? GL_TRUE : GL_FALSE);
            DEV_CHECK_GL_ERROR("Failed to set GL color mask");

            for (int rt = 0; rt < _countof(m_ColorWriteMasks); ++rt)
                m_ColorWriteMasks[rt] = WriteMask;
        }
        m_bIndependentWriteMasks = bIsIndependent;
    }
}

void GLContextState::GetColorWriteMask(Uint32 RTIndex, Uint32& WriteMask, Bool& bIsIndependent)
{
    if (!m_bIndependentWriteMasks)
        RTIndex = 0;
    WriteMask      = m_ColorWriteMasks[RTIndex];
    bIsIndependent = m_bIndependentWriteMasks;
}

void GLContextState::SetNumPatchVertices(Int32 NumVertices)
{
#if GL_ARB_tessellation_shader
    if (NumVertices != m_NumPatchVertices)
    {
        m_NumPatchVertices = NumVertices;
        glPatchParameteri(GL_PATCH_VERTICES, static_cast<GLint>(NumVertices));
        DEV_CHECK_GL_ERROR("Failed to set the number of patch vertices");
    }
#endif
}

} // namespace Diligent
