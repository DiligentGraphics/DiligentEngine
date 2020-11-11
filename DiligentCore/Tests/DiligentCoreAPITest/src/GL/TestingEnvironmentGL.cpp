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

namespace Diligent
{

namespace Testing
{

void CreateTestingSwapChainGL(IRenderDevice*       pDevice,
                              IDeviceContext*      pContext,
                              const SwapChainDesc& SCDesc,
                              ISwapChain**         ppSwapChain);

TestingEnvironmentGL::TestingEnvironmentGL(const CreateInfo&    CI,
                                           const SwapChainDesc& SCDesc) :
    TestingEnvironment{CI, SCDesc}
{
    // Initialize GLEW
    auto err = glewInit();
    if (GLEW_OK != err)
        LOG_ERROR_AND_THROW("Failed to initialize GLEW");

    if (m_pSwapChain == nullptr)
    {
        CreateTestingSwapChainGL(m_pDevice, m_pDeviceContext, SCDesc, &m_pSwapChain);
    }

    glGenVertexArrays(1, &m_DummyVAO);
}

TestingEnvironmentGL::~TestingEnvironmentGL()
{
    glDeleteVertexArrays(1, &m_DummyVAO);
}

GLuint TestingEnvironmentGL::CompileGLShader(const std::string& Source, GLenum ShaderType)
{
    GLuint glShader = glCreateShader(ShaderType);

    const char* ShaderStrings[] = {Source.c_str()};
    GLint       Lenghts[]       = {static_cast<GLint>(Source.length())};

    // Provide source strings (the strings will be saved in internal OpenGL memory)
    glShaderSource(glShader, _countof(ShaderStrings), ShaderStrings, Lenghts);
    // When the shader is compiled, it will be compiled as if all of the given strings were concatenated end-to-end.
    glCompileShader(glShader);
    GLint compiled = GL_FALSE;
    // Get compilation status
    glGetShaderiv(glShader, GL_COMPILE_STATUS, &compiled);
    if (!compiled)
    {
        int infoLogLen = 0;
        // The function glGetShaderiv() tells how many bytes to allocate; the length includes the NULL terminator.
        glGetShaderiv(glShader, GL_INFO_LOG_LENGTH, &infoLogLen);

        std::vector<GLchar> infoLog(infoLogLen);
        if (infoLogLen > 0)
        {
            int charsWritten = 0;
            // Get the log. infoLogLen is the size of infoLog. This tells OpenGL how many bytes at maximum it will write
            // charsWritten is a return value, specifying how many bytes it actually wrote. One may pass NULL if he
            // doesn't care
            glGetShaderInfoLog(glShader, infoLogLen, &charsWritten, infoLog.data());
            VERIFY(charsWritten == infoLogLen - 1, "Unexpected info log length");
            LOG_ERROR("Failed to compile GL shader\n", infoLog.data());
        }
    }

    return glShader;
}

GLuint TestingEnvironmentGL::LinkProgram(GLuint Shaders[], GLuint NumShaders)
{
    auto glProg = glCreateProgram();

    for (Uint32 i = 0; i < NumShaders; ++i)
    {
        glAttachShader(glProg, Shaders[i]);
        VERIFY_EXPR(glGetError() == GL_NO_ERROR);
    }

    glLinkProgram(glProg);
    int IsLinked = GL_FALSE;
    glGetProgramiv(glProg, GL_LINK_STATUS, &IsLinked);
    if (!IsLinked)
    {
        int LengthWithNull = 0, Length = 0;
        // Notice that glGetProgramiv is used to get the length for a shader program, not glGetShaderiv.
        // The length of the info log includes a null terminator.
        glGetProgramiv(glProg, GL_INFO_LOG_LENGTH, &LengthWithNull);

        // The maxLength includes the NULL character
        std::vector<char> shaderProgramInfoLog(LengthWithNull);

        // Notice that glGetProgramInfoLog  is used, not glGetShaderInfoLog.
        glGetProgramInfoLog(glProg, LengthWithNull, &Length, shaderProgramInfoLog.data());
        VERIFY(Length == LengthWithNull - 1, "Incorrect program info log len");
        LOG_ERROR_MESSAGE("Failed to link shader program:\n", shaderProgramInfoLog.data(), '\n');
    }

    for (Uint32 i = 0; i < NumShaders; ++i)
    {
        glDetachShader(glProg, Shaders[i]);
    }
    return glProg;
}

void TestingEnvironmentGL::Reset()
{
    TestingEnvironment::Reset();

    glUseProgram(0);
    glBindProgramPipeline(0);
    glBindVertexArray(0);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    GLint iMaxCombinedTexUnits = 0;
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &iMaxCombinedTexUnits);
    VERIFY_EXPR(glGetError() == GL_NO_ERROR);
    for (int i = 0; i < iMaxCombinedTexUnits; ++i)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
        glBindTexture(GL_TEXTURE_3D, 0);
    }
    glActiveTexture(GL_TEXTURE0);
    VERIFY_EXPR(glGetError() == GL_NO_ERROR);

    GLint iMaxUniformBufferBindings = 0;
    glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &iMaxUniformBufferBindings);
    VERIFY_EXPR(glGetError() == GL_NO_ERROR);
    for (int i = 0; i < iMaxUniformBufferBindings; ++i)
    {
        glBindBufferBase(GL_UNIFORM_BUFFER, i, 0);
    }
    VERIFY_EXPR(glGetError() == GL_NO_ERROR);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, 0);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glBindBuffer(GL_QUERY_BUFFER, 0);
    VERIFY_EXPR(glGetError() == GL_NO_ERROR);

    if (glPolygonMode != nullptr)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

TestingEnvironment* CreateTestingEnvironmentGL(const TestingEnvironment::CreateInfo& CI,
                                               const SwapChainDesc&                  SCDesc)
{
    try
    {
        return new TestingEnvironmentGL{CI, SCDesc};
    }
    catch (...)
    {
        return nullptr;
    }
}

} // namespace Testing

} // namespace Diligent
