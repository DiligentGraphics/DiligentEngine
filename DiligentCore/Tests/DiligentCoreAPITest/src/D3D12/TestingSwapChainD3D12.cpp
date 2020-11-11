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

#include <vector>

#include "D3D12/TestingEnvironmentD3D12.hpp"
#include "D3D12/TestingSwapChainD3D12.hpp"
#include "../include/DXGITypeConversions.hpp"

#include "RenderDeviceD3D12.h"
#include "DeviceContextD3D12.h"

namespace Diligent
{

namespace Testing
{

TestingSwapChainD3D12::TestingSwapChainD3D12(IReferenceCounters*  pRefCounters,
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
    RefCntAutoPtr<IRenderDeviceD3D12> pRenderDeviceD3D12{pDevice, IID_RenderDeviceD3D12};

    auto* pd3d12Device = pRenderDeviceD3D12->GetD3D12Device();

    D3D12_HEAP_PROPERTIES HeapProps = {};
    HeapProps.Type                  = D3D12_HEAP_TYPE_DEFAULT;
    HeapProps.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProps.MemoryPoolPreference  = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProps.CreationNodeMask      = 1;
    HeapProps.VisibleNodeMask       = 1;

    D3D12_RESOURCE_DESC TexDesc = {};
    TexDesc.Dimension           = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    TexDesc.Alignment           = 0;
    TexDesc.Width               = m_SwapChainDesc.Width;
    TexDesc.Height              = m_SwapChainDesc.Height;
    TexDesc.DepthOrArraySize    = 1;
    TexDesc.MipLevels           = 1;
    TexDesc.Format              = TexFormatToDXGI_Format(SCDesc.ColorBufferFormat);
    TexDesc.SampleDesc.Count    = 1;
    TexDesc.SampleDesc.Quality  = 0;
    TexDesc.Layout              = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    TexDesc.Flags               = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    D3D12_CLEAR_VALUE ClearColorValue = {};
    ClearColorValue.Format            = TexDesc.Format;
    auto hr =
        pd3d12Device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &TexDesc, m_RenderTargetState, &ClearColorValue,
                                              __uuidof(m_pd3d12RenderTaget),
                                              reinterpret_cast<void**>(static_cast<ID3D12Resource**>(&m_pd3d12RenderTaget)));
    VERIFY(SUCCEEDED(hr), "Failed to create D3D12 render target");

    {
        pd3d12Device->GetCopyableFootprints(&TexDesc, 0, 1, 0, &m_StagingBufferFootprint, nullptr, nullptr, &m_StagingBufferSize);

        D3D12_HEAP_PROPERTIES DownloadHeapProps;
        DownloadHeapProps.Type                 = D3D12_HEAP_TYPE_READBACK;
        DownloadHeapProps.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        DownloadHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        DownloadHeapProps.CreationNodeMask     = 1;
        DownloadHeapProps.VisibleNodeMask      = 1;

        D3D12_RESOURCE_DESC BufferDesc;
        BufferDesc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        BufferDesc.Alignment          = 0;
        BufferDesc.Width              = m_StagingBufferSize;
        BufferDesc.Height             = 1;
        BufferDesc.DepthOrArraySize   = 1;
        BufferDesc.MipLevels          = 1;
        BufferDesc.Format             = DXGI_FORMAT_UNKNOWN;
        BufferDesc.SampleDesc.Count   = 1;
        BufferDesc.SampleDesc.Quality = 0;
        BufferDesc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        BufferDesc.Flags              = D3D12_RESOURCE_FLAG_NONE;

        hr = pd3d12Device->CreateCommittedResource(&DownloadHeapProps, D3D12_HEAP_FLAG_NONE,
                                                   &BufferDesc, D3D12_RESOURCE_STATE_COPY_DEST,
                                                   nullptr, __uuidof(m_pd3d12StagingBuffer),
                                                   reinterpret_cast<void**>(static_cast<ID3D12Resource**>(&m_pd3d12StagingBuffer)));
        if (FAILED(hr))
            LOG_ERROR_AND_THROW("Failed to create committed staging buffer in an upload heap");
    }

