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


#pragma once

#include <vector>

#include "D3D12ResourceBase.hpp"
#include "TextureViewD3D12.h"
#include "TextureD3D12.h"
#include "BufferD3D12.h"
#include "DescriptorHeap.hpp"

namespace Diligent
{


struct DWParam
{
    // clang-format off
    DWParam( FLOAT f ) : Float{f} {}
    DWParam( UINT u )  : Uint {u} {}
    DWParam( INT i )   : Int  {i} {}

    void operator= ( FLOAT f ) { Float = f; }
    void operator= ( UINT u )  { Uint = u; }
    void operator= ( INT i )   { Int = i; }
    // clang-format on

    union
    {
        FLOAT Float;
        UINT  Uint;
        INT   Int;
    };
};


class CommandContext
{
public:
    CommandContext(class CommandListManager& CmdListManager);

    // clang-format off
    CommandContext             (const CommandContext&)  = delete;
    CommandContext& operator = (const CommandContext&)  = delete;
    CommandContext             (      CommandContext&&) = delete;
    CommandContext& operator = (      CommandContext&&) = delete;
    // clang-format on

    ~CommandContext();

    // Submit the command buffer and reset it.  This is encouraged to keep the GPU busy and reduce latency.
    // Taking too long to build command lists and submit them can idle the GPU.
    ID3D12GraphicsCommandList* Close(CComPtr<ID3D12CommandAllocator>& pAllocator);
    void                       Reset(CommandListManager& CmdListManager);

    class GraphicsContext&  AsGraphicsContext();
    class GraphicsContext1& AsGraphicsContext1();
    class GraphicsContext2& AsGraphicsContext2();
    class GraphicsContext3& AsGraphicsContext3();
    class GraphicsContext4& AsGraphicsContext4();
    class GraphicsContext5& AsGraphicsContext5();
    class GraphicsContext6& AsGraphicsContext6();
    class ComputeContext&   AsComputeContext();

    void ClearUAVFloat(D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle,
                       D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle,
                       ID3D12Resource*             pd3d12Resource,
                       const float*                Color)
    {
        FlushResourceBarriers();
        m_pCommandList->ClearUnorderedAccessViewFloat(GpuHandle, CpuHandle, pd3d12Resource, Color, 0, nullptr);
    }

    void ClearUAVUint(D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle,
                      D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle,
                      ID3D12Resource*             pd3d12Resource,
                      const UINT*                 Color)
    {
        FlushResourceBarriers();
        m_pCommandList->ClearUnorderedAccessViewUint(GpuHandle, CpuHandle, pd3d12Resource, Color, 0, nullptr);
    }

    void CopyResource(ID3D12Resource* pDstRes, ID3D12Resource* pSrcRes)
    {
        m_pCommandList->CopyResource(pDstRes, pSrcRes);
    }

    void TransitionResource(ITextureD3D12* pTexture, RESOURCE_STATE NewState);
    void TransitionResource(IBufferD3D12* pBuffer, RESOURCE_STATE NewState);
    void TransitionResource(const StateTransitionDesc& Barrier);
    //void BeginResourceTransition(GpuResource& Resource, D3D12_RESOURCE_STATES NewState, bool FlushImmediate = false);

    void ResolveSubresource(ID3D12Resource* pDstResource,
                            UINT            DstSubresource,
                            ID3D12Resource* pSrcResource,
                            UINT            SrcSubresource,
                            DXGI_FORMAT     Format)
    {
        FlushResourceBarriers();
        m_pCommandList->ResolveSubresource(pDstResource, DstSubresource, pSrcResource, SrcSubresource, Format);
    }

    void FlushResourceBarriers()
    {
        if (!m_PendingResourceBarriers.empty())
        {
            m_pCommandList->ResourceBarrier(static_cast<UINT>(m_PendingResourceBarriers.size()), m_PendingResourceBarriers.data());
            m_PendingResourceBarriers.clear();
        }
    }


