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

#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <array>

#if (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK))
#    include <MoltenGLSLToSPIRVConverter/GLSLToSPIRVConverter.h>
#else
#    define ENABLE_HLSL
#    include "SPIRV/GlslangToSpv.h"
#endif

#include "GLSLangUtils.hpp"
#include "DebugUtilities.hpp"
#include "DataBlobImpl.hpp"
#include "RefCntAutoPtr.hpp"
#include "ShaderToolsCommon.hpp"

#include "spirv-tools/optimizer.hpp"

// clang-format off
static const char g_HLSLDefinitions[] =
{
#include "../../GraphicsEngineD3DBase/include/HLSLDefinitions_inc.fxh"
};
// clang-format on

namespace Diligent
{

namespace GLSLangUtils
{

void InitializeGlslang()
{
    ::glslang::InitializeProcess();
}

void FinalizeGlslang()
{
    ::glslang::FinalizeProcess();
}

static EShLanguage ShaderTypeToShLanguage(SHADER_TYPE ShaderType)
{
    static_assert(SHADER_TYPE_LAST == 0x080, "Please handle the new shader type in the switch below");
    switch (ShaderType)
    {
        // clang-format off
        case SHADER_TYPE_VERTEX:        return EShLangVertex;
        case SHADER_TYPE_HULL:          return EShLangTessControl;
        case SHADER_TYPE_DOMAIN:        return EShLangTessEvaluation;
        case SHADER_TYPE_GEOMETRY:      return EShLangGeometry;
        case SHADER_TYPE_PIXEL:         return EShLangFragment;
        case SHADER_TYPE_COMPUTE:       return EShLangCompute;
        case SHADER_TYPE_AMPLIFICATION: return EShLangTaskNV;
        case SHADER_TYPE_MESH:          return EShLangMeshNV;
        // clang-format on
        default:
            UNEXPECTED("Unexpected shader type");
            return EShLangCount;
    }
}

static TBuiltInResource InitResources()
{
    TBuiltInResource Resources;

    Resources.maxLights                                 = 32;
    Resources.maxClipPlanes                             = 6;
    Resources.maxTextureUnits                           = 32;
    Resources.maxTextureCoords                          = 32;
    Resources.maxVertexAttribs                          = 64;
    Resources.maxVertexUniformComponents                = 4096;
    Resources.maxVaryingFloats                          = 64;
    Resources.maxVertexTextureImageUnits                = 32;
    Resources.maxCombinedTextureImageUnits              = 80;
    Resources.maxTextureImageUnits                      = 32;
    Resources.maxFragmentUniformComponents              = 4096;
    Resources.maxDrawBuffers                            = 32;
    Resources.maxVertexUniformVectors                   = 128;
    Resources.maxVaryingVectors                         = 8;
    Resources.maxFragmentUniformVectors                 = 16;
    Resources.maxVertexOutputVectors                    = 16;
    Resources.maxFragmentInputVectors                   = 15;
    Resources.minProgramTexelOffset                     = -8;
    Resources.maxProgramTexelOffset                     = 7;
    Resources.maxClipDistances                          = 8;
    Resources.maxComputeWorkGroupCountX                 = 65535;
    Resources.maxComputeWorkGroupCountY                 = 65535;
    Resources.maxComputeWorkGroupCountZ                 = 65535;
    Resources.maxComputeWorkGroupSizeX                  = 1024;
    Resources.maxComputeWorkGroupSizeY                  = 1024;
    Resources.maxComputeWorkGroupSizeZ                  = 64;
    Resources.maxComputeUniformComponents               = 1024;
    Resources.maxComputeTextureImageUnits               = 16;
    Resources.maxComputeImageUniforms                   = 8;
    Resources.maxComputeAtomicCounters                  = 8;
    Resources.maxComputeAtomicCounterBuffers            = 1;
    Resources.maxVaryingComponents                      = 60;
    Resources.maxVertexOutputComponents                 = 64;
    Resources.maxGeometryInputComponents                = 64;
    Resources.maxGeometryOutputComponents               = 128;
    Resources.maxFragmentInputComponents                = 128;
    Resources.maxImageUnits                             = 8;
    Resources.maxCombinedImageUnitsAndFragmentOutputs   = 8;
    Resources.maxCombinedShaderOutputResources          = 8;
    Resources.maxImageSamples                           = 0;
    Resources.maxVertexImageUniforms                    = 0;
    Resources.maxTessControlImageUniforms               = 0;
    Resources.maxTessEvaluationImageUniforms            = 0;
    Resources.maxGeometryImageUniforms                  = 0;
    Resources.maxFragmentImageUniforms                  = 8;
    Resources.maxCombinedImageUniforms                  = 8;
    Resources.maxGeometryTextureImageUnits              = 16;
    Resources.maxGeometryOutputVertices                 = 256;
    Resources.maxGeometryTotalOutputComponents          = 1024;
    Resources.maxGeometryUniformComponents              = 1024;
    Resources.maxGeometryVaryingComponents              = 64;
    Resources.maxTessControlInputComponents             = 128;
    Resources.maxTessControlOutputComponents            = 128;
    Resources.maxTessControlTextureImageUnits           = 16;
    Resources.maxTessControlUniformComponents           = 1024;
    Resources.maxTessControlTotalOutputComponents       = 4096;
    Resources.maxTessEvaluationInputComponents          = 128;
    Resources.maxTessEvaluationOutputComponents         = 128;
    Resources.maxTessEvaluationTextureImageUnits        = 16;
    Resources.maxTessEvaluationUniformComponents        = 1024;
    Resources.maxTessPatchComponents                    = 120;
    Resources.maxPatchVertices                          = 32;
    Resources.maxTessGenLevel                           = 64;
    Resources.maxViewports                              = 16;
    Resources.maxVertexAtomicCounters                   = 0;
    Resources.maxTessControlAtomicCounters              = 0;
    Resources.maxTessEvaluationAtomicCounters           = 0;
    Resources.maxGeometryAtomicCounters                 = 0;
    Resources.maxFragmentAtomicCounters                 = 8;
    Resources.maxCombinedAtomicCounters                 = 8;
    Resources.maxAtomicCounterBindings                  = 1;
    Resources.maxVertexAtomicCounterBuffers             = 0;
    Resources.maxTessControlAtomicCounterBuffers        = 0;
    Resources.maxTessEvaluationAtomicCounterBuffers     = 0;
    Resources.maxGeometryAtomicCounterBuffers           = 0;
    Resources.maxFragmentAtomicCounterBuffers           = 1;
    Resources.maxCombinedAtomicCounterBuffers           = 1;
    Resources.maxAtomicCounterBufferSize                = 16384;
    Resources.maxTransformFeedbackBuffers               = 4;
    Resources.maxTransformFeedbackInterleavedComponents = 64;
    Resources.maxCullDistances                          = 8;
    Resources.maxCombinedClipAndCullDistances           = 8;
    Resources.maxSamples                                = 4;
    Resources.maxMeshOutputVerticesNV                   = 256;
    Resources.maxMeshOutputPrimitivesNV                 = 512;
    Resources.maxMeshWorkGroupSizeX_NV                  = 32;
    Resources.maxMeshWorkGroupSizeY_NV                  = 1;
    Resources.maxMeshWorkGroupSizeZ_NV                  = 1;
    Resources.maxTaskWorkGroupSizeX_NV                  = 32;
    Resources.maxTaskWorkGroupSizeY_NV                  = 1;
    Resources.maxTaskWorkGroupSizeZ_NV                  = 1;
    Resources.maxMeshViewCountNV                        = 4;

    Resources.limits.nonInductiveForLoops                 = 1;
    Resources.limits.whileLoops                           = 1;
    Resources.limits.doWhileLoops                         = 1;
    Resources.limits.generalUniformIndexing               = 1;
    Resources.limits.generalAttributeMatrixVectorIndexing = 1;
    Resources.limits.generalVaryingIndexing               = 1;
    Resources.limits.generalSamplerIndexing               = 1;
    Resources.limits.generalVariableIndexing              = 1;
    Resources.limits.generalConstantMatrixVectorIndexing  = 1;

    return Resources;
}

class IoMapResolver final : public ::glslang::TIoMapResolver
{
public:
    // Should return true if the resulting/current binding would be okay.
    // Basic idea is to do aliasing binding checks with this.
    virtual bool validateBinding(EShLanguage stage, ::glslang::TVarEntryInfo& ent) override final
    {
        return true;
    }

