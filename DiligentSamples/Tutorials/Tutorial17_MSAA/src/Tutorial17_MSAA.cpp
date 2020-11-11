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

#include <array>

#include "Tutorial17_MSAA.hpp"
#include "MapHelper.hpp"
#include "GraphicsUtilities.h"
#include "TextureUtilities.h"
#include "../../Common/src/TexturedCube.hpp"
#include "imgui.h"
#include "ImGuiUtils.hpp"

namespace Diligent
{

SampleBase* CreateSample()
{
    return new Tutorial17_MSAA();
}

void Tutorial17_MSAA::CreateCubePSO()
{
    // Create a shader source stream factory to load shaders from files.
    RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
    m_pEngineFactory->CreateDefaultShaderSourceStreamFactory(nullptr, &pShaderSourceFactory);

    m_pCubePSO = TexturedCube::CreatePipelineState(m_pDevice,
                                                   m_pSwapChain->GetDesc().ColorBufferFormat,
                                                   DepthBufferFormat,
                                                   pShaderSourceFactory,
                                                   "cube.vsh",
                                                   "cube.psh",
                                                   nullptr, 0,
                                                   m_SampleCount);

    // Since we did not explcitly specify the type for 'Constants' variable, default
    // type (SHADER_RESOURCE_VARIABLE_TYPE_STATIC) will be used. Static variables never
    // change and are bound directly through the pipeline state object.
    m_pCubePSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "Constants")->Set(m_CubeVSConstants);

    m_pCubeSRB.Release();
    // Since we are using mutable variable, we must create a shader resource binding object
    // http://diligentgraphics.com/2016/03/23/resource-binding-model-in-diligent-engine-2-0/
    m_pCubePSO->CreateShaderResourceBinding(&m_pCubeSRB, true);
    // Set cube texture SRV in the SRB
    m_pCubeSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture")->Set(m_CubeTextureSRV);
}

void Tutorial17_MSAA::UpdateUI()
{
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        std::array<std::pair<Uint8, const char*>, 4> ComboItems;

        Uint32 NumItems = 0;

        ComboItems[NumItems++] = std::make_pair(Uint8{1}, "1");
        if (m_SupportedSampleCounts & 0x02)
            ComboItems[NumItems++] = std::make_pair(Uint8{2}, "2");
        if (m_SupportedSampleCounts & 0x04)
            ComboItems[NumItems++] = std::make_pair(Uint8{4}, "4");
        if (m_SupportedSampleCounts & 0x08)
            ComboItems[NumItems++] = std::make_pair(Uint8{8}, "8");
        if (ImGui::Combo("Sample count", &m_SampleCount, ComboItems.data(), NumItems))
        {
            CreateCubePSO();
            CreateMSAARenderTarget();
        }

        ImGui::Checkbox("Rotate gird", &m_bRotateGrid);
    }
    ImGui::End();
}

void Tutorial17_MSAA::Initialize(const SampleInitInfo& InitInfo)
{
    SampleBase::Initialize(InitInfo);

    const auto& ColorFmtInfo = m_pDevice->GetTextureFormatInfoExt(m_pSwapChain->GetDesc().ColorBufferFormat);
    const auto& DepthFmtInfo = m_pDevice->GetTextureFormatInfoExt(DepthBufferFormat);
    m_SupportedSampleCounts  = ColorFmtInfo.SampleCounts & DepthFmtInfo.SampleCounts;
    if (m_SupportedSampleCounts & 0x04)
        m_SampleCount = 4;
    else if (m_SupportedSampleCounts & 0x02)
        m_SampleCount = 2;
    else
    {
        LOG_WARNING_MESSAGE(ColorFmtInfo.Name, " + ", DepthFmtInfo.Name, " pair does not allow multisampling on this device");
        m_SampleCount = 1;
    }

    // Create dynamic uniform buffer that will store our transformation matrix
    // Dynamic buffers can be frequently updated by the CPU
    CreateUniformBuffer(m_pDevice, sizeof(float4x4), "VS constants CB", &m_CubeVSConstants);

    // Load textured cube
    m_CubeVertexBuffer = TexturedCube::CreateVertexBuffer(m_pDevice);
    m_CubeIndexBuffer  = TexturedCube::CreateIndexBuffer(m_pDevice);
    m_CubeTextureSRV   = TexturedCube::LoadTexture(m_pDevice, "DGLogo.png")->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);

    CreateCubePSO();
}

