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

#include "TestingSwapChainBase.hpp"

#ifndef NOMINMAX
#    define NOMINMAX
#endif

#include <d3d12.h>
#include <atlcomcli.h>

namespace Diligent
{

namespace Testing
{

class TestingSwapChainD3D12 : public TestingSwapChainBase<ISwapChain>
{
public:
    using TBase = TestingSwapChainBase<ISwapChain>;
    TestingSwapChainD3D12(IReferenceCounters*  pRefCounters,
                          IRenderDevice*       pDevice,
                          IDeviceContext*      pContext,
                          const SwapChainDesc& SCDesc);

    virtual void TakeSnapshot() override final;

    void TransitionRenderTarget(ID3D12GraphicsCommandList* pCmdList,
                                D3D12_RESOURCE_STATES      RTVState);

    void TransitionBuffers(ID3D12GraphicsCommandList* pCmdList,
                           D3D12_RESOURCE_STATES      RTVState,
                           D3D12_RESOURCE_STATES      DSVState);

    D3D12_CPU_DESCRIPTOR_HANDLE GetRTVDescriptorHandle()
    {
        return m_RTVDescriptorHandle;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GetDSVDescriptorHandle()
    {
        return m_DSVDescriptorHandle;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GetUAVDescriptorHandle()
    {
        return m_UAVDescriptorHandle;
    }

    ID3D12DescriptorHeap* GetUAVDescriptorHeap()
    {
        return m_pd3d12CbvSrvUavDescriptorHeap;
    }

    ID3D12Resource* GetD3D12RenderTarget()
    {
        return m_pd3d12RenderTaget;
    }

private:
    CComPtr<ID3D12Resource> m_pd3d12RenderTaget;
    CComPtr<ID3D12Resource> m_pd3d12DepthBuffer;
    CComPtr<ID3D12Resource> m_pd3d12StagingBuffer;

    CComPtr<ID3D12DescriptorHeap> m_pd3d12RTVDescriptorHeap;
    CComPtr<ID3D12DescriptorHeap> m_pd3d12DSVDescriptorHeap;
    CComPtr<ID3D12DescriptorHeap> m_pd3d12CbvSrvUavDescriptorHeap;

    D3D12_RESOURCE_STATES m_RenderTargetState = D3D12_RESOURCE_STATE_RENDER_TARGET;
    D3D12_RESOURCE_STATES m_DepthBufferState  = D3D12_RESOURCE_STATE_DEPTH_WRITE;

    D3D12_CPU_DESCRIPTOR_HANDLE m_RTVDescriptorHandle = {};
    D3D12_CPU_DESCRIPTOR_HANDLE m_DSVDescriptorHandle = {};
    D3D12_CPU_DESCRIPTOR_HANDLE m_UAVDescriptorHandle = {};

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT m_StagingBufferFootprint = {};
    UINT64                             m_StagingBufferSize      = 0;
};

} // namespace Testing

} // namespace Diligent