    struct ShaderDescriptorHeaps
    {
        ID3D12DescriptorHeap* pSrvCbvUavHeap;
        ID3D12DescriptorHeap* pSamplerHeap;
        ShaderDescriptorHeaps(ID3D12DescriptorHeap* _pSrvCbvUavHeap = nullptr, ID3D12DescriptorHeap* _pSamplerHeap = nullptr) :
            pSrvCbvUavHeap{_pSrvCbvUavHeap},
            pSamplerHeap{_pSamplerHeap}
        {}
        bool operator==(const ShaderDescriptorHeaps& rhs) const
        {
            return pSrvCbvUavHeap == rhs.pSrvCbvUavHeap && pSamplerHeap == rhs.pSamplerHeap;
        }
        operator bool() const
        {
            return pSrvCbvUavHeap != nullptr || pSamplerHeap != nullptr;
        }
    };
    void SetDescriptorHeaps(ShaderDescriptorHeaps& Heaps);

    void ExecuteIndirect(ID3D12CommandSignature* pCmdSignature, ID3D12Resource* pBuff, Uint64 ArgsOffset)
    {
        FlushResourceBarriers();
        m_pCommandList->ExecuteIndirect(pCmdSignature, 1, pBuff, ArgsOffset, nullptr, 0);
    }

    void                       SetID(const Char* ID) { m_ID = ID; }
    ID3D12GraphicsCommandList* GetCommandList() { return m_pCommandList; }

    DescriptorHeapAllocation AllocateDynamicGPUVisibleDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE Type, UINT Count = 1)
    {
        VERIFY(m_DynamicGPUDescriptorAllocators != nullptr, "Dynamic GPU descriptor llocators have not been initialized. Did you forget to call SetDynamicGPUDescriptorAllocators() after resetting the context?");
        VERIFY(Type >= D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV && Type <= D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, "Invalid heap type");
        return m_DynamicGPUDescriptorAllocators[Type].Allocate(Count);
    }

    void ResourceBarrier(const D3D12_RESOURCE_BARRIER& Barrier)
    {
        m_PendingResourceBarriers.emplace_back(Barrier);
    }

    void InsertUAVBarrier(ID3D12Resource* pd3d12Resource);

    void SetPipelineState(ID3D12PipelineState* pPSO)
    {
        if (pPSO != m_pCurPipelineState)
        {
            m_pCommandList->SetPipelineState(m_pCurPipelineState = pPSO);
        }
    }

    void SetDynamicGPUDescriptorAllocators(DynamicSuballocationsManager* Allocators)
    {
        m_DynamicGPUDescriptorAllocators = Allocators;
    }

    void BeginQuery(ID3D12QueryHeap* pQueryHeap, D3D12_QUERY_TYPE Type, UINT Index)
    {
        m_pCommandList->BeginQuery(pQueryHeap, Type, Index);
    }

    void EndQuery(ID3D12QueryHeap* pQueryHeap, D3D12_QUERY_TYPE Type, UINT Index)
    {
        m_pCommandList->EndQuery(pQueryHeap, Type, Index);
    }

    void ResolveQueryData(ID3D12QueryHeap* pQueryHeap,
                          D3D12_QUERY_TYPE Type,
                          UINT             StartIndex,
                          UINT             NumQueries,
                          ID3D12Resource*  pDestinationBuffer,
                          UINT64           AlignedDestinationBufferOffset)
    {
        m_pCommandList->ResolveQueryData(pQueryHeap, Type, StartIndex, NumQueries, pDestinationBuffer, AlignedDestinationBufferOffset);
    }

protected:
    void InsertAliasBarrier(D3D12ResourceBase& Before, D3D12ResourceBase& After, bool FlushImmediate = false);

    CComPtr<ID3D12GraphicsCommandList> m_pCommandList;
    CComPtr<ID3D12CommandAllocator>    m_pCurrentAllocator;

    ID3D12PipelineState* m_pCurPipelineState         = nullptr;
    ID3D12RootSignature* m_pCurGraphicsRootSignature = nullptr;
    ID3D12RootSignature* m_pCurComputeRootSignature  = nullptr;

