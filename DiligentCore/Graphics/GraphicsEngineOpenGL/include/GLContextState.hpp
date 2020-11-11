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

#include "GraphicsTypes.h"
#include "GLObjectWrapper.hpp"
#include "UniqueIdentifier.hpp"
#include "GLContext.hpp"

namespace Diligent
{

class GLContextState
{
public:
    GLContextState(class RenderDeviceGLImpl* pDeviceGL);

    // clang-format off

    void SetProgram        (const GLObjectWrappers::GLProgramObj&     GLProgram);
    void SetPipeline       (const GLObjectWrappers::GLPipelineObj&    GLPipeline);
    void BindVAO           (const GLObjectWrappers::GLVertexArrayObj& VAO);
    void BindFBO           (const GLObjectWrappers::GLFrameBufferObj& FBO);
    void SetActiveTexture  (Int32 Index);
    void BindTexture       (Int32 Index, GLenum BindTarget, const GLObjectWrappers::GLTextureObj& Tex);
    void BindUniformBuffer (Int32 Index,       const GLObjectWrappers::GLBufferObj& Buff);
    void BindBuffer        (GLenum BindTarget, const GLObjectWrappers::GLBufferObj& Buff, bool ResetVAO);
    void BindSampler       (Uint32 Index,      const GLObjectWrappers::GLSamplerObj& GLSampler);
    void BindImage         (Uint32 Index, class TextureViewGLImpl* pTexView, GLint MipLevel, GLboolean IsLayered, GLint Layer, GLenum Access, GLenum Format);
    void BindImage         (Uint32 Index, class BufferViewGLImpl* pBuffView, GLenum Access, GLenum Format);
    void BindStorageBlock  (Int32 Index, const GLObjectWrappers::GLBufferObj& Buff, GLintptr Offset, GLsizeiptr Size);

    void EnsureMemoryBarrier(Uint32 RequiredBarriers, class AsyncWritableResource *pRes = nullptr);
    void SetPendingMemoryBarriers(Uint32 PendingBarriers);
    
    void EnableDepthTest        (bool bEnable);
    void EnableDepthWrites      (bool bEnable);
    void SetDepthFunc           (COMPARISON_FUNCTION CmpFunc);
    void EnableStencilTest      (bool bEnable);
    void SetStencilWriteMask    (Uint8 StencilWriteMask);
    void SetStencilRef          (GLenum Face, Int32 Ref);
    void SetStencilFunc         (GLenum Face, COMPARISON_FUNCTION Func, Int32 Ref, Uint32 Mask);
    void SetStencilOp           (GLenum Face, STENCIL_OP StencilFailOp, STENCIL_OP StencilDepthFailOp, STENCIL_OP StencilPassOp);
    void SetFillMode            (FILL_MODE FillMode);
    void SetCullMode            (CULL_MODE CullMode);
    void SetFrontFace           (bool FrontCounterClockwise);
    void SetDepthBias           (float DepthBias, float fSlopeScaledDepthBias);
    void SetDepthClamp          (bool bEnableDepthClamp);
    void EnableScissorTest      (bool bEnableScissorTest);

    void SetBlendFactors(const float* BlendFactors);
    void SetBlendState(const BlendStateDesc& BSDsc, Uint32 SampleMask);

    Bool GetDepthWritesEnabled(){ return m_DSState.m_DepthWritesEnableState; }
    Bool GetScissorTestEnabled(){ return m_RSState.ScissorTestEnable; }
    void GetColorWriteMask(Uint32 RTIndex, Uint32& WriteMask, Bool& bIsIndependent);
    void SetColorWriteMask(Uint32 RTIndex, Uint32 WriteMask, Bool bIsIndependent);

    void GetBoundImage(Uint32 Index, GLuint& GLHandle, GLint& MipLevel, GLboolean& IsLayered, GLint& Layer, GLenum& Access, GLenum& Format) const;

    // clang-format on

    void SetNumPatchVertices(Int32 NumVertices);
    void Invalidate();

    void InvalidateVAO()
    {
        m_VAOId = -1;
        // Resetting VAO after that with BindVAO(GLVertexArrayObj::Null()) will still have the effect because
        // null VAO's ID is 0, not -1.
    }

    void InvalidateFBO()
    {
        m_FBOId = -1;
    }
    bool IsValidVAOBound() const { return m_VAOId > 0; }

    void SetCurrentGLContext(GLContext::NativeGLContextType Context) { m_CurrentGLContext = Context; }

    GLContext::NativeGLContextType GetCurrentGLContext() const { return m_CurrentGLContext; }

    struct ContextCaps
    {
        bool  bFillModeSelectionSupported = true;
        GLint m_iMaxCombinedTexUnits      = 0;
        GLint m_iMaxDrawBuffers           = 0;
        GLint m_iMaxUniformBufferBindings = 0;
    };
    const ContextCaps& GetContextCaps() { return m_Caps; }

private:
    // It is unsafe to use GL handle to keep track of bound objects
    // When an object is released, GL is free to reuse its handle for
    // the new created objects.
    // Even using pointers is not safe as when an object is created,
    // the system can reuse the same address
    // The safest way is to keep global unique ID for all objects

    UniqueIdentifier              m_GLProgId     = -1;
    UniqueIdentifier              m_GLPipelineId = -1;
    UniqueIdentifier              m_VAOId        = -1;
    UniqueIdentifier              m_FBOId        = -1;
    std::vector<UniqueIdentifier> m_BoundTextures;
    std::vector<UniqueIdentifier> m_BoundSamplers;
    std::vector<UniqueIdentifier> m_BoundUniformBuffers;

