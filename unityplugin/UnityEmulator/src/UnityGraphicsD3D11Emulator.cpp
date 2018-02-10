
#include <stdexcept>

#include "UnityGraphicsD3D11Impl.h"

#include "UnityGraphicsD3D11Emulator.h"
#include "IUnityGraphicsD3D11.h"
#include "DebugUtilities.h"
#include "Errors.h"


#if defined(_DEBUG)
// Check for SDK Layer support.
inline bool SdkLayersAvailable()
{
	HRESULT hr = D3D11CreateDevice(
		nullptr,
		D3D_DRIVER_TYPE_NULL,       // There is no need to create a real hardware device.
		0,
		D3D11_CREATE_DEVICE_DEBUG,  // Check for the SDK layers.
		nullptr,                    // Any feature level will do.
		0,
		D3D11_SDK_VERSION,          // Always set this to D3D11_SDK_VERSION for Windows Store apps.
		nullptr,                    // No need to keep the D3D device reference.
		nullptr,                    // No need to know the feature level.
		nullptr                     // No need to keep the D3D device context reference.
		);

	return SUCCEEDED(hr);
}
#endif

void UnityGraphicsD3D11Impl::CreateDeviceAndContext()
{
	// This flag adds support for surfaces with a different color channel ordering
	// than the API default. It is required for compatibility with Direct2D.
    // D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    UINT creationFlags = 0;

#if defined(_DEBUG)
	if (SdkLayersAvailable())
	{
		// If the project is in a debug build, enable debugging via SDK Layers with this flag.
		creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
	}
#endif

	// This array defines the set of DirectX hardware feature levels this app will support.
	// Note the ordering should be preserved.
	// Don't forget to declare your application's minimum required feature level in its
	// description.  All applications are assumed to support 9.1 unless otherwise stated.
	D3D_FEATURE_LEVEL featureLevels[] = 
	{
#if PLATFORM_UNIVERSAL_WINDOWS
		D3D_FEATURE_LEVEL_11_1,
#endif
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};

	// Create the Direct3D 11 API device object and a corresponding context.
    D3D_FEATURE_LEVEL d3dFeatureLevel = D3D_FEATURE_LEVEL_11_0;
	HRESULT hr = D3D11CreateDevice(
		nullptr,					// Specify nullptr to use the default adapter.
		D3D_DRIVER_TYPE_HARDWARE,	// Create a device using the hardware graphics driver.
		0,							// Should be 0 unless the driver is D3D_DRIVER_TYPE_SOFTWARE.
		creationFlags,				// Set debug and Direct2D compatibility flags.
		featureLevels,				// List of feature levels this app can support.
		ARRAYSIZE(featureLevels),	// Size of the list above.
		D3D11_SDK_VERSION,			// Always set this to D3D11_SDK_VERSION for Windows Store apps.
		&m_d3d11Device,				// Returns the Direct3D device created.
		&d3dFeatureLevel,			// Returns feature level of device created.
		&m_d3d11Context				// Returns the device immediate context.
		);

	if (FAILED(hr))
	{
		// If the initialization fails, fall back to the WARP device.
		// For more information on WARP, see: 
		// http://go.microsoft.com/fwlink/?LinkId=286690
		hr = D3D11CreateDevice(
				nullptr,
				D3D_DRIVER_TYPE_WARP, // Create a WARP device instead of a hardware device.
				0,
				creationFlags,
				featureLevels,
				ARRAYSIZE(featureLevels),
				D3D11_SDK_VERSION,
				&m_d3d11Device,
				&d3dFeatureLevel,
				&m_d3d11Context
				);
        throw std::runtime_error("Failed to create D3D11 native device and immediate context");
        return;
	}
}

