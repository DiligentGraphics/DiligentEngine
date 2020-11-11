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

#include "ShaderToolsCommon.hpp"
#include "DebugUtilities.hpp"
#include "DataBlobImpl.hpp"

namespace Diligent
{

namespace
{

const ShaderMacro VSMacros[] = {{"VERTEX_SHADER", "1"}, {}};
const ShaderMacro PSMacros[] = {{"FRAGMENT_SHADER", "1"}, {"PIXEL_SHADER", "1"}, {}};
const ShaderMacro GSMacros[] = {{"GEOMETRY_SHADER", "1"}, {}};
const ShaderMacro HSMacros[] = {{"TESS_CONTROL_SHADER", "1"}, {"HULL_SHADER", "1"}, {}};
const ShaderMacro DSMacros[] = {{"TESS_EVALUATION_SHADER", "1"}, {"DOMAIN_SHADER", "1"}, {}};
const ShaderMacro CSMacros[] = {{"COMPUTE_SHADER", "1"}, {}};
const ShaderMacro ASMacros[] = {{"TASK_SHADER", "1"}, {"AMPLIFICATION_SHADER", "1"}, {}};
const ShaderMacro MSMacros[] = {{"MESH_SHADER", "1"}, {}};

} // namespace

const ShaderMacro* GetShaderTypeMacros(SHADER_TYPE Type)
{
    switch (Type)
    {
        // clang-format off
        case SHADER_TYPE_VERTEX:        return VSMacros;
        case SHADER_TYPE_PIXEL:         return PSMacros;
        case SHADER_TYPE_GEOMETRY:      return GSMacros;
        case SHADER_TYPE_HULL:          return HSMacros;
        case SHADER_TYPE_DOMAIN:        return DSMacros;
        case SHADER_TYPE_COMPUTE:       return CSMacros;
        case SHADER_TYPE_AMPLIFICATION: return ASMacros;
        case SHADER_TYPE_MESH:          return MSMacros;
        // clang-format on
        default:
            UNEXPECTED("Unexpected shader type");
            return nullptr;
    }
}

void AppendShaderMacros(std::string& Source, const ShaderMacro* Macros)
{
    if (Macros == nullptr)
        return;

    for (auto* pMacro = Macros; pMacro->Name != nullptr && pMacro->Definition != nullptr; ++pMacro)
    {
        Source += "#define ";
        Source += pMacro->Name;
        Source += ' ';
        Source += pMacro->Definition;
        Source += "\n";
    }
}

void AppendShaderTypeDefinitions(std::string& Source, SHADER_TYPE Type)
{
    AppendShaderMacros(Source, GetShaderTypeMacros(Type));
}


const char* ReadShaderSourceFile(const char*                      SourceCode,
                                 IShaderSourceInputStreamFactory* pShaderSourceStreamFactory,
                                 const char*                      FilePath,
                                 RefCntAutoPtr<IDataBlob>&        pFileData,
                                 size_t&                          SourceCodeLen) noexcept(false)
{
    SourceCodeLen = 0;
    if (SourceCode != nullptr)
    {
        VERIFY(pShaderSourceStreamFactory == nullptr, "pShaderSourceStreamFactory must be null when SourceCode is not null");
        SourceCodeLen = strlen(SourceCode);
    }
    else
    {
        if (pShaderSourceStreamFactory != nullptr)
        {
            if (FilePath != nullptr)
            {
                RefCntAutoPtr<IFileStream> pSourceStream;
                pShaderSourceStreamFactory->CreateInputStream(FilePath, &pSourceStream);
                if (pSourceStream == nullptr)
                    LOG_ERROR_AND_THROW("Failed to load shader source file '", FilePath, '\'');

                pFileData = MakeNewRCObj<DataBlobImpl>{}(0);
                pSourceStream->ReadBlob(pFileData);
                SourceCode    = reinterpret_cast<char*>(pFileData->GetDataPtr());
                SourceCodeLen = pFileData->GetSize();
            }
            else
            {
                UNEXPECTED("FilePath is null");
            }
        }
        else
        {
            UNEXPECTED("Input stream factory is null");
        }
    }

    return SourceCode;
}

void AppendShaderSourceCode(std::string& Source, const ShaderCreateInfo& ShaderCI) noexcept(false)
{
    RefCntAutoPtr<IDataBlob> pFileData;

    size_t SourceCodeLen = 0;

    const auto* SourceCode =
        ReadShaderSourceFile(ShaderCI.Source, ShaderCI.pShaderSourceStreamFactory,
                             ShaderCI.FilePath, pFileData, SourceCodeLen);
    Source.append(SourceCode, SourceCodeLen);
}

} // namespace Diligent
