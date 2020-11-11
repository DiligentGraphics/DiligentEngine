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
/// Definition of the Diligent::IBufferViewGL interface

#include "../../GraphicsEngine/interface/BufferView.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {927A865B-3CEB-4743-9A22-2A1397A73E6D}
static const INTERFACE_ID IID_BufferViewGL =
    {0x927a865b, 0x3ceb, 0x4743, {0x9a, 0x22, 0x2a, 0x13, 0x97, 0xa7, 0x3e, 0x6d}};

#define DILIGENT_INTERFACE_NAME IBufferViewGL
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IBufferViewGLInclusiveMethods \
    IBufferViewInclusiveMethods
//IBufferViewGLMethods BufferViewGL

#if DILIGENT_CPP_INTERFACE

/// Exposes OpenGL-specific functionality of a buffer view object.
DILIGENT_BEGIN_INTERFACE(IBufferViewGL, IBufferView){};
DILIGENT_END_INTERFACE

#endif

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

typedef struct IBufferViewGLVtbl
{
    IBufferViewGLInclusiveMethods;
} IBufferViewGLVtbl;

typedef struct IBufferViewGL
{
    struct IBufferViewGLVtbl* pVtbl;
} IBufferViewGL;

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