void Tutorial17_MSAA::WindowResize(Uint32 Width, Uint32 Height)
{
    CreateMSAARenderTarget();
}

void Tutorial17_MSAA::CreateMSAARenderTarget()
{
    if (m_SampleCount == 1)
        return;

    const auto& SCDesc = m_pSwapChain->GetDesc();
    // Create window-size multi-sampled offscreen render target
    TextureDesc ColorDesc;
    ColorDesc.Name           = "Multisampled render target";
    ColorDesc.Type           = RESOURCE_DIM_TEX_2D;
    ColorDesc.BindFlags      = BIND_RENDER_TARGET;
    ColorDesc.Width          = SCDesc.Width;
    ColorDesc.Height         = SCDesc.Height;
    ColorDesc.MipLevels      = 1;
    ColorDesc.Format         = SCDesc.ColorBufferFormat;
    bool NeedsSRGBConversion = m_pDevice->GetDeviceCaps().IsD3DDevice() && (ColorDesc.Format == TEX_FORMAT_RGBA8_UNORM_SRGB || ColorDesc.Format == TEX_FORMAT_BGRA8_UNORM_SRGB);
    if (NeedsSRGBConversion)
    {
        // Internally Direct3D swap chain images are not SRGB, and ResolveSubresource
        // requires source and destination formats to match exactly or be typeless.
        // So we will have to create a typeless texture and use SRGB render target view with it.
        ColorDesc.Format = ColorDesc.Format == TEX_FORMAT_RGBA8_UNORM_SRGB ? TEX_FORMAT_RGBA8_TYPELESS : TEX_FORMAT_BGRA8_TYPELESS;
    }

    // Set the desired number of samples
    ColorDesc.SampleCount = m_SampleCount;
    // Define optimal clear value
    ColorDesc.ClearValue.Format   = SCDesc.ColorBufferFormat;
    ColorDesc.ClearValue.Color[0] = 0.125f;
    ColorDesc.ClearValue.Color[1] = 0.125f;
    ColorDesc.ClearValue.Color[2] = 0.125f;
    ColorDesc.ClearValue.Color[3] = 1.f;
    RefCntAutoPtr<ITexture> pColor;
    m_pDevice->CreateTexture(ColorDesc, nullptr, &pColor);

    // Store the render target view
    m_pMSColorRTV.Release();
    if (NeedsSRGBConversion)
    {
        TextureViewDesc RTVDesc;
        RTVDesc.ViewType = TEXTURE_VIEW_RENDER_TARGET;
        RTVDesc.Format   = SCDesc.ColorBufferFormat;
        pColor->CreateView(RTVDesc, &m_pMSColorRTV);
    }
    else
    {
        m_pMSColorRTV = pColor->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);
    }


    // Create window-size multi-sampled depth buffer
    TextureDesc DepthDesc = ColorDesc;
    DepthDesc.Name        = "Multisampled depth buffer";
    DepthDesc.Format      = DepthBufferFormat;
    DepthDesc.BindFlags   = BIND_DEPTH_STENCIL;
    // Define optimal clear value
    DepthDesc.ClearValue.Format               = DepthDesc.Format;
    DepthDesc.ClearValue.DepthStencil.Depth   = 1;
    DepthDesc.ClearValue.DepthStencil.Stencil = 0;

    RefCntAutoPtr<ITexture> pDepth;
    m_pDevice->CreateTexture(DepthDesc, nullptr, &pDepth);
    // Store the depth-stencil view
    m_pMSDepthDSV = pDepth->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL);
}

