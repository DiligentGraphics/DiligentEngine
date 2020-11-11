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

#include <cstring>
#include <sstream>

#include "GLSLUtils.hpp"
#include "DebugUtilities.hpp"
#if !DILIGENT_NO_HLSL
#    include "HLSL2GLSLConverterImpl.hpp"
#endif
#include "RefCntAutoPtr.hpp"
#include "DataBlobImpl.hpp"
#include "ShaderToolsCommon.hpp"

namespace Diligent
{

String BuildGLSLSourceString(const ShaderCreateInfo& ShaderCI,
                             const DeviceCaps&       deviceCaps,
                             TargetGLSLCompiler      TargetCompiler,
                             const char*             ExtraDefinitions)
{
    // clang-format off
    VERIFY(ShaderCI.SourceLanguage == SHADER_SOURCE_LANGUAGE_DEFAULT ||
           ShaderCI.SourceLanguage == SHADER_SOURCE_LANGUAGE_GLSL    ||
           ShaderCI.SourceLanguage == SHADER_SOURCE_LANGUAGE_HLSL,
           "Unsupported shader source language");
    // clang-format on

    String GLSLSource;

    const auto ShaderType = ShaderCI.Desc.ShaderType;

#if PLATFORM_WIN32 || PLATFORM_LINUX
    GLSLSource.append(
        "#version 430 core\n"
        "#define DESKTOP_GL 1\n");
#    if PLATFORM_WIN32
    GLSLSource.append("#define PLATFORM_WIN32 1\n");
#    elif PLATFORM_LINUX
    GLSLSource.append("#define PLATFORM_LINUX 1\n");
#    else
#        error Unexpected platform
#    endif
#elif PLATFORM_MACOS
    if (TargetCompiler == TargetGLSLCompiler::glslang)
        GLSLSource.append("#version 430 core\n");
    else if (TargetCompiler == TargetGLSLCompiler::driver)
        GLSLSource.append("#version 410 core\n");
    else
        UNEXPECTED("Unexpected target GLSL compiler");

    GLSLSource.append(
        "#define DESKTOP_GL 1\n"
        "#define PLATFORM_MACOS 1\n");

#elif PLATFORM_ANDROID || PLATFORM_IOS
    bool IsES30        = false;
    bool IsES31OrAbove = false;
    bool IsES32OrAbove = false;
    if (deviceCaps.DevType == RENDER_DEVICE_TYPE_VULKAN)
    {
        IsES30        = false;
        IsES31OrAbove = true;
        IsES32OrAbove = false;
        GLSLSource.append("#version 310 es\n");
    }
    else if (deviceCaps.DevType == RENDER_DEVICE_TYPE_GLES)
    {
        IsES30        = deviceCaps.MajorVersion == 3 && deviceCaps.MinorVersion == 0;
        IsES31OrAbove = deviceCaps.MajorVersion > 3 || (deviceCaps.MajorVersion == 3 && deviceCaps.MinorVersion >= 1);
        IsES32OrAbove = deviceCaps.MajorVersion > 3 || (deviceCaps.MajorVersion == 3 && deviceCaps.MinorVersion >= 2);
        std::stringstream versionss;
        versionss << "#version " << deviceCaps.MajorVersion << deviceCaps.MinorVersion << "0 es\n";
        GLSLSource.append(versionss.str());
    }
    else
    {
        UNEXPECTED("Unexpected device type");
    }

    if (deviceCaps.Features.SeparablePrograms && !IsES31OrAbove)
        GLSLSource.append("#extension GL_EXT_separate_shader_objects : enable\n");

    if (deviceCaps.TexCaps.CubemapArraysSupported && !IsES32OrAbove)
        GLSLSource.append("#extension GL_EXT_texture_cube_map_array : enable\n");

    if (ShaderType == SHADER_TYPE_GEOMETRY && !IsES32OrAbove)
        GLSLSource.append("#extension GL_EXT_geometry_shader : enable\n");

    if ((ShaderType == SHADER_TYPE_HULL || ShaderType == SHADER_TYPE_DOMAIN) && !IsES32OrAbove)
        GLSLSource.append("#extension GL_EXT_tessellation_shader : enable\n");

    GLSLSource.append(
        "#ifndef GL_ES\n"
        "#  define GL_ES 1\n"
        "#endif\n");

#    if PLATFORM_ANDROID
    GLSLSource.append("#define PLATFORM_ANDROID 1\n");
#    elif PLATFORM_IOS
    GLSLSource.append("#define PLATFORM_IOS 1\n");
#    else
#        error "Unexpected platform"
#    endif

    GLSLSource.append(
        "precision highp float;\n"
        "precision highp int;\n"
        //"precision highp uint;\n"  // This line causes shader compilation error on NVidia!

        "precision highp sampler2D;\n"
        "precision highp sampler3D;\n"
        "precision highp samplerCube;\n"
        "precision highp samplerCubeShadow;\n"

        "precision highp sampler2DShadow;\n"
        "precision highp sampler2DArray;\n"
        "precision highp sampler2DArrayShadow;\n"

        "precision highp isampler2D;\n"
        "precision highp isampler3D;\n"
        "precision highp isamplerCube;\n"
        "precision highp isampler2DArray;\n"

        "precision highp usampler2D;\n"
        "precision highp usampler3D;\n"
        "precision highp usamplerCube;\n"
        "precision highp usampler2DArray;\n" // clang-format off
        ); // clang-format on

    if (IsES32OrAbove)
    {
        GLSLSource.append(
            "precision highp samplerBuffer;\n"
            "precision highp isamplerBuffer;\n"
            "precision highp usamplerBuffer;\n" // clang-format off
        ); // clang-format on
    }

    if (deviceCaps.TexCaps.CubemapArraysSupported)
    {
        GLSLSource.append(
            "precision highp samplerCubeArray;\n"
            "precision highp samplerCubeArrayShadow;\n"
            "precision highp isamplerCubeArray;\n"
            "precision highp usamplerCubeArray;\n" // clang-format off
        ); // clang-format on
    }

    if (deviceCaps.TexCaps.Texture2DMSSupported)
    {
        GLSLSource.append(
            "precision highp sampler2DMS;\n"
            "precision highp isampler2DMS;\n"
            "precision highp usampler2DMS;\n" // clang-format off
        ); // clang-format on
    }

    if (deviceCaps.Features.ComputeShaders)
    {
        GLSLSource.append(
            "precision highp image2D;\n"
            "precision highp image3D;\n"
            "precision highp imageCube;\n"
            "precision highp image2DArray;\n"

            "precision highp iimage2D;\n"
            "precision highp iimage3D;\n"
            "precision highp iimageCube;\n"
            "precision highp iimage2DArray;\n"

            "precision highp uimage2D;\n"
            "precision highp uimage3D;\n"
            "precision highp uimageCube;\n"
            "precision highp uimage2DArray;\n" // clang-format off
        ); // clang-format on
        if (IsES32OrAbove)
        {
            GLSLSource.append(
                "precision highp imageBuffer;\n"
                "precision highp iimageBuffer;\n"
                "precision highp uimageBuffer;\n" // clang-format off
            ); // clang-format on
        }
    }

    if (IsES30 && deviceCaps.Features.SeparablePrograms && ShaderType == SHADER_TYPE_VERTEX)
    {
        // From https://www.khronos.org/registry/OpenGL/extensions/EXT/EXT_separate_shader_objects.gles.txt:
        //
        // When using GLSL ES 3.00 shaders in separable programs, gl_Position and
        // gl_PointSize built-in outputs must be redeclared according to Section 7.5
        // of the OpenGL Shading Language Specification...
        //
        // add to GLSL ES 3.00 new section 7.5, Built-In Redeclaration and
        // Separable Programs:
        //
        // "The following vertex shader outputs may be redeclared at global scope to
        // specify a built-in output interface, with or without special qualifiers:
        //
        //     gl_Position
        //     gl_PointSize
        //
        //   When compiling shaders using either of the above variables, both such
        //   variables must be redeclared prior to use.  ((Note:  This restriction
        //   applies only to shaders using version 300 that enable the
        //   EXT_separate_shader_objects extension; shaders not enabling the
        //   extension do not have this requirement.))  A separable program object
        //   will fail to link if any attached shader uses one of the above variables
        //   without redeclaration."
        GLSLSource.append("out vec4 gl_Position;\n");
    }

#elif
#    error "Undefined platform"
#endif

    // It would be much more convenient to use row_major matrices.
    // But unfortunatelly on NVIDIA, the following directive
    // layout(std140, row_major) uniform;
    // does not have any effect on matrices that are part of structures
    // So we have to use column-major matrices which are default in both
    // DX and GLSL.
    GLSLSource.append(
        "layout(std140) uniform;\n");

    if (ShaderType == SHADER_TYPE_VERTEX && TargetCompiler == TargetGLSLCompiler::glslang)
    {
        // https://github.com/KhronosGroup/GLSL/blob/master/extensions/khr/GL_KHR_vulkan_glsl.txt
        GLSLSource.append("#define gl_VertexID gl_VertexIndex\n"
                          "#define gl_InstanceID gl_InstanceIndex\n");
    }

    AppendShaderTypeDefinitions(GLSLSource, ShaderType);

    if (ExtraDefinitions != nullptr)
    {
        GLSLSource.append(ExtraDefinitions);
    }

    AppendShaderMacros(GLSLSource, ShaderCI.Macros);

    RefCntAutoPtr<IDataBlob> pFileData;
    size_t                   SourceLen = 0;

    const auto* ShaderSource = ReadShaderSourceFile(ShaderCI.Source, ShaderCI.pShaderSourceStreamFactory, ShaderCI.FilePath, pFileData, SourceLen);

    if (ShaderCI.SourceLanguage == SHADER_SOURCE_LANGUAGE_HLSL)
    {
#if DILIGENT_NO_HLSL
        LOG_ERROR_AND_THROW("Unable to convert HLSL source to GLSL: HLSL support is disabled");
#else
        if (!ShaderCI.UseCombinedTextureSamplers)
        {
            LOG_ERROR_AND_THROW("Combined texture samplers are required to convert HLSL source to GLSL");
        }
        // Convert HLSL to GLSL
        const auto& Converter = HLSL2GLSLConverterImpl::GetInstance();

        HLSL2GLSLConverterImpl::ConversionAttribs Attribs;
        Attribs.pSourceStreamFactory = ShaderCI.pShaderSourceStreamFactory;
        Attribs.ppConversionStream   = ShaderCI.ppConversionStream;
        Attribs.HLSLSource           = ShaderSource;
        Attribs.NumSymbols           = SourceLen;
        Attribs.EntryPoint           = ShaderCI.EntryPoint;
        Attribs.ShaderType           = ShaderCI.Desc.ShaderType;
        Attribs.IncludeDefinitions   = true;
        Attribs.InputFileName        = ShaderCI.FilePath;
        Attribs.SamplerSuffix        = ShaderCI.CombinedSamplerSuffix;
        // Separate shader objects extension also allows input/output layout qualifiers for
        // all shader stages.
        // https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_separate_shader_objects.txt
        // (search for "Input Layout Qualifiers" and "Output Layout Qualifiers").
        Attribs.UseInOutLocationQualifiers = deviceCaps.Features.SeparablePrograms;
        auto ConvertedSource               = Converter.Convert(Attribs);

        GLSLSource.append(ConvertedSource);
#endif
    }
    else
    {
        GLSLSource.append(ShaderSource, SourceLen);
    }

    return GLSLSource;
}

} // namespace Diligent
