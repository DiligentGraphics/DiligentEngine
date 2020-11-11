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

#include <d3d11.h>
#include "DiligentCore/Graphics/GraphicsEngineD3D11/interface/RenderDeviceD3D11.h"

void TestRenderDeviceD3D11CInterface(IRenderDeviceD3D11* pDevice)
{
    ID3D11Device* pd3d11Device = IRenderDeviceD3D11_GetD3D11Device(pDevice);
    (void)pd3d11Device;

    ID3D11Buffer* pd3d11Buffer = NULL;
    IBuffer*      pBuff        = NULL;
    BufferDesc    BuffDesc     = {0};
    IRenderDeviceD3D11_CreateBufferFromD3DResource(pDevice, pd3d11Buffer, &BuffDesc, RESOURCE_STATE_CONSTANT_BUFFER, &pBuff);

    ID3D11Texture1D* pd3d11Texture1D = NULL;
    ITexture*        pTex1D          = NULL;
    IRenderDeviceD3D11_CreateTexture1DFromD3DResource(pDevice, pd3d11Texture1D, RESOURCE_STATE_SHADER_RESOURCE, &pTex1D);

    ID3D11Texture2D* pd3d11Texture2D = NULL;
    ITexture*        pTex2D          = NULL;
    IRenderDeviceD3D11_CreateTexture2DFromD3DResource(pDevice, pd3d11Texture2D, RESOURCE_STATE_SHADER_RESOURCE, &pTex2D);

    ID3D11Texture3D* pd3d11Texture3D = NULL;
    ITexture*        pTex3D          = NULL;
    IRenderDeviceD3D11_CreateTexture3DFromD3DResource(pDevice, pd3d11Texture3D, RESOURCE_STATE_SHADER_RESOURCE, &pTex3D);
}
