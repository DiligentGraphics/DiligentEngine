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

#include "D3D12/TestingEnvironmentD3D12.hpp"
#include "D3D12/TestingSwapChainD3D12.hpp"
#include "../include/DXGITypeConversions.hpp"
#include "../include/d3dx12_win.h"

#include "RenderDeviceD3D12.h"
#include "DeviceContextD3D12.h"

#include "InlineShaders/DrawCommandTestHLSL.h"

namespace Diligent
{

namespace Testing
{

namespace
{
class TriangleRenderer
{
public:
    TriangleRenderer(ID3D12Device*                    pd3d12Device,
                     const std::string&               PSSource,
                     DXGI_FORMAT                      RTVFmt,
                     UINT                             SampleCount,
                     const D3D12_ROOT_SIGNATURE_DESC& RootSignatureDesc)
    {
        CComPtr<ID3DBlob> pVSByteCode, pPSByteCode;

        auto hr = CompileD3DShader(HLSL::DrawTest_ProceduralTriangleVS, "main", nullptr, "vs_5_0", &pVSByteCode);
        VERIFY_EXPR(SUCCEEDED(hr));

        hr = CompileD3DShader(PSSource, "main", nullptr, "ps_5_0", &pPSByteCode);
        VERIFY_EXPR(SUCCEEDED(hr));

        CComPtr<ID3DBlob> signature;
        D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, nullptr);
        pd3d12Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), __uuidof(m_pd3d12RootSignature), reinterpret_cast<void**>(static_cast<ID3D12RootSignature**>(&m_pd3d12RootSignature)));

        D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc = {};

        PSODesc.pRootSignature     = m_pd3d12RootSignature;
        PSODesc.VS.pShaderBytecode = pVSByteCode->GetBufferPointer();
        PSODesc.VS.BytecodeLength  = pVSByteCode->GetBufferSize();
        PSODesc.PS.pShaderBytecode = pPSByteCode->GetBufferPointer();
        PSODesc.PS.BytecodeLength  = pPSByteCode->GetBufferSize();

        PSODesc.BlendState        = CD3DX12_BLEND_DESC{D3D12_DEFAULT};
        PSODesc.RasterizerState   = CD3DX12_RASTERIZER_DESC{D3D12_DEFAULT};
        PSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC{D3D12_DEFAULT};

        PSODesc.RasterizerState.CullMode         = D3D12_CULL_MODE_NONE;
        PSODesc.DepthStencilState.DepthEnable    = FALSE;
        PSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

        PSODesc.SampleMask = 0xFFFFFFFF;

        PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        PSODesc.NumRenderTargets      = 1;
        PSODesc.RTVFormats[0]         = RTVFmt;
        PSODesc.SampleDesc.Count      = SampleCount;
        PSODesc.SampleDesc.Quality    = 0;
        PSODesc.NodeMask              = 0;
        PSODesc.Flags                 = D3D12_PIPELINE_STATE_FLAG_NONE;

        hr = pd3d12Device->CreateGraphicsPipelineState(&PSODesc, __uuidof(m_pd3d12PSO), reinterpret_cast<void**>(static_cast<ID3D12PipelineState**>(&m_pd3d12PSO)));
        VERIFY_EXPR(SUCCEEDED(hr));
    }

    void Draw(ID3D12GraphicsCommandList* pCmdList, Uint32 ViewportWidth, Uint32 ViewportHeight, D3D12_GPU_DESCRIPTOR_HANDLE DescriptorTable = D3D12_GPU_DESCRIPTOR_HANDLE{})
    {
        D3D12_VIEWPORT d3d12VP = {};
        d3d12VP.Width          = static_cast<float>(ViewportWidth);
        d3d12VP.Height         = static_cast<float>(ViewportHeight);
        d3d12VP.MaxDepth       = 1;
        pCmdList->RSSetViewports(1, &d3d12VP);
        D3D12_RECT Rect = {0, 0, static_cast<LONG>(ViewportWidth), static_cast<LONG>(ViewportHeight)};
        pCmdList->RSSetScissorRects(1, &Rect);

        pCmdList->SetPipelineState(m_pd3d12PSO);
        pCmdList->SetGraphicsRootSignature(m_pd3d12RootSignature);
        if (DescriptorTable.ptr != 0)
            pCmdList->SetGraphicsRootDescriptorTable(0, DescriptorTable);
        pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        pCmdList->DrawInstanced(6, 1, 0, 0);
    }

private:
    CComPtr<ID3D12RootSignature> m_pd3d12RootSignature;
    CComPtr<ID3D12PipelineState> m_pd3d12PSO;
};
} // namespace

