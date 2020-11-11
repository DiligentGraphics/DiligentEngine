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

#include "DeviceContextD3D12.h"

#include "InlineShaders/MeshShaderTestHLSL.h"

namespace Diligent
{

namespace Testing
{

#ifdef D3D12_H_HAS_MESH_SHADER

namespace
{
#    ifdef _MSC_VER
#        pragma warning(push)
#        pragma warning(disable : 4324)
#    endif

template <typename InnerStructType, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE SubObjType>
struct alignas(void*) PSS_SubObject
{
    const D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type{SubObjType};
    InnerStructType                           Obj{};

    PSS_SubObject() {}

    PSS_SubObject& operator=(const InnerStructType& obj)
    {
        Obj = obj;
        return *this;
    }

    InnerStructType* operator->() { return &Obj; }
    InnerStructType* operator&() { return &Obj; }
    InnerStructType& operator*() { return Obj; }
};

#    ifdef _MSC_VER
#        pragma warning(pop)
#    endif

struct MESH_SHADER_PIPELINE_STATE_DESC
{
    PSS_SubObject<D3D12_PIPELINE_STATE_FLAGS, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_FLAGS>            Flags;
    PSS_SubObject<UINT, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_NODE_MASK>                              NodeMask;
    PSS_SubObject<ID3D12RootSignature*, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE>         pRootSignature;
    PSS_SubObject<D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS>                    PS;
    PSS_SubObject<D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS>                    AS;
    PSS_SubObject<D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS>                    MS;
    PSS_SubObject<D3D12_BLEND_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND>                      BlendState;
    PSS_SubObject<D3D12_DEPTH_STENCIL_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL>      DepthStencilState;
    PSS_SubObject<D3D12_RASTERIZER_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER>            RasterizerState;
    PSS_SubObject<DXGI_SAMPLE_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC>                SampleDesc;
    PSS_SubObject<UINT, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK>                            SampleMask;
    PSS_SubObject<DXGI_FORMAT, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT>            DSVFormat;
    PSS_SubObject<D3D12_RT_FORMAT_ARRAY, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS> RTVFormatArray;
    PSS_SubObject<D3D12_CACHED_PIPELINE_STATE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CACHED_PSO>      CachedPSO;
};

static void CreateMeshPipeline(ID3D12Device* pd3d12Device, ID3DBlob* pAS, ID3DBlob* pMS, ID3DBlob* pPS, DXGI_FORMAT ColorFormat, D3D12_FILL_MODE FillMode, CComPtr<ID3D12RootSignature>& pd3d12RootSignature, CComPtr<ID3D12PipelineState>& pd3d12PSO)
{
    MESH_SHADER_PIPELINE_STATE_DESC d3d12PSODesc = {};

    if (pAS)
    {
        d3d12PSODesc.AS->pShaderBytecode = pAS->GetBufferPointer();
        d3d12PSODesc.AS->BytecodeLength  = pAS->GetBufferSize();
    }

    if (pMS)
    {
        d3d12PSODesc.MS->pShaderBytecode = pMS->GetBufferPointer();
        d3d12PSODesc.MS->BytecodeLength  = pMS->GetBufferSize();
    }

    if (pPS)
    {
        d3d12PSODesc.PS->pShaderBytecode = pPS->GetBufferPointer();
        d3d12PSODesc.PS->BytecodeLength  = pPS->GetBufferSize();
    }

    D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc = {};
    CComPtr<ID3DBlob>         signature;

    D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, nullptr);
    pd3d12Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), __uuidof(pd3d12RootSignature), reinterpret_cast<void**>(static_cast<ID3D12RootSignature**>(&pd3d12RootSignature)));

    d3d12PSODesc.pRootSignature = pd3d12RootSignature;

    d3d12PSODesc.BlendState        = CD3DX12_BLEND_DESC{D3D12_DEFAULT};
    d3d12PSODesc.RasterizerState   = CD3DX12_RASTERIZER_DESC{D3D12_DEFAULT};
    d3d12PSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC{D3D12_DEFAULT};

