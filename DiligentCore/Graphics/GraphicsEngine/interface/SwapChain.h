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
/// Definition of the Diligent::ISwapChain interface and related data structures

#include "../../../Primitives/interface/Object.h"
#include "TextureView.h"
#include "GraphicsTypes.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)


// {1C703B77-6607-4EEC-B1FE-15C82D3B4130}
static const INTERFACE_ID IID_SwapChain =
    {0x1c703b77, 0x6607, 0x4eec, {0xb1, 0xfe, 0x15, 0xc8, 0x2d, 0x3b, 0x41, 0x30}};

#define DILIGENT_INTERFACE_NAME ISwapChain
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define ISwapChainInclusiveMethods \
    IObjectInclusiveMethods;       \
    ISwapChainMethods SwapChain

// clang-format off

/// Swap chain interface

/// The swap chain is created by a platform-dependent function
DILIGENT_BEGIN_INTERFACE(ISwapChain, IObject)
{
    /// Presents a rendered image to the user
    VIRTUAL void METHOD(Present)(THIS_
                                 Uint32 SyncInterval DEFAULT_VALUE(1)) PURE;

    /// Returns the swap chain desctription
    VIRTUAL const SwapChainDesc REF METHOD(GetDesc)(THIS) CONST PURE;

    /// Changes the swap chain size

    /// \param [in] NewWidth     - New logical swap chain width (not accounting for pre-transform), in pixels.
    /// \param [in] NewHeight    - New logical swap chain height (not accounting for pre-transform), in pixels.
    /// \param [in] NewTransform - New surface transform, see Diligent::SURFACE_TRANSFORM.
    ///
    /// \note When resizing non-primary swap chains, the engine unbinds the
    ///       swap chain buffers from the output.
    ///
    ///       New width and height should not account for surface pre-transform. For example,
    ///       if the window size is 1920 x 1080, but the surface is pre-rotated by 90 degrees,
    ///       NewWidth should still be 1920, and NewHeight should still be 1080. It is highly
    ///       recommended to always use SURFACE_TRANSFORM_OPTIMAL to let the engine select
    ///       the most optimal pre-transform. However SURFACE_TRANSFORM_ROTATE_90 will also work in
    ///       the scenario above. After the swap chain has been resized, its actual width will be 1080,
    ///       actual height will be 1920, and PreTransform will be SURFACE_TRANSFORM_ROTATE_90.
    VIRTUAL void METHOD(Resize)(THIS_
                                Uint32            NewWidth,
                                Uint32            NewHeight,
                                SURFACE_TRANSFORM NewTransform DEFAULT_VALUE(SURFACE_TRANSFORM_OPTIMAL)) PURE;

    /// Sets fullscreen mode (only supported on Win32 platform)
    VIRTUAL void METHOD(SetFullscreenMode)(THIS_ const DisplayModeAttribs REF DisplayMode) PURE;

    /// Sets windowed mode (only supported on Win32 platform)
    VIRTUAL void METHOD(SetWindowedMode)(THIS) PURE;
    
    /// Sets the maximum number of frames that the swap chain is allowed to queue for rendering.

    /// This value is only relevant for D3D11 and D3D12 backends and ignored for others.
    /// By default it matches the number of buffers in the swap chain. For example, for a 2-buffer
    /// swap chain, the CPU can enqueue frames 0 and 1, but Present command of frame 2
    /// will block until frame 0 is presented. If in the example above the maximum frame latency is set
    /// to 1, then Present command of frame 1 will block until Present of frame 0 is complete.
    VIRTUAL void METHOD(SetMaximumFrameLatency)(THIS_
                                                Uint32 MaxLatency) PURE;

    /// Returns render target view of the current back buffer in the swap chain

    /// \note For Direct3D12 and Vulkan backends, the function returns
    /// different pointer for every offscreen buffer in the swap chain
    /// (flipped by every call to ISwapChain::Present()). For Direct3D11
    /// backend it always returns the same pointer. For OpenGL/GLES backends
    /// the method returns null.
    ///
    /// The method does *NOT* call AddRef() on the returned interface,
    /// so Release() must not be called.
    VIRTUAL ITextureView* METHOD(GetCurrentBackBufferRTV)(THIS) PURE;

    /// Returns depth-stencil view of the depth buffer

    /// The method does *NOT* call AddRef() on the returned interface,
    /// so Release() must not be called.
    VIRTUAL ITextureView* METHOD(GetDepthBufferDSV)(THIS) PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define ISwapChain_Present(This, ...)                CALL_IFACE_METHOD(SwapChain, Present,                 This, __VA_ARGS__)
#    define ISwapChain_GetDesc(This)                     CALL_IFACE_METHOD(SwapChain, GetDesc,                 This)
#    define ISwapChain_Resize(This, ...)                 CALL_IFACE_METHOD(SwapChain, Resize,                  This, __VA_ARGS__)
#    define ISwapChain_SetFullscreenMode(This, ...)      CALL_IFACE_METHOD(SwapChain, SetFullscreenMode,       This, __VA_ARGS__)
#    define ISwapChain_SetWindowedMode(This)             CALL_IFACE_METHOD(SwapChain, SetWindowedMode,         This)
#    define ISwapChain_SetMaximumFrameLatency(This, ...) CALL_IFACE_METHOD(SwapChain, SetMaximumFrameLatency,  This, __VA_ARGS__)
#    define ISwapChain_GetCurrentBackBufferRTV(This)     CALL_IFACE_METHOD(SwapChain, GetCurrentBackBufferRTV, This)
#    define ISwapChain_GetDepthBufferDSV(This)           CALL_IFACE_METHOD(SwapChain, GetDepthBufferDSV,       This)

// clang-format on

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
