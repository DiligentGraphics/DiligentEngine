
#include <array>

#define NOMINMAX
#include <D3D12.h>
#include <dxgi1_4.h>
#include <atlbase.h>
#include "UnityGraphicsD3D12Impl.h"
#include "DiligentGraphicsAdapterD3D12.h"
#include "UnityGraphicsD3D12Emulator.h"
#include "SwapChainD3D12.h"
#include "TextureD3D12.h"
#include "RenderDeviceD3D12.h"
#include "DeviceContextD3D12.h"
#include "CommandQueueD3D12.h"
#include "EngineFactoryD3D12.h"
#include "SwapChainBase.h"
#include "DefaultRawMemoryAllocator.h"
#include "DXGITypeConversions.h"

using namespace Diligent;

namespace
{

class ProxyCommandQueueD3D12 : public ObjectBase<ICommandQueueD3D12>
{
public:
    using TBase = ObjectBase<ICommandQueueD3D12>;
    ProxyCommandQueueD3D12(IReferenceCounters *pRefCounters, UnityGraphicsD3D12Impl& GraphicsD3D12Impl) : 
        TBase(pRefCounters),
        m_GraphicsD3D12Impl(GraphicsD3D12Impl)
    {
    }

    ~ProxyCommandQueueD3D12()
    {
    }

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE( IID_CommandQueueD3D12, TBase )

	// Returns the fence value that will be signaled next time
    virtual Uint64 GetNextFenceValue()override final
    {
        return m_GraphicsD3D12Impl.GetNextFenceValue();
    }

	// Executes a given command list
    virtual Uint64 Submit(ID3D12GraphicsCommandList* commandList)override final
    {
        return m_GraphicsD3D12Impl.ExecuteCommandList(commandList);
    }

    // Returns D3D12 command queue. May return null if queue is anavailable
    virtual ID3D12CommandQueue* GetD3D12CommandQueue()
    {
        return nullptr;
    }

    /// Returns value of the last completed fence
    virtual Uint64 GetCompletedFenceValue()override final
    {
        return m_GraphicsD3D12Impl.GetCompletedFenceValue();
    }

    /// Blocks execution until all pending GPU commands are complete
    virtual Uint64 WaitForIdle()override final
    {
        return m_GraphicsD3D12Impl.IdleGPU();
    }

    virtual void SignalFence(ID3D12Fence* pFence, Uint64 Value)override final
    {
        m_GraphicsD3D12Impl.GetCommandQueue()->Signal(pFence, Value);
    }

private:
    UnityGraphicsD3D12Impl& m_GraphicsD3D12Impl;
};


class ProxySwapChainD3D12 : public SwapChainBase<ISwapChainD3D12>
{
public:
    using TBase = SwapChainBase<ISwapChainD3D12>;

    ProxySwapChainD3D12( IReferenceCounters *pRefCounters,
                         IRenderDevice *pDevice,
                         IDeviceContext *pDeviceContext,
                         const SwapChainDesc& SCDesc ) : 
        TBase(pRefCounters, pDevice, pDeviceContext,SCDesc)
    {
    }
        
    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_SwapChainD3D12, TBase)

    virtual IDXGISwapChain *GetDXGISwapChain()override final
    {
        UNEXPECTED("DXGI swap chain cannot be requested through the proxy swap chain");
        return nullptr;
    }

    virtual ITextureViewD3D12* GetCurrentBackBufferRTV()
    {
        return m_RTVs[m_CurrentBackBufferIndex];
    }

    virtual ITextureViewD3D12* GetDepthBufferDSV()
    {
        return m_DSV;
    }

    virtual void Present(Uint32 SyncInterval)override final
    {
        UNEXPECTED("Present is not expected to be called directly");
    }

    virtual void SetFullscreenMode(const DisplayModeAttribs &DisplayMode)override final
    {
        UNEXPECTED("Fullscreen mode cannot be set through the proxy swap chain");
    }

    virtual void SetWindowedMode()override final
    {
        UNEXPECTED("Windowed mode cannot be set through the proxy swap chain");
    }

    virtual void Resize(Uint32 NewWidth, Uint32 NewHeight)override final
    {
        TBase::Resize(NewWidth, NewHeight, 0);
    }

    void ReleaseBuffers()
    {
        m_BackBuffers.clear();
        m_RTVs.clear();
        m_DepthBuffer.Release();
        m_DSV.Release();
    }

    void SetBackBufferIndex(Uint32 BackBufferIndex) { m_CurrentBackBufferIndex = BackBufferIndex; }