    d3d12PSODesc.RasterizerState->CullMode         = D3D12_CULL_MODE_BACK;
    d3d12PSODesc.RasterizerState->FillMode         = FillMode;
    d3d12PSODesc.DepthStencilState->DepthEnable    = FALSE;
    d3d12PSODesc.DepthStencilState->DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

    d3d12PSODesc.SampleMask = 0xFFFFFFFF;

    d3d12PSODesc.RTVFormatArray->NumRenderTargets = 1;
    d3d12PSODesc.RTVFormatArray->RTFormats[0]     = ColorFormat;
    d3d12PSODesc.DSVFormat                        = DXGI_FORMAT_UNKNOWN;
    d3d12PSODesc.SampleDesc->Count                = 1;
    d3d12PSODesc.SampleDesc->Quality              = 0;
    d3d12PSODesc.NodeMask                         = 0;
    d3d12PSODesc.Flags                            = D3D12_PIPELINE_STATE_FLAG_NONE;

    d3d12PSODesc.CachedPSO->pCachedBlob           = nullptr;
    d3d12PSODesc.CachedPSO->CachedBlobSizeInBytes = 0;

    D3D12_PIPELINE_STATE_STREAM_DESC streamDesc;
    streamDesc.SizeInBytes                   = sizeof(d3d12PSODesc);
    streamDesc.pPipelineStateSubobjectStream = &d3d12PSODesc;

    CComPtr<ID3D12Device2> device2;
    auto                   hr = pd3d12Device->QueryInterface(IID_PPV_ARGS(&device2));
    ASSERT_HRESULT_SUCCEEDED(hr) << "Failed to get ID3D12Device2";

    hr = device2->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&pd3d12PSO));
    ASSERT_HRESULT_SUCCEEDED(hr) << "Failed to create pipeline state";
}

} // namespace

