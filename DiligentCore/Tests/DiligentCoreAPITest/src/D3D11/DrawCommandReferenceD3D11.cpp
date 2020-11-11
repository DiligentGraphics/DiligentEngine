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

#include "D3D11/TestingEnvironmentD3D11.hpp"
#include "D3D11/TestingSwapChainD3D11.hpp"

#include "InlineShaders/DrawCommandTestHLSL.h"

namespace Diligent
{

namespace Testing
{

static void DrawProceduralTriangles(ID3D11DeviceContext*    pd3d11Context,
                                    ISwapChain*             pSwapChain,
                                    ID3D11RenderTargetView* pRTV,
                                    Uint32                  Width,
                                    Uint32                  Height,
                                    const float             ClearColor[])
{
    auto* pEnvD3D11 = TestingEnvironmentD3D11::GetInstance();

    auto pVS = pEnvD3D11->CreateVertexShader(HLSL::DrawTest_ProceduralTriangleVS);
    ASSERT_NE(pVS, nullptr);

    auto pPS = pEnvD3D11->CreatePixelShader(HLSL::DrawTest_PS);
    ASSERT_NE(pPS, nullptr);

    pd3d11Context->VSSetShader(pVS, nullptr, 0);
    pd3d11Context->PSSetShader(pPS, nullptr, 0);
    pd3d11Context->RSSetState(pEnvD3D11->GetNoCullRS());
    pd3d11Context->OMSetBlendState(pEnvD3D11->GetDefaultBS(), nullptr, 0xFFFFFFFF);
    pd3d11Context->OMSetDepthStencilState(pEnvD3D11->GetDisableDepthDSS(), 0);
    pd3d11Context->IASetInputLayout(nullptr);

    ID3D11RenderTargetView* pd3d11RTVs[] = {pRTV};
    pd3d11Context->OMSetRenderTargets(1, pd3d11RTVs, nullptr);
    pd3d11Context->ClearRenderTargetView(pd3d11RTVs[0], ClearColor);

    D3D11_VIEWPORT d3dVP = {};

    d3dVP.Width    = static_cast<float>(Width);
    d3dVP.Height   = static_cast<float>(Height);
    d3dVP.MaxDepth = 1;
    pd3d11Context->RSSetViewports(1, &d3dVP);

    pd3d11Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pd3d11Context->Draw(6, 0);
}

void RenderDrawCommandReferenceD3D11(ISwapChain* pSwapChain, const float* pClearColor)
{
    auto* pEnvD3D11              = TestingEnvironmentD3D11::GetInstance();
    auto* pd3d11Context          = pEnvD3D11->GetD3D11Context();
    auto* pTestingSwapChainD3D11 = ValidatedCast<TestingSwapChainD3D11>(pSwapChain);

    pd3d11Context->ClearState();

    const auto& SCDesc = pTestingSwapChainD3D11->GetDesc();
    float       Zero[] = {0, 0, 0, 0};
    DrawProceduralTriangles(pd3d11Context, pTestingSwapChainD3D11, pTestingSwapChainD3D11->GetD3D11RTV(), SCDesc.Width, SCDesc.Height, pClearColor != nullptr ? pClearColor : Zero);

    pd3d11Context->ClearState();
}

void RenderPassMSResolveReferenceD3D11(ISwapChain* pSwapChain, const float* pClearColor)
{
    auto* pEnvD3D11              = TestingEnvironmentD3D11::GetInstance();
    auto* pd3d11Context          = pEnvD3D11->GetD3D11Context();
    auto* pd3d11Device           = pEnvD3D11->GetD3D11Device();
    auto* pTestingSwapChainD3D11 = ValidatedCast<TestingSwapChainD3D11>(pSwapChain);

    const auto& SCDesc = pTestingSwapChainD3D11->GetDesc();

    D3D11_TEXTURE2D_DESC MSTexDesc = {};

    MSTexDesc.Width     = SCDesc.Width;
    MSTexDesc.Height    = SCDesc.Height;
    MSTexDesc.MipLevels = 1;
    MSTexDesc.ArraySize = 1;
    switch (SCDesc.ColorBufferFormat)
    {
        case TEX_FORMAT_RGBA8_UNORM:
            MSTexDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            break;

        default:
            UNSUPPORTED("Unsupported swap chain format");
    }
    MSTexDesc.SampleDesc.Count   = 4;
    MSTexDesc.SampleDesc.Quality = 0;
    MSTexDesc.Usage              = D3D11_USAGE_DEFAULT;
    MSTexDesc.BindFlags          = D3D11_BIND_RENDER_TARGET;
    MSTexDesc.CPUAccessFlags     = 0;
    MSTexDesc.MiscFlags          = 0;

    CComPtr<ID3D11Texture2D> pd3d11MSTex;
    pd3d11Device->CreateTexture2D(&MSTexDesc, nullptr, &pd3d11MSTex);
    ASSERT_TRUE(pd3d11MSTex != nullptr);

    CComPtr<ID3D11RenderTargetView> pd3d11RTV;
    pd3d11Device->CreateRenderTargetView(pd3d11MSTex, nullptr, &pd3d11RTV);
    ASSERT_TRUE(pd3d11RTV != nullptr);

    pd3d11Context->ClearState();

    DrawProceduralTriangles(pd3d11Context, pTestingSwapChainD3D11, pd3d11RTV, SCDesc.Width, SCDesc.Height, pClearColor);

    pd3d11Context->ResolveSubresource(pTestingSwapChainD3D11->GetD3D11RenderTarget(), 0, pd3d11MSTex, 0, MSTexDesc.Format);

    pd3d11Context->ClearState();
}

void RenderPassInputAttachmentReferenceD3D11(ISwapChain* pSwapChain, const float* pClearColor)
{
    auto* pEnvD3D11              = TestingEnvironmentD3D11::GetInstance();
    auto* pd3d11Context          = pEnvD3D11->GetD3D11Context();
    auto* pd3d11Device           = pEnvD3D11->GetD3D11Device();
    auto* pTestingSwapChainD3D11 = ValidatedCast<TestingSwapChainD3D11>(pSwapChain);

    const auto& SCDesc = pTestingSwapChainD3D11->GetDesc();

    D3D11_TEXTURE2D_DESC InptAttTexDesc = {};

    InptAttTexDesc.Width     = SCDesc.Width;
    InptAttTexDesc.Height    = SCDesc.Height;
    InptAttTexDesc.MipLevels = 1;
    InptAttTexDesc.ArraySize = 1;
    switch (SCDesc.ColorBufferFormat)
    {
        case TEX_FORMAT_RGBA8_UNORM:
            InptAttTexDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            break;

        default:
            UNSUPPORTED("Unsupported swap chain format");
    }
    InptAttTexDesc.SampleDesc.Count   = 1;
    InptAttTexDesc.SampleDesc.Quality = 0;
    InptAttTexDesc.Usage              = D3D11_USAGE_DEFAULT;
    InptAttTexDesc.BindFlags          = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    InptAttTexDesc.CPUAccessFlags     = 0;
    InptAttTexDesc.MiscFlags          = 0;

    CComPtr<ID3D11Texture2D> pd3d11InptAtt;
    pd3d11Device->CreateTexture2D(&InptAttTexDesc, nullptr, &pd3d11InptAtt);
    ASSERT_TRUE(pd3d11InptAtt != nullptr);

    CComPtr<ID3D11RenderTargetView> pd3d11RTV;
    pd3d11Device->CreateRenderTargetView(pd3d11InptAtt, nullptr, &pd3d11RTV);
    ASSERT_TRUE(pd3d11RTV != nullptr);

    CComPtr<ID3D11ShaderResourceView> pd3d11SRV;
    pd3d11Device->CreateShaderResourceView(pd3d11InptAtt, nullptr, &pd3d11SRV);
    ASSERT_TRUE(pd3d11SRV != nullptr);

    pd3d11Context->ClearState();

    float Zero[] = {0, 0, 0, 0};
    DrawProceduralTriangles(pd3d11Context, pTestingSwapChainD3D11, pd3d11RTV, SCDesc.Width, SCDesc.Height, Zero);

    ID3D11RenderTargetView* pd3d11RTVs[] = {pTestingSwapChainD3D11->GetD3D11RTV()};
    pd3d11Context->OMSetRenderTargets(1, pd3d11RTVs, nullptr);
    pd3d11Context->ClearRenderTargetView(pd3d11RTVs[0], pClearColor);

    auto pInputAttachmentPS = pEnvD3D11->CreatePixelShader(HLSL::InputAttachmentTest_PS);
    ASSERT_NE(pInputAttachmentPS, nullptr);
    pd3d11Context->PSSetShader(pInputAttachmentPS, nullptr, 0);

    ID3D11ShaderResourceView* pSRVs[] = {pd3d11SRV};
    pd3d11Context->PSSetShaderResources(0, 1, pSRVs);

    pd3d11Context->Draw(6, 0);

    pd3d11Context->ClearState();
}

} // namespace Testing

} // namespace Diligent
