#include <d3d11.h>
#include <dxgi1_2.h>

#include "DiligentGraphicsAdapterD3D11.h"
#include "UnityGraphicsD3D11Emulator.h"
#include "RenderDeviceFactoryD3D11.h"
#include "SwapChainBase.h"
#include "SwapChainD3D11.h"
#include "DefaultRawMemoryAllocator.h"
#include "UnityGraphicsD3D11Impl.h"
#include "DXGITypeConversions.h"
#include "RenderDeviceD3D11.h"

using namespace Diligent;

namespace 
{

class ProxySwapChainD3D11 : public SwapChainBase<ISwapChainD3D11>
{
public:
    using TBase = SwapChainBase<ISwapChainD3D11>;

    ProxySwapChainD3D11( IReferenceCounters *pRefCounters,
                         IRenderDevice *pDevice,
                         IDeviceContext *pDeviceContext,
                         const SwapChainDesc& SCDesc ) : 
        TBase(pRefCounters, pDevice, pDeviceContext,SCDesc)
    {}
        
    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_SwapChainD3D11, TBase)

    virtual IDXGISwapChain *GetDXGISwapChain()override final
    {
        UNEXPECTED("DXGI swap chain cannot be requested through the proxy swap chain");
        return nullptr;
    }

    virtual void Present(Uint32 SyncInterval)override final
    {
        UNEXPECTED("Present is not expected to be called directly");
    }

    virtual void Resize(Uint32 NewWidth, Uint32 NewHeight)override final
    {
        TBase::Resize(NewWidth, NewHeight, 0);
    }

    void ReleaseViews()
    {
        m_pRTV.Release();
        m_pDSV.Release();
    }

    void CreateViews(ID3D11RenderTargetView* pd3d11RTV, ID3D11DepthStencilView* pd3d11DSV)
    {
        CComPtr<ID3D11Resource> pd3dBackBuffer;
        pd3d11RTV->GetResource(&pd3dBackBuffer);
        CComPtr<ID3D11Texture2D> pd3dTex2DBackBuffer;
        pd3dBackBuffer->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pd3dTex2DBackBuffer));

        CComPtr<ID3D11Resource> pd3dDepthBuffer;
        pd3d11DSV->GetResource(&pd3dDepthBuffer);
        CComPtr<ID3D11Texture2D> pd3dTex2DDepthBuffer;
        pd3dDepthBuffer->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pd3dTex2DDepthBuffer));

        RefCntAutoPtr<IRenderDeviceD3D11> pRenderDeviceD3D11(m_pRenderDevice, IID_RenderDeviceD3D11);
        RefCntAutoPtr<ITexture> pBackBuffer;
        pRenderDeviceD3D11->CreateTextureFromD3DResource(pd3dTex2DBackBuffer, RESOURCE_STATE_UNDEFINED, &pBackBuffer);
        TextureViewDesc RTVDesc;
        RTVDesc.ViewType = TEXTURE_VIEW_RENDER_TARGET;
        RTVDesc.Format = m_SwapChainDesc.ColorBufferFormat;
        RefCntAutoPtr<ITextureView> pRTV;
        pBackBuffer->CreateView(RTVDesc, &pRTV);
        m_pRTV = RefCntAutoPtr<ITextureViewD3D11>(pRTV, IID_TextureViewD3D11);

        RefCntAutoPtr<ITexture> pDepthBuffer;
        pRenderDeviceD3D11->CreateTextureFromD3DResource(pd3dTex2DDepthBuffer, RESOURCE_STATE_UNDEFINED, &pDepthBuffer);
        m_pDSV = RefCntAutoPtr<ITextureViewD3D11>(pDepthBuffer->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL), IID_TextureViewD3D11);
    }

    virtual void SetFullscreenMode(const DisplayModeAttribs &DisplayMode)override final
    {
        UNEXPECTED("Fullscreen mode cannot be set through the proxy swap chain");
    }

    virtual void SetWindowedMode()override final
    {
        UNEXPECTED("Windowed mode cannot be set through the proxy swap chain");
    }

    virtual ITextureViewD3D11* GetCurrentBackBufferRTV()override final{return m_pRTV;}
    virtual ITextureViewD3D11* GetDepthBufferDSV()      override final{return m_pDSV;}
    
    bool BuffersInitialized() const {return m_pRTV && m_pDSV;}

