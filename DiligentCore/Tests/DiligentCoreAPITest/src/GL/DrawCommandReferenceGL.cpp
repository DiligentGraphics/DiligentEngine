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

#include "InlineShaders/DrawCommandTestGLSL.h"

namespace Diligent
{

namespace Testing
{

namespace
{

class TriangleRenderer
{

public:
    TriangleRenderer(const std::string& FSSource)
    {
        auto* pEnv = TestingEnvironmentGL::GetInstance();

        GLuint glShaders[2] = {};
        for (int i = 0; i < _countof(glShaders); ++i)
        {
            if (glShaders[i] != 0)
                glDeleteShader(glShaders[i]);
        }

        glShaders[0] = pEnv->CompileGLShader(GLSL::DrawTest_ProceduralTriangleVS, GL_VERTEX_SHADER);
        VERIFY_EXPR(glShaders[0] != 0u);
        glShaders[1] = pEnv->CompileGLShader(FSSource, GL_FRAGMENT_SHADER);
        VERIFY_EXPR(glShaders[1] != 0u);
        m_glProg = pEnv->LinkProgram(glShaders, 2);
        VERIFY_EXPR(m_glProg != 0u);

        for (int i = 0; i < _countof(glShaders); ++i)
        {
            if (glShaders[i] != 0)
                glDeleteShader(glShaders[i]);
        }
    }

    ~TriangleRenderer()
    {
        if (m_glProg != 0)
            glDeleteProgram(m_glProg);
    }

    void Draw(Uint32 Width, Uint32 Height, const float* pClearColor)
    {
        auto* pEnv = TestingEnvironmentGL::GetInstance();

        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_SCISSOR_TEST);
        glDisable(GL_BLEND);
        glDisable(GL_CULL_FACE);
        if (glPolygonMode != nullptr)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
        glViewport(0, 0, Width, Height);
        if (pClearColor != nullptr)
            glClearColor(pClearColor[0], pClearColor[1], pClearColor[2], pClearColor[3]);
        else
            glClearColor(0.f, 0.f, 0.f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(m_glProg);
        glBindVertexArray(pEnv->GetDummyVAO());
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        glUseProgram(0);

        ASSERT_TRUE(glGetError() == GL_NO_ERROR);
    }

private:
    GLuint m_glProg = 0;
};

} // namespace

void RenderDrawCommandReferenceGL(ISwapChain* pSwapChain, const float* pClearColor)
{
    auto* pEnv                = TestingEnvironmentGL::GetInstance();
    auto* pContext            = pEnv->GetDeviceContext();
    auto* pTestingSwapChainGL = ValidatedCast<TestingSwapChainGL>(pSwapChain);

    const auto& SCDesc = pTestingSwapChainGL->GetDesc();

    TriangleRenderer TriRenderer{GLSL::DrawTest_FS};

    pTestingSwapChainGL->BindFramebuffer();

    TriRenderer.Draw(SCDesc.Width, SCDesc.Height, pClearColor);

    // Make sure Diligent Engine will reset all GL states
    pContext->InvalidateState();
}

void RenderPassMSResolveReferenceGL(ISwapChain* pSwapChain, const float* pClearColor)
{
    auto* pEnv                = TestingEnvironmentGL::GetInstance();
    auto* pContext            = pEnv->GetDeviceContext();
    auto* pTestingSwapChainGL = ValidatedCast<TestingSwapChainGL>(pSwapChain);

    const auto& SCDesc = pTestingSwapChainGL->GetDesc();

    TriangleRenderer TriRenderer{GLSL::DrawTest_FS};

    GLuint glMSTex = 0;
    glGenTextures(1, &glMSTex);
    ASSERT_EQ(glGetError(), GLenum{GL_NO_ERROR});

    GLenum fmt = 0;
    switch (SCDesc.ColorBufferFormat)
    {
        case TEX_FORMAT_RGBA8_UNORM:
            fmt = GL_RGBA8;
            break;

        default:
            UNSUPPORTED("Unsupported swap chain format");
    }
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, glMSTex);
    glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, fmt, SCDesc.Width, SCDesc.Height, GL_TRUE);
    ASSERT_EQ(glGetError(), GLenum{GL_NO_ERROR});

