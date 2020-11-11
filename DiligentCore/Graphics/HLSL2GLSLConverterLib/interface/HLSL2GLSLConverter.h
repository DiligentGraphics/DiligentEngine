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
/// Definition of the Diligent::IHLSL2GLSLConverter interface

#include "../../GraphicsEngine/interface/Shader.h"
#include "../../../Primitives/interface/DataBlob.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {1FDE020A-9C73-4A76-8AEF-C2C6C2CF0EA5}
static const INTERFACE_ID IID_HLSL2GLSLConversionStream =
    {0x1fde020a, 0x9c73, 0x4a76, {0x8a, 0xef, 0xc2, 0xc6, 0xc2, 0xcf, 0xe, 0xa5}};


#define DILIGENT_INTERFACE_NAME IHLSL2GLSLConversionStream
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IHLSL2GLSLConversionStreamInclusiveMethods \
    IObjectInclusiveMethods;                       \
    IHLSL2GLSLConversionStreamMethods HLSL2GLSLConversionStream

// clang-format off

DILIGENT_BEGIN_INTERFACE(IHLSL2GLSLConversionStream, IObject)
{
    VIRTUAL void METHOD(Convert)(THIS_
                                 const Char* EntryPoint,
                                 SHADER_TYPE ShaderType,
                                 bool        IncludeDefintions,
                                 const char* SamplerSuffix,
                                 bool        UseInOutLocationQualifiers,
                                 IDataBlob** ppGLSLSource) PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define IHLSL2GLSLConversionStream_Convert(This, ...) CALL_IFACE_METHOD(HLSL2GLSLConversionStream, Convert, This, __VA_ARGS__)

// clang-format on

#endif


// {44A21160-77E0-4DDC-A57E-B8B8B65B5342}
static const INTERFACE_ID IID_HLSL2GLSLConverter =
    {0x44a21160, 0x77e0, 0x4ddc, {0xa5, 0x7e, 0xb8, 0xb8, 0xb6, 0x5b, 0x53, 0x42}};

#define DILIGENT_INTERFACE_NAME IHLSL2GLSLConverter
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IHLSL2GLSLConverterInclusiveMethods \
    IObjectInclusiveMethods;                \
    IHLSL2GLSLConverterMethods HLSL2GLSLConverter

// clang-format off

/// Interface to the buffer object implemented in OpenGL
DILIGENT_BEGIN_INTERFACE(IHLSL2GLSLConverter, IObject)
{
    VIRTUAL void METHOD(CreateStream)(THIS_
                                      const Char*                      InputFileName,
                                      IShaderSourceInputStreamFactory* pSourceStreamFactory,
                                      const Char*                      HLSLSource,
                                      size_t                           NumSymbols,
                                      IHLSL2GLSLConversionStream**     ppStream) CONST PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define IHLSL2GLSLConverter_CreateStream(This, ...) CALL_IFACE_METHOD(HLSL2GLSLConverter, CreateStream, This, __VA_ARGS__)

// clang-format on

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