    // Should return a value >= 0 if the current binding should be overridden.
    // Return -1 if the current binding (including no binding) should be kept.
    virtual int resolveBinding(EShLanguage stage, ::glslang::TVarEntryInfo& ent) override final
    {
        // We do not care about actual binding value here.
        // We only need decoration to be present in SPIRV
        return 0;
    }

    // Should return a value >= 0 if the current set should be overridden.
    // Return -1 if the current set (including no set) should be kept.
    virtual int resolveSet(EShLanguage stage, ::glslang::TVarEntryInfo& ent) override final
    {
        // We do not care about actual descriptor set value here.
        // We only need decoration to be present in SPIRV
        return 0;
    }

    // Should return a value >= 0 if the current location should be overridden.
    // Return -1 if the current location (including no location) should be kept.
    virtual int resolveUniformLocation(EShLanguage stage, ::glslang::TVarEntryInfo& ent) override final
    {
        return -1;
    }

    // Should return true if the resulting/current setup would be okay.
    // Basic idea is to do aliasing checks and reject invalid semantic names.
    virtual bool validateInOut(EShLanguage stage, ::glslang::TVarEntryInfo& ent) override final
    {
        return true;
    }

    // Should return a value >= 0 if the current location should be overridden.
    // Return -1 if the current location (including no location) should be kept.
    virtual int resolveInOutLocation(EShLanguage stage, ::glslang::TVarEntryInfo& ent) override final
    {
        return -1;
    }

