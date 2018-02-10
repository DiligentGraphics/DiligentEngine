#pragma once

#if PLATFORM_WIN32
    #include <d3d11.h>
#elif PLATFORM_UNIVERSAL_WINDOWS
    #include <d3d11_2.h>
#endif

#include <dxgi1_2.h>
#include <atlbase.h>

#include <memory>

class UnityGraphicsD3D11Impl
{
public:
    void CreateDeviceAndContext();
    void CreateSwapChain(void* pNativeWndHandle, unsigned int Width, unsigned int Height);
    void CreateRTVandDSV();
    void Present();
    void ResizeSwapChain(UINT NewWidth, UINT NewHeight);


    ID3D11Device* GetD3D11Device() { return m_d3d11Device; }
    IDXGISwapChain* GetDXGISwapChain() { return m_SwapChain; }
    ID3D11DeviceContext* GetD3D11Context() { return m_d3d11Context; }
    ID3D11RenderTargetView* GetRTV() { return m_BackBufferRTV; }
    ID3D11DepthStencilView* GetDSV() { return m_DepthBufferDSV; }
    UINT GetBackBufferWidth()const { return m_BackBufferWidth; }
    UINT GetBackBufferHeight()const { return m_BackBufferHeight; }
    IDXGISwapChain* GetSwapChain(){ return m_SwapChain; }

private:
    UINT m_BackBufferWidth = 0, m_BackBufferHeight = 0;
    CComPtr<ID3D11Device> m_d3d11Device;
    CComPtr<ID3D11DeviceContext> m_d3d11Context;
    CComPtr<IDXGISwapChain> m_SwapChain;
    CComPtr<ID3D11RenderTargetView> m_BackBufferRTV;
    CComPtr<ID3D11DepthStencilView> m_DepthBufferDSV;
};
