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
/// Declaration of functions that initialize Direct3D11-based engine implementation

#include "../../GraphicsEngine/interface/EngineFactory.h"
#include "../../GraphicsEngine/interface/RenderDevice.h"
#include "../../GraphicsEngine/interface/DeviceContext.h"
#include "../../GraphicsEngine/interface/SwapChain.h"

#if ENGINE_DLL
#    include "../../GraphicsEngine/interface/LoadEngineDll.h"
#endif

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {62663A30-AAF0-4A9A-9729-9EAC6BF789F2}
static const struct INTERFACE_ID IID_EngineFactoryD3D11 =
    {0x62663a30, 0xaaf0, 0x4a9a, {0x97, 0x29, 0x9e, 0xac, 0x6b, 0xf7, 0x89, 0xf2}};

#define DILIGENT_INTERFACE_NAME IEngineFactoryD3D11
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IEngineFactoryD3D11InclusiveMethods \
    IEngineFactoryInclusiveMethods;         \
    IEngineFactoryD3D11Methods EngineFactoryD3D11

// clang-format off

/// Engine factory for Direct3D11 rendering backend.
DILIGENT_BEGIN_INTERFACE(IEngineFactoryD3D11, IEngineFactory)
{
    /// Creates a render device and device contexts for Direct3D11-based engine implementation.

    /// \param [in] EngineCI  - Engine creation info.
    /// \param [out] ppDevice - Address of the memory location where pointer to
    ///                         the created device will be written.
    /// \param [out] ppContexts - Address of the memory location where pointers to
    ///                           the contexts will be written. Immediate context goes at
    ///                           position 0. If EngineCI.NumDeferredContexts > 0,
    ///                           pointers to deferred contexts are written afterwards.
    VIRTUAL void METHOD(CreateDeviceAndContextsD3D11)(THIS_ 
                                                      const EngineD3D11CreateInfo REF EngineCI,
                                                      IRenderDevice**                    ppDevice,
                                                      IDeviceContext**                   ppContexts) PURE;


    /// Creates a swap chain for Direct3D11-based engine implementation.

    /// \param [in] pDevice           - Pointer to the render device.
    /// \param [in] pImmediateContext - Pointer to the immediate device context.
    /// \param [in] SCDesc            - Swap chain description.
    /// \param [in] FSDesc            - Fullscreen mode description.
    /// \param [in] Window            - Platform-specific native window description that
    ///                                 the swap chain will be associated with:
    ///                                 * On Win32 platform, this is the window handle (HWND)
    ///                                 * On Universal Windows Platform, this is the reference
    ///                                   to the core window (Windows::UI::Core::CoreWindow)
    ///
    /// \param [out] ppSwapChain    - Address of the memory location where pointer to the new
    ///                               swap chain will be written.
    VIRTUAL void METHOD(CreateSwapChainD3D11)(THIS_
                                              IRenderDevice*               pDevice,
                                              IDeviceContext*              pImmediateContext,
                                              const SwapChainDesc REF      SCDesc,
                                              const FullScreenModeDesc REF FSDesc,
                                              const NativeWindow REF       Window,
                                              ISwapChain**                 ppSwapChain) PURE;


    /// Attaches to existing Direct3D11 render device and immediate context.

    /// \param [in] pd3d11NativeDevice     - pointer to the native Direct3D11 device.
    /// \param [in] pd3d11ImmediateContext - pointer to the native Direct3D11 immediate context.
    /// \param [in] EngineCI               - Engine creation info.
    /// \param [out] ppDevice              - Address of the memory location where pointer to
    ///                                      the created device will be written.
    /// \param [out] ppContexts - Address of the memory location where pointers to
    ///                           the contexts will be written. Immediate context goes at
    ///                           position 0. If EngineCI.NumDeferredContexts > 0,
    ///                           pointers to the deferred contexts are written afterwards.
    VIRTUAL void METHOD(AttachToD3D11Device)(THIS_
                                             void*                           pd3d11NativeDevice,
                                             void*                           pd3d11ImmediateContext,
                                             const EngineD3D11CreateInfo REF EngineCI,
                                             IRenderDevice**                 ppDevice,
                                             IDeviceContext**                ppContexts) PURE;


    /// Enumerates adapters available on this machine.

    /// \param [in]     MinFeatureLevel - Minimum required feature level.
    /// \param [in,out] NumAdapters - Number of adapters. If Adapters is null, this value
    ///                               will be overwritten with the number of adapters available
    ///                               on this system. If Adapters is not null, this value should
    ///                               contain the maximum number of elements reserved in the array
    ///                               pointed to by Adapters. In the latter case, this value
    ///                               is overwritten with the actual number of elements written to
    ///                               Adapters.
    /// \param [out]    Adapters - Pointer to the array conataining adapter information. If
    ///                            null is provided, the number of available adapters is
    ///                            written to NumAdapters.
    VIRTUAL void METHOD(EnumerateAdapters)(THIS_
                                           DIRECT3D_FEATURE_LEVEL MinFeatureLevel,
                                           Uint32 REF             NumAdapters,
                                           GraphicsAdapterInfo*   Adapters) PURE;


    /// Enumerates available display modes for the specified output of the specified adapter.

    /// \param [in] MinFeatureLevel - Minimum feature level of the adapter that was given to EnumerateAdapters().
    /// \param [in] AdapterId       - Id of the adapter enumerated by EnumerateAdapters().
    /// \param [in] OutputId        - Adapter output id.
    /// \param [in] Format          - Display mode format.
    /// \param [in, out] NumDisplayModes - Number of display modes. If DisplayModes is null, this
    ///                                    value is overwritten with the number of display modes
    ///                                    available for this output. If DisplayModes is not null,
    ///                                    this value should contain the maximum number of elements
    ///                                    to be written to DisplayModes array. It is overwritten with
    ///                                    the actual number of display modes written.
    VIRTUAL void METHOD(EnumerateDisplayModes)(THIS_
                                               DIRECT3D_FEATURE_LEVEL MinFeatureLevel,
                                               Uint32                 AdapterId,
                                               Uint32                 OutputId,
                                               TEXTURE_FORMAT         Format,
                                               Uint32 REF             NumDisplayModes,
                                               DisplayModeAttribs*    DisplayModes) PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define IEngineFactoryD3D11_CreateDeviceAndContextsD3D11(This, ...) CALL_IFACE_METHOD(EngineFactoryD3D11, CreateDeviceAndContextsD3D11, This, __VA_ARGS__)
#    define IEngineFactoryD3D11_CreateSwapChainD3D11(This, ...)         CALL_IFACE_METHOD(EngineFactoryD3D11, CreateSwapChainD3D11,         This, __VA_ARGS__)
#    define IEngineFactoryD3D11_AttachToD3D11Device(This, ...)          CALL_IFACE_METHOD(EngineFactoryD3D11, AttachToD3D11Device,          This, __VA_ARGS__)
#    define IEngineFactoryD3D11_EnumerateAdapters(This, ...)            CALL_IFACE_METHOD(EngineFactoryD3D11, EnumerateAdapters,            This, __VA_ARGS__)
#    define IEngineFactoryD3D11_EnumerateDisplayModes(This, ...)        CALL_IFACE_METHOD(EngineFactoryD3D11, EnumerateDisplayModes,        This, __VA_ARGS__)

// clang-format on

#endif


#if ENGINE_DLL

typedef struct IEngineFactoryD3D11* (*GetEngineFactoryD3D11Type)();

inline GetEngineFactoryD3D11Type LoadGraphicsEngineD3D11()
{
    return (GetEngineFactoryD3D11Type)LoadEngineDll("GraphicsEngineD3D11", "GetEngineFactoryD3D11");
}

#else

struct IEngineFactoryD3D11* DILIGENT_GLOBAL_FUNCTION(GetEngineFactoryD3D11)();

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
