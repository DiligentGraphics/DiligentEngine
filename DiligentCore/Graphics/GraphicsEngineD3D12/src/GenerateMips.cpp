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

// The source code in this file is derived from ColorBuffer.cpp and GraphicsCore.cpp developed by Minigraph
// Original source files header:

//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard
//


#include "pch.h"
#include "d3dx12_win.h"

#include "RenderDeviceD3D12Impl.hpp"
#include "GenerateMips.hpp"
#include "CommandContext.hpp"
#include "TextureViewD3D12Impl.hpp"
#include "TextureD3D12Impl.hpp"

#include "GenerateMips/GenerateMipsLinearCS.h"
#include "GenerateMips/GenerateMipsLinearOddCS.h"
#include "GenerateMips/GenerateMipsLinearOddXCS.h"
#include "GenerateMips/GenerateMipsLinearOddYCS.h"
#include "GenerateMips/GenerateMipsGammaCS.h"
#include "GenerateMips/GenerateMipsGammaOddCS.h"
#include "GenerateMips/GenerateMipsGammaOddXCS.h"
#include "GenerateMips/GenerateMipsGammaOddYCS.h"

namespace Diligent
{
GenerateMipsHelper::GenerateMipsHelper(ID3D12Device* pd3d12Device)
{
    CD3DX12_ROOT_PARAMETER Params[3];
    Params[0].InitAsConstants(6, 0);
    CD3DX12_DESCRIPTOR_RANGE SRVRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    Params[1].InitAsDescriptorTable(1, &SRVRange);
    CD3DX12_DESCRIPTOR_RANGE UAVRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 4, 0);
    Params[2].InitAsDescriptorTable(1, &UAVRange);
    CD3DX12_STATIC_SAMPLER_DESC SamplerLinearClampDesc(
        0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
    CD3DX12_ROOT_SIGNATURE_DESC RootSigDesc;
    RootSigDesc.NumParameters     = _countof(Params);
    RootSigDesc.pParameters       = Params;
    RootSigDesc.NumStaticSamplers = 1;
    RootSigDesc.pStaticSamplers   = &SamplerLinearClampDesc;
    RootSigDesc.Flags             = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    CComPtr<ID3DBlob> signature;
    CComPtr<ID3DBlob> error;

    HRESULT hr = D3D12SerializeRootSignature(&RootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);

    hr = pd3d12Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), __uuidof(m_pGenerateMipsRS), reinterpret_cast<void**>(static_cast<ID3D12RootSignature**>(&m_pGenerateMipsRS)));
    CHECK_D3D_RESULT_THROW(hr, "Failed to create root signature for mipmap generation");

    D3D12_COMPUTE_PIPELINE_STATE_DESC PSODesc = {};

    PSODesc.pRootSignature = m_pGenerateMipsRS;
    PSODesc.NodeMask       = 0;
    PSODesc.Flags          = D3D12_PIPELINE_STATE_FLAG_NONE;

#define CreatePSO(PSO, ShaderByteCode)                                                                                                                                  \
    PSODesc.CS.pShaderBytecode = ShaderByteCode;                                                                                                                        \
    PSODesc.CS.BytecodeLength  = sizeof(ShaderByteCode);                                                                                                                \
    hr                         = pd3d12Device->CreateComputePipelineState(&PSODesc, __uuidof(PSO), reinterpret_cast<void**>(static_cast<ID3D12PipelineState**>(&PSO))); \
    CHECK_D3D_RESULT_THROW(hr, "Failed to create Pipeline state for mipmap generation");                                                                                \
    PSO->SetName(L"Generate mips PSO");

    CreatePSO(m_pGenerateMipsLinearPSO[0], g_pGenerateMipsLinearCS);
    CreatePSO(m_pGenerateMipsLinearPSO[1], g_pGenerateMipsLinearOddXCS);
    CreatePSO(m_pGenerateMipsLinearPSO[2], g_pGenerateMipsLinearOddYCS);
    CreatePSO(m_pGenerateMipsLinearPSO[3], g_pGenerateMipsLinearOddCS);
    CreatePSO(m_pGenerateMipsGammaPSO[0], g_pGenerateMipsGammaCS);
    CreatePSO(m_pGenerateMipsGammaPSO[1], g_pGenerateMipsGammaOddXCS);
    CreatePSO(m_pGenerateMipsGammaPSO[2], g_pGenerateMipsGammaOddYCS);
    CreatePSO(m_pGenerateMipsGammaPSO[3], g_pGenerateMipsGammaOddCS);
}

