/*
 *  Copyright 2019-2020 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF ANY PROPRIETARY RIGHTS.
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
/// Definition of the Diligent::IShaderResourceBindingMtl interface and related data structures

#include "../../GraphicsEngine/interface/ShaderResourceBinding.h"

namespace Diligent
{

// {4D7AF38E-4650-4A11-B33B-FFC70A8CD68B}
static const INTERFACE_ID IID_ShaderResourceBindingMtl =
    {0x4d7af38e, 0x4650, 0x4a11, {0xb3, 0x3b, 0xff, 0xc7, 0xa, 0x8c, 0xd6, 0x8b}};

/// Exposes Metal-specific functionality of a shader resource binding object.
class IShaderResourceBindingMtl : public IShaderResourceBinding
{
public:
};

} // namespace Diligent
