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

#ifndef NOMINMAX
#    define NOMINMAX
#endif
#include <Windows.h>
#include <d3d11.h>

#ifndef ENGINE_DLL
#    define ENGINE_DLL 1
#endif

#include "DiligentCore/Graphics/GraphicsEngineD3D11/interface/EngineFactoryD3D11.h"

void TestEngineFactoryD3D11CInterface()
{
    GetEngineFactoryD3D11Type    GetEngineFactoryD3D11 = LoadGraphicsEngineD3D11();
    struct IEngineFactoryD3D11*  pFactory              = GetEngineFactoryD3D11();
    struct EngineD3D11CreateInfo EngineCI              = {0};
    IRenderDevice*               pDevice               = NULL;
    IDeviceContext*              pCtx                  = NULL;
    IEngineFactoryD3D11_CreateDeviceAndContextsD3D11(pFactory, &EngineCI, &pDevice, &pCtx);

    struct SwapChainDesc      SCDesc           = {0};
    struct FullScreenModeDesc FSDes            = {0};
    void*                     pNativeWndHandle = NULL;
    ISwapChain*               pSwapChain       = NULL;
    IEngineFactoryD3D11_CreateSwapChainD3D11(pFactory, pDevice, pCtx, &SCDesc, &FSDes, pNativeWndHandle, &pSwapChain);

    void* pd3d11NativeDevice     = NULL;
    void* pd3d11ImmediateContext = NULL;
    IEngineFactoryD3D11_AttachToD3D11Device(pFactory, pd3d11NativeDevice, pd3d11ImmediateContext, &EngineCI, &pDevice, &pCtx);

    Uint32                      NumAdapters = 0;
    struct GraphicsAdapterInfo* Adapters    = NULL;
    IEngineFactoryD3D11_EnumerateAdapters(pFactory, DIRECT3D_FEATURE_LEVEL_11_0, &NumAdapters, Adapters);

    Uint32                     NumDisplayModes = 0;
    struct DisplayModeAttribs* DisplayModes    = NULL;
    IEngineFactoryD3D11_EnumerateDisplayModes(pFactory, DIRECT3D_FEATURE_LEVEL_11_0, 0, 0, TEX_FORMAT_RGBA8_UNORM, &NumDisplayModes, DisplayModes);
}
