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

#include "DeviceContextD3D12.h"

#include "InlineShaders/ComputeShaderTestHLSL.h"

namespace Diligent
{

namespace Testing
{

void ComputeShaderReferenceD3D12(ISwapChain* pSwapChain)
{
    auto* pEnv                   = TestingEnvironmentD3D12::GetInstance();
    auto* pContext               = pEnv->GetDeviceContext();
    auto* pd3d12Device           = pEnv->GetD3D12Device();
    auto* pTestingSwapChainD3D12 = ValidatedCast<TestingSwapChainD3D12>(pSwapChain);

    const auto& SCDesc = pSwapChain->GetDesc();

    CComPtr<ID3DBlob> pCSByteCode;

    auto hr = CompileD3DShader(HLSL::FillTextureCS, "main", nullptr, "cs_5_0", &pCSByteCode);
    ASSERT_HRESULT_SUCCEEDED(hr) << "Failed to compile compute shader";

    D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc = {};

    D3D12_ROOT_PARAMETER Param = {};
    Param.ParameterType        = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    Param.ShaderVisibility     = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_DESCRIPTOR_RANGE DescriptorRange = {};

    DescriptorRange.RegisterSpace                     = 0;
    DescriptorRange.BaseShaderRegister                = 0;
    DescriptorRange.NumDescriptors                    = 1;
    DescriptorRange.OffsetInDescriptorsFromTableStart = 0;
    DescriptorRange.RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;

    Param.DescriptorTable.NumDescriptorRanges = 1;
    Param.DescriptorTable.pDescriptorRanges   = &DescriptorRange;

    RootSignatureDesc.Flags         = D3D12_ROOT_SIGNATURE_FLAG_NONE;
    RootSignatureDesc.NumParameters = 1;
    RootSignatureDesc.pParameters   = &Param;

    CComPtr<ID3DBlob>            signature;
    CComPtr<ID3D12RootSignature> pd3d12RootSignature;
    hr = D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, nullptr);
    ASSERT_HRESULT_SUCCEEDED(hr);

    hr = pd3d12Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), __uuidof(pd3d12RootSignature), reinterpret_cast<void**>(static_cast<ID3D12RootSignature**>(&pd3d12RootSignature)));
    ASSERT_HRESULT_SUCCEEDED(hr);

    D3D12_COMPUTE_PIPELINE_STATE_DESC PSODesc = {};

    PSODesc.pRootSignature     = pd3d12RootSignature;
    PSODesc.CS.pShaderBytecode = pCSByteCode->GetBufferPointer();
    PSODesc.CS.BytecodeLength  = pCSByteCode->GetBufferSize();

    CComPtr<ID3D12PipelineState> pd3d12PSO;
    hr = pd3d12Device->CreateComputePipelineState(&PSODesc, __uuidof(pd3d12PSO), reinterpret_cast<void**>(static_cast<ID3D12PipelineState**>(&pd3d12PSO)));
    ASSERT_HRESULT_SUCCEEDED(hr) << "Failed to create pipeline state";

    auto pCmdList = pEnv->CreateGraphicsCommandList();

    pCmdList->SetPipelineState(pd3d12PSO);
    pCmdList->SetComputeRootSignature(pd3d12RootSignature);

    ID3D12DescriptorHeap* pHeaps[] = {pTestingSwapChainD3D12->GetUAVDescriptorHeap()};
    pCmdList->SetDescriptorHeaps(_countof(pHeaps), pHeaps);

    pCmdList->SetComputeRootDescriptorTable(0, pHeaps[0]->GetGPUDescriptorHandleForHeapStart());
    pCmdList->Dispatch((SCDesc.Width + 15) / 16, (SCDesc.Height + 15) / 16, 1);

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
