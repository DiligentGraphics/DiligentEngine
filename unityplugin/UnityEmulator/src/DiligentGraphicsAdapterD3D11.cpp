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

    virtual ID3D11RenderTargetView *GetRTV()override final{ return m_pRTV; }

    virtual ID3D11DepthStencilView *GetDSV()override final { return m_pDSV; }

    virtual void Present(Uint32 SyncInterval)override final
    {
        UNEXPECTED("Present is not expected to be called directly");
    }

    virtual void Resize(Uint32 NewWidth, Uint32 NewHeight)override final
    {
        TBase::Resize(NewWidth, NewHeight, 0);
    }

    void SetSwapChainAttribs(ID3D11RenderTargetView *pRTV, ID3D11DepthStencilView *pDSV, Uint32 Width, Uint32 Height)
    {
        TBase::Resize(Width, Height, 0);
        m_pRTV = pRTV;
        m_pDSV = pDSV;
    }

    void ResetSwapChainAttribs()
    {
        m_pRTV = nullptr;
        m_pDSV = nullptr;
    }

    virtual void SetFullscreenMode(const DisplayModeAttribs &DisplayMode)override final
    {
        UNEXPECTED("Fullscreen mode cannot be set through the proxy swap chain");
    }

    virtual void SetWindowedMode()override final
    {
        UNEXPECTED("Windowed mode cannot be set through the proxy swap chain");
    }

private:
    ID3D11RenderTargetView *m_pRTV = nullptr;
    ID3D11DepthStencilView *m_pDSV = nullptr;
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
    GraphicsD3D11Impl->GetRTV()->GetDesc(&RTVDesc);
    D3D11_DEPTH_STENCIL_VIEW_DESC DSVDesc;
    GraphicsD3D11Impl->GetDSV()->GetDesc(&DSVDesc);

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

    m_pDeviceCtx->SetSwapChain(m_pProxySwapChain);
}

void DiligentGraphicsAdapterD3D11::BeginFrame()
{
    auto *UnityGraphicsD3D11Impl = m_UnityGraphicsD3D11.GetGraphicsImpl();
    auto *pRTV = UnityGraphicsD3D11Impl->GetRTV();
    auto *pDSV = UnityGraphicsD3D11Impl->GetDSV();
    auto Width = UnityGraphicsD3D11Impl->GetBackBufferWidth();
    auto Height = UnityGraphicsD3D11Impl->GetBackBufferHeight();
    VERIFY_EXPR(pRTV != nullptr && pDSV != nullptr && Width != 0 && Height != 0);
    ValidatedCast<ProxySwapChainD3D11>(m_pProxySwapChain.RawPtr())->SetSwapChainAttribs(pRTV, pDSV, Width, Height);
}
    
void DiligentGraphicsAdapterD3D11::EndFrame()
{
    ValidatedCast<ProxySwapChainD3D11>(m_pProxySwapChain.RawPtr())->ResetSwapChainAttribs();
    m_pDeviceCtx->InvalidateState();
}

bool DiligentGraphicsAdapterD3D11::UsesReverseZ()
{ 
    return m_UnityGraphicsD3D11.UsesReverseZ(); 
}