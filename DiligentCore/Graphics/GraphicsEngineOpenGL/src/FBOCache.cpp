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

#include "FBOCache.hpp"
#include "RenderDeviceGLImpl.hpp"
#include "TextureBaseGL.hpp"
#include "GLContextState.hpp"

namespace Diligent
{

bool FBOCache::FBOCacheKey::operator==(const FBOCacheKey& Key) const
{
    if (Hash != 0 && Key.Hash != 0 && Hash != Key.Hash)
        return false;

    if (NumRenderTargets != Key.NumRenderTargets)
        return false;
    for (Uint32 rt = 0; rt < NumRenderTargets; ++rt)
    {
        if (RTIds[rt] != Key.RTIds[rt])
            return false;
        if (RTIds[rt])
        {
            if (!(RTVDescs[rt] == Key.RTVDescs[rt]))
                return false;
        }
    }
    if (DSId != Key.DSId)
        return false;
    if (DSId)
    {
        if (!(DSVDesc == Key.DSVDesc))
            return false;
    }
    return true;
}

std::size_t FBOCache::FBOCacheKeyHashFunc::operator()(const FBOCacheKey& Key) const
{
    if (Key.Hash == 0)
    {
        std::hash<TextureViewDesc> TexViewDescHasher;
        Key.Hash = 0;
        HashCombine(Key.Hash, Key.NumRenderTargets);
        for (Uint32 rt = 0; rt < Key.NumRenderTargets; ++rt)
        {
            HashCombine(Key.Hash, Key.RTIds[rt]);
            if (Key.RTIds[rt])
                HashCombine(Key.Hash, TexViewDescHasher(Key.RTVDescs[rt]));
        }
        HashCombine(Key.Hash, Key.DSId);
        if (Key.DSId)
            HashCombine(Key.Hash, TexViewDescHasher(Key.DSVDesc));
    }
    return Key.Hash;
}


FBOCache::FBOCache()
{
    m_Cache.max_load_factor(0.5f);
    m_TexIdToKey.max_load_factor(0.5f);
}

FBOCache::~FBOCache()
{
    VERIFY(m_Cache.empty(), "FBO cache is not empty. Are there any unreleased objects?");
    VERIFY(m_TexIdToKey.empty(), "TexIdToKey cache is not empty.");
}

void FBOCache::OnReleaseTexture(ITexture* pTexture)
{
    ThreadingTools::LockHelper CacheLock(m_CacheLockFlag);

    auto* pTexGL = ValidatedCast<TextureBaseGL>(pTexture);
    // Find all FBOs that this texture used in
    auto EqualRange = m_TexIdToKey.equal_range(pTexGL->GetUniqueID());
    for (auto It = EqualRange.first; It != EqualRange.second; ++It)
    {
        m_Cache.erase(It->second);
    }
    m_TexIdToKey.erase(EqualRange.first, EqualRange.second);
}

GLObjectWrappers::GLFrameBufferObj FBOCache::CreateFBO(GLContextState&    ContextState,
                                                       Uint32             NumRenderTargets,
                                                       TextureViewGLImpl* ppRTVs[],
                                                       TextureViewGLImpl* pDSV)
{
    GLObjectWrappers::GLFrameBufferObj FBO{true};

    ContextState.BindFBO(FBO);

    // Initialize the FBO
    for (Uint32 rt = 0; rt < NumRenderTargets; ++rt)
    {
        if (auto* pRTView = ppRTVs[rt])
        {
            const auto& RTVDesc     = pRTView->GetDesc();
            auto*       pColorTexGL = pRTView->GetTexture<TextureBaseGL>();
            pColorTexGL->AttachToFramebuffer(RTVDesc, GL_COLOR_ATTACHMENT0 + rt);
        }
    }

    if (pDSV != nullptr)
    {
        const auto& DSVDesc         = pDSV->GetDesc();
        auto*       pDepthTexGL     = pDSV->GetTexture<TextureBaseGL>();
        GLenum      AttachmentPoint = 0;
        if (DSVDesc.Format == TEX_FORMAT_D32_FLOAT ||
            DSVDesc.Format == TEX_FORMAT_D16_UNORM)
        {
#ifdef DILIGENT_DEBUG
            {
                const auto GLTexFmt = pDepthTexGL->GetGLTexFormat();
                VERIFY(GLTexFmt == GL_DEPTH_COMPONENT32F || GLTexFmt == GL_DEPTH_COMPONENT16,
                       "Inappropriate internal texture format (", GLTexFmt,
                       ") for depth attachment. GL_DEPTH_COMPONENT32F or GL_DEPTH_COMPONENT16 is expected");
            }
#endif
            AttachmentPoint = GL_DEPTH_ATTACHMENT;
        }
        else if (DSVDesc.Format == TEX_FORMAT_D32_FLOAT_S8X24_UINT ||
                 DSVDesc.Format == TEX_FORMAT_D24_UNORM_S8_UINT)
        {
#ifdef DILIGENT_DEBUG
            {
                const auto GLTexFmt = pDepthTexGL->GetGLTexFormat();
                VERIFY(GLTexFmt == GL_DEPTH24_STENCIL8 || GLTexFmt == GL_DEPTH32F_STENCIL8,
                       "Inappropriate internal texture format (", GLTexFmt,
                       ") for depth-stencil attachment. GL_DEPTH24_STENCIL8 or GL_DEPTH32F_STENCIL8 is expected");
            }
#endif
            AttachmentPoint = GL_DEPTH_STENCIL_ATTACHMENT;
        }
        else
        {
            UNEXPECTED(GetTextureFormatAttribs(DSVDesc.Format).Name, " is not valid depth-stencil view format");
        }
        pDepthTexGL->AttachToFramebuffer(DSVDesc, AttachmentPoint);
    }

    // We now need to set mapping between shader outputs and
    // color attachments. This largely redundant step is performed
    // by glDrawBuffers()
    // clang-format off
    static const GLenum DrawBuffers[] = 
    { 
        GL_COLOR_ATTACHMENT0, 
        GL_COLOR_ATTACHMENT1, 
        GL_COLOR_ATTACHMENT2, 
        GL_COLOR_ATTACHMENT3,
        GL_COLOR_ATTACHMENT4,
        GL_COLOR_ATTACHMENT5,
        GL_COLOR_ATTACHMENT6,
        GL_COLOR_ATTACHMENT7,
        GL_COLOR_ATTACHMENT8,
        GL_COLOR_ATTACHMENT9,
        GL_COLOR_ATTACHMENT10,
        GL_COLOR_ATTACHMENT11,
        GL_COLOR_ATTACHMENT12,
        GL_COLOR_ATTACHMENT13,
        GL_COLOR_ATTACHMENT14,
        GL_COLOR_ATTACHMENT15
    };
    // clang-format on

    // The state set by glDrawBuffers() is part of the state of the framebuffer.
    // So it can be set up once and left it set.
    glDrawBuffers(NumRenderTargets, DrawBuffers);
    CHECK_GL_ERROR("Failed to set draw buffers via glDrawBuffers()");

    GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (Status != GL_FRAMEBUFFER_COMPLETE)
    {
        const Char* StatusString = "Unknown";
        switch (Status)
        {
            // clang-format off
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:         StatusString = "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";         break;
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: StatusString = "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"; break;
            case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:        StatusString = "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";        break;
            case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:        StatusString = "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";        break;
            case GL_FRAMEBUFFER_UNSUPPORTED:                   StatusString = "GL_FRAMEBUFFER_UNSUPPORTED";                   break;
            case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:        StatusString = "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";        break;
            case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:      StatusString = "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";      break;
                // clang-format on
        }
        LOG_ERROR("Framebuffer is incomplete. FB status: ", StatusString);
        UNEXPECTED("Framebuffer is incomplete");
    }
    return FBO;
}

const GLObjectWrappers::GLFrameBufferObj& FBOCache::GetFBO(Uint32             NumRenderTargets,
                                                           TextureViewGLImpl* ppRTVs[],
                                                           TextureViewGLImpl* pDSV,
                                                           GLContextState&    ContextState)
{
    // Pop null render targets from the end of the list
    while (NumRenderTargets > 0 && ppRTVs[NumRenderTargets - 1] == nullptr)
        --NumRenderTargets;

    VERIFY(NumRenderTargets != 0 || pDSV != nullptr, "At least one render target or a depth-stencil buffer must be provided");

    // Lock the cache
    ThreadingTools::LockHelper CacheLock(m_CacheLockFlag);

    // Construct the key
    FBOCacheKey Key;
    VERIFY(NumRenderTargets < MAX_RENDER_TARGETS, "Too many render targets are being set");
    NumRenderTargets     = std::min(NumRenderTargets, MAX_RENDER_TARGETS);
    Key.NumRenderTargets = NumRenderTargets;
    for (Uint32 rt = 0; rt < NumRenderTargets; ++rt)
    {
        auto* pRTView = ppRTVs[rt];
        if (pRTView == nullptr)
            continue;

        auto* pColorTexGL = pRTView->GetTexture<TextureBaseGL>();
        pColorTexGL->TextureMemoryBarrier(
            GL_FRAMEBUFFER_BARRIER_BIT, // Reads and writes via framebuffer object attachments after the
                                        // barrier will reflect data written by shaders prior to the barrier.
                                        // Additionally, framebuffer writes issued after the barrier will wait
                                        // on the completion of all shader writes issued prior to the barrier.
            ContextState);

        Key.RTIds[rt]    = pColorTexGL->GetUniqueID();
        Key.RTVDescs[rt] = pRTView->GetDesc();
    }

    if (pDSV)
    {
        auto* pDepthTexGL = pDSV->GetTexture<TextureBaseGL>();
        pDepthTexGL->TextureMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT, ContextState);
        Key.DSId    = pDepthTexGL->GetUniqueID();
        Key.DSVDesc = pDSV->GetDesc();
    }

    // Try to find FBO in the map
    auto It = m_Cache.find(Key);
    if (It != m_Cache.end())
    {
        return It->second;
    }
    else
    {
        // Create a new FBO
        auto NewFBO = CreateFBO(ContextState, NumRenderTargets, ppRTVs, pDSV);

        auto NewElems = m_Cache.emplace(std::make_pair(Key, std::move(NewFBO)));
        // New element must be actually inserted
        VERIFY(NewElems.second, "New element was not inserted");
        if (Key.DSId != 0)
            m_TexIdToKey.insert(std::make_pair(Key.DSId, Key));
        for (Uint32 rt = 0; rt < NumRenderTargets; ++rt)
        {
            if (Key.RTIds[rt] != 0)
                m_TexIdToKey.insert(std::make_pair(Key.RTIds[rt], Key));
        }

        return NewElems.first->second;
    }
}

} // namespace Diligent