    struct BoundImageInfo
    {
        UniqueIdentifier InterfaceID = -1;
        GLuint           GLHandle    = 0;
        GLint            MipLevel    = 0;
        GLboolean        IsLayered   = 0;
        GLint            Layer       = 0;
        GLenum           Access      = 0;
        GLenum           Format      = 0;

        BoundImageInfo(){};

        BoundImageInfo(UniqueIdentifier _UniqueID,
                       GLuint           _GLHandle,
                       GLint            _MipLevel,
                       GLboolean        _IsLayered,
                       GLint            _Layer,
                       GLenum           _Access,
                       GLenum           _Format) :
            // clang-format off
            InterfaceID{_UniqueID},
            GLHandle   {_GLHandle},
            MipLevel   {_MipLevel},
            IsLayered  {_IsLayered},
            Layer      {_Layer},
            Access     {_Access},
            Format     {_Format}
        // clang-format on
        {}

        bool operator==(const BoundImageInfo& rhs) const
        {
            // clang-format off
            return InterfaceID == rhs.InterfaceID &&
                   GLHandle    == rhs.GLHandle &&
                   MipLevel    == rhs.MipLevel &&
                   IsLayered   == rhs.IsLayered &&
                   Layer       == rhs.Layer &&
                   Access      == rhs.Access &&
                   Format      == rhs.Format;
            // clang-format on
        }
    };
    std::vector<BoundImageInfo> m_BoundImages;

    struct BoundSSBOInfo
    {
        BoundSSBOInfo() {}
        BoundSSBOInfo(UniqueIdentifier _BufferID,
                      GLintptr         _Offset,
                      GLsizeiptr       _Size) :
            // clang-format off
            BufferID{_BufferID},
            Offset  {_Offset},
            Size    {_Size}
        // clang-format on
        {}
        UniqueIdentifier BufferID = -1;
        GLintptr         Offset   = 0;
        GLsizeiptr       Size     = 0;

        bool operator==(const BoundSSBOInfo& rhs) const
        {
            // clang-format off
            return BufferID == rhs.BufferID &&
                   Offset   == rhs.Offset &&
                   Size     == rhs.Size;
            // clang-format on
        }
    };
    std::vector<BoundSSBOInfo> m_BoundStorageBlocks;

    Uint32 m_PendingMemoryBarriers = 0;

    class EnableStateHelper
    {
    public:
        enum class ENABLE_STATE : Int32
        {
            UNKNOWN,
            ENABLED,
            DISABLED
        };

        bool operator==(bool bEnabled) const
        {
            return (bEnabled && m_EnableState == ENABLE_STATE::ENABLED) ||
                (!bEnabled && m_EnableState == ENABLE_STATE::DISABLED);
        }
        bool operator!=(bool bEnabled) const
        {
            return !(*this == bEnabled);
        }

        const EnableStateHelper& operator=(bool bEnabled)
        {
            m_EnableState = bEnabled ? ENABLE_STATE::ENABLED : ENABLE_STATE::DISABLED;
            return *this;
        }

        operator bool() const
        {
            return m_EnableState == ENABLE_STATE::ENABLED;
        }

    private:
        ENABLE_STATE m_EnableState = ENABLE_STATE::UNKNOWN;
    };

    struct DepthStencilGLState
    {
        EnableStateHelper   m_DepthEnableState;
        EnableStateHelper   m_DepthWritesEnableState;
        COMPARISON_FUNCTION m_DepthCmpFunc = COMPARISON_FUNC_UNKNOWN;
        EnableStateHelper   m_StencilTestEnableState;
        Uint16              m_StencilReadMask  = 0xFFFF;
        Uint16              m_StencilWriteMask = 0xFFFF;
        struct StencilOpState
        {
            COMPARISON_FUNCTION Func               = COMPARISON_FUNC_UNKNOWN;
            STENCIL_OP          StencilFailOp      = STENCIL_OP_UNDEFINED;
            STENCIL_OP          StencilDepthFailOp = STENCIL_OP_UNDEFINED;
            STENCIL_OP          StencilPassOp      = STENCIL_OP_UNDEFINED;
            Int32               Ref                = std::numeric_limits<Int32>::min();
            Uint32              Mask               = static_cast<Uint32>(-1);
        } m_StencilOpState[2];
    } m_DSState;

    struct RasterizerGLState
    {
        FILL_MODE         FillMode = FILL_MODE_UNDEFINED;
        CULL_MODE         CullMode = CULL_MODE_UNDEFINED;
        EnableStateHelper FrontCounterClockwise;
        float             fDepthBias            = std::numeric_limits<float>::max();
        float             fSlopeScaledDepthBias = std::numeric_limits<float>::max();
        EnableStateHelper DepthClampEnable;
        EnableStateHelper ScissorTestEnable;
    } m_RSState;

    ContextCaps m_Caps;

    Uint32            m_ColorWriteMasks[MAX_RENDER_TARGETS] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    EnableStateHelper m_bIndependentWriteMasks;
    Int32             m_iActiveTexture   = -1;
    Int32             m_NumPatchVertices = -1;

    GLContext::NativeGLContextType m_CurrentGLContext = {};
};

} // namespace Diligent
