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

#include "HLSLUtils.hpp"
#include "DebugUtilities.hpp"
#include "ShaderToolsCommon.hpp"

namespace Diligent
{

// clang-format off
static const char g_HLSLDefinitions[] =
{
#include "../../GraphicsEngineD3DBase/include/HLSLDefinitions_inc.fxh"
};
// clang-format on


String BuildHLSLSourceString(const ShaderCreateInfo& ShaderCI,
                             const char*             ExtraDefinitions)
{
    String HLSLSource;

    HLSLSource.append(g_HLSLDefinitions);
    AppendShaderTypeDefinitions(HLSLSource, ShaderCI.Desc.ShaderType);

    if (ExtraDefinitions != nullptr)
        HLSLSource += ExtraDefinitions;

    if (ShaderCI.Macros != nullptr)
    {
        HLSLSource += '\n';
        AppendShaderMacros(HLSLSource, ShaderCI.Macros);
    }

    AppendShaderSourceCode(HLSLSource, ShaderCI);

    return HLSLSource;
}

String GetHLSLProfileString(SHADER_TYPE ShaderType, ShaderVersion ShaderModel)
{
    String strShaderProfile;
    switch (ShaderType)
    {
        // clang-format off
        case SHADER_TYPE_VERTEX:        strShaderProfile = "vs"; break;
        case SHADER_TYPE_PIXEL:         strShaderProfile = "ps"; break;
        case SHADER_TYPE_GEOMETRY:      strShaderProfile = "gs"; break;
        case SHADER_TYPE_HULL:          strShaderProfile = "hs"; break;
        case SHADER_TYPE_DOMAIN:        strShaderProfile = "ds"; break;
        case SHADER_TYPE_COMPUTE:       strShaderProfile = "cs"; break;
        case SHADER_TYPE_AMPLIFICATION: strShaderProfile = "as"; break;
        case SHADER_TYPE_MESH:          strShaderProfile = "ms"; break;
        // clang-format on
        default: UNEXPECTED("Unknown shader type");
    }

    strShaderProfile += "_";
    strShaderProfile += std::to_string(ShaderModel.Major);
    strShaderProfile += "_";
    strShaderProfile += std::to_string(ShaderModel.Minor);

    return strShaderProfile;
}

} // namespace Diligent