    // Should return a value >= 0 if the current component index should be overridden.
    // Return -1 if the current component index (including no index) should be kept.
    virtual int resolveInOutComponent(EShLanguage stage, ::glslang::TVarEntryInfo& ent) override final
    {
        return -1;
    }

    // Should return a value >= 0 if the current color index should be overridden.
    // Return -1 if the current color index (including no index) should be kept.
    virtual int resolveInOutIndex(EShLanguage stage, ::glslang::TVarEntryInfo& ent) override final
    {
        return -1;
    }

    // Notification of a uniform variable
    virtual void notifyBinding(EShLanguage stage, ::glslang::TVarEntryInfo& ent) override final
    {
    }

    // Notification of a in or out variable
    virtual void notifyInOut(EShLanguage stage, ::glslang::TVarEntryInfo& ent) override final
    {
    }

    // Called by mapIO when it starts its notify pass for the given stage
    virtual void beginNotifications(EShLanguage stage) override final
    {
    }

    // Called by mapIO when it has finished the notify pass
    virtual void endNotifications(EShLanguage stage) override final
    {
    }

    // Called by mipIO when it starts its resolve pass for the given stage
    virtual void beginResolve(EShLanguage stage) override final
    {
    }

    // Called by mapIO when it has finished the resolve pass
    virtual void endResolve(EShLanguage stage) override final
    {
    }

    // Called by mapIO when it starts its symbol collect for teh given stage
    virtual void beginCollect(EShLanguage stage) override final
    {
    }

    // Called by mapIO when it has finished the symbol collect
    virtual void endCollect(EShLanguage stage) override final
    {
    }

    // Called by TSlotCollector to resolve storage locations or bindings
    virtual void reserverStorageSlot(::glslang::TVarEntryInfo& ent, TInfoSink& infoSink) override final
    {
    }

    // Called by TSlotCollector to resolve resource locations or bindings
    virtual void reserverResourceSlot(::glslang::TVarEntryInfo& ent, TInfoSink& infoSink) override final
    {
    }