    static constexpr int MaxPendingBarriers = 16;

    std::vector<D3D12_RESOURCE_BARRIER, STDAllocatorRawMem<D3D12_RESOURCE_BARRIER>> m_PendingResourceBarriers;

    ShaderDescriptorHeaps m_BoundDescriptorHeaps;

    DynamicSuballocationsManager* m_DynamicGPUDescriptorAllocators = nullptr;

    String m_ID;

    D3D12_PRIMITIVE_TOPOLOGY m_PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

    Uint32 m_MaxInterfaceVer = 0;
};


class GraphicsContext : public CommandContext
{
public:
    void ClearRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE RTV, const float* Color)
    {
        FlushResourceBarriers();
        m_pCommandList->ClearRenderTargetView(RTV, Color, 0, nullptr);
    }

    void ClearDepthStencil(D3D12_CPU_DESCRIPTOR_HANDLE DSV, D3D12_CLEAR_FLAGS ClearFlags, float Depth, UINT8 Stencil)
    {
        FlushResourceBarriers();
        m_pCommandList->ClearDepthStencilView(DSV, ClearFlags, Depth, Stencil, 0, nullptr);
    }

    void SetRootSignature(ID3D12RootSignature* pRootSig)
    {
        if (pRootSig != m_pCurGraphicsRootSignature)
        {
            m_pCommandList->SetGraphicsRootSignature(m_pCurGraphicsRootSignature = pRootSig);
        }
    }

    void SetViewports(UINT NumVPs, const D3D12_VIEWPORT* pVPs)
    {
        m_pCommandList->RSSetViewports(NumVPs, pVPs);
    }

    void SetScissorRects(UINT NumRects, const D3D12_RECT* pRects)
    {
        m_pCommandList->RSSetScissorRects(NumRects, pRects);
    }

    void SetStencilRef(UINT StencilRef)
    {
        m_pCommandList->OMSetStencilRef(StencilRef);
    }

    void SetBlendFactor(const float* BlendFactor)
    {
        m_pCommandList->OMSetBlendFactor(BlendFactor);
    }

    void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY Topology)
    {
        if (m_PrimitiveTopology != Topology)
        {
            m_PrimitiveTopology = Topology;
            m_pCommandList->IASetPrimitiveTopology(Topology);
        }
    }

    void SetConstants(UINT RootIndex, UINT NumConstants, const void* pConstants)
    {
        m_pCommandList->SetGraphicsRoot32BitConstants(RootIndex, NumConstants, pConstants, 0);
    }

    void SetConstants(UINT RootIndex, DWParam X)
    {
        m_pCommandList->SetGraphicsRoot32BitConstant(RootIndex, X.Uint, 0);
    }

    void SetConstants(UINT RootIndex, DWParam X, DWParam Y)
    {
        m_pCommandList->SetGraphicsRoot32BitConstant(RootIndex, X.Uint, 0);
        m_pCommandList->SetGraphicsRoot32BitConstant(RootIndex, Y.Uint, 1);
    }

    void SetConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z)
    {
        m_pCommandList->SetGraphicsRoot32BitConstant(RootIndex, X.Uint, 0);
        m_pCommandList->SetGraphicsRoot32BitConstant(RootIndex, Y.Uint, 1);
        m_pCommandList->SetGraphicsRoot32BitConstant(RootIndex, Z.Uint, 2);
    }

    void SetConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z, DWParam W)
    {
        m_pCommandList->SetGraphicsRoot32BitConstant(RootIndex, X.Uint, 0);
        m_pCommandList->SetGraphicsRoot32BitConstant(RootIndex, Y.Uint, 1);
        m_pCommandList->SetGraphicsRoot32BitConstant(RootIndex, Z.Uint, 2);
        m_pCommandList->SetGraphicsRoot32BitConstant(RootIndex, W.Uint, 3);
    }

    void SetConstantBuffer(UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV)
    {
        m_pCommandList->SetGraphicsRootConstantBufferView(RootIndex, CBV);
    }

    void SetDescriptorTable(UINT RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle)
    {
        m_pCommandList->SetGraphicsRootDescriptorTable(RootIndex, FirstHandle);
    }

    void SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW& IBView)
    {
        m_pCommandList->IASetIndexBuffer(&IBView);
    }

    void SetVertexBuffers(UINT StartSlot, UINT Count, const D3D12_VERTEX_BUFFER_VIEW VBViews[])
    {
        m_pCommandList->IASetVertexBuffers(StartSlot, Count, VBViews);
    }

    void Draw(UINT VertexCountPerInstance, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation)
    {
        FlushResourceBarriers();
        m_pCommandList->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
    }

    void DrawIndexed(UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation)
    {
        FlushResourceBarriers();
        m_pCommandList->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
    }
};