void UnityGraphicsD3D11Impl::CreateSwapChain(void* pNativeWndHandle, unsigned int Width, unsigned int Height)
{
    m_BackBufferWidth = 0;
    m_BackBufferHeight = 0;

#if PLATFORM_WIN32
    auto hWnd = reinterpret_cast<HWND>(pNativeWndHandle);
    RECT rc;
    GetClientRect( hWnd, &rc );
    VERIFY_EXPR( static_cast<LONG>(Width) == rc.right - rc.left);
    VERIFY_EXPR(static_cast<LONG>(Height) == rc.bottom - rc.top);
#endif

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = m_BackBufferWidth = Width;
    swapChainDesc.Height = m_BackBufferHeight = Height;
    //  Flip model swapchains (DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL and DXGI_SWAP_EFFECT_FLIP_DISCARD) only support the following Formats: 
    //  - DXGI_FORMAT_R16G16B16A16_FLOAT 
    //  - DXGI_FORMAT_B8G8R8A8_UNORM
    //  - DXGI_FORMAT_R8G8B8A8_UNORM
    //  - DXGI_FORMAT_R10G10B10A2_UNORM
    // If RGBA8_UNORM_SRGB swap chain is required, we will create RGBA8_UNORM swap chain, but
    // create RGBA8_UNORM_SRGB render target view
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.Scaling = DXGI_SCALING_NONE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED; // Not used
    swapChainDesc.Flags = 0;

    CComPtr<IDXGISwapChain1> pSwapChain1;

#if PLATFORM_WIN32
	// This sequence obtains the DXGI factory that was used to create the Direct3D device above.
	CComPtr<IDXGIDevice> pDXGIDevice;
	m_d3d11Device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>( static_cast<IDXGIDevice**>(&pDXGIDevice) ) );
    CComPtr<IDXGIAdapter> pDXGIAdapter;
    pDXGIDevice->GetAdapter(&pDXGIAdapter);
	CComPtr<IDXGIFactory2> pDXGIFactory;
    pDXGIAdapter->GetParent(__uuidof(pDXGIFactory), reinterpret_cast<void**>( static_cast<IDXGIFactory2**>(&pDXGIFactory) ));

    auto hr = pDXGIFactory->CreateSwapChainForHwnd(m_d3d11Device, hWnd, &swapChainDesc, nullptr, nullptr, &pSwapChain1);
    if(FAILED(hr))
        throw std::runtime_error( "Failed to create DXGI swap chain" );

#elif PLATFORM_UNIVERSAL_WINDOWS

	CComPtr<IDXGIDevice3> pDXGIDevice;
	m_d3d11Device->QueryInterface(__uuidof(IDXGIDevice3), reinterpret_cast<void**>(static_cast<IDXGIDevice3**>(&pDXGIDevice)));
    CComPtr<IDXGIAdapter> pDXGIAdapter;
    pDXGIDevice->GetAdapter(&pDXGIAdapter);
	CComPtr<IDXGIFactory2> pDXGIFactory;
    pDXGIAdapter->GetParent(__uuidof(pDXGIFactory), reinterpret_cast<void**>(static_cast<IDXGIFactory2**>(&pDXGIFactory)));
    
    HRESULT hr = pDXGIFactory->CreateSwapChainForCoreWindow(
		m_d3d11Device,
		reinterpret_cast<IUnknown*>(pNativeWndHandle),
		&swapChainDesc,
		nullptr,
		&pSwapChain1);
    if(FAILED(hr))
        throw std::runtime_error( "Failed to create DXGI swap chain" );

	// Ensure that DXGI does not queue more than one frame at a time. This both reduces latency and
	// ensures that the application will only render after each VSync, minimizing power consumption.
    pDXGIDevice->SetMaximumFrameLatency( 1 );

#endif

    pSwapChain1->QueryInterface( __uuidof(m_SwapChain), reinterpret_cast<void**>(static_cast<IDXGISwapChain**>(&m_SwapChain)) );

    CreateRTVandDSV();
}

