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
/// Definition of the Diligent::ISamplerGL interface

#include "../../GraphicsEngine/interface/Sampler.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {3E9EB89E-E955-447A-9D13-92C10541F727}
static const INTERFACE_ID IID_SamplerGL =
    {0x3e9eb89e, 0xe955, 0x447a, {0x9d, 0x13, 0x92, 0xc1, 0x5, 0x41, 0xf7, 0x27}};

#define DILIGENT_INTERFACE_NAME ISamplerGL
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define ISamplerGLInclusiveMethods \
    ISamplerInclusiveMethods
//ISamplerGLMethods SamplerGL

#if DILIGENT_CPP_INTERFACE

/// Exposes OpenGL-specific functionality of a sampler object.
DILIGENT_BEGIN_INTERFACE(ISamplerGL, ISampler){};
DILIGENT_END_INTERFACE

#endif

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

typedef struct ISamplerGLVtbl
{
    ISamplerGLInclusiveMethods;
} ISamplerGLVtbl;

typedef struct ISamplerGL
{
    struct ISamplerGLVtbl* pVtbl;
} ISamplerGL;

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
