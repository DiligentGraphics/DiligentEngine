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
/// Defines printers for Diligent Engine types

#include <iostream>

#include "GraphicsAccessories.hpp"

namespace Diligent
{

#define DEFINE_TYPE_PRINTER(Type, PrintFunction, ...)                  \
    inline std::ostream& operator<<(std::ostream& os, const Type& Obj) \
    {                                                                  \
        return os << PrintFunction(Obj, ##__VA_ARGS__);                \
    }

DEFINE_TYPE_PRINTER(VALUE_TYPE, GetValueTypeString)
DEFINE_TYPE_PRINTER(TEXTURE_VIEW_TYPE, GetTexViewTypeLiteralName)
DEFINE_TYPE_PRINTER(BUFFER_VIEW_TYPE, GetBufferViewTypeLiteralName)
DEFINE_TYPE_PRINTER(SHADER_TYPE, GetShaderTypeLiteralName)
DEFINE_TYPE_PRINTER(SHADER_RESOURCE_VARIABLE_TYPE, GetShaderVariableTypeLiteralName, true)
DEFINE_TYPE_PRINTER(SHADER_RESOURCE_TYPE, GetShaderResourceTypeLiteralName, true)
DEFINE_TYPE_PRINTER(FILTER_TYPE, GetFilterTypeLiteralName, true)
DEFINE_TYPE_PRINTER(TEXTURE_ADDRESS_MODE, GetTextureAddressModeLiteralName, true)
DEFINE_TYPE_PRINTER(COMPARISON_FUNCTION, GetComparisonFunctionLiteralName, true)
DEFINE_TYPE_PRINTER(STENCIL_OP, GetStencilOpLiteralName)
DEFINE_TYPE_PRINTER(BLEND_FACTOR, GetBlendFactorLiteralName)
DEFINE_TYPE_PRINTER(BLEND_OPERATION, GetBlendOperationLiteralName)
DEFINE_TYPE_PRINTER(FILL_MODE, GetFillModeLiteralName)
DEFINE_TYPE_PRINTER(CULL_MODE, GetCullModeLiteralName)
DEFINE_TYPE_PRINTER(MAP_TYPE, GetMapTypeString)
DEFINE_TYPE_PRINTER(USAGE, GetUsageString)
DEFINE_TYPE_PRINTER(RESOURCE_DIMENSION, GetResourceDimString)
DEFINE_TYPE_PRINTER(TextureDesc, GetTextureDescString)
DEFINE_TYPE_PRINTER(BufferFormat, GetBufferFormatString)
DEFINE_TYPE_PRINTER(BUFFER_MODE, GetBufferModeString)
DEFINE_TYPE_PRINTER(BufferDesc, GetBufferDescString)
DEFINE_TYPE_PRINTER(RESOURCE_STATE, GetResourceStateFlagString)
#undef DEFINE_TYPE_PRINTER

} // namespace Diligent