private:
    RefCntAutoPtr<ITextureViewD3D11> m_pRTV;
    RefCntAutoPtr<ITextureViewD3D11> m_pDSV;
};

}

DiligentGraphicsAdapterD3D11::DiligentGraphicsAdapterD3D11(const UnityGraphicsD3D11Emulator& UnityGraphicsD3D11)noexcept :
    m_UnityGraphicsD3D11(UnityGraphicsD3D11)
{
    auto *GraphicsD3D11Impl = m_UnityGraphicsD3D11.GetGraphicsImpl();
    ID3D11Device *pd3d11Device = GraphicsD3D11Impl->GetD3D11Device();
    ID3D11DeviceContext *pd3d11Context = GraphicsD3D11Impl->GetD3D11Context();
    auto *pFactoryD3d11 = GetEngineFactoryD3D11();
    EngineD3D11Attribs Attribs;
    pFactoryD3d11->AttachToD3D11Device(pd3d11Device, pd3d11Context, Attribs, &m_pDevice, &m_pDeviceCtx, 0);
}

void DiligentGraphicsAdapterD3D11::InitProxySwapChain()
{
    auto *GraphicsD3D11Impl = m_UnityGraphicsD3D11.GetGraphicsImpl();
    D3D11_RENDER_TARGET_VIEW_DESC RTVDesc;
    auto *pBackBufferRTV = GraphicsD3D11Impl->GetRTV();
    pBackBufferRTV->GetDesc(&RTVDesc);
    D3D11_DEPTH_STENCIL_VIEW_DESC DSVDesc;
    auto *pDepthBufferDSV = GraphicsD3D11Impl->GetDSV();
    pDepthBufferDSV->GetDesc(&DSVDesc);

    SwapChainDesc SCDesc;
    SCDesc.ColorBufferFormat = DXGI_FormatToTexFormat(RTVDesc.Format);
    SCDesc.DepthBufferFormat = DXGI_FormatToTexFormat(DSVDesc.Format);
    SCDesc.Width = GraphicsD3D11Impl->GetBackBufferWidth();
    SCDesc.Height = GraphicsD3D11Impl->GetBackBufferHeight();
    // These fields are irrelevant
    SCDesc.SamplesCount = 0;
    SCDesc.BufferCount = 0;

    auto &DefaultAllocator = DefaultRawMemoryAllocator::GetAllocator();
    auto pProxySwapChainD3D11 = NEW_RC_OBJ(DefaultAllocator, "ProxySwapChainD3D11 instance", ProxySwapChainD3D11)(m_pDevice, m_pDeviceCtx, SCDesc);
    pProxySwapChainD3D11->QueryInterface(IID_SwapChain, reinterpret_cast<IObject**>(static_cast<ISwapChain**>(&m_pProxySwapChain)));
    pProxySwapChainD3D11->CreateViews(pBackBufferRTV, pDepthBufferDSV);

    m_pDeviceCtx->SetSwapChain(m_pProxySwapChain);
}

void DiligentGraphicsAdapterD3D11::BeginFrame()
{
}

void DiligentGraphicsAdapterD3D11::PreSwapChainResize()
{
    auto *pProxySwapChainD3D11 = m_pProxySwapChain.RawPtr<ProxySwapChainD3D11>();
    pProxySwapChainD3D11->ReleaseViews();
}

void DiligentGraphicsAdapterD3D11::PostSwapChainResize()
{
    auto *GraphicsD3D11Impl = m_UnityGraphicsD3D11.GetGraphicsImpl();
    auto *pProxySwapChainD3D11 = m_pProxySwapChain.RawPtr<ProxySwapChainD3D11>();
    pProxySwapChainD3D11->Resize(GraphicsD3D11Impl->GetBackBufferWidth(), GraphicsD3D11Impl->GetBackBufferHeight());
    pProxySwapChainD3D11->CreateViews(GraphicsD3D11Impl->GetRTV(), GraphicsD3D11Impl->GetDSV());
}
    
void DiligentGraphicsAdapterD3D11::EndFrame()
{
    m_pDeviceCtx->InvalidateState();
}

bool DiligentGraphicsAdapterD3D11::UsesReverseZ()
{ 
    return m_UnityGraphicsD3D11.UsesReverseZ(); 
}