void GenerateMipsHelper::GenerateMips(ID3D12Device* pd3d12Device, TextureViewD3D12Impl* pTexView, CommandContext& Ctx) const
{
    auto& ComputeCtx = Ctx.AsComputeContext();
    ComputeCtx.SetRootSignature(m_pGenerateMipsRS);
    auto*       pTexD3D12 = pTexView->GetTexture<TextureD3D12Impl>();
    const auto& TexDesc   = pTexD3D12->GetDesc();
    const auto& ViewDesc  = pTexView->GetDesc();

    bool IsAllSlices =
        (TexDesc.Type != RESOURCE_DIM_TEX_1D_ARRAY &&
         TexDesc.Type != RESOURCE_DIM_TEX_2D_ARRAY &&
         TexDesc.Type != RESOURCE_DIM_TEX_CUBE_ARRAY) ||
        TexDesc.ArraySize == ViewDesc.NumArraySlices;
    bool IsAllMips = ViewDesc.NumMipLevels == TexDesc.MipLevels;

    auto SRVDescriptorHandle = pTexView->GetTexArraySRV();

    if (!pTexD3D12->IsInKnownState())
    {
        LOG_ERROR_MESSAGE("Unable to generate mips for texture '", TexDesc.Name, "' because the texture state is unknown");
        return;
    }

    if (pTexD3D12->GetState() == RESOURCE_STATE_UNDEFINED)
    {
        // If texture state is undefined, transition it to shader resource state.
        // We need all subresources to be in a defined state at the end of the procedure.
        Ctx.TransitionResource(pTexD3D12, RESOURCE_STATE_SHADER_RESOURCE);
    }

    const auto OriginalState = pTexD3D12->GetState();
    pTexD3D12->SetState(RESOURCE_STATE_UNKNOWN); // Switch to manual state management

    // If we are processing the entire texture, we will leave it in SHADER_RESOURCE layout.
    // Otherwise we will transition affected subresources back to original layout.
    const auto FinalState = (IsAllSlices && IsAllMips) ? RESOURCE_STATE_SHADER_RESOURCE : OriginalState;

    auto BottomMip = ViewDesc.NumMipLevels - 1;
    for (uint32_t TopMip = 0; TopMip < BottomMip;)
    {
        uint32_t SrcWidth  = std::max(TexDesc.Width >> (TopMip + ViewDesc.MostDetailedMip), 1u);
        uint32_t SrcHeight = std::max(TexDesc.Height >> (TopMip + ViewDesc.MostDetailedMip), 1u);
        uint32_t DstWidth  = std::max(SrcWidth >> 1, 1u);
        uint32_t DstHeight = std::max(SrcHeight >> 1, 1u);

        // Determine if the first downsample is more than 2:1.  This happens whenever
        // the source width or height is odd.
        uint32_t NonPowerOfTwo = (SrcWidth & 1) | (SrcHeight & 1) << 1;
        if (TexDesc.Format == TEX_FORMAT_RGBA8_UNORM_SRGB)
            ComputeCtx.SetPipelineState(m_pGenerateMipsGammaPSO[NonPowerOfTwo]);
        else
            ComputeCtx.SetPipelineState(m_pGenerateMipsLinearPSO[NonPowerOfTwo]);

        // We can downsample up to four times, but if the ratio between levels is not
        // exactly 2:1, we have to shift our blend weights, which gets complicated or
        // expensive.  Maybe we can update the code later to compute sample weights for
        // each successive downsample.  We use _BitScanForward to count number of zeros
        // in the low bits.  Zeros indicate we can divide by two without truncating.
        uint32_t AdditionalMips;
        _BitScanForward((unsigned long*)&AdditionalMips, DstWidth | DstHeight);
        uint32_t NumMips = 1 + (AdditionalMips > 3 ? 3 : AdditionalMips);
        if (TopMip + NumMips > BottomMip)
            NumMips = BottomMip - TopMip;

        // These are clamped to 1 after computing additional mips because clamped
        // dimensions should not limit us from downsampling multiple times.  (E.g.
        // 16x1 -> 8x1 -> 4x1 -> 2x1 -> 1x1.)
        if (DstWidth == 0)
            DstWidth = 1;
        if (DstHeight == 0)
            DstHeight = 1;

        D3D12_DESCRIPTOR_HEAP_TYPE HeapType        = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        auto                       DescriptorAlloc = Ctx.AllocateDynamicGPUVisibleDescriptor(HeapType, 5);

        CommandContext::ShaderDescriptorHeaps Heaps{DescriptorAlloc.GetDescriptorHeap()};
        ComputeCtx.SetDescriptorHeaps(Heaps);
        Ctx.GetCommandList()->SetComputeRootDescriptorTable(1, DescriptorAlloc.GetGpuHandle(0));
        Ctx.GetCommandList()->SetComputeRootDescriptorTable(2, DescriptorAlloc.GetGpuHandle(1));
        struct RootCBData
        {
            Uint32 SrcMipLevel;  // Texture level of source mip
            Uint32 NumMipLevels; // Number of OutMips to write: [1, 4]
            Uint32 FirstArraySlice;
            Uint32 Dummy;
            float  TexelSize[2]; // 1.0 / OutMip1.Dimensions
        };
        RootCBData CBData{
            TopMip, // Mip levels are relateive to the view's most detailed mip
            NumMips,
            0, // Array slices are relative to the view's first array slice
            0,
            1.0f / static_cast<float>(DstWidth), 1.0f / static_cast<float>(DstHeight)};

        Ctx.GetCommandList()->SetComputeRoot32BitConstants(0, 6, &CBData, 0);

        // TODO: Shouldn't we transition top mip to shader resource state?
        D3D12_CPU_DESCRIPTOR_HANDLE DstDescriptorRange     = DescriptorAlloc.GetCpuHandle();
        const Uint32                MaxMipsHandledByCS     = 4; // Max number of mip levels processed by one CS shader invocation
        UINT                        DstRangeSize           = 1 + MaxMipsHandledByCS;
        D3D12_CPU_DESCRIPTOR_HANDLE SrcDescriptorRanges[5] = {};

        SrcDescriptorRanges[0] = SRVDescriptorHandle;
        UINT SrcRangeSizes[5]  = {1, 1, 1, 1, 1};
        // On Resource Binding Tier 2 hardware, all descriptor tables of type CBV and UAV declared in the set
        // Root Signature must be populated and initialized, even if the shaders do not need the descriptor.
        // So we must populate all 4 slots even though we may actually process less than 4 mip levels
        // Copy top mip level UAV descriptor handle to all unused slots
        for (Uint32 u = 0; u < MaxMipsHandledByCS; ++u)
            SrcDescriptorRanges[1 + u] = pTexView->GetMipLevelUAV(TopMip + std::min(u + 1, NumMips));

        pd3d12Device->CopyDescriptors(1, &DstDescriptorRange, &DstRangeSize, 1 + MaxMipsHandledByCS, SrcDescriptorRanges, SrcRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        // Transition top mip level to the shader resource state
        StateTransitionDesc SrcMipBarrier{pTexD3D12, TopMip == 0 ? OriginalState : RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_SHADER_RESOURCE, false};
        if (SrcMipBarrier.OldState != SrcMipBarrier.NewState)
        {
            SrcMipBarrier.FirstMipLevel   = ViewDesc.MostDetailedMip + TopMip;
            SrcMipBarrier.MipLevelsCount  = 1;
            SrcMipBarrier.FirstArraySlice = ViewDesc.FirstArraySlice;
            SrcMipBarrier.ArraySliceCount = ViewDesc.NumArraySlices;
            Ctx.TransitionResource(SrcMipBarrier);
        }

        // Transition dst mip levels to UAV state
        StateTransitionDesc DstMipsBarrier{pTexD3D12, OriginalState, RESOURCE_STATE_UNORDERED_ACCESS, false};
        if (DstMipsBarrier.OldState != DstMipsBarrier.NewState)
        {
            DstMipsBarrier.FirstMipLevel   = ViewDesc.MostDetailedMip + TopMip + 1;
            DstMipsBarrier.MipLevelsCount  = NumMips;
            DstMipsBarrier.FirstArraySlice = ViewDesc.FirstArraySlice;
            DstMipsBarrier.ArraySliceCount = ViewDesc.NumArraySlices;
            Ctx.TransitionResource(DstMipsBarrier);
        }

        ComputeCtx.Dispatch((DstWidth + 7) / 8, (DstHeight + 7) / 8, ViewDesc.NumArraySlices);

        // Transition the lowest level back to original layout or leave it in RESOURCE_STATE_SHADER_RESOURCE
        // if all subresources are processed
        if (SrcMipBarrier.NewState != FinalState)
        {
            SrcMipBarrier.OldState = SrcMipBarrier.NewState;
            SrcMipBarrier.NewState = FinalState;
            Ctx.TransitionResource(SrcMipBarrier);
        }

        if (DstMipsBarrier.NewState != FinalState)
        {
            DstMipsBarrier.OldState = DstMipsBarrier.NewState;
            DstMipsBarrier.NewState = FinalState;
            // Do not transition the bottom level if we have more mips to process
            if (TopMip + NumMips < BottomMip)
                --DstMipsBarrier.MipLevelsCount;
            if (DstMipsBarrier.MipLevelsCount > 0)
                Ctx.TransitionResource(DstMipsBarrier);
        }

        TopMip += NumMips;
    }

    // Set state
    pTexD3D12->SetState(FinalState);
}
} // namespace Diligent
