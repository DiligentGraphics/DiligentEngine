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

#include <array>

#include "ShaderGLImpl.hpp"
#include "RenderDeviceGLImpl.hpp"
#include "DeviceContextGLImpl.hpp"
#include "DataBlobImpl.hpp"
#include "GLSLUtils.hpp"
#include "ShaderToolsCommon.hpp"

using namespace Diligent;

namespace Diligent
{

ShaderGLImpl::ShaderGLImpl(IReferenceCounters*     pRefCounters,
                           RenderDeviceGLImpl*     pDeviceGL,
                           const ShaderCreateInfo& ShaderCI,
                           bool                    bIsDeviceInternal) :
    // clang-format off
    TShaderBase
    {
        pRefCounters,
        pDeviceGL,
        ShaderCI.Desc,
        bIsDeviceInternal
    },
    m_GLShaderObj{true, GLObjectWrappers::GLShaderObjCreateReleaseHelper{GetGLShaderType(m_Desc.ShaderType)}}
// clang-format on
{
    DEV_CHECK_ERR(ShaderCI.ByteCode == nullptr, "'ByteCode' must be null when shader is created from the source code or a file");
    DEV_CHECK_ERR(ShaderCI.ByteCodeSize == 0, "'ByteCodeSize' must be 0 when shader is created from the source code or a file");
    DEV_CHECK_ERR(ShaderCI.ShaderCompiler == SHADER_COMPILER_DEFAULT, "only default compiler is supported in OpenGL");

    const auto& deviceCaps = pDeviceGL->GetDeviceCaps();

    // Note: there is a simpler way to create the program:
    //m_uiShaderSeparateProg = glCreateShaderProgramv(GL_VERTEX_SHADER, _countof(ShaderStrings), ShaderStrings);
    // NOTE: glCreateShaderProgramv() is considered equivalent to both a shader compilation and a program linking
    // operation. Since it performs both at the same time, compiler or linker errors can be encountered. However,
    // since this function only returns a program object, compiler-type errors will be reported as linker errors
    // through the following API:
    // GLint isLinked = 0;
    // glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
    // The log can then be queried in the same way


    // Each element in the length array may contain the length of the corresponding string
    // (the null character is not counted as part of the string length).
    // Not specifying lengths causes shader compilation errors on Android
    std::array<const char*, 1> ShaderStrings = {};
    std::array<GLint, 1>       Lenghts       = {};

    std::string              GLSLSourceString;
    RefCntAutoPtr<IDataBlob> pSourceFileData;
    if (ShaderCI.SourceLanguage == SHADER_SOURCE_LANGUAGE_GLSL_VERBATIM)
    {
        if (ShaderCI.Macros != nullptr)
        {
            LOG_WARNING_MESSAGE("Shader macros are ignored when compiling GLSL verbatim in OpenGL backend");
        }

        // Read the source file directly and use it as is
        size_t SourceLen = 0;
        ShaderStrings[0] = ReadShaderSourceFile(ShaderCI.Source, ShaderCI.pShaderSourceStreamFactory, ShaderCI.FilePath, pSourceFileData, SourceLen);
        Lenghts[0]       = static_cast<GLint>(SourceLen);
    }
    else
    {
        // Build the full source code string that will contain GLSL version declaration,
        // platform definitions, user-provided shader macros, etc.
        GLSLSourceString = BuildGLSLSourceString(ShaderCI, deviceCaps, TargetGLSLCompiler::driver);
        ShaderStrings[0] = GLSLSourceString.c_str();
        Lenghts[0]       = static_cast<GLint>(GLSLSourceString.length());
    }


    // Provide source strings (the strings will be saved in internal OpenGL memory)
    glShaderSource(m_GLShaderObj, static_cast<GLsizei>(ShaderStrings.size()), ShaderStrings.data(), Lenghts.data());
    // When the shader is compiled, it will be compiled as if all of the given strings were concatenated end-to-end.
    glCompileShader(m_GLShaderObj);
    GLint compiled = GL_FALSE;
    // Get compilation status
    glGetShaderiv(m_GLShaderObj, GL_COMPILE_STATUS, &compiled);
    if (!compiled)
    {
        std::string FullSource;
        for (const auto* str : ShaderStrings)
            FullSource.append(str);

        std::stringstream ErrorMsgSS;
        ErrorMsgSS << "Failed to compile shader file '" << (ShaderCI.Desc.Name != nullptr ? ShaderCI.Desc.Name : "") << '\'' << std::endl;
        int infoLogLen = 0;
        // The function glGetShaderiv() tells how many bytes to allocate; the length includes the NULL terminator.
        glGetShaderiv(m_GLShaderObj, GL_INFO_LOG_LENGTH, &infoLogLen);

        std::vector<GLchar> infoLog(infoLogLen);
        if (infoLogLen > 0)
        {
            int charsWritten = 0;
            // Get the log. infoLogLen is the size of infoLog. This tells OpenGL how many bytes at maximum it will write
            // charsWritten is a return value, specifying how many bytes it actually wrote. One may pass NULL if he
            // doesn't care
            glGetShaderInfoLog(m_GLShaderObj, infoLogLen, &charsWritten, infoLog.data());
            VERIFY(charsWritten == infoLogLen - 1, "Unexpected info log length");
            ErrorMsgSS << "InfoLog:" << std::endl
                       << infoLog.data() << std::endl;
        }

        if (ShaderCI.ppCompilerOutput != nullptr)
        {
            // infoLogLen accounts for null terminator
            auto* pOutputDataBlob = MakeNewRCObj<DataBlobImpl>()(infoLogLen + FullSource.length() + 1);
            char* DataPtr         = reinterpret_cast<char*>(pOutputDataBlob->GetDataPtr());
            if (infoLogLen > 0)
                memcpy(DataPtr, infoLog.data(), infoLogLen);
            memcpy(DataPtr + infoLogLen, FullSource.data(), FullSource.length() + 1);
            pOutputDataBlob->QueryInterface(IID_DataBlob, reinterpret_cast<IObject**>(ShaderCI.ppCompilerOutput));
        }
        else
        {
            // Dump full source code to debug output
            LOG_INFO_MESSAGE("Failed shader full source: \n\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n",
                             FullSource,
                             "\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n");
        }

        LOG_ERROR_AND_THROW(ErrorMsgSS.str().c_str());
    }

    if (deviceCaps.Features.SeparablePrograms)
    {
        ShaderGLImpl*                  ThisShader[]         = {this};
        GLObjectWrappers::GLProgramObj Program              = LinkProgram(ThisShader, 1, true);
        Uint32                         UniformBufferBinding = 0;
        Uint32                         SamplerBinding       = 0;
        Uint32                         ImageBinding         = 0;
        Uint32                         StorageBufferBinding = 0;
        auto                           pImmediateCtx        = m_pDevice->GetImmediateContext();
        VERIFY_EXPR(pImmediateCtx);
        auto& GLState = pImmediateCtx.RawPtr<DeviceContextGLImpl>()->GetContextState();
        m_Resources.LoadUniforms(m_Desc.ShaderType, Program, GLState, UniformBufferBinding, SamplerBinding, ImageBinding, StorageBufferBinding);
    }
}

ShaderGLImpl::~ShaderGLImpl()
{
}

IMPLEMENT_QUERY_INTERFACE(ShaderGLImpl, IID_ShaderGL, TShaderBase)


GLObjectWrappers::GLProgramObj ShaderGLImpl::LinkProgram(ShaderGLImpl** ppShaders, Uint32 NumShaders, bool IsSeparableProgram)
{
    VERIFY(!IsSeparableProgram || NumShaders == 1, "Number of shaders must be 1 when separable program is created");

    GLObjectWrappers::GLProgramObj GLProg(true);

    // GL_PROGRAM_SEPARABLE parameter must be set before linking!
    if (IsSeparableProgram)
        glProgramParameteri(GLProg, GL_PROGRAM_SEPARABLE, GL_TRUE);

    for (Uint32 i = 0; i < NumShaders; ++i)
    {
        auto* pCurrShader = ppShaders[i];
        glAttachShader(GLProg, pCurrShader->m_GLShaderObj);
        CHECK_GL_ERROR("glAttachShader() failed");
    }

    //With separable program objects, interfaces between shader stages may
    //involve the outputs from one program object and the inputs from a
    //second program object. For such interfaces, it is not possible to
    //detect mismatches at link time, because the programs are linked
    //separately. When each such program is linked, all inputs or outputs
    //interfacing with another program stage are treated as active. The
    //linker will generate an executable that assumes the presence of a
    //compatible program on the other side of the interface. If a mismatch
    //between programs occurs, no GL error will be generated, but some or all
    //of the inputs on the interface will be undefined.
    glLinkProgram(GLProg);
    CHECK_GL_ERROR("glLinkProgram() failed");
    int IsLinked = GL_FALSE;
    glGetProgramiv(GLProg, GL_LINK_STATUS, &IsLinked);
    CHECK_GL_ERROR("glGetProgramiv() failed");
    if (!IsLinked)
    {
        int LengthWithNull = 0, Length = 0;
        // Notice that glGetProgramiv is used to get the length for a shader program, not glGetShaderiv.
        // The length of the info log includes a null terminator.
        glGetProgramiv(GLProg, GL_INFO_LOG_LENGTH, &LengthWithNull);

        // The maxLength includes the NULL character
        std::vector<char> shaderProgramInfoLog(LengthWithNull);

        // Notice that glGetProgramInfoLog  is used, not glGetShaderInfoLog.
        glGetProgramInfoLog(GLProg, LengthWithNull, &Length, shaderProgramInfoLog.data());
        VERIFY(Length == LengthWithNull - 1, "Incorrect program info log len");
        LOG_ERROR_MESSAGE("Failed to link shader program:\n", shaderProgramInfoLog.data(), '\n');
        UNEXPECTED("glLinkProgram failed");
    }

    for (Uint32 i = 0; i < NumShaders; ++i)
    {
        auto* pCurrShader = ValidatedCast<ShaderGLImpl>(ppShaders[i]);
        glDetachShader(GLProg, pCurrShader->m_GLShaderObj);
        CHECK_GL_ERROR("glDetachShader() failed");
    }

    return GLProg;
}

Uint32 ShaderGLImpl::GetResourceCount() const
{
    if (m_pDevice->GetDeviceCaps().Features.SeparablePrograms)
    {
        return m_Resources.GetVariableCount();
    }
    else
    {
        LOG_WARNING_MESSAGE("Shader resource queries are not available when separate shader objects are unsupported");
        return 0;
    }
}

void ShaderGLImpl::GetResourceDesc(Uint32 Index, ShaderResourceDesc& ResourceDesc) const
{
    if (m_pDevice->GetDeviceCaps().Features.SeparablePrograms)
    {
        DEV_CHECK_ERR(Index < GetResourceCount(), "Index is out of range");
        ResourceDesc = m_Resources.GetResourceDesc(Index);
    }
    else
    {
        LOG_WARNING_MESSAGE("Shader resource queries are not available when separate shader objects are unsupported");
    }
}

} // namespace Diligent