    void CreateBuffers(IDXGISwapChain3 *pDXGISwapChain, ID3D12Resource *pd3d12DepthBuffer)
    {
        DXGI_SWAP_CHAIN_DESC1 SwapChainDesc;
        pDXGISwapChain->GetDesc1(&SwapChainDesc);
        m_SwapChainDesc.BufferCount = SwapChainDesc.BufferCount;
        m_SwapChainDesc.SamplesCount = SwapChainDesc.SampleDesc.Count;
        m_SwapChainDesc.Width = SwapChainDesc.Width;
        m_SwapChainDesc.Height = SwapChainDesc.Height;
        m_SwapChainDesc.ColorBufferFormat = DXGI_FormatToTexFormat(SwapChainDesc.Format);
        if (m_SwapChainDesc.ColorBufferFormat == TEX_FORMAT_RGBA8_UNORM)
            m_SwapChainDesc.ColorBufferFormat = TEX_FORMAT_RGBA8_UNORM_SRGB;

        const auto DepthBufferDesc = pd3d12DepthBuffer->GetDesc();
        m_SwapChainDesc.DepthBufferFormat = DXGI_FormatToTexFormat(DepthBufferDesc.Format);

        RefCntAutoPtr<IRenderDeviceD3D12> pRenderDeviceD3D12(m_pRenderDevice, IID_RenderDeviceD3D12);

        m_BackBuffers.reserve(m_SwapChainDesc.BufferCount);
        m_RTVs.reserve(m_SwapChainDesc.BufferCount);
        for(Uint32 backbuff = 0; backbuff < m_SwapChainDesc.BufferCount; ++backbuff)
        {
		    CComPtr<ID3D12Resource> pd3d12BackBuffer;
            auto hr = pDXGISwapChain->GetBuffer(backbuff, __uuidof(pd3d12BackBuffer), reinterpret_cast<void**>( static_cast<ID3D12Resource**>(&pd3d12BackBuffer) ));
            if(FAILED(hr))
                LOG_ERROR_AND_THROW("Failed to get back buffer ", backbuff," from the swap chain");
            RefCntAutoPtr<ITexture> pBackBuffer;
            pRenderDeviceD3D12->CreateTextureFromD3DResource(pd3d12BackBuffer, RESOURCE_STATE_UNDEFINED, &pBackBuffer);
            m_BackBuffers.emplace_back( RefCntAutoPtr<ITextureD3D12>(pBackBuffer, IID_TextureD3D12) );
            TextureViewDesc TexViewDesc;
            TexViewDesc.ViewType = TEXTURE_VIEW_RENDER_TARGET;
            TexViewDesc.Format = TEX_FORMAT_RGBA8_UNORM_SRGB;
            RefCntAutoPtr<ITextureView> pRTV;
            pBackBuffer->CreateView(TexViewDesc, &pRTV);
            m_RTVs.emplace_back(RefCntAutoPtr<ITextureViewD3D12>(pRTV, IID_TextureViewD3D12));
        }

        RefCntAutoPtr<ITexture> pDepthBuffer;
        pRenderDeviceD3D12->CreateTextureFromD3DResource(pd3d12DepthBuffer, RESOURCE_STATE_UNDEFINED, &pDepthBuffer);
        m_DepthBuffer = RefCntAutoPtr<ITextureD3D12>(pDepthBuffer, IID_TextureD3D12);
        auto *pDSV = m_DepthBuffer->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL);
        m_DSV = RefCntAutoPtr<ITextureViewD3D12>(pDSV, IID_TextureViewD3D12);
    }

    ITextureD3D12* GetCurrentBackBuffer() { return m_BackBuffers[m_CurrentBackBufferIndex]; }
    ITextureD3D12* GetDepthBuffer() { return m_DepthBuffer; }

private:
    std::vector<RefCntAutoPtr<ITextureD3D12>> m_BackBuffers;
    std::vector<RefCntAutoPtr<ITextureViewD3D12>> m_RTVs;
    RefCntAutoPtr<ITextureD3D12> m_DepthBuffer;
    RefCntAutoPtr<ITextureViewD3D12> m_DSV;
    Uint32 m_CurrentBackBufferIndex = 0;
};

}


DiligentGraphicsAdapterD3D12::DiligentGraphicsAdapterD3D12(UnityGraphicsD3D12Emulator& UnityGraphicsD3D12)noexcept :
    m_UnityGraphicsD3D12(UnityGraphicsD3D12)
{
    auto *GraphicsImpl = UnityGraphicsD3D12.GetGraphicsImpl();
    auto *d3d12Device = GraphicsImpl->GetD3D12Device();

    auto &DefaultAllocator = DefaultRawMemoryAllocator::GetAllocator();
    auto CmdQueue = NEW_RC_OBJ(DefaultAllocator, "UnityCommandQueueImpl instance", ProxyCommandQueueD3D12)(*GraphicsImpl);

    auto *pFactoryD3D12 = GetEngineFactoryD3D12();
    EngineD3D12CreateInfo Attribs;
    std::array<ICommandQueueD3D12*, 1> CmdQueues = {CmdQueue};
    pFactoryD3D12->AttachToD3D12Device(d3d12Device, CmdQueues.size(), CmdQueues.data(), Attribs, &m_pDevice, &m_pDeviceCtx);
}

