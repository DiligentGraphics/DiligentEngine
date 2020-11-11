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

#include "SwapChainGL.h"
#include "SwapChainGLBase.hpp"
#include "GLObjectWrapper.hpp"

namespace Diligent
{

/// Swap chain implementation in OpenGL backend.
class SwapChainGLImpl final : public SwapChainGLBase<ISwapChainGL>
{
public:
    using TSwapChainGLBase = SwapChainGLBase<ISwapChainGL>;

    SwapChainGLImpl(IReferenceCounters*        pRefCounters,
                    const EngineGLCreateInfo&  InitAttribs,
                    const SwapChainDesc&       SwapChainDesc,
                    class RenderDeviceGLImpl*  pRenderDeviceGL,
                    class DeviceContextGLImpl* pImmediateContextGL);
    ~SwapChainGLImpl();

    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override final;

    /// Implementation of ISwapChain::Present() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE Present(Uint32 SyncInterval) override final;

    /// Implementation of ISwapChain::Resize() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE Resize(Uint32 NewWidth, Uint32 NewHeight, SURFACE_TRANSFORM NewPreTransform) override final;

    /// Implementation of ISwapChain::SetFullscreenMode() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE SetFullscreenMode(const DisplayModeAttribs& DisplayMode) override final;

    /// Implementation of ISwapChain::SetWindowedMode() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE SetWindowedMode() override final;

    /// Implementation of ISwapChainGL::GetDefaultFBO().
    virtual GLuint DILIGENT_CALL_TYPE GetDefaultFBO() const override final { return 0; }
};

} // namespace Diligent
