#include <vector>
#include <array>

#include "PlatformBase.h"

// Direct3D 12 implementation of RenderAPI.

#if SUPPORT_D3D12

#include <algorithm>

#define NOMINMAX
#include <d3d12.h>

#include "RenderAPI.h"
#include "Unity/IUnityGraphicsD3D12.h"
#include "EngineFactoryD3D12.h"
#include "RenderDeviceD3D12.h"
#include "CommandQueueD3D12.h"
#include "TextureD3D12.h"
#include "ObjectBase.h"
#include "DefaultRawMemoryAllocator.h"

using namespace Diligent;

class UnityCommandQueueImpl : public ObjectBase<ICommandQueueD3D12>
{
public:
    using TBase = ObjectBase<ICommandQueueD3D12>;
    UnityCommandQueueImpl(IReferenceCounters *pRefCounters, IUnityGraphicsD3D12v2* pUnityGraphicsD3D12) : 
        TBase(pRefCounters),
        m_pUnityGraphicsD3D12(pUnityGraphicsD3D12),
        m_WaitForGPUEventHandle( CreateEvent(nullptr, false, false, nullptr) )
    {
        VERIFY_EXPR(m_WaitForGPUEventHandle != INVALID_HANDLE_VALUE);
    }

    ~UnityCommandQueueImpl()
    {
        CloseHandle(m_WaitForGPUEventHandle);
    }

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE( IID_CommandQueueD3D12, TBase )

	// Returns the fence value that will be signaled next time
    virtual Uint64 GetNextFenceValue()override final
    {
        return m_pUnityGraphicsD3D12->GetNextFrameFenceValue();
    }

	// Executes a given command list
    virtual Uint64 Submit(ID3D12GraphicsCommandList* commandList)override final
    {
        auto NextFenceValue = m_pUnityGraphicsD3D12->GetNextFrameFenceValue();
        m_CurrentFenceValue = m_pUnityGraphicsD3D12->ExecuteCommandList(commandList, static_cast<int>(m_ResourcesToTransition.size()), m_ResourcesToTransition.empty() ? nullptr : m_ResourcesToTransition.data());
        VERIFY(m_CurrentFenceValue >= NextFenceValue, "Current fence value returned by ExecuteCommandList() is less than the next fence value previously queried through GetNextFrameFenceValue()");
        m_ResourcesToTransition.clear();
        return std::max(m_CurrentFenceValue, NextFenceValue);
    }

    // Returns D3D12 command queue. May return null if queue is anavailable
    virtual ID3D12CommandQueue* GetD3D12CommandQueue()
    {
        return nullptr;
    }

    // Returns value of the last completed fence
    virtual Uint64 GetCompletedFenceValue()
    {
        return m_pUnityGraphicsD3D12->GetFrameFence()->GetCompletedValue();
    }

    // Blocks execution until all pending GPU commands are complete
    virtual Uint64 WaitForIdle()
    {
        if (m_CurrentFenceValue < GetCompletedFenceValue())
        {
            auto d3d12Fence = m_pUnityGraphicsD3D12->GetFrameFence();
            d3d12Fence->SetEventOnCompletion(m_CurrentFenceValue, m_WaitForGPUEventHandle);
            WaitForSingleObject(m_WaitForGPUEventHandle, INFINITE);
            VERIFY(GetCompletedFenceValue() >= m_CurrentFenceValue, "Unexpected signaled fence value");
        }
        return GetCompletedFenceValue();
    }

    void TransitionResource(const UnityGraphicsD3D12ResourceState &ResourceState)
    {
        m_ResourcesToTransition.push_back(ResourceState);
    }

    void SignalFence(ID3D12Fence* pFence, Uint64 Value)
    {
        UNSUPPORTED("Signalling fence via unity command graphics is not supported");
    }

private:
    IUnityGraphicsD3D12v2* const m_pUnityGraphicsD3D12;
    HANDLE m_WaitForGPUEventHandle = {};
    UINT64 m_CurrentFenceValue = 0;
    std::vector<UnityGraphicsD3D12ResourceState> m_ResourcesToTransition;
};


class RenderAPI_D3D12 : public RenderAPI
{
public:
	virtual void ProcessDeviceEvent(UnityGfxDeviceEventType type, IUnityInterfaces* interfaces)override final;

	virtual bool GetUsesReverseZ()override final { return true; }

    virtual void BeginRendering()override final;

    virtual void EndRendering()override final;

    virtual bool IsDX()override final { return true; }