void DiligentGraphicsAdapterD3D12::InitProxySwapChain()
{
    auto *GraphicsImpl = m_UnityGraphicsD3D12.GetGraphicsImpl();
    auto &DefaultAllocator = DefaultRawMemoryAllocator::GetAllocator();
    SwapChainDesc SCDesc;
    auto ProxySwapChain = NEW_RC_OBJ(DefaultAllocator, "UnityCommandQueueImpl instance", ProxySwapChainD3D12)(m_pDevice, m_pDeviceCtx, SCDesc);
    ProxySwapChain->CreateBuffers(GraphicsImpl->GetDXGISwapChain(), GraphicsImpl->GetDepthBuffer());
    m_pProxySwapChain = ProxySwapChain;
    m_pDeviceCtx->SetSwapChain(ProxySwapChain);
}

void DiligentGraphicsAdapterD3D12::PreSwapChainResize()
{
    auto *pProxySwapChainD3D12 = m_pProxySwapChain.RawPtr<ProxySwapChainD3D12>();
    auto *pDeviceD3D12 = m_pDevice.RawPtr<IRenderDeviceD3D12>();
    pProxySwapChainD3D12->ReleaseBuffers();
    auto *GraphicsImpl = m_UnityGraphicsD3D12.GetGraphicsImpl();
    pDeviceD3D12->ReleaseStaleResources();
    // We must idle GPU
    GraphicsImpl->IdleGPU();
    // And call FinishFrame() to release references to swap chain resources
    m_pDeviceCtx->FinishFrame();
    pDeviceD3D12->ReleaseStaleResources();
}

void DiligentGraphicsAdapterD3D12::PostSwapChainResize()
{
    auto *GraphicsImpl = m_UnityGraphicsD3D12.GetGraphicsImpl();
    auto *pProxySwapChainD3D12 = m_pProxySwapChain.RawPtr<ProxySwapChainD3D12>();
    pProxySwapChainD3D12->CreateBuffers(GraphicsImpl->GetDXGISwapChain(), GraphicsImpl->GetDepthBuffer());
}

void DiligentGraphicsAdapterD3D12::BeginFrame()
{
    auto *GraphicsImpl = m_UnityGraphicsD3D12.GetGraphicsImpl();
    auto *pProxySwapChainD3D12 = m_pProxySwapChain.RawPtr<ProxySwapChainD3D12>();
    pProxySwapChainD3D12->SetBackBufferIndex(GraphicsImpl->GetCurrentBackBufferIndex());
    // Unity graphics emulator transitions render target to D3D12_RESOURCE_STATE_RENDER_TARGET,
    // and depth buffer to D3D12_RESOURCE_STATE_DEPTH_WRITE state
    pProxySwapChainD3D12->GetCurrentBackBuffer()->SetD3D12ResourceState(D3D12_RESOURCE_STATE_RENDER_TARGET);
    pProxySwapChainD3D12->GetDepthBuffer()->SetD3D12ResourceState(D3D12_RESOURCE_STATE_DEPTH_WRITE);
}
    
void DiligentGraphicsAdapterD3D12::EndFrame()
{
    // Unity graphics emulator expects render target to be D3D12_RESOURCE_STATE_RENDER_TARGET,
    // and depth buffer to be in D3D12_RESOURCE_STATE_DEPTH_WRITE state
    auto *pCtxD3D12 = m_pDeviceCtx.RawPtr<IDeviceContextD3D12>();
    auto *pProxySwapChainD3D12 = m_pProxySwapChain.RawPtr<ProxySwapChainD3D12>();
    auto *pCurrentBackBuffer = pProxySwapChainD3D12->GetCurrentBackBuffer();
    auto *pDepthBuffer = pProxySwapChainD3D12->GetDepthBuffer();
    pCtxD3D12->TransitionTextureState(pCurrentBackBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
    pCtxD3D12->TransitionTextureState(pDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    m_pDeviceCtx->Flush();
    m_pDeviceCtx->FinishFrame();
    m_pDeviceCtx->InvalidateState();
    m_pDevice.RawPtr<IRenderDeviceD3D12>()->ReleaseStaleResources();
}

bool DiligentGraphicsAdapterD3D12::UsesReverseZ()
{ 
    return m_UnityGraphicsD3D12.UsesReverseZ(); 
}