    if (SCDesc.DepthBufferFormat != TEX_FORMAT_UNKNOWN)
    {
        TexDesc.Flags  = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        TexDesc.Format = TexFormatToDXGI_Format(SCDesc.DepthBufferFormat);

        D3D12_CLEAR_VALUE ClearDepthValue  = {};
        ClearDepthValue.Format             = TexDesc.Format;
        ClearDepthValue.DepthStencil.Depth = 1;
        hr =
            pd3d12Device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &TexDesc, m_DepthBufferState, &ClearDepthValue,
                                                  __uuidof(m_pd3d12DepthBuffer),
                                                  reinterpret_cast<void**>(static_cast<ID3D12Resource**>(&m_pd3d12DepthBuffer)));
        VERIFY(SUCCEEDED(hr), "Failed to create D3D12 depth buffer");
    }

    D3D12_DESCRIPTOR_HEAP_DESC DescriptorHeapDesc = {};
    DescriptorHeapDesc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    DescriptorHeapDesc.NumDescriptors             = 1;
    DescriptorHeapDesc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    DescriptorHeapDesc.NodeMask                   = 0;
    hr =
        pd3d12Device->CreateDescriptorHeap(&DescriptorHeapDesc,
                                           __uuidof(m_pd3d12RTVDescriptorHeap),
                                           reinterpret_cast<void**>(static_cast<ID3D12DescriptorHeap**>(&m_pd3d12RTVDescriptorHeap)));
    VERIFY(SUCCEEDED(hr), "Failed to create D3D12 RTV descriptor heap");
    m_RTVDescriptorHandle = m_pd3d12RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    pd3d12Device->CreateRenderTargetView(m_pd3d12RenderTaget, nullptr, m_RTVDescriptorHandle);

    DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    hr =
        pd3d12Device->CreateDescriptorHeap(&DescriptorHeapDesc,
                                           __uuidof(m_pd3d12DSVDescriptorHeap),
                                           reinterpret_cast<void**>(static_cast<ID3D12DescriptorHeap**>(&m_pd3d12DSVDescriptorHeap)));
    VERIFY(SUCCEEDED(hr), "Failed to create D3D12 DSV descriptor heap");
    m_DSVDescriptorHandle = m_pd3d12DSVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    pd3d12Device->CreateDepthStencilView(m_pd3d12DepthBuffer, nullptr, m_DSVDescriptorHandle);

    DescriptorHeapDesc.Type  = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    DescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    hr =
        pd3d12Device->CreateDescriptorHeap(&DescriptorHeapDesc,
                                           __uuidof(m_pd3d12CbvSrvUavDescriptorHeap),
                                           reinterpret_cast<void**>(static_cast<ID3D12DescriptorHeap**>(&m_pd3d12CbvSrvUavDescriptorHeap)));
    VERIFY(SUCCEEDED(hr), "Failed to create D3D12 UAV descriptor heap");
    m_UAVDescriptorHandle = m_pd3d12CbvSrvUavDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    pd3d12Device->CreateUnorderedAccessView(m_pd3d12RenderTaget, nullptr, nullptr, m_UAVDescriptorHandle);
}

