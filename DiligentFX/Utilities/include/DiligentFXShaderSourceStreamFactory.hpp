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

#include <unordered_map>
#include "BasicFileStream.hpp"
#include "Shader.h"
#include "HashUtils.hpp"

namespace Diligent
{

class DiligentFXShaderSourceStreamFactory final : public IShaderSourceInputStreamFactory
{
public:
    static DiligentFXShaderSourceStreamFactory& GetInstance();

    virtual void DILIGENT_CALL_TYPE CreateInputStream(const Char* Name, IFileStream** ppStream) override final;

    virtual void DILIGENT_CALL_TYPE CreateInputStream2(const Char*                             Name,
                                                       CREATE_SHADER_SOURCE_INPUT_STREAM_FLAGS Flags,
                                                       IFileStream**                           ppStream) override final;

    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override final
    {
        UNSUPPORTED("This method is not implemented and should never be called");
    }

    virtual ReferenceCounterValueType DILIGENT_CALL_TYPE AddRef() override final
    {
        UNSUPPORTED("This method is not implemented and should never be called");
        return 0;
    }

    virtual ReferenceCounterValueType DILIGENT_CALL_TYPE Release() override final
    {
        UNSUPPORTED("This method is not implemented and should never be called");
        return 0;
    }

    virtual IReferenceCounters* DILIGENT_CALL_TYPE GetReferenceCounters() const override final
    {
        UNSUPPORTED("This method is not implemented and should never be called");
        return nullptr;
    }

private:
    DiligentFXShaderSourceStreamFactory();
    std::unordered_map<HashMapStringKey, const Char*, HashMapStringKey::Hasher> m_NameToSourceMap;
};

} // namespace Diligent
