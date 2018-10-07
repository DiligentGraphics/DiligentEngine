#pragma once

#include <stdexcept>
#include <deque>

#define NOMINMAX
#include <D3D12.h>
#include <dxgi1_4.h>
#include <atlbase.h>

#include "IUnityGraphicsD3D12.h"
#include "ResourceStateTransitionHandler.h"

class UnityGraphicsD3D12Impl
{
public:

    UnityGraphicsD3D12Impl();
    ~UnityGraphicsD3D12Impl();

    void CreateDeviceAndCommandQueue();

    void CreateSwapChain(void* pNativeWndHandle, unsigned int Width, unsigned int Height);

    void InitBuffersAndViews();
    
    void Present();
    void ResizeSwapChain( UINT NewWidth, UINT NewHeight );

    CComPtr<ID3D12CommandAllocator> GetCommandAllocator();

    void DiscardCommandAllocator();

    bool IsFenceCompleted(UINT64 FenceValue);

    UINT64 IdleGPU();

    IDXGISwapChain3* GetDXGISwapChain() { return m_SwapChain; }
    ID3D12Resource* GetDepthBuffer() { return m_DepthStencilBuffer; }
    ID3D12Device* GetD3D12Device() { return m_D3D12Device; }
    ID3D12CommandQueue* GetCommandQueue() { return m_D3D12CmdQueue; }
    ID3D12Fence* GetFrameFence() { return m_D3D12FrameFence; }
    ID3D12Resource *GetCurrentBackBuffer() { return m_RenderTargets[m_FrameIndex]; }
    UINT GetCurrentBackBufferIndex() { return m_FrameIndex; }

    UINT GetBackBufferWidth()const { return m_BackBufferWidth; }
    UINT GetBackBufferHeight()const { return m_BackBufferHeight; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetDSV() { return m_DSVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(); }
    D3D12_CPU_DESCRIPTOR_HANDLE GetRTV() 
    { 
        auto RTVHandle = m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        RTVHandle.ptr += m_rtvDescriptorSize * m_FrameIndex; 
        return RTVHandle;
    }
    UINT64 GetNextFenceValue() { return m_NextFenceValue; }
    UINT64 GetCompletedFenceValue() { return m_D3D12FrameFence->GetCompletedValue(); }
    UINT64 ExecuteCommandList(ID3D12CommandList *pCmdList);
    void SetTransitionHandler(IResourceStateTransitionHandler *pTransitionHandler) { m_pStateTransitionHandler = pTransitionHandler; }
    void TransitonResourceStates(int stateCount, UnityGraphicsD3D12ResourceState* states);
    IDXGISwapChain3* GetSwapChain(){ return m_SwapChain; }

private:

    UINT m_BackBufferWidth, m_BackBufferHeight;
    static constexpr UINT m_BackBuffersCount = 3;

    UINT m_FrameIndex = 0;
    CComPtr<ID3D12Device> m_D3D12Device;
    CComPtr<ID3D12CommandQueue> m_D3D12CmdQueue;
    CComPtr<IDXGISwapChain3> m_SwapChain;
    CComPtr<ID3D12Fence> m_D3D12FrameFence;
    UINT64 m_NextFenceValue = 1;
    UINT64 m_CompletedFenceValue = 0;
    UINT m_rtvDescriptorSize;
    CComPtr<ID3D12DescriptorHeap> m_RTVDescriptorHeap;
    CComPtr<ID3D12DescriptorHeap> m_DSVDescriptorHeap;
    CComPtr<ID3D12Resource> m_RenderTargets[m_BackBuffersCount];
    CComPtr<ID3D12Resource> m_DepthStencilBuffer;
    CComPtr<ID3D12CommandAllocator> m_CmdAllocator;
    HANDLE m_WaitForGPUEventHandle = {};

    std::deque< std::pair<UINT64, CComPtr<ID3D12CommandAllocator> > > m_DiscardedAllocators;
    IResourceStateTransitionHandler *m_pStateTransitionHandler = nullptr;
};