class GraphicsContext1 : public GraphicsContext
{
};

class GraphicsContext2 : public GraphicsContext1
{
};

class GraphicsContext3 : public GraphicsContext2
{
};


class GraphicsContext4 : public GraphicsContext3
{
public:
    void BeginRenderPass(UINT                                        NumRenderTargets,
                         const D3D12_RENDER_PASS_RENDER_TARGET_DESC* pRenderTargets,
                         const D3D12_RENDER_PASS_DEPTH_STENCIL_DESC* pDepthStencil,
                         D3D12_RENDER_PASS_FLAGS                     Flags)
    {
        FlushResourceBarriers();
        static_cast<ID3D12GraphicsCommandList4*>(m_pCommandList.p)->BeginRenderPass(NumRenderTargets, pRenderTargets, pDepthStencil, Flags);
    }

    void EndRenderPass()
    {
        static_cast<ID3D12GraphicsCommandList4*>(m_pCommandList.p)->EndRenderPass();
    }
};

class GraphicsContext5 : public GraphicsContext4
{
};

class GraphicsContext6 : public GraphicsContext5
{
public:
    void DrawMesh(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ)
    {
#ifdef D3D12_H_HAS_MESH_SHADER
        FlushResourceBarriers();
        static_cast<ID3D12GraphicsCommandList6*>(m_pCommandList.p)->DispatchMesh(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
#else
        UNSUPPORTED("DrawMesh is not supported in current D3D12 header");
#endif
    }
};

class ComputeContext : public CommandContext
{
public:
    void SetRootSignature(ID3D12RootSignature* pRootSig)
    {
        if (pRootSig != m_pCurComputeRootSignature)
        {
            m_pCommandList->SetComputeRootSignature(m_pCurComputeRootSignature = pRootSig);
        }
    }

    void SetConstants(UINT RootIndex, UINT NumConstants, const void* pConstants)
    {
        m_pCommandList->SetComputeRoot32BitConstants(RootIndex, NumConstants, pConstants, 0);
    }

    void SetConstants(UINT RootIndex, DWParam X)
    {
        m_pCommandList->SetComputeRoot32BitConstant(RootIndex, X.Uint, 0);
    }

    void SetConstants(UINT RootIndex, DWParam X, DWParam Y)
    {
        m_pCommandList->SetComputeRoot32BitConstant(RootIndex, X.Uint, 0);
        m_pCommandList->SetComputeRoot32BitConstant(RootIndex, Y.Uint, 1);
    }

    void SetConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z)
    {
        m_pCommandList->SetComputeRoot32BitConstant(RootIndex, X.Uint, 0);
        m_pCommandList->SetComputeRoot32BitConstant(RootIndex, Y.Uint, 1);
        m_pCommandList->SetComputeRoot32BitConstant(RootIndex, Z.Uint, 2);
    }

