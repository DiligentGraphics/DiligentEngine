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
#include "DefaultShaderSourceStreamFactory.h"
#include "ObjectBase.hpp"
#include "RefCntAutoPtr.hpp"
#include "EngineMemory.h"
#include "BasicFileStream.hpp"

namespace Diligent
{

class DefaultShaderSourceStreamFactory final : public ObjectBase<IShaderSourceInputStreamFactory>
{
public:
    DefaultShaderSourceStreamFactory(IReferenceCounters* pRefCounters, const Char* SearchDirectories);

    virtual void DILIGENT_CALL_TYPE CreateInputStream(const Char* Name, IFileStream** ppStream) override final;

    virtual void DILIGENT_CALL_TYPE CreateInputStream2(const Char*                             Name,
                                                       CREATE_SHADER_SOURCE_INPUT_STREAM_FLAGS Flags,
                                                       IFileStream**                           ppStream) override final;

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_IShaderSourceInputStreamFactory, ObjectBase<IShaderSourceInputStreamFactory>);

private:
    std::vector<String> m_SearchDirectories;
};

DefaultShaderSourceStreamFactory::DefaultShaderSourceStreamFactory(IReferenceCounters* pRefCounters, const Char* SearchDirectories) :
    ObjectBase<IShaderSourceInputStreamFactory>(pRefCounters)
{
    while (SearchDirectories)
    {
        const char* Semicolon = strchr(SearchDirectories, ';');
        String      SearchPath;
        if (Semicolon == nullptr)
        {
            SearchPath        = SearchDirectories;
            SearchDirectories = nullptr;
        }
        else
        {
            SearchPath        = String(SearchDirectories, Semicolon);
            SearchDirectories = Semicolon + 1;
        }

        if (SearchPath.length() > 0)
        {
            if (SearchPath.back() != '\\' && SearchPath.back() != '/')
                SearchPath.push_back('\\');
            m_SearchDirectories.push_back(SearchPath);
        }
    }
    m_SearchDirectories.push_back("");
}

void DefaultShaderSourceStreamFactory::CreateInputStream(const Char*   Name,
                                                         IFileStream** ppStream)
{
    CreateInputStream2(Name, CREATE_SHADER_SOURCE_INPUT_STREAM_FLAG_NONE, ppStream);
}

void DefaultShaderSourceStreamFactory::CreateInputStream2(const Char*                             Name,
                                                          CREATE_SHADER_SOURCE_INPUT_STREAM_FLAGS Flags,
                                                          IFileStream**                           ppStream)
{
    bool                                     bFileCreated = false;
    Diligent::RefCntAutoPtr<BasicFileStream> pBasicFileStream;
    for (const auto& SearchDir : m_SearchDirectories)
    {
        String FullPath = SearchDir + ((Name[0] == '\\' || Name[0] == '/') ? Name + 1 : Name);
        if (!FileSystem::FileExists(FullPath.c_str()))
            continue;
        pBasicFileStream = MakeNewRCObj<BasicFileStream>()(FullPath.c_str(), EFileAccessMode::Read);
        if (pBasicFileStream->IsValid())
        {
            bFileCreated = true;
            break;
        }
        else
        {
            pBasicFileStream.Release();
        }
    }
    if (bFileCreated)
    {
        pBasicFileStream->QueryInterface(IID_FileStream, reinterpret_cast<IObject**>(ppStream));
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

void CreateDefaultShaderSourceStreamFactory(const Char*                       SearchDirectories,
                                            IShaderSourceInputStreamFactory** ppShaderSourceStreamFactory)
{

    auto&                             Allocator = GetRawAllocator();
    DefaultShaderSourceStreamFactory* pStreamFactory =
        NEW_RC_OBJ(Allocator, "DefaultShaderSourceStreamFactory instance", DefaultShaderSourceStreamFactory)(SearchDirectories);
    pStreamFactory->QueryInterface(IID_IShaderSourceInputStreamFactory, reinterpret_cast<IObject**>(ppShaderSourceStreamFactory));
}

} // namespace Diligent
