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

#include "../include/D3D11/TestingSwapChainD3D11.hpp"
#include "../include/DXGITypeConversions.hpp"

#include "RenderDeviceD3D11.h"
#include "DeviceContextD3D11.h"

namespace Diligent
{

namespace Testing
{

TestingSwapChainD3D11::TestingSwapChainD3D11(IReferenceCounters*  pRefCounters,
                                             IRenderDevice*       pDevice,
                                             IDeviceContext*      pContext,
                                             const SwapChainDesc& SCDesc) :
    TBase //
    {
        pRefCounters,
        pDevice,
        pContext,
        SCDesc //
    }
{
    RefCntAutoPtr<IRenderDeviceD3D11>  pRenderDeviceD3D11{pDevice, IID_RenderDeviceD3D11};
    RefCntAutoPtr<IDeviceContextD3D11> pContextD3D11{pContext, IID_DeviceContextD3D11};

    auto* pd3d11Device = pRenderDeviceD3D11->GetD3D11Device();
    m_pd3d11Context    = pContextD3D11->GetD3D11DeviceContext();

    D3D11_TEXTURE2D_DESC TexDesc = {};

    TexDesc.Width              = SCDesc.Width;
    TexDesc.Height             = SCDesc.Height;
    TexDesc.MipLevels          = 1;
    TexDesc.ArraySize          = 1;
    TexDesc.Format             = TexFormatToDXGI_Format(SCDesc.ColorBufferFormat);
    TexDesc.SampleDesc.Count   = 1;
    TexDesc.SampleDesc.Quality = 0;
    TexDesc.Usage              = D3D11_USAGE_DEFAULT;
    TexDesc.BindFlags          = BIND_RENDER_TARGET | BIND_UNORDERED_ACCESS;
    TexDesc.CPUAccessFlags     = 0;
    TexDesc.MiscFlags          = 0;

    auto hr = pd3d11Device->CreateTexture2D(&TexDesc, nullptr, &m_pd3d11RenderTarget);
    VERIFY(SUCCEEDED(hr), "Failed to create D3D11 render target");

    hr = pd3d11Device->CreateRenderTargetView(m_pd3d11RenderTarget, nullptr, &m_pd3d11RTV);
    VERIFY(SUCCEEDED(hr), "Failed to create D3D11 render target view");

    hr = pd3d11Device->CreateUnorderedAccessView(m_pd3d11RenderTarget, nullptr, &m_pd3d11UAV);
    VERIFY(SUCCEEDED(hr), "Failed to create D3D11 unordered access view");

    TexDesc.BindFlags      = 0;
    TexDesc.Usage          = D3D11_USAGE_STAGING;
    TexDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    hr = pd3d11Device->CreateTexture2D(&TexDesc, nullptr, &m_pd3d11StagingTex);
    VERIFY(SUCCEEDED(hr), "Failed to create staging D3D11 texture");

    if (SCDesc.DepthBufferFormat != TEX_FORMAT_UNKNOWN)
    {
        TexDesc.Usage          = D3D11_USAGE_DEFAULT;
        TexDesc.Format         = TexFormatToDXGI_Format(SCDesc.DepthBufferFormat);
        TexDesc.BindFlags      = BIND_DEPTH_STENCIL;
        TexDesc.CPUAccessFlags = 0;

        hr = pd3d11Device->CreateTexture2D(&TexDesc, nullptr, &m_pd3d11DepthBuffer);
        VERIFY(SUCCEEDED(hr), "Failed to create D3D11 depth buffer");

        hr = pd3d11Device->CreateDepthStencilView(m_pd3d11DepthBuffer, nullptr, &m_pd3d11DSV);
        VERIFY(SUCCEEDED(hr), "Failed to create D3D11 depth-stencil view");
    }
}

void TestingSwapChainD3D11::TakeSnapshot()
{
    m_pd3d11Context->CopyResource(m_pd3d11StagingTex, m_pd3d11RenderTarget);
    D3D11_MAPPED_SUBRESOURCE MappedData;

    auto hr = m_pd3d11Context->Map(m_pd3d11StagingTex, 0, D3D11_MAP_READ, 0, &MappedData);
    if (SUCCEEDED(hr))
    {
        m_ReferenceDataPitch = MappedData.RowPitch;
        m_ReferenceData.resize(m_ReferenceDataPitch * m_SwapChainDesc.Height);
        for (Uint32 row = 0; row < m_SwapChainDesc.Height; ++row)
        {
            memcpy(&m_ReferenceData[row * m_ReferenceDataPitch],
                   reinterpret_cast<const Uint8*>(MappedData.pData) + row * m_ReferenceDataPitch,
                   m_ReferenceDataPitch);
        }
    }
    else
    {
        ADD_FAILURE() << "Failed to map stagin texture";
        return;
    }

    m_pd3d11Context->Unmap(m_pd3d11StagingTex, 0);
    m_pd3d11Context->ClearState();
}

void CreateTestingSwapChainD3D11(IRenderDevice*       pDevice,
                                 IDeviceContext*      pContext,
                                 const SwapChainDesc& SCDesc,
                                 ISwapChain**         ppSwapChain)
{
    TestingSwapChainD3D11* pTestingSC(MakeNewRCObj<TestingSwapChainD3D11>()(pDevice, pContext, SCDesc));
    pTestingSC->QueryInterface(IID_SwapChain, reinterpret_cast<IObject**>(ppSwapChain));
}

} // namespace Testing

} // namespace Diligent