void RenderDrawCommandReferenceD3D12(ISwapChain* pSwapChain, const float* pClearColor)
{
    auto* pEnv                   = TestingEnvironmentD3D12::GetInstance();
    auto* pContext               = pEnv->GetDeviceContext();
    auto* pd3d12Device           = pEnv->GetD3D12Device();
    auto* pTestingSwapChainD3D12 = ValidatedCast<TestingSwapChainD3D12>(pSwapChain);

    const auto& SCDesc = pSwapChain->GetDesc();

    D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc = {};
    RootSignatureDesc.Flags                     = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    TriangleRenderer TriRenderer{pd3d12Device, HLSL::DrawTest_PS, TexFormatToDXGI_Format(SCDesc.ColorBufferFormat), 1, RootSignatureDesc};

    auto pCmdList = pEnv->CreateGraphicsCommandList();
    pTestingSwapChainD3D12->TransitionRenderTarget(pCmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    auto RTVDesriptorHandle = pTestingSwapChainD3D12->GetRTVDescriptorHandle();
    pCmdList->OMSetRenderTargets(1, &RTVDesriptorHandle, FALSE, nullptr);

    float Zero[] = {0, 0, 0, 0};
    pCmdList->ClearRenderTargetView(RTVDesriptorHandle, pClearColor != nullptr ? pClearColor : Zero, 0, nullptr);

    TriRenderer.Draw(pCmdList, SCDesc.Width, SCDesc.Height);

    pCmdList->Close();
    ID3D12CommandList* pCmdLits[] = {pCmdList};

    RefCntAutoPtr<IDeviceContextD3D12> pContextD3D12{pContext, IID_DeviceContextD3D12};

    auto* pQeueD3D12  = pContextD3D12->LockCommandQueue();
    auto* pd3d12Queue = pQeueD3D12->GetD3D12CommandQueue();

    pd3d12Queue->ExecuteCommandLists(_countof(pCmdLits), pCmdLits);
    pEnv->IdleCommandQueue(pd3d12Queue);

    pContextD3D12->UnlockCommandQueue();
}

void RenderPassMSResolveReferenceD3D12(ISwapChain* pSwapChain, const float* pClearColor)
{
    auto* pEnv                   = TestingEnvironmentD3D12::GetInstance();
    auto* pContext               = pEnv->GetDeviceContext();
    auto* pd3d12Device           = pEnv->GetD3D12Device();
    auto* pTestingSwapChainD3D12 = ValidatedCast<TestingSwapChainD3D12>(pSwapChain);

    const auto& SCDesc = pSwapChain->GetDesc();

    D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc = {};
    RootSignatureDesc.Flags                     = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    TriangleRenderer TriRenderer{pd3d12Device, HLSL::DrawTest_PS, TexFormatToDXGI_Format(SCDesc.ColorBufferFormat), 4, RootSignatureDesc};

    // Create multisample texture
    D3D12_HEAP_PROPERTIES HeapProps = {};
    HeapProps.Type                  = D3D12_HEAP_TYPE_DEFAULT;
    HeapProps.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProps.MemoryPoolPreference  = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProps.CreationNodeMask      = 1;
    HeapProps.VisibleNodeMask       = 1;

    D3D12_RESOURCE_DESC TexDesc = {};
    TexDesc.Dimension           = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    TexDesc.Alignment           = 0;
    TexDesc.Width               = SCDesc.Width;
    TexDesc.Height              = SCDesc.Height;
    TexDesc.DepthOrArraySize    = 1;
    TexDesc.MipLevels           = 1;
    TexDesc.Format              = TexFormatToDXGI_Format(SCDesc.ColorBufferFormat);
    TexDesc.SampleDesc.Count    = 4;
    TexDesc.SampleDesc.Quality  = 0;
    TexDesc.Layout              = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    TexDesc.Flags               = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    float Zero[4] = {};
    if (pClearColor == nullptr)
        pClearColor = Zero;

    D3D12_CLEAR_VALUE ClearColorValue = {};

    ClearColorValue.Format = TexDesc.Format;
    for (Uint32 i = 0; i < 4; ++i)
        ClearColorValue.Color[i] = pClearColor[i];

    RefCntAutoPtr<ID3D12Resource> pd3d12MSTex;

    auto hr = pd3d12Device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &TexDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &ClearColorValue,
                                                    __uuidof(pd3d12MSTex),
                                                    reinterpret_cast<void**>(static_cast<ID3D12Resource**>(&pd3d12MSTex)));
    VERIFY(SUCCEEDED(hr), "Failed to create D3D12 MS render target texture");


    // Create RTV descriptor heap
    CComPtr<ID3D12DescriptorHeap> pd3d12RTVDescriptorHeap;

    D3D12_DESCRIPTOR_HEAP_DESC DescriptorHeapDesc = {};
    DescriptorHeapDesc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    DescriptorHeapDesc.NumDescriptors             = 1;
    DescriptorHeapDesc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    DescriptorHeapDesc.NodeMask                   = 0;

    hr = pd3d12Device->CreateDescriptorHeap(&DescriptorHeapDesc,
                                            __uuidof(pd3d12RTVDescriptorHeap),
                                            reinterpret_cast<void**>(static_cast<ID3D12DescriptorHeap**>(&pd3d12RTVDescriptorHeap)));
    // Init RTV descriptor handle
    VERIFY(SUCCEEDED(hr), "Failed to create D3D12 RTV descriptor heap");
    auto RTVDescriptorHandle = pd3d12RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    pd3d12Device->CreateRenderTargetView(pd3d12MSTex, nullptr, RTVDescriptorHandle);

    auto pCmdList = pEnv->CreateGraphicsCommandList();
    pTestingSwapChainD3D12->TransitionRenderTarget(pCmdList, D3D12_RESOURCE_STATE_RESOLVE_DEST);

    // Prepare render pass description
    D3D12_RENDER_PASS_RENDER_TARGET_DESC RenderPassRT    = {};
    RenderPassRT.cpuDescriptor                           = RTVDescriptorHandle;
    RenderPassRT.BeginningAccess.Type                    = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
    RenderPassRT.BeginningAccess.Clear.ClearValue.Format = TexDesc.Format;
    for (Uint32 i = 0; i < 4; ++i)
        RenderPassRT.BeginningAccess.Clear.ClearValue.Color[i] = pClearColor[i];

    RenderPassRT.EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE;
    auto& ResolveParams            = RenderPassRT.EndingAccess.Resolve;
    ResolveParams.pSrcResource     = pd3d12MSTex;
    ResolveParams.pDstResource     = pTestingSwapChainD3D12->GetD3D12RenderTarget();
    ResolveParams.SubresourceCount = 1;
    D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS SubresParams[1];
    SubresParams[0].SrcSubresource = 0;
    SubresParams[0].DstSubresource = 0;
    SubresParams[0].DstX           = 0;
    SubresParams[0].DstY           = 0;
    SubresParams[0].SrcRect.left   = 0;
    SubresParams[0].SrcRect.top    = 0;
    SubresParams[0].SrcRect.right  = static_cast<LONG>(TexDesc.Width);
    SubresParams[0].SrcRect.bottom = static_cast<LONG>(TexDesc.Height);

    ResolveParams.pSubresourceParameters = SubresParams;
    ResolveParams.Format                 = TexDesc.Format;
    ResolveParams.ResolveMode            = D3D12_RESOLVE_MODE_AVERAGE;
    ResolveParams.PreserveResolveSource  = FALSE;

    static_cast<ID3D12GraphicsCommandList4*>(pCmdList.p)->BeginRenderPass(1, &RenderPassRT, nullptr, D3D12_RENDER_PASS_FLAG_NONE);

    TriRenderer.Draw(pCmdList, SCDesc.Width, SCDesc.Height);

    static_cast<ID3D12GraphicsCommandList4*>(pCmdList.p)->EndRenderPass();

    pCmdList->Close();
    ID3D12CommandList* pCmdLits[] = {pCmdList};

    RefCntAutoPtr<IDeviceContextD3D12> pContextD3D12{pContext, IID_DeviceContextD3D12};

    auto* pQeueD3D12  = pContextD3D12->LockCommandQueue();
    auto* pd3d12Queue = pQeueD3D12->GetD3D12CommandQueue();

    pd3d12Queue->ExecuteCommandLists(_countof(pCmdLits), pCmdLits);
    pEnv->IdleCommandQueue(pd3d12Queue);

    pContextD3D12->UnlockCommandQueue();
}

