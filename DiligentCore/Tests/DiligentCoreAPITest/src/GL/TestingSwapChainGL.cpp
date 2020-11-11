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

#include "GL/TestingEnvironmentGL.hpp"
#include "GL/TestingSwapChainGL.hpp"

namespace Diligent
{

namespace Testing
{

TestingSwapChainGL::TestingSwapChainGL(IReferenceCounters*  pRefCounters,
                                       IRenderDevice*       pDevice,
                                       IDeviceContext*      pContext,
                                       const SwapChainDesc& SCDesc) :
    TBase //
    {
        pRefCounters,
        pDevice,
        pContext,
        SCDesc //
    }
{
    {
        glGenTextures(1, &m_RenderTarget);
        if (glGetError() != GL_NO_ERROR)
            LOG_ERROR_AND_THROW("Failed to create render target texture");

        GLenum RenderTargetFmt = 0;
        switch (m_SwapChainDesc.ColorBufferFormat)
        {
            case TEX_FORMAT_RGBA8_UNORM:
                RenderTargetFmt = GL_RGBA8;
                break;

            default:
                UNSUPPORTED("Texture format ", GetTextureFormatAttribs(m_SwapChainDesc.ColorBufferFormat).Name, " is not a supported color buffer format");
        }

        glBindTexture(GL_TEXTURE_2D, m_RenderTarget);
        //                          levels    format          width                     height
        glTexStorage2D(GL_TEXTURE_2D, 1, RenderTargetFmt, m_SwapChainDesc.Width, m_SwapChainDesc.Height);
        if (glGetError() != GL_NO_ERROR)
            LOG_ERROR_AND_THROW("Failed to allocate render target texture");
    }


    {
        glGenTextures(1, &m_DepthBuffer);
        if (glGetError() != GL_NO_ERROR)
            LOG_ERROR_AND_THROW("Failed to create depth texture");

        GLenum DepthBufferFmt = 0;
        switch (m_SwapChainDesc.DepthBufferFormat)
        {
            case TEX_FORMAT_D32_FLOAT:
                DepthBufferFmt = GL_DEPTH_COMPONENT32F;
                break;

            default:
                UNSUPPORTED("Texture format ", GetTextureFormatAttribs(m_SwapChainDesc.DepthBufferFormat).Name, " is not a supported depth buffer format");
        }

        glBindTexture(GL_TEXTURE_2D, m_DepthBuffer);
        //                          levels    format          width                     height
        glTexStorage2D(GL_TEXTURE_2D, 1, DepthBufferFmt, m_SwapChainDesc.Width, m_SwapChainDesc.Height);
        if (glGetError() != GL_NO_ERROR)
            LOG_ERROR_AND_THROW("Failed to allocate render target texture");
    }

    {
        glGenFramebuffers(1, &m_FBO);
        if (glGetError() != GL_NO_ERROR)
            LOG_ERROR_AND_THROW("Failed to create FBO");

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FBO);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FBO);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_RenderTarget, 0);
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_RenderTarget, 0);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_DepthBuffer, 0);
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_DepthBuffer, 0);
        static const GLenum DrawBuffers[] = {GL_COLOR_ATTACHMENT0};
        glDrawBuffers(1, DrawBuffers);
        GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (Status != GL_FRAMEBUFFER_COMPLETE)
            LOG_ERROR_AND_THROW("FBO is incomplete");
    }

    // Make sure Diligent Engine will reset all GL states
    pContext->InvalidateState();
}

void TestingSwapChainGL::BindFramebuffer()
{
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FBO);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FBO);
}

TestingSwapChainGL::~TestingSwapChainGL()
{
    if (m_RenderTarget != 0)
        glDeleteTextures(1, &m_RenderTarget);
    if (m_DepthBuffer != 0)
        glDeleteTextures(1, &m_DepthBuffer);
    if (m_FBO != 0)
        glDeleteFramebuffers(1, &m_FBO);
}

void TestingSwapChainGL::TakeSnapshot()
{
    m_ReferenceDataPitch = m_SwapChainDesc.Width * 4;
    m_ReferenceData.resize(m_SwapChainDesc.Height * m_ReferenceDataPitch);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FBO);
    glReadPixels(0, 0, m_SwapChainDesc.Width, m_SwapChainDesc.Height, GL_RGBA, GL_UNSIGNED_BYTE, m_ReferenceData.data());
    VERIFY(glGetError() == GL_NO_ERROR, "Failed to read pixels from the framebuffer");
}

void CreateTestingSwapChainGL(IRenderDevice*       pDevice,
                              IDeviceContext*      pContext,
                              const SwapChainDesc& SCDesc,
                              ISwapChain**         ppSwapChain)
{
    try
    {
        TestingSwapChainGL* pTestingSC(MakeNewRCObj<TestingSwapChainGL>()(pDevice, pContext, SCDesc));
        pTestingSC->QueryInterface(IID_SwapChain, reinterpret_cast<IObject**>(ppSwapChain));
    }
    catch (...)
    {
        *ppSwapChain = nullptr;
    }
}

} // namespace Testing

} // namespace Diligent