    GLuint glMSFB = 0;
    glGenFramebuffers(1, &glMSFB);
    ASSERT_EQ(glGetError(), GLenum{GL_NO_ERROR});

    glBindFramebuffer(GL_FRAMEBUFFER, glMSFB);
    ASSERT_EQ(glGetError(), GLenum{GL_NO_ERROR});
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, glMSTex, 0);
    ASSERT_EQ(glCheckFramebufferStatus(GL_FRAMEBUFFER), GLenum{GL_FRAMEBUFFER_COMPLETE});

    TriRenderer.Draw(SCDesc.Width, SCDesc.Height, pClearColor);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, glMSFB);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, pTestingSwapChainGL->GetFBO());

    // Blit will perform MS resolve
    glBlitFramebuffer(0, 0, static_cast<GLint>(SCDesc.Width), static_cast<GLint>(SCDesc.Height),
                      0, 0, static_cast<GLint>(SCDesc.Width), static_cast<GLint>(SCDesc.Height),
                      GL_COLOR_BUFFER_BIT,
                      GL_NEAREST // Filter is ignored
    );
    ASSERT_EQ(glGetError(), GLenum{GL_NO_ERROR});

    glDeleteFramebuffers(1, &glMSFB);
    glDeleteTextures(1, &glMSTex);

    // Make sure Diligent Engine will reset all GL states
    pContext->InvalidateState();
}

void RenderPassInputAttachmentReferenceGL(ISwapChain* pSwapChain, const float* pClearColor)
{
    auto* pEnv                = TestingEnvironmentGL::GetInstance();
    auto* pContext            = pEnv->GetDeviceContext();
    auto* pTestingSwapChainGL = ValidatedCast<TestingSwapChainGL>(pSwapChain);

    const auto& SCDesc = pTestingSwapChainGL->GetDesc();

    TriangleRenderer TriRenderer{GLSL::DrawTest_FS};

    TriangleRenderer TriRendererInptAtt{GLSL::InputAttachmentTestGL_FS};

    GLuint glInputAttTex = 0;
    glGenTextures(1, &glInputAttTex);
    ASSERT_EQ(glGetError(), GLenum{GL_NO_ERROR});

    GLenum fmt = 0;
    switch (SCDesc.ColorBufferFormat)
    {
        case TEX_FORMAT_RGBA8_UNORM:
            fmt = GL_RGBA8;
            break;

        default:
            UNSUPPORTED("Unsupported swap chain format");
    }
    glBindTexture(GL_TEXTURE_2D, glInputAttTex);
    glTexStorage2D(GL_TEXTURE_2D, 1, fmt, SCDesc.Width, SCDesc.Height);
    ASSERT_EQ(glGetError(), GLenum{GL_NO_ERROR});

    GLuint glInputAttFB = 0;
    glGenFramebuffers(1, &glInputAttFB);
    ASSERT_EQ(glGetError(), GLenum{GL_NO_ERROR});

    glBindFramebuffer(GL_FRAMEBUFFER, glInputAttFB);
    ASSERT_EQ(glGetError(), GLenum{GL_NO_ERROR});
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, glInputAttTex, 0);
    ASSERT_EQ(glCheckFramebufferStatus(GL_FRAMEBUFFER), GLenum{GL_FRAMEBUFFER_COMPLETE});

    TriRenderer.Draw(SCDesc.Width, SCDesc.Height, nullptr);

    pTestingSwapChainGL->BindFramebuffer();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, glInputAttTex);

    TriRendererInptAtt.Draw(SCDesc.Width, SCDesc.Height, pClearColor);

    glDeleteFramebuffers(1, &glInputAttFB);
    glDeleteTextures(1, &glInputAttTex);

    // Make sure Diligent Engine will reset all GL states
    pContext->InvalidateState();
}

} // namespace Testing

} // namespace Diligent