    // Called by mapIO.addStage to set shader stage mask to mark a stage be added to this pipeline
    virtual void addStage(EShLanguage stage) override final
    {
    }
};

static void LogCompilerError(const char* DebugOutputMessage,
                             const char* InfoLog,
                             const char* InfoDebugLog,
                             const char* ShaderSource,
                             size_t      SourceCodeLen,
                             IDataBlob** ppCompilerOutput)
{
    std::string ErrorLog(InfoLog);
    if (*InfoDebugLog != '\0')
    {
        ErrorLog.push_back('\n');
        ErrorLog.append(InfoDebugLog);
    }
    LOG_ERROR_MESSAGE(DebugOutputMessage, ErrorLog);

    if (ppCompilerOutput != nullptr)
    {
        auto* pOutputDataBlob = MakeNewRCObj<DataBlobImpl>()(SourceCodeLen + 1 + ErrorLog.length() + 1);
        char* DataPtr         = reinterpret_cast<char*>(pOutputDataBlob->GetDataPtr());
        memcpy(DataPtr, ErrorLog.data(), ErrorLog.length() + 1);
        memcpy(DataPtr + ErrorLog.length() + 1, ShaderSource, SourceCodeLen + 1);
        pOutputDataBlob->QueryInterface(IID_DataBlob, reinterpret_cast<IObject**>(ppCompilerOutput));
    }
}

static std::vector<unsigned int> CompileShaderInternal(::glslang::TShader&           Shader,
                                                       EShMessages                   messages,
                                                       ::glslang::TShader::Includer* pIncluder,
                                                       const char*                   ShaderSource,
                                                       size_t                        SourceCodeLen,
                                                       IDataBlob**                   ppCompilerOutput)
{
    Shader.setAutoMapBindings(true);
    TBuiltInResource Resources = InitResources();

    auto ParseResult = pIncluder != nullptr ?
        Shader.parse(&Resources, 100, false, messages, *pIncluder) :
        Shader.parse(&Resources, 100, false, messages);
    if (!ParseResult)
    {
        LogCompilerError("Failed to parse shader source: \n", Shader.getInfoLog(), Shader.getInfoDebugLog(), ShaderSource, SourceCodeLen, ppCompilerOutput);
        return {};
    }

    ::glslang::TProgram Program;
    Program.addShader(&Shader);
    if (!Program.link(messages))
    {
        LogCompilerError("Failed to link program: \n", Program.getInfoLog(), Program.getInfoDebugLog(), ShaderSource, SourceCodeLen, ppCompilerOutput);
        return {};
    }

    IoMapResolver Resovler;
    // This step is essential to set bindings and descriptor sets
    Program.mapIO(&Resovler);

    std::vector<unsigned int> spirv;
    ::glslang::GlslangToSpv(*Program.getIntermediate(Shader.getStage()), spirv);

    return std::move(spirv);
}


class IncluderImpl : public ::glslang::TShader::Includer
{
public:
    IncluderImpl(IShaderSourceInputStreamFactory* pInputStreamFactory) :
        m_pInputStreamFactory(pInputStreamFactory)
    {}

    // For the "system" or <>-style includes; search the "system" paths.
    virtual IncludeResult* includeSystem(const char* headerName,
                                         const char* /*includerName*/,
                                         size_t /*inclusionDepth*/)
    {
        DEV_CHECK_ERR(m_pInputStreamFactory != nullptr, "The shader source conains #include directives, but no input stream factory was provided");
        RefCntAutoPtr<IFileStream> pSourceStream;
        m_pInputStreamFactory->CreateInputStream(headerName, &pSourceStream);
        if (pSourceStream == nullptr)
        {
            LOG_ERROR("Failed to open shader include file '", headerName, "'. Check that the file exists");
            return nullptr;
        }

        RefCntAutoPtr<IDataBlob> pFileData(MakeNewRCObj<DataBlobImpl>()(0));
        pSourceStream->ReadBlob(pFileData);
        auto* pNewInclude =
            new IncludeResult{
                headerName,
                reinterpret_cast<const char*>(pFileData->GetDataPtr()),
                pFileData->GetSize(),
                nullptr};

        m_IncludeRes.emplace(pNewInclude);
        m_DataBlobs.emplace(pNewInclude, std::move(pFileData));
        return pNewInclude;
    }

    // For the "local"-only aspect of a "" include. Should not search in the
    // "system" paths, because on returning a failure, the parser will
    // call includeSystem() to look in the "system" locations.
    virtual IncludeResult* includeLocal(const char* headerName,
                                        const char* includerName,
                                        size_t      inclusionDepth)
    {
        return nullptr;
    }

    // Signals that the parser will no longer use the contents of the
    // specified IncludeResult.
    virtual void releaseInclude(IncludeResult* IncldRes)
    {
        m_DataBlobs.erase(IncldRes);
    }

private:
    IShaderSourceInputStreamFactory* const                       m_pInputStreamFactory;
    std::unordered_set<std::unique_ptr<IncludeResult>>           m_IncludeRes;
    std::unordered_map<IncludeResult*, RefCntAutoPtr<IDataBlob>> m_DataBlobs;
};

std::vector<unsigned int> HLSLtoSPIRV(const ShaderCreateInfo& ShaderCI,
                                      const char*             ExtraDefinitions,
                                      IDataBlob**             ppCompilerOutput)
{
    EShLanguage        ShLang = ShaderTypeToShLanguage(ShaderCI.Desc.ShaderType);
    ::glslang::TShader Shader{ShLang};
    EShMessages        messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules | EShMsgReadHlsl | EShMsgHlslLegalization);

    VERIFY_EXPR(ShaderCI.SourceLanguage == SHADER_SOURCE_LANGUAGE_HLSL);

