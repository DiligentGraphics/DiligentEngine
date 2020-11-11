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

#include "InlineShaders/ComputeShaderTestGLSL.h"

namespace Diligent
{

namespace Testing
{

void ComputeShaderReferenceGL(ISwapChain* pSwapChain)
{
    auto* pEnv                = TestingEnvironmentGL::GetInstance();
    auto* pContext            = pEnv->GetDeviceContext();
    auto* pTestingSwapChainGL = ValidatedCast<TestingSwapChainGL>(pSwapChain);

    const auto& SCDesc = pTestingSwapChainGL->GetDesc();

    GLuint glCS;

    glCS = pEnv->CompileGLShader(GLSL::FillTextureCS, GL_COMPUTE_SHADER);
    ASSERT_NE(glCS, 0u);

    auto glProg = pEnv->LinkProgram(&glCS, 1);
    ASSERT_NE(glProg, 0u);

    glUseProgram(glProg);

    GLenum glFormat = 0;
    switch (SCDesc.ColorBufferFormat)
    {
        case TEX_FORMAT_RGBA8_UNORM:
            glFormat = GL_RGBA8;
            break;

        default:
            UNEXPECTED("Unexpected texture format");
    }
    glBindImageTexture(0, pTestingSwapChainGL->GetRenderTargetGLHandle(), 0, 0, 0, GL_WRITE_ONLY, glFormat);
    glDispatchCompute((SCDesc.Width + 15) / 16, (SCDesc.Height + 15) / 16, 1);
    VERIFY_EXPR(glGetError() == GL_NO_ERROR);

    glUseProgram(0);

    if (glCS != 0)
        glDeleteShader(glCS);

    if (glProg != 0)
        glDeleteProgram(glProg);


    // Make sure Diligent Engine will reset all GL states
    pContext->InvalidateState();
}

} // namespace Testing

} // namespace Diligent