    virtual void AttachToNativeRenderTexture(void *nativeRenderTargetHandle, void *nativeDepthTextureHandle)override final;

private:
	IUnityGraphicsD3D12v2* m_UnityGraphicsD3D12 = nullptr;
    RefCntAutoPtr<IRenderDeviceD3D12> m_RenderDeviceD3D12;
    RefCntAutoPtr<UnityCommandQueueImpl> m_CmdQueue;

    RefCntAutoPtr<ITextureD3D12> m_RenderTarget;
    RefCntAutoPtr<ITextureD3D12> m_DepthBuffer;
};


RenderAPI* CreateRenderAPI_D3D12()
{
	return new RenderAPI_D3D12();
}

const UINT kNodeMask = 0;

void RenderAPI_D3D12::ProcessDeviceEvent(UnityGfxDeviceEventType type, IUnityInterfaces* interfaces)
{
    switch (type)
    {
        case kUnityGfxDeviceEventInitialize:
        {
            m_UnityGraphicsD3D12 = interfaces->Get<IUnityGraphicsD3D12v2>();
            auto d3d12Device = m_UnityGraphicsD3D12->GetDevice();
            auto &DefaultAllocator = DefaultRawMemoryAllocator::GetAllocator();
            m_CmdQueue = NEW_RC_OBJ(DefaultAllocator, "UnityCommandQueueImpl instance", UnityCommandQueueImpl)(m_UnityGraphicsD3D12);
            auto *pFactoryD3D12 = GetEngineFactoryD3D12();
            EngineD3D12CreateInfo Attribs;
            std::array<ICommandQueueD3D12*, 1> CmdQueues = {m_CmdQueue};
            pFactoryD3D12->AttachToD3D12Device(d3d12Device, CmdQueues.size(), CmdQueues.data(), Attribs, &m_Device, &m_Context);
            m_Device->QueryInterface(IID_RenderDeviceD3D12, reinterpret_cast<IObject**>(static_cast<IRenderDeviceD3D12**>(&m_RenderDeviceD3D12)));
        }
        break;

        case kUnityGfxDeviceEventShutdown:
            m_Context.Release();
            m_Device.Release();
            m_RenderDeviceD3D12.Release();
        break;
    }
}

void RenderAPI_D3D12::BeginRendering()
{
    m_CmdQueue->TransitionResource({ m_RenderTarget->GetD3D12Texture(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RENDER_TARGET });
    m_CmdQueue->TransitionResource({ m_DepthBuffer->GetD3D12Texture(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_DEPTH_WRITE });
    m_RenderTarget->SetD3D12ResourceState(D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_DepthBuffer->SetD3D12ResourceState(D3D12_RESOURCE_STATE_DEPTH_WRITE);
    ITextureView *RTVs[] = { m_RTV };
    m_Context->SetRenderTargets(1, RTVs, m_DSV, RESOURCE_STATE_TRANSITION_MODE_VERIFY);
}

void RenderAPI_D3D12::EndRendering()
{
    m_Context->Flush();
    m_Context->FinishFrame();
    m_Context->InvalidateState();
    m_RenderDeviceD3D12->ReleaseStaleResources();
}

void RenderAPI_D3D12::AttachToNativeRenderTexture(void *nativeRenderTargetHandle, void *nativeDepthTextureHandle)
{
    if (nativeRenderTargetHandle != nullptr && nativeDepthTextureHandle != nullptr)
    {
        m_RenderTarget.Release();
        m_DepthBuffer.Release();

        RefCntAutoPtr<IRenderDeviceD3D12> pDeviceD3D12(m_Device, IID_RenderDeviceD3D12);

        {
            auto *pd3d12RenderTarget = reinterpret_cast<ID3D12Resource*>(nativeRenderTargetHandle);
            RefCntAutoPtr<ITexture> pRenderTarget;
            pDeviceD3D12->CreateTextureFromD3DResource(pd3d12RenderTarget, RESOURCE_STATE_UNDEFINED, &pRenderTarget);
            pRenderTarget->QueryInterface(IID_TextureD3D12, reinterpret_cast<IObject**>(static_cast<ITextureD3D12**>(&m_RenderTarget)));
        }

        {
            auto *pd3d12DepthBuffer = reinterpret_cast<ID3D12Resource *>(nativeDepthTextureHandle);
            RefCntAutoPtr<ITexture> pDepthBuffer;
            pDeviceD3D12->CreateTextureFromD3DResource(pd3d12DepthBuffer, RESOURCE_STATE_UNDEFINED, &pDepthBuffer);
            pDepthBuffer->QueryInterface(IID_TextureD3D12, reinterpret_cast<IObject**>(static_cast<ITextureD3D12**>(&m_DepthBuffer)));
        }
        CreateTextureViews(m_RenderTarget, m_DepthBuffer);
    }
}

#endif // #if SUPPORT_D3D12
