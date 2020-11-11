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

#pragma once

#include <sstream>

#include "BasicTypes.h"
#include "GraphicsTypes.h"
#include "Shader.h"
#include "RefCountedObjectImpl.hpp"
#include "Errors.hpp"
#include "DataBlobImpl.hpp"

namespace Diligent
{

String BuildHLSLSourceString(const ShaderCreateInfo& ShaderCI,
                             const char*             ExtraDefinitions = nullptr);

String GetHLSLProfileString(SHADER_TYPE ShaderType, ShaderVersion ShaderModel);

template <typename BlobType>
void HandleHLSLCompilerResult(bool               CompilationSucceeded,
                              BlobType*          pCompilerMsgBlob,
                              const std::string& ShaderSource,
                              const char*        ShaderName,
                              IDataBlob**        ppOutputLog) noexcept(false)
{
    const char*  CompilerMsg    = pCompilerMsgBlob ? static_cast<const char*>(pCompilerMsgBlob->GetBufferPointer()) : nullptr;
    const size_t CompilerMsgLen = CompilerMsg ? pCompilerMsgBlob->GetBufferSize() : 0;

    if (ppOutputLog != nullptr)
    {
        const auto ShaderSourceLen = ShaderSource.length();
        auto*      pOutputLogBlob  = MakeNewRCObj<DataBlobImpl>{}(ShaderSourceLen + 1 + CompilerMsgLen + 1);

        auto* log = static_cast<char*>(pOutputLogBlob->GetDataPtr());

        if (CompilerMsg != nullptr)
            memcpy(log, CompilerMsg, CompilerMsgLen);
        log[CompilerMsgLen] = 0; // Explicitly set null terminator
        log += CompilerMsgLen + 1;

        memcpy(log, ShaderSource.data(), ShaderSourceLen);
        log[ShaderSourceLen] = 0;

        pOutputLogBlob->QueryInterface(IID_DataBlob, reinterpret_cast<IObject**>(ppOutputLog));
    }

    if (!CompilationSucceeded || CompilerMsgLen != 0)
    {
        std::stringstream ss;
        ss << (CompilationSucceeded ? "Compiler output for shader '" : "Failed to compile shader '")
           << (ShaderName != nullptr ? ShaderName : "<unknown>")
           << "'";
        if (CompilerMsg != nullptr && CompilerMsgLen != 0)
        {
            ss << ":" << std::endl
               << std::string{CompilerMsg, CompilerMsgLen};
        }
        else if (!CompilationSucceeded)
        {
            ss << " (no shader log available).";
        }

        if (CompilationSucceeded)
            LOG_INFO_MESSAGE(ss.str());
        else
            LOG_ERROR_AND_THROW(ss.str());
    }
}

} // namespace Diligent