    Shader.setEnvInput(::glslang::EShSourceHlsl, ShLang, ::glslang::EShClientVulkan, 100);
    Shader.setEnvClient(::glslang::EShClientVulkan, ::glslang::EShTargetVulkan_1_0);
    Shader.setEnvTarget(::glslang::EShTargetSpv, ::glslang::EShTargetSpv_1_0);
    Shader.setHlslIoMapping(true);
    Shader.setEntryPoint(ShaderCI.EntryPoint);
    Shader.setEnvTargetHlslFunctionality1();

    RefCntAutoPtr<IDataBlob> pFileData;
    size_t                   SourceCodeLen = 0;

    const char* SourceCode = ReadShaderSourceFile(ShaderCI.Source, ShaderCI.pShaderSourceStreamFactory, ShaderCI.FilePath, pFileData, SourceCodeLen);

    std::string Defines = g_HLSLDefinitions;
    AppendShaderTypeDefinitions(Defines, ShaderCI.Desc.ShaderType);

    if (ExtraDefinitions != nullptr)
        Defines += ExtraDefinitions;

    if (ShaderCI.Macros != nullptr)
    {
        Defines += '\n';
        AppendShaderMacros(Defines, ShaderCI.Macros);
    }
    Shader.setPreamble(Defines.c_str());

    const char* ShaderStrings[]       = {SourceCode};
    const int   ShaderStringLenghts[] = {static_cast<int>(SourceCodeLen)};
    const char* Names[]               = {ShaderCI.FilePath != nullptr ? ShaderCI.FilePath : ""};
    Shader.setStringsWithLengthsAndNames(ShaderStrings, ShaderStringLenghts, Names, 1);

    IncluderImpl Includer{ShaderCI.pShaderSourceStreamFactory};

    auto SPIRV = CompileShaderInternal(Shader, messages, &Includer, SourceCode, SourceCodeLen, ppCompilerOutput);
    if (SPIRV.empty())
        return SPIRV;

    // SPIR-V bytecode generated from HLSL must be legalized to
    // turn it into a valid vulkan SPIR-V shader
    spvtools::Optimizer SpirvOptimizer(SPV_ENV_VULKAN_1_0);
    SpirvOptimizer.RegisterLegalizationPasses();
    SpirvOptimizer.RegisterPerformancePasses();
    std::vector<uint32_t> LegalizedSPIRV;
    if (SpirvOptimizer.Run(SPIRV.data(), SPIRV.size(), &LegalizedSPIRV))
    {
        return std::move(LegalizedSPIRV);
    }
    else
    {
        LOG_ERROR("Failed to legalize SPIR-V shader generated by HLSL front-end. This may result in undefined behavior.");
        return std::move(SPIRV);
    }
}

std::vector<unsigned int> GLSLtoSPIRV(SHADER_TYPE                      ShaderType,
                                      const char*                      ShaderSource,
                                      int                              SourceCodeLen,
                                      const ShaderMacro*               Macros,
                                      IShaderSourceInputStreamFactory* pShaderSourceStreamFactory,
                                      IDataBlob**                      ppCompilerOutput)
{
    VERIFY_EXPR(ShaderSource != nullptr && SourceCodeLen > 0);

    EShLanguage        ShLang = ShaderTypeToShLanguage(ShaderType);
    ::glslang::TShader Shader(ShLang);

    EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

    const char* ShaderStrings[] = {ShaderSource};
    int         Lenghts[]       = {SourceCodeLen};
    Shader.setStringsWithLengths(ShaderStrings, Lenghts, 1);

    std::string Defines;
    if (Macros != nullptr)
    {
        AppendShaderMacros(Defines, Macros);
        Shader.setPreamble(Defines.c_str());
    }

    IncluderImpl Includer{pShaderSourceStreamFactory};

    auto SPIRV = CompileShaderInternal(Shader, messages, &Includer, ShaderSource, SourceCodeLen, ppCompilerOutput);
    if (SPIRV.empty())
        return SPIRV;

    spvtools::Optimizer SpirvOptimizer(SPV_ENV_VULKAN_1_0);
    SpirvOptimizer.RegisterPerformancePasses();
    std::vector<uint32_t> OptimizedSPIRV;
    if (SpirvOptimizer.Run(SPIRV.data(), SPIRV.size(), &OptimizedSPIRV))
    {
        return std::move(OptimizedSPIRV);
    }
    else
    {
        LOG_ERROR("Failed to optimize SPIR-V.");
        return std::move(SPIRV);
    }
}

} // namespace GLSLangUtils

} // namespace Diligent
