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
/// Defines Diligent::IEngineFactory interface

#include "../../../Primitives/interface/Object.h"
#include "APIInfo.h"


#if PLATFORM_ANDROID
struct ANativeActivity;
struct AAssetManager;
#endif

DILIGENT_BEGIN_NAMESPACE(Diligent)

struct IShaderSourceInputStreamFactory;

// {D932B052-4ED6-4729-A532-F31DEEC100F3}
static const INTERFACE_ID IID_EngineFactory =
    {0xd932b052, 0x4ed6, 0x4729, {0xa5, 0x32, 0xf3, 0x1d, 0xee, 0xc1, 0x0, 0xf3}};

#define DILIGENT_INTERFACE_NAME IEngineFactory
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IEngineFactoryInclusiveMethods \
    IObjectInclusiveMethods;           \
    IEngineFactoryMethods EngineFactory

// clang-format off

/// Engine factory base interface
DILIGENT_BEGIN_INTERFACE(IEngineFactory, IObject)
{
    /// Returns API info structure
    VIRTUAL const APIInfo REF METHOD(GetAPIInfo)(THIS) CONST PURE;

    /// Creates default shader source input stream factory
    /// \param [in]  SearchDirectories           - Semicolon-seprated list of search directories.
    /// \param [out] ppShaderSourceStreamFactory - Memory address where pointer to the shader source stream factory will be written.
    VIRTUAL void METHOD(CreateDefaultShaderSourceStreamFactory)(
                        THIS_
                        const Char*                              SearchDirectories,
                        struct IShaderSourceInputStreamFactory** ppShaderSourceFactory) CONST PURE;

#if PLATFORM_ANDROID
    /// On Android platform, it is necessary to initialize the file system before
    /// CreateDefaultShaderSourceStreamFactory() method can be called.
    /// \param [in] NativeActivity          - Pointer to the native activity object (ANativeActivity).
    /// \param [in] NativeActivityClassName - Native activity class name.
    /// \param [in] AssetManager            - Pointer to the asset manager (AAssetManager).
    ///
    /// \remarks See AndroidFileSystem::Init.
    VIRTUAL void METHOD(InitAndroidFileSystem)(THIS_
                                               struct ANativeActivity*  NativeActivity,
                                               const char*              NativeActivityClassName,
                                               struct AAssetManager*    AssetManager) CONST PURE;
#endif
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define IEngineFactory_GetAPIInfo(This)                                  CALL_IFACE_METHOD(EngineFactory, GetAPIInfo,                             This)
#    define IEngineFactory_CreateDefaultShaderSourceStreamFactory(This, ...) CALL_IFACE_METHOD(EngineFactory, CreateDefaultShaderSourceStreamFactory, This, __VA_ARGS__)
#    define IEngineFactory_InitAndroidFileSystem(This, ...)                  CALL_IFACE_METHOD(EngineFactory, InitAndroidFileSystem,                  This, __VA_ARGS__)

// clang-format on

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
