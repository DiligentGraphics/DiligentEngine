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

/// \file
/// Definition of the Diligent::IShaderVk interface

#include "../../../Primitives/interface/CommonDefinitions.h"
#if DILIGENT_CPP_INTERFACE
#    include <vector>
#endif

#include "../../GraphicsEngine/interface/Shader.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {8B0C91B4-B1D8-4E03-9250-A70E131A59FA}
static const INTERFACE_ID IID_ShaderVk =
    {0x8b0c91b4, 0xb1d8, 0x4e03, {0x92, 0x50, 0xa7, 0xe, 0x13, 0x1a, 0x59, 0xfa}};

#define IShaderVkInclusiveMethods \
    IShaderInclusiveMethods;      \
    IShaderVkMethods ShaderVk

#if DILIGENT_CPP_INTERFACE

/// Exposes Vulkan-specific functionality of a shader object.
class IShaderVk : public IShader
{
public:
    /// Returns SPIRV bytecode
    virtual const std::vector<uint32_t>& DILIGENT_CALL_TYPE GetSPIRV() const = 0;
};

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
