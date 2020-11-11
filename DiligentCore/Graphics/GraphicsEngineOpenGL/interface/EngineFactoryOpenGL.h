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
/// Declaration of functions that create OpenGL-based engine implementation

#include "../../GraphicsEngine/interface/EngineFactory.h"
#include "../../GraphicsEngine/interface/RenderDevice.h"
#include "../../GraphicsEngine/interface/DeviceContext.h"
#include "../../GraphicsEngine/interface/SwapChain.h"

#include "../../HLSL2GLSLConverterLib/interface/HLSL2GLSLConverter.h"


#if PLATFORM_ANDROID || PLATFORM_LINUX || PLATFORM_MACOS || PLATFORM_IOS || (PLATFORM_WIN32 && !defined(_MSC_VER))
// https://gcc.gnu.org/wiki/Visibility
#    define API_QUALIFIER __attribute__((visibility("default")))
#elif PLATFORM_WIN32
#    define API_QUALIFIER
#else
#    error Unsupported platform
#endif

#if ENGINE_DLL && PLATFORM_WIN32 && defined(_MSC_VER)
#    include "../../GraphicsEngine/interface/LoadEngineDll.h"
#    define EXPLICITLY_LOAD_ENGINE_GL_DLL 1
#endif

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {9BAAC767-02CC-4FFA-9E4B-E1340F572C49}
static const INTERFACE_ID IID_EngineFactoryOpenGL =
    {0x9baac767, 0x2cc, 0x4ffa, {0x9e, 0x4b, 0xe1, 0x34, 0xf, 0x57, 0x2c, 0x49}};

#define DILIGENT_INTERFACE_NAME IEngineFactoryOpenGL
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IEngineFactoryOpenGLInclusiveMethods \
    IEngineFactoryInclusiveMethods;          \
    IEngineFactoryOpenGLMethods EngineFactoryOpenGL

// clang-format off

DILIGENT_BEGIN_INTERFACE(IEngineFactoryOpenGL, IEngineFactory)
{
    VIRTUAL void METHOD(CreateDeviceAndSwapChainGL)(THIS_
                                                    const EngineGLCreateInfo REF EngineCI,
                                                    IRenderDevice**              ppDevice,
                                                    IDeviceContext**             ppImmediateContext,
                                                    const SwapChainDesc REF      SCDesc,
                                                    ISwapChain**                 ppSwapChain) PURE;

    VIRTUAL void METHOD(CreateHLSL2GLSLConverter)(THIS_
                                                  IHLSL2GLSLConverter** ppConverter) PURE;

    VIRTUAL void METHOD(AttachToActiveGLContext)(THIS_
                                                 const EngineGLCreateInfo REF EngineCI,
                                                 IRenderDevice**              ppDevice,
                                                 IDeviceContext**             ppImmediateContext) PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define IEngineFactoryOpenGL_CreateDeviceAndSwapChainGL(This, ...) CALL_IFACE_METHOD(EngineFactoryOpenGL, CreateDeviceAndSwapChainGL, This, __VA_ARGS__)
#    define IEngineFactoryOpenGL_CreateHLSL2GLSLConverter(This, ...)   CALL_IFACE_METHOD(EngineFactoryOpenGL, CreateHLSL2GLSLConverter,   This, __VA_ARGS__)
#    define IEngineFactoryOpenGL_AttachToActiveGLContext(This, ...)    CALL_IFACE_METHOD(EngineFactoryOpenGL, AttachToActiveGLContext,    This, __VA_ARGS__)

// clang-format on

#endif


#if EXPLICITLY_LOAD_ENGINE_GL_DLL

typedef struct IEngineFactoryOpenGL* (*GetEngineFactoryOpenGLType)();

inline GetEngineFactoryOpenGLType LoadGraphicsEngineOpenGL()
{
    return (GetEngineFactoryOpenGLType)LoadEngineDll("GraphicsEngineOpenGL", "GetEngineFactoryOpenGL");
}

#else

// Do not forget to call System.loadLibrary("GraphicsEngineOpenGL") in Java on Android!
API_QUALIFIER
struct IEngineFactoryOpenGL* DILIGENT_GLOBAL_FUNCTION(GetEngineFactoryOpenGL)();

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