    void SetConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z, DWParam W)
    {
        m_pCommandList->SetComputeRoot32BitConstant(RootIndex, X.Uint, 0);
        m_pCommandList->SetComputeRoot32BitConstant(RootIndex, Y.Uint, 1);
        m_pCommandList->SetComputeRoot32BitConstant(RootIndex, Z.Uint, 2);
        m_pCommandList->SetComputeRoot32BitConstant(RootIndex, W.Uint, 3);
    }


    void SetConstantBuffer(UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV)
    {
        m_pCommandList->SetComputeRootConstantBufferView(RootIndex, CBV);
    }

    void SetDescriptorTable(UINT RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle)
    {
        m_pCommandList->SetComputeRootDescriptorTable(RootIndex, FirstHandle);
    }

    void Dispatch(size_t GroupCountX = 1, size_t GroupCountY = 1, size_t GroupCountZ = 1)
    {
        FlushResourceBarriers();
        m_pCommandList->Dispatch((UINT)GroupCountX, (UINT)GroupCountY, (UINT)GroupCountZ);
    }
};

inline GraphicsContext& CommandContext::AsGraphicsContext()
{
    return static_cast<GraphicsContext&>(*this);
}

inline GraphicsContext1& CommandContext::AsGraphicsContext1()
{
    VERIFY(m_MaxInterfaceVer >= 1, "Maximum supported interface version is ", m_MaxInterfaceVer);
    return static_cast<GraphicsContext1&>(*this);
}

inline GraphicsContext2& CommandContext::AsGraphicsContext2()
{
    VERIFY(m_MaxInterfaceVer >= 2, "Maximum supported interface version is ", m_MaxInterfaceVer);
    return static_cast<GraphicsContext2&>(*this);
}

inline GraphicsContext3& CommandContext::AsGraphicsContext3()
{
    VERIFY(m_MaxInterfaceVer >= 3, "Maximum supported interface version is ", m_MaxInterfaceVer);
    return static_cast<GraphicsContext3&>(*this);
}

inline GraphicsContext4& CommandContext::AsGraphicsContext4()
{
    VERIFY(m_MaxInterfaceVer >= 4, "Maximum supported interface version is ", m_MaxInterfaceVer);
    return static_cast<GraphicsContext4&>(*this);
}

inline GraphicsContext5& CommandContext::AsGraphicsContext5()
{
    VERIFY(m_MaxInterfaceVer >= 5, "Maximum supported interface version is ", m_MaxInterfaceVer);
    return static_cast<GraphicsContext5&>(*this);
}

inline GraphicsContext6& CommandContext::AsGraphicsContext6()
{
    VERIFY(m_MaxInterfaceVer >= 6, "Maximum supported interface version is ", m_MaxInterfaceVer);
    return static_cast<GraphicsContext6&>(*this);
}

inline ComputeContext& CommandContext::AsComputeContext()
{
    return static_cast<ComputeContext&>(*this);
}

inline void CommandContext::SetDescriptorHeaps(ShaderDescriptorHeaps& Heaps)
{
#ifdef DILIGENT_DEBUG
    VERIFY(Heaps.pSrvCbvUavHeap != nullptr || Heaps.pSamplerHeap != nullptr, "At least one heap is expected to be set");
    VERIFY(Heaps.pSrvCbvUavHeap == nullptr || Heaps.pSrvCbvUavHeap->GetDesc().Type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, "Invalid heap type provided in pSrvCbvUavHeap");
    VERIFY(Heaps.pSamplerHeap == nullptr || Heaps.pSamplerHeap->GetDesc().Type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, "Invalid heap type provided in pSamplerHeap");
#endif

    if (!(Heaps == m_BoundDescriptorHeaps))
    {
        m_BoundDescriptorHeaps = Heaps;

        ID3D12DescriptorHeap** ppHeaps  = reinterpret_cast<ID3D12DescriptorHeap**>(&Heaps);
        UINT                   NumHeaps = (ppHeaps[0] != nullptr ? 1 : 0) + (ppHeaps[1] != nullptr ? 1 : 0);
        if (ppHeaps[0] == nullptr)
            ++ppHeaps;

        m_pCommandList->SetDescriptorHeaps(NumHeaps, ppHeaps);
    }
}

} // namespace Diligent