// Render a frame
void Tutorial17_MSAA::Render()
{
    const float ClearColor[] = {0.125f, 0.125f, 0.125f, 1.0f};

    ITextureView* pRTV = nullptr;
    ITextureView* pDSV = nullptr;
    if (m_SampleCount > 1)
    {
        // Set off-screen multi-sampled render target and depth-stencil buffer
        pRTV = m_pMSColorRTV;
        pDSV = m_pMSDepthDSV;
    }
    else
    {
        // Render directly to the current swap chain back buffer.
        pRTV = m_pSwapChain->GetCurrentBackBufferRTV();
        pDSV = m_pSwapChain->GetDepthBufferDSV();
    }

    m_pImmediateContext->SetRenderTargets(1, &pRTV, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    m_pImmediateContext->ClearRenderTarget(pRTV, ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    m_pImmediateContext->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1.0f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    {
        // Map the cube's constant buffer and fill it in with its view-projection matrix
        MapHelper<float4x4> CBConstants(m_pImmediateContext, m_CubeVSConstants, MAP_WRITE, MAP_FLAG_DISCARD);
        *CBConstants = m_WorldViewProjMatrix.Transpose();
    }

    // Bind vertex and index buffers
    Uint32   offset   = 0;
    IBuffer* pBuffs[] = {m_CubeVertexBuffer};
    m_pImmediateContext->SetVertexBuffers(0, 1, pBuffs, &offset, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    m_pImmediateContext->SetIndexBuffer(m_CubeIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    // Set the cube's pipeline state
    m_pImmediateContext->SetPipelineState(m_pCubePSO);

    // Commit the cube shader's resources
    m_pImmediateContext->CommitShaderResources(m_pCubeSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    // Draw the grid
    DrawIndexedAttribs DrawAttrs;
    DrawAttrs.IndexType    = VT_UINT32; // Index type
    DrawAttrs.NumIndices   = 36;
    DrawAttrs.NumInstances = 49;
    DrawAttrs.Flags        = DRAW_FLAG_VERIFY_ALL; // Verify the state of vertex and index buffers
    m_pImmediateContext->DrawIndexed(DrawAttrs);

    if (m_SampleCount > 1)
    {
        // Resolve multi-sampled render taget into the current swap chain back buffer.
        auto pCurrentBackBuffer = m_pSwapChain->GetCurrentBackBufferRTV()->GetTexture();

        ResolveTextureSubresourceAttribs ResolveAttribs;
        ResolveAttribs.SrcTextureTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
        ResolveAttribs.DstTextureTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
        m_pImmediateContext->ResolveTextureSubresource(m_pMSColorRTV->GetTexture(), pCurrentBackBuffer, ResolveAttribs);
    }
}

void Tutorial17_MSAA::Update(double CurrTime, double ElapsedTime)
{
    SampleBase::Update(CurrTime, ElapsedTime);
    UpdateUI();

    if (m_bRotateGrid)
        m_fCurrentTime += static_cast<float>(ElapsedTime);

    // Set cube rotation
    float4x4 Model = float4x4::RotationZ(m_fCurrentTime * 0.1f);

    float4x4 View = float4x4::Translation(0.0f, 0.0f, 30.0f);

    // Get pretransform matrix that rotates the scene according the surface orientation
    auto SrfPreTransform = GetSurfacePretransformMatrix(float3{0, 0, 1});

    // Get projection matrix adjusted to the current screen orientation
    auto Proj = GetAdjustedProjectionMatrix(PI_F / 4.0f, 0.1f, 100.f);

    // Compute world-view-projection matrix
    m_WorldViewProjMatrix = Model * View * SrfPreTransform * Proj;
}

} // namespace Diligent