void UnityGraphicsD3D11Impl::CreateRTVandDSV()
{
    m_BackBufferRTV.Release();
    m_DepthBufferDSV.Release();

    // Create a render target view
    CComPtr<ID3D11Texture2D> pBackBuffer;
    auto hr = m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(static_cast<ID3D11Texture2D**>(&pBackBuffer)));
    VERIFY_EXPR(SUCCEEDED(hr));

    D3D11_RENDER_TARGET_VIEW_DESC RTVDesc = {};
    RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    // We need to explicitly specify RTV format, as we may need to create RGBA8_UNORM_SRGB RTV for
    // a RGBA8_UNORM swap chain
    RTVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    RTVDesc.Texture2D.MipSlice = 0;
    hr = m_d3d11Device->CreateRenderTargetView(pBackBuffer, &RTVDesc, &m_BackBufferRTV);
    VERIFY_EXPR(SUCCEEDED(hr));

    DXGI_SWAP_CHAIN_DESC SwapChainDesc;
    m_SwapChain->GetDesc(&SwapChainDesc);
    // Create depth buffer
    D3D11_TEXTURE2D_DESC DepthBufferDesc;
    DepthBufferDesc.Width = m_BackBufferWidth;
    DepthBufferDesc.Height = m_BackBufferHeight;
    DepthBufferDesc.MipLevels = 1;
    DepthBufferDesc.ArraySize = 1;
    auto DepthFormat = DXGI_FORMAT_D32_FLOAT;
    DepthBufferDesc.Format = DepthFormat;
    DepthBufferDesc.SampleDesc.Count = 1;
    DepthBufferDesc.SampleDesc.Quality = 0;
    DepthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    DepthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    DepthBufferDesc.CPUAccessFlags = 0;
    DepthBufferDesc.MiscFlags = 0;
    CComPtr<ID3D11Texture2D> ptex2DDepthBuffer;
    hr = m_d3d11Device->CreateTexture2D(&DepthBufferDesc, NULL, &ptex2DDepthBuffer);
    VERIFY_EXPR(SUCCEEDED(hr));

    // Create DSV
    m_d3d11Device->CreateDepthStencilView(ptex2DDepthBuffer, NULL, &m_DepthBufferDSV);
    VERIFY_EXPR(SUCCEEDED(hr));

    ID3D11RenderTargetView *DefaultRTV[] = { GetRTV() };
    m_d3d11Context->OMSetRenderTargets(1, DefaultRTV, GetDSV());
}


void UnityGraphicsD3D11Impl::Present()
{
    UINT SyncInterval = 1; // 0
#if PLATFORM_UNIVERSAL_WINDOWS
    SyncInterval = 1; // Interval 0 is not supported on Windows Phone 
#endif

    ID3D11RenderTargetView *NullTRV[] = { nullptr };
    m_d3d11Context->OMSetRenderTargets(1, NullTRV, nullptr);

    m_SwapChain->Present( SyncInterval, 0 );
    // A successful Present call for DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL SwapChains unbinds 
    // backbuffer 0 from all GPU writeable bind points.
    // We need to rebind all render targets to make sure that
    // the back buffer is not unbound
}

void UnityGraphicsD3D11Impl::ResizeSwapChain( UINT NewWidth, UINT NewHeight )
{
    if (NewWidth == 0 || NewHeight == 0)
        return;

    ID3D11RenderTargetView *NullTRV[] = { nullptr };
    m_d3d11Context->OMSetRenderTargets(1, NullTRV, nullptr);
    // Swap chain cannot be resized until all references are released
    m_BackBufferRTV.Release();
    m_DepthBufferDSV.Release();
    try
    {
        m_BackBufferWidth  = NewWidth;
        m_BackBufferHeight = NewHeight;
        DXGI_SWAP_CHAIN_DESC SCDes;
        memset( &SCDes, 0, sizeof( SCDes ) );
        m_SwapChain->GetDesc( &SCDes );
        auto hr = m_SwapChain->ResizeBuffers(SCDes.BufferCount, m_BackBufferWidth, m_BackBufferHeight, SCDes.BufferDesc.Format, 
                                                SCDes.Flags);
        if(FAILED(hr))
            throw std::runtime_error("Failed to resize the DXGI swap chain");


        CreateRTVandDSV();
    }
    catch( const std::runtime_error & )
    {
        LOG_ERROR( "Failed to resize the swap chain" );
    }
}


std::unique_ptr<UnityGraphicsD3D11Impl> UnityGraphicsD3D11Emulator::m_GraphicsImpl;

UnityGraphicsD3D11Emulator::UnityGraphicsD3D11Emulator()
{
    VERIFY(!m_GraphicsImpl, "Another emulator has already been initialized");
    m_GraphicsImpl.reset( new UnityGraphicsD3D11Impl );
    GeUnityInterfaces().RegisterInterface(IUnityGraphicsD3D11_GUID, GetUnityGraphicsAPIInterface());
}

