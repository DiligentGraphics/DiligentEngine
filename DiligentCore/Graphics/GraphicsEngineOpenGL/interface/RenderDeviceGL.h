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
/// Definition of the Diligent::IRenderDeviceGL interface

#include "../../GraphicsEngine/interface/RenderDevice.h"

/// Namespace for the OpenGL implementation of the graphics engine
DILIGENT_BEGIN_NAMESPACE(Diligent)

// {B4B395B9-AC99-4E8A-B7E1-9DCA0D485618}
static const INTERFACE_ID IID_RenderDeviceGL =
    {0xb4b395b9, 0xac99, 0x4e8a, {0xb7, 0xe1, 0x9d, 0xca, 0xd, 0x48, 0x56, 0x18}};

#define DILIGENT_INTERFACE_NAME IRenderDeviceGL
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IRenderDeviceGLInclusiveMethods \
    IRenderDeviceInclusiveMethods;      \
    IRenderDeviceGLMethods RenderDeviceGL

// clang-format off

/// Exposes OpenGL-specific functionality of a render device.
DILIGENT_BEGIN_INTERFACE(IRenderDeviceGL, IRenderDevice)
{
    /// Creates a texture from OpenGL handle

    /// \param [in] GLHandle      - OpenGL texture handle.
    /// \param [in] GLBindTarget  - OpenGL bind target. If this parameter is null, the engine will
    ///                             automatically select the target based on texture
    ///                             type (e.g. RESOURCE_DIM_TEX_2D will map to GL_TEXTURE_2D).
    ///                             An application should typically use this paramter when the texture
    ///                             has non-standard bind target such as, GL_TEXTURE_EXTERNAL_OES.
    /// \param [in] TexDesc       - Texture description. The engine can automatically
    ///                             set texture width, height, depth, mip levels count, and format.
    ///                             Remaining fields should be set up by the app.
    /// \param [in]  InitialState - Initial texture state. See Diligent::RESOURCE_STATE.
    /// \param [out] ppTexture    - Address of the memory location where the pointer to the
    ///                             texture interface will be stored.
    ///                             The function calls AddRef(), so that the new object will contain
    ///                             one reference.
    /// \note  Diligent engine texture object does not take ownership of the GL resource,
    ///        and the application must not destroy it while it is in use by the engine.
    VIRTUAL void METHOD(CreateTextureFromGLHandle)(THIS_
                                                   Uint32                GLHandle,
                                                   Uint32                GLBindTarget,
                                                   const TextureDesc REF TexDesc,
                                                   RESOURCE_STATE        InitialState,
                                                   ITexture**            ppTexture) PURE;

    /// Creates a buffer from OpenGL handle

    /// \param [in] GLHandle      - OpenGL buffer handle
    /// \param [in] BuffDesc      - Buffer description. The engine can automatically
    ///                             recover buffer size, but the rest of the fields need to
    ///                             be set by the client.
    /// \param [in]  InitialState - Initial buffer state. See Diligent::RESOURCE_STATE.
    /// \param [out] ppBuffer     - Address of the memory location where the pointer to the
    ///                             texture interface will be stored.
    ///                             The function calls AddRef(), so that the new object will contain
    ///                             one reference.
    /// \note  Diligent engine buffer object does not take ownership of the GL resource,
    ///        and the application must not destroy it while it is in use by the engine.
    VIRTUAL void METHOD(CreateBufferFromGLHandle)(THIS_
                                                  Uint32               GLHandle,
                                                  const BufferDesc REF BuffDesc,
                                                  RESOURCE_STATE       InitialState,
                                                  IBuffer**            ppBuffer) PURE;


    /// Creates a dummy texture with null handle.

    /// The main usage of dummy textures is to serve as render target and depth buffer
    /// proxies in a swap chain. When dummy color buffer is set as render target, the
    /// engine binds default FBO provided by the swap chain.
    ///
    /// \param [in]  TexDesc      - Texture description.
    /// \param [in]  InitialState - Initial texture state. See Diligent::RESOURCE_STATE.
    /// \param [out] ppTexture    - Address of the memory location where the pointer to the
    ///                             texture interface will be stored.
    ///                             The function calls AddRef(), so that the new object will contain
    ///                             one reference.
    /// \note  Only RESOURCE_DIM_TEX_2D dummy textures are supported.
    VIRTUAL void METHOD(CreateDummyTexture)(THIS_
                                            const TextureDesc REF TexDesc,
                                            RESOURCE_STATE        InitialState,
                                            ITexture**            ppTexture) PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define IRenderDeviceGL_CreateTextureFromGLHandle(This, ...)CALL_IFACE_METHOD(RenderDeviceGL, CreateTextureFromGLHandle, This, __VA_ARGS__)
#    define IRenderDeviceGL_CreateBufferFromGLHandle(This, ...) CALL_IFACE_METHOD(RenderDeviceGL, CreateBufferFromGLHandle,  This, __VA_ARGS__)
#    define IRenderDeviceGL_CreateDummyTexture(This, ...)       CALL_IFACE_METHOD(RenderDeviceGL, CreateDummyTexture,        This, __VA_ARGS__)

// clang-format on

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