void TestingSwapChainD3D12::TransitionBuffers(ID3D12GraphicsCommandList* pCmdList,
                                              D3D12_RESOURCE_STATES      RTVState,
                                              D3D12_RESOURCE_STATES      DSVState)
{
    std::vector<D3D12_RESOURCE_BARRIER> Barriers;
    if (m_RenderTargetState != RTVState)
    {
        D3D12_RESOURCE_BARRIER Barrier = {};
        Barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        Barrier.Transition.pResource   = m_pd3d12RenderTaget;
        Barrier.Transition.StateBefore = m_RenderTargetState;
        Barrier.Transition.StateAfter  = RTVState;
        Barrier.Transition.Subresource = 0;

        m_RenderTargetState = RTVState;

        Barriers.emplace_back(Barrier);
    }

    if (m_DepthBufferState != DSVState)
    {
        D3D12_RESOURCE_BARRIER Barrier = {};
        Barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        Barrier.Transition.pResource   = m_pd3d12DepthBuffer;
        Barrier.Transition.StateBefore = m_DepthBufferState;
        Barrier.Transition.StateAfter  = DSVState;
        Barrier.Transition.Subresource = 0;

        m_DepthBufferState = DSVState;

        Barriers.emplace_back(Barrier);
    }

    if (!Barriers.empty())
    {
        pCmdList->ResourceBarrier(static_cast<UINT>(Barriers.size()), Barriers.data());
    }
}

void TestingSwapChainD3D12::TransitionRenderTarget(ID3D12GraphicsCommandList* pCmdList,
                                                   D3D12_RESOURCE_STATES      RTVState)
{
    TransitionBuffers(pCmdList, RTVState, m_DepthBufferState);
}

void TestingSwapChainD3D12::TakeSnapshot()
{
    auto* pEnv     = TestingEnvironmentD3D12::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();
    auto  pCmdList = pEnv->CreateGraphicsCommandList();

    TransitionRenderTarget(pCmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);

    D3D12_TEXTURE_COPY_LOCATION SrcLocation = {};

    SrcLocation.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    SrcLocation.pResource        = m_pd3d12RenderTaget;
    SrcLocation.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION DstLocation = {};

    DstLocation.Type            = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    DstLocation.pResource       = m_pd3d12StagingBuffer;
    DstLocation.PlacedFootprint = m_StagingBufferFootprint;

    pCmdList->CopyTextureRegion(&DstLocation, 0, 0, 0, &SrcLocation, nullptr);

    pCmdList->Close();
    ID3D12CommandList* pCmdLits[] = {pCmdList};

    RefCntAutoPtr<IDeviceContextD3D12> pContextD3D12{pContext, IID_DeviceContextD3D12};

    auto* pQeueD3D12  = pContextD3D12->LockCommandQueue();
    auto* pd3d12Queue = pQeueD3D12->GetD3D12CommandQueue();

    pd3d12Queue->ExecuteCommandLists(1, pCmdLits);
    pEnv->IdleCommandQueue(pd3d12Queue);

    D3D12_RANGE InvalidateRange = {0, static_cast<SIZE_T>(m_StagingBufferSize)};
    void*       pStagingDataPtr = nullptr;
    m_pd3d12StagingBuffer->Map(0, &InvalidateRange, &pStagingDataPtr);
    m_ReferenceDataPitch = m_SwapChainDesc.Width * 4;
    m_ReferenceData.resize(m_SwapChainDesc.Height * m_ReferenceDataPitch);
    for (Uint32 row = 0; row < m_SwapChainDesc.Height; ++row)
    {
        memcpy(&m_ReferenceData[row * m_ReferenceDataPitch],
               reinterpret_cast<const Uint8*>(pStagingDataPtr) + m_StagingBufferFootprint.Footprint.RowPitch * row,
               m_ReferenceDataPitch);
    }
    m_pd3d12StagingBuffer->Unmap(0, nullptr);

    pContextD3D12->UnlockCommandQueue();
}

void CreateTestingSwapChainD3D12(IRenderDevice*       pDevice,
                                 IDeviceContext*      pContext,
                                 const SwapChainDesc& SCDesc,
                                 ISwapChain**         ppSwapChain)
{
    TestingSwapChainD3D12* pTestingSC(MakeNewRCObj<TestingSwapChainD3D12>()(pDevice, pContext, SCDesc));
    pTestingSC->QueryInterface(IID_SwapChain, reinterpret_cast<IObject**>(ppSwapChain));
}

} // namespace Testing

} // namespace Diligent