UnityGraphicsD3D11Emulator& UnityGraphicsD3D11Emulator::GetInstance()
{
    static UnityGraphicsD3D11Emulator TheInstance;
    return TheInstance;
}

void UnityGraphicsD3D11Emulator::CreateD3D11DeviceAndContext()
{
    m_GraphicsImpl->CreateDeviceAndContext();
}

void UnityGraphicsD3D11Emulator::CreateSwapChain(void *pNativeWndHandle, unsigned int Width, unsigned int Height)
{
    m_GraphicsImpl->CreateSwapChain(pNativeWndHandle, Width, Height);
}

void UnityGraphicsD3D11Emulator::Present()
{
    m_GraphicsImpl->Present();
}

void UnityGraphicsD3D11Emulator::Release()
{
    m_GraphicsImpl.reset();
}

void UnityGraphicsD3D11Emulator::ResizeSwapChain(unsigned int Width, unsigned int Height)
{
    m_GraphicsImpl->ResizeSwapChain(Width, Height);
}

bool UnityGraphicsD3D11Emulator::SwapChainInitialized()
{
    return m_GraphicsImpl->GetSwapChain() != nullptr;
}

void* UnityGraphicsD3D11Emulator::GetD3D11Device()
{
    return m_GraphicsImpl->GetD3D11Device();
}

void* UnityGraphicsD3D11Emulator::GetDXGISwapChain()
{
    return m_GraphicsImpl->GetDXGISwapChain();
}

void UnityGraphicsD3D11Emulator::GetBackBufferSize(unsigned int &Width, unsigned int &Height)
{
    Width  = m_GraphicsImpl->GetBackBufferWidth();
    Height = m_GraphicsImpl->GetBackBufferHeight();
}

static ID3D11Device* UNITY_INTERFACE_API UnityGraphicsD3D11_GetDevice()
{
    auto *GraphicsImpl = UnityGraphicsD3D11Emulator::GetGraphicsImpl();
    return GraphicsImpl != nullptr ? GraphicsImpl->GetD3D11Device() : nullptr;
}

UnityGraphicsD3D11Impl* UnityGraphicsD3D11Emulator::GetGraphicsImpl()
{
    return m_GraphicsImpl.get();
}

IUnityInterface* UnityGraphicsD3D11Emulator::GetUnityGraphicsAPIInterface()
{
    static IUnityGraphicsD3D11 UnityGraphicsD3D11;
    UnityGraphicsD3D11.GetDevice = UnityGraphicsD3D11_GetDevice;
    return &UnityGraphicsD3D11;
}

UnityGfxRenderer UnityGraphicsD3D11Emulator::GetUnityGfxRenderer()
{
    return kUnityGfxRendererD3D11;
}

void UnityGraphicsD3D11Emulator::BeginFrame()
{
    auto d3d11Ctx = m_GraphicsImpl->GetD3D11Context();
    ID3D11RenderTargetView *DefaultRTV[] = { m_GraphicsImpl->GetRTV() };
    d3d11Ctx->OMSetRenderTargets(1, DefaultRTV, m_GraphicsImpl->GetDSV());
    D3D11_VIEWPORT Viewport;
    Viewport.TopLeftX = 0;
    Viewport.TopLeftY = 0;
    Viewport.Width = static_cast<float>( m_GraphicsImpl->GetBackBufferWidth() );
    Viewport.Height = static_cast<float>( m_GraphicsImpl->GetBackBufferHeight() );
    Viewport.MinDepth = 0;
    Viewport.MaxDepth = 1;
    d3d11Ctx->RSSetViewports(1, &Viewport);
    float ClearColor[] = { 0, 0, 0.5f, 1 };
    d3d11Ctx->ClearRenderTargetView(DefaultRTV[0], ClearColor);
    d3d11Ctx->ClearDepthStencilView(m_GraphicsImpl->GetDSV(), D3D11_CLEAR_DEPTH, UsesReverseZ() ? 0.f : 1.f, 0);
}

void UnityGraphicsD3D11Emulator::EndFrame()
{
}