void MeshShaderDrawReferenceD3D12(ISwapChain* pSwapChain)
{
    auto* pEnv                   = TestingEnvironmentD3D12::GetInstance();
    auto* pContext               = pEnv->GetDeviceContext();
    auto* pd3d12Device           = pEnv->GetD3D12Device();
    auto* pTestingSwapChainD3D12 = ValidatedCast<TestingSwapChainD3D12>(pSwapChain);

    const auto& SCDesc = pSwapChain->GetDesc();

    CComPtr<ID3DBlob> pMSByteCode, pPSByteCode;

    auto hr = pEnv->CompileDXILShader(HLSL::MeshShaderTest_MS, L"main", nullptr, 0, L"ms_6_5", &pMSByteCode);
    ASSERT_HRESULT_SUCCEEDED(hr) << "Failed to compile mesh shader";

    hr = pEnv->CompileDXILShader(HLSL::MeshShaderTest_PS, L"main", nullptr, 0, L"ps_6_5", &pPSByteCode);
    ASSERT_HRESULT_SUCCEEDED(hr) << "Failed to compile pixel shader";

    CComPtr<ID3D12RootSignature> pd3d12RootSignature;
    CComPtr<ID3D12PipelineState> pd3d12PSO;
    CreateMeshPipeline(pd3d12Device, nullptr, pMSByteCode, pPSByteCode, TexFormatToDXGI_Format(SCDesc.ColorBufferFormat), D3D12_FILL_MODE_SOLID,
                       /*out*/ pd3d12RootSignature, /*out*/ pd3d12PSO);

    auto pCmdList = pEnv->CreateGraphicsCommandList();

    CComPtr<ID3D12GraphicsCommandList6> pCmdList6;
    ASSERT_HRESULT_SUCCEEDED(pCmdList->QueryInterface(IID_PPV_ARGS(&pCmdList6))) << "Failed to get ID3D12GraphicsCommandList6";

    pTestingSwapChainD3D12->TransitionRenderTarget(pCmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    D3D12_VIEWPORT d3d12VP = {};
    d3d12VP.Width          = static_cast<float>(SCDesc.Width);
    d3d12VP.Height         = static_cast<float>(SCDesc.Height);
    d3d12VP.MaxDepth       = 1;
    pCmdList->RSSetViewports(1, &d3d12VP);

    D3D12_RECT Rect = {0, 0, static_cast<LONG>(SCDesc.Width), static_cast<LONG>(SCDesc.Height)};
    pCmdList->RSSetScissorRects(1, &Rect);

    auto RTVDesriptorHandle = pTestingSwapChainD3D12->GetRTVDescriptorHandle();

    pCmdList->OMSetRenderTargets(1, &RTVDesriptorHandle, FALSE, nullptr);

    float ClearColor[] = {0.f, 0.f, 0.f, 0.f};
    pCmdList->ClearRenderTargetView(RTVDesriptorHandle, ClearColor, 0, nullptr);

    pCmdList->SetPipelineState(pd3d12PSO);
    pCmdList->SetGraphicsRootSignature(pd3d12RootSignature);
    pCmdList6->DispatchMesh(1, 1, 1);

    pCmdList->Close();
    ID3D12CommandList* pCmdLits[] = {pCmdList};

    RefCntAutoPtr<IDeviceContextD3D12> pContextD3D12{pContext, IID_DeviceContextD3D12};

    auto* pQeueD3D12  = pContextD3D12->LockCommandQueue();
    auto* pd3d12Queue = pQeueD3D12->GetD3D12CommandQueue();

    pd3d12Queue->ExecuteCommandLists(_countof(pCmdLits), pCmdLits);
    pEnv->IdleCommandQueue(pd3d12Queue);

    pContextD3D12->UnlockCommandQueue();
}


void MeshShaderIndirectDrawReferenceD3D12(ISwapChain* pSwapChain)
{
    auto* pEnv                   = TestingEnvironmentD3D12::GetInstance();
    auto* pContext               = pEnv->GetDeviceContext();
    auto* pd3d12Device           = pEnv->GetD3D12Device();
    auto* pTestingSwapChainD3D12 = ValidatedCast<TestingSwapChainD3D12>(pSwapChain);

    const auto& SCDesc = pSwapChain->GetDesc();

    CComPtr<ID3DBlob> pMSByteCode, pPSByteCode;

    auto hr = pEnv->CompileDXILShader(HLSL::MeshShaderTest_MS, L"main", nullptr, 0, L"ms_6_5", &pMSByteCode);
    ASSERT_HRESULT_SUCCEEDED(hr) << "Failed to compile mesh shader";

    hr = pEnv->CompileDXILShader(HLSL::MeshShaderTest_PS, L"main", nullptr, 0, L"ps_6_5", &pPSByteCode);
    ASSERT_HRESULT_SUCCEEDED(hr) << "Failed to compile pixel shader";

    CComPtr<ID3D12RootSignature> pd3d12RootSignature;
    CComPtr<ID3D12PipelineState> pd3d12PSO;
    CreateMeshPipeline(pd3d12Device, nullptr, pMSByteCode, pPSByteCode, TexFormatToDXGI_Format(SCDesc.ColorBufferFormat), D3D12_FILL_MODE_SOLID,
                       /*out*/ pd3d12RootSignature, /*out*/ pd3d12PSO);

    D3D12_COMMAND_SIGNATURE_DESC    CmdSignatureDesc = {};
    D3D12_INDIRECT_ARGUMENT_DESC    IndirectArg      = {};
    CComPtr<ID3D12CommandSignature> pDrawMeshSignature;

    CmdSignatureDesc.NodeMask         = 0;
    CmdSignatureDesc.NumArgumentDescs = 1;
    CmdSignatureDesc.pArgumentDescs   = &IndirectArg;
    CmdSignatureDesc.ByteStride       = sizeof(UINT) * 3;
    IndirectArg.Type                  = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH;
    hr                                = pd3d12Device->CreateCommandSignature(&CmdSignatureDesc, nullptr, IID_PPV_ARGS(&pDrawMeshSignature));

    ASSERT_HRESULT_SUCCEEDED(hr) << "Failed to create draw mesh indirect command signature";

    Uint32 IndirectBufferData[3] = {1, 1, 1};

    BufferDesc IndirectBufferDesc;
    IndirectBufferDesc.Name          = "Indirect buffer";
    IndirectBufferDesc.Usage         = USAGE_IMMUTABLE;
    IndirectBufferDesc.uiSizeInBytes = sizeof(IndirectBufferData);
    IndirectBufferDesc.BindFlags     = BIND_INDIRECT_DRAW_ARGS;

    BufferData InitData;
    InitData.pData    = &IndirectBufferData;
    InitData.DataSize = IndirectBufferDesc.uiSizeInBytes;

    RefCntAutoPtr<IBuffer> pBuffer;
    pEnv->GetDevice()->CreateBuffer(IndirectBufferDesc, &InitData, &pBuffer);

    auto* pIndirectBuffer = static_cast<ID3D12Resource*>(pBuffer->GetNativeHandle());

    auto pCmdList = pEnv->CreateGraphicsCommandList();
    pTestingSwapChainD3D12->TransitionRenderTarget(pCmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    D3D12_VIEWPORT d3d12VP = {};
    d3d12VP.Width          = static_cast<float>(SCDesc.Width);
    d3d12VP.Height         = static_cast<float>(SCDesc.Height);
    d3d12VP.MaxDepth       = 1;
    pCmdList->RSSetViewports(1, &d3d12VP);

    D3D12_RECT Rect = {0, 0, static_cast<LONG>(SCDesc.Width), static_cast<LONG>(SCDesc.Height)};
    pCmdList->RSSetScissorRects(1, &Rect);

    auto RTVDesriptorHandle = pTestingSwapChainD3D12->GetRTVDescriptorHandle();

    pCmdList->OMSetRenderTargets(1, &RTVDesriptorHandle, FALSE, nullptr);

    float ClearColor[] = {0.f, 0.f, 0.f, 0.f};
    pCmdList->ClearRenderTargetView(RTVDesriptorHandle, ClearColor, 0, nullptr);

    pCmdList->SetPipelineState(pd3d12PSO);
    pCmdList->SetGraphicsRootSignature(pd3d12RootSignature);
    pCmdList->ExecuteIndirect(pDrawMeshSignature, 1, pIndirectBuffer, 0, nullptr, 0);

    pCmdList->Close();
    ID3D12CommandList* pCmdLits[] = {pCmdList};

    RefCntAutoPtr<IDeviceContextD3D12> pContextD3D12{pContext, IID_DeviceContextD3D12};

    auto* pQeueD3D12  = pContextD3D12->LockCommandQueue();
    auto* pd3d12Queue = pQeueD3D12->GetD3D12CommandQueue();

    pd3d12Queue->ExecuteCommandLists(_countof(pCmdLits), pCmdLits);
    pEnv->IdleCommandQueue(pd3d12Queue);

    pContextD3D12->UnlockCommandQueue();
}


void AmplificationShaderDrawReferenceD3D12(ISwapChain* pSwapChain)
{
    auto* pEnv                   = TestingEnvironmentD3D12::GetInstance();
    auto* pContext               = pEnv->GetDeviceContext();
    auto* pd3d12Device           = pEnv->GetD3D12Device();
    auto* pTestingSwapChainD3D12 = ValidatedCast<TestingSwapChainD3D12>(pSwapChain);

    const auto& SCDesc = pSwapChain->GetDesc();

    CComPtr<ID3DBlob> pASByteCode, pMSByteCode, pPSByteCode;

    auto hr = pEnv->CompileDXILShader(HLSL::AmplificationShaderTest_AS, L"main", nullptr, 0, L"as_6_5", &pASByteCode);
    ASSERT_HRESULT_SUCCEEDED(hr) << "Failed to compile amplification shader";

    hr = pEnv->CompileDXILShader(HLSL::AmplificationShaderTest_MS, L"main", nullptr, 0, L"ms_6_5", &pMSByteCode);
    ASSERT_HRESULT_SUCCEEDED(hr) << "Failed to compile mesh shader";

    hr = pEnv->CompileDXILShader(HLSL::AmplificationShaderTest_PS, L"main", nullptr, 0, L"ps_6_5", &pPSByteCode);
    ASSERT_HRESULT_SUCCEEDED(hr) << "Failed to compile pixel shader";

    CComPtr<ID3D12RootSignature> pd3d12RootSignature;
    CComPtr<ID3D12PipelineState> pd3d12PSO;
    CreateMeshPipeline(pd3d12Device, pASByteCode, pMSByteCode, pPSByteCode, TexFormatToDXGI_Format(SCDesc.ColorBufferFormat), D3D12_FILL_MODE_SOLID,
                       /*out*/ pd3d12RootSignature, /*out*/ pd3d12PSO);

    auto pCmdList = pEnv->CreateGraphicsCommandList();

    CComPtr<ID3D12GraphicsCommandList6> pCmdList6;
    ASSERT_HRESULT_SUCCEEDED(pCmdList->QueryInterface(IID_PPV_ARGS(&pCmdList6))) << "Failed to get ID3D12GraphicsCommandList6";

    pTestingSwapChainD3D12->TransitionRenderTarget(pCmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    D3D12_VIEWPORT d3d12VP = {};
    d3d12VP.Width          = static_cast<float>(SCDesc.Width);
    d3d12VP.Height         = static_cast<float>(SCDesc.Height);
    d3d12VP.MaxDepth       = 1;
    pCmdList->RSSetViewports(1, &d3d12VP);

    D3D12_RECT Rect = {0, 0, static_cast<LONG>(SCDesc.Width), static_cast<LONG>(SCDesc.Height)};
    pCmdList->RSSetScissorRects(1, &Rect);

    auto RTVDesriptorHandle = pTestingSwapChainD3D12->GetRTVDescriptorHandle();

    pCmdList->OMSetRenderTargets(1, &RTVDesriptorHandle, FALSE, nullptr);

    float ClearColor[] = {0.f, 0.f, 0.f, 0.f};
    pCmdList->ClearRenderTargetView(RTVDesriptorHandle, ClearColor, 0, nullptr);

    pCmdList->SetPipelineState(pd3d12PSO);
    pCmdList->SetGraphicsRootSignature(pd3d12RootSignature);
    pCmdList6->DispatchMesh(8, 1, 1);

    pCmdList->Close();
    ID3D12CommandList* pCmdLits[] = {pCmdList};

    RefCntAutoPtr<IDeviceContextD3D12> pContextD3D12{pContext, IID_DeviceContextD3D12};

    auto* pQeueD3D12  = pContextD3D12->LockCommandQueue();
    auto* pd3d12Queue = pQeueD3D12->GetD3D12CommandQueue();

    pd3d12Queue->ExecuteCommandLists(_countof(pCmdLits), pCmdLits);
    pEnv->IdleCommandQueue(pd3d12Queue);

    pContextD3D12->UnlockCommandQueue();
}

#else

void MeshShaderDrawReferenceD3D12(ISwapChain* pSwapChain)
{
    LOG_ERROR_AND_THROW("Direct12 headers don't support mesh shader");
}

void MeshShaderIndirectDrawReferenceD3D12(ISwapChain* pSwapChain)
{
    LOG_ERROR_AND_THROW("Direct12 headers don't support mesh shader");
}

void AmplificationShaderDrawReferenceD3D12(ISwapChain* pSwapChain)
{
    LOG_ERROR_AND_THROW("Direct12 headers don't support mesh shader");
}

#endif // D3D12_H_HAS_MESH_SHADER

} // namespace Testing

} // namespace Diligent