void RenderPassInputAttachmentReferenceD3D12(ISwapChain* pSwapChain, const float* pClearColor)
{
    auto* pEnv                   = TestingEnvironmentD3D12::GetInstance();
    auto* pContext               = pEnv->GetDeviceContext();
    auto* pd3d12Device           = pEnv->GetD3D12Device();
    auto* pTestingSwapChainD3D12 = ValidatedCast<TestingSwapChainD3D12>(pSwapChain);

    const auto& SCDesc = pSwapChain->GetDesc();

    D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc = {};
    RootSignatureDesc.Flags                     = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    TriangleRenderer TriRenderer{pd3d12Device, HLSL::DrawTest_PS, TexFormatToDXGI_Format(SCDesc.ColorBufferFormat), 1, RootSignatureDesc};

    // Prepare root signature desc
    D3D12_ROOT_PARAMETER RootParams[1] = {};
    RootParams[0].ParameterType        = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

    D3D12_DESCRIPTOR_RANGE DescriptorRange[1]            = {};
    DescriptorRange[0].RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    DescriptorRange[0].NumDescriptors                    = 1;
    DescriptorRange[0].BaseShaderRegister                = 0;
    DescriptorRange[0].RegisterSpace                     = 0;
    DescriptorRange[0].OffsetInDescriptorsFromTableStart = 0;

    RootParams[0].DescriptorTable.NumDescriptorRanges = _countof(DescriptorRange);
    RootParams[0].DescriptorTable.pDescriptorRanges   = DescriptorRange;
    RootParams[0].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_PIXEL;

    RootSignatureDesc.NumParameters = _countof(RootParams);
    RootSignatureDesc.pParameters   = RootParams;

    TriangleRenderer TriRendererInptAtt{pd3d12Device, HLSL::InputAttachmentTest_PS, TexFormatToDXGI_Format(SCDesc.ColorBufferFormat), 1, RootSignatureDesc};

    // Create input attachment texture
    D3D12_HEAP_PROPERTIES HeapProps = {};
    HeapProps.Type                  = D3D12_HEAP_TYPE_DEFAULT;
    HeapProps.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProps.MemoryPoolPreference  = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProps.CreationNodeMask      = 1;
    HeapProps.VisibleNodeMask       = 1;

    D3D12_RESOURCE_DESC TexDesc = {};
    TexDesc.Dimension           = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    TexDesc.Alignment           = 0;
    TexDesc.Width               = SCDesc.Width;
    TexDesc.Height              = SCDesc.Height;
    TexDesc.DepthOrArraySize    = 1;
    TexDesc.MipLevels           = 1;
    TexDesc.Format              = TexFormatToDXGI_Format(SCDesc.ColorBufferFormat);
    TexDesc.SampleDesc.Count    = 1;
    TexDesc.SampleDesc.Quality  = 0;
    TexDesc.Layout              = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    TexDesc.Flags               = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    float Zero[4] = {};
    if (pClearColor == nullptr)
        pClearColor = Zero;

    D3D12_CLEAR_VALUE ClearColorValue = {};

    ClearColorValue.Format = TexDesc.Format;
    for (Uint32 i = 0; i < 4; ++i)
        ClearColorValue.Color[i] = 0;

    RefCntAutoPtr<ID3D12Resource> pd3d12Tex;

    auto hr = pd3d12Device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &TexDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &ClearColorValue,
                                                    __uuidof(pd3d12Tex),
                                                    reinterpret_cast<void**>(static_cast<ID3D12Resource**>(&pd3d12Tex)));
    VERIFY(SUCCEEDED(hr), "Failed to create D3D12 MS render target texture");


    // Create RTV descriptor heap
    CComPtr<ID3D12DescriptorHeap> pd3d12RTVDescriptorHeap;
    {
        D3D12_DESCRIPTOR_HEAP_DESC DescriptorHeapDesc = {};
        DescriptorHeapDesc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        DescriptorHeapDesc.NumDescriptors             = 1;
        DescriptorHeapDesc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        DescriptorHeapDesc.NodeMask                   = 0;

        hr = pd3d12Device->CreateDescriptorHeap(&DescriptorHeapDesc,
                                                __uuidof(pd3d12RTVDescriptorHeap),
                                                reinterpret_cast<void**>(static_cast<ID3D12DescriptorHeap**>(&pd3d12RTVDescriptorHeap)));
    }
    // Init RTV descriptor handle
    VERIFY(SUCCEEDED(hr), "Failed to create D3D12 RTV descriptor heap");
    auto RTVDescriptorHandle = pd3d12RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    pd3d12Device->CreateRenderTargetView(pd3d12Tex, nullptr, RTVDescriptorHandle);


    // Create SRV descriptor head
    CComPtr<ID3D12DescriptorHeap> pd3d12SRVDescriptorHeap;
    {
        D3D12_DESCRIPTOR_HEAP_DESC DescriptorHeapDesc = {};
        DescriptorHeapDesc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        DescriptorHeapDesc.NumDescriptors             = 1;
        DescriptorHeapDesc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        DescriptorHeapDesc.NodeMask                   = 0;

        hr = pd3d12Device->CreateDescriptorHeap(&DescriptorHeapDesc,
                                                __uuidof(pd3d12SRVDescriptorHeap),
                                                reinterpret_cast<void**>(static_cast<ID3D12DescriptorHeap**>(&pd3d12SRVDescriptorHeap)));
    }
    auto SrvCpuDescriptorHandle = pd3d12SRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    auto SrvGpuDescriptorHandle = pd3d12SRVDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
    pd3d12Device->CreateShaderResourceView(pd3d12Tex, nullptr, SrvCpuDescriptorHandle);

    auto  pCmdList  = pEnv->CreateGraphicsCommandList();
    auto* pCmdList4 = static_cast<ID3D12GraphicsCommandList4*>(pCmdList.p);

    {
        // Start the first subpass
        D3D12_RENDER_PASS_RENDER_TARGET_DESC RenderPassRT    = {};
        RenderPassRT.cpuDescriptor                           = RTVDescriptorHandle;
        RenderPassRT.BeginningAccess.Type                    = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
        RenderPassRT.BeginningAccess.Clear.ClearValue.Format = TexDesc.Format;
        for (Uint32 i = 0; i < 4; ++i)
            RenderPassRT.BeginningAccess.Clear.ClearValue.Color[i] = 0;

        RenderPassRT.EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;

        pCmdList4->BeginRenderPass(1, &RenderPassRT, nullptr, D3D12_RENDER_PASS_FLAG_NONE);

        TriRenderer.Draw(pCmdList, SCDesc.Width, SCDesc.Height);

        pCmdList4->EndRenderPass();
    }

    // Transition input attachment texture from render target to shader resource
    {
        D3D12_RESOURCE_BARRIER Barrier = {};
        Barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        Barrier.Transition.pResource   = pd3d12Tex;
        Barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        Barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        Barrier.Transition.Subresource = 0;
        pCmdList->ResourceBarrier(1, &Barrier);
    }

    pTestingSwapChainD3D12->TransitionRenderTarget(pCmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    {
        // Start the second subpass
        D3D12_RENDER_PASS_RENDER_TARGET_DESC RenderPassRT    = {};
        RenderPassRT.cpuDescriptor                           = pTestingSwapChainD3D12->GetRTVDescriptorHandle();
        RenderPassRT.BeginningAccess.Type                    = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
        RenderPassRT.BeginningAccess.Clear.ClearValue.Format = TexDesc.Format;
        for (Uint32 i = 0; i < 4; ++i)
            RenderPassRT.BeginningAccess.Clear.ClearValue.Color[i] = pClearColor[i];

        RenderPassRT.EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;

        pCmdList4->BeginRenderPass(1, &RenderPassRT, nullptr, D3D12_RENDER_PASS_FLAG_NONE);

        pCmdList4->SetDescriptorHeaps(1, &pd3d12SRVDescriptorHeap.p);
        TriRendererInptAtt.Draw(pCmdList, SCDesc.Width, SCDesc.Height, SrvGpuDescriptorHandle);

        pCmdList4->EndRenderPass();
    }

    pCmdList->Close();
    ID3D12CommandList* pCmdLits[] = {pCmdList};

    RefCntAutoPtr<IDeviceContextD3D12> pContextD3D12{pContext, IID_DeviceContextD3D12};

    auto* pQeueD3D12  = pContextD3D12->LockCommandQueue();
    auto* pd3d12Queue = pQeueD3D12->GetD3D12CommandQueue();

    pd3d12Queue->ExecuteCommandLists(_countof(pCmdLits), pCmdLits);
    pEnv->IdleCommandQueue(pd3d12Queue);

    pContextD3D12->UnlockCommandQueue();
}

} // namespace Testing

} // namespace Diligent
