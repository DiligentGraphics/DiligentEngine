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

#include "../include/DiligentFXShaderSourceStreamFactory.hpp"
#include "MemoryFileStream.hpp"
#include "StringDataBlobImpl.hpp"
#include "RefCntAutoPtr.hpp"
#include "../../../shaders_inc/shaders_list.h"

namespace Diligent
{

DiligentFXShaderSourceStreamFactory& DiligentFXShaderSourceStreamFactory::GetInstance()
{
    static DiligentFXShaderSourceStreamFactory TheFactory;
    return TheFactory;
}

DiligentFXShaderSourceStreamFactory::DiligentFXShaderSourceStreamFactory()
{
    for (size_t i = 0; i < _countof(g_Shaders); ++i)
    {
        m_NameToSourceMap.emplace(g_Shaders[i].FileName, g_Shaders[i].Source);
    }
}

void DiligentFXShaderSourceStreamFactory::CreateInputStream(const Char*   Name,
                                                            IFileStream** ppStream)
{
    CreateInputStream2(Name, CREATE_SHADER_SOURCE_INPUT_STREAM_FLAG_NONE, ppStream);
}

void DiligentFXShaderSourceStreamFactory::CreateInputStream2(const Char*                             Name,
                                                             CREATE_SHADER_SOURCE_INPUT_STREAM_FLAGS Flags,
                                                             IFileStream**                           ppStream)
{
    auto SourceIt = m_NameToSourceMap.find(Name);
    if (SourceIt != m_NameToSourceMap.end())
    {
        RefCntAutoPtr<StringDataBlobImpl> pDataBlob(MakeNewRCObj<StringDataBlobImpl>()(SourceIt->second));
        RefCntAutoPtr<MemoryFileStream>   pMemStream(MakeNewRCObj<MemoryFileStream>()(pDataBlob));

        pMemStream->QueryInterface(IID_FileStream, reinterpret_cast<IObject**>(ppStream));
    }
    else
    {
        *ppStream = nullptr;
        if ((Flags & CREATE_SHADER_SOURCE_INPUT_STREAM_FLAG_SILENT) == 0)
        {
            LOG_ERROR("Failed to create input stream for source file ", Name);
        }
    }
}

} // namespace Diligent
