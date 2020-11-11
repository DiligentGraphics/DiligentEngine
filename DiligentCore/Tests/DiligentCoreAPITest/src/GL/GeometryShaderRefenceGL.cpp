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

#include "InlineShaders/GeometryShaderTestGLSL.h"

namespace Diligent
{

namespace Testing
{

void GeometryShaderReferenceGL(ISwapChain* pSwapChain)
{
    auto* pEnv                = TestingEnvironmentGL::GetInstance();
    auto* pContext            = pEnv->GetDeviceContext();
    auto* pTestingSwapChainGL = ValidatedCast<TestingSwapChainGL>(pSwapChain);

    const auto& SCDesc = pTestingSwapChainGL->GetDesc();

    GLuint glShaders[3] = {};

    glShaders[0] = pEnv->CompileGLShader(GLSL::GSTest_VS, GL_VERTEX_SHADER);
    ASSERT_NE(glShaders[0], 0u);
    glShaders[1] = pEnv->CompileGLShader(GLSL::GSTest_GS, GL_GEOMETRY_SHADER);
    ASSERT_NE(glShaders[1], 0u);
    glShaders[2] = pEnv->CompileGLShader(GLSL::GSTest_FS, GL_FRAGMENT_SHADER);
    ASSERT_NE(glShaders[2], 0u);
    auto glProg = pEnv->LinkProgram(glShaders, _countof(glShaders));
    ASSERT_NE(glProg, 0u);

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    if (glPolygonMode != nullptr)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    pTestingSwapChainGL->BindFramebuffer();
    glViewport(0, 0, SCDesc.Width, SCDesc.Height);
    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(glProg);
    glBindVertexArray(pEnv->GetDummyVAO());
    glDrawArrays(GL_POINTS, 0, 2);
    glBindVertexArray(0);
    glUseProgram(0);

    ASSERT_TRUE(glGetError() == GL_NO_ERROR);

    for (int i = 0; i < _countof(glShaders); ++i)
    {
        if (glShaders[i] != 0)
            glDeleteShader(glShaders[i]);
    }
    if (glProg != 0)
        glDeleteProgram(glProg);

    // Make sure Diligent Engine will reset all GL states
    pContext->InvalidateState();
}

} // namespace Testing

} // namespace Diligent
