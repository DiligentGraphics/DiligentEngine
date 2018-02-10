
#include "UnityGraphicsD3D12Impl.h"
#include "UnityGraphicsD3D12Emulator.h"
#include "IUnityGraphicsD3D12.h"
#include "DebugUtilities.h"
#include "Errors.h"

UnityGraphicsD3D12Impl::UnityGraphicsD3D12Impl() :
    m_WaitForGPUEventHandle( CreateEvent(nullptr, false, false, nullptr) )
{
    VERIFY_EXPR(m_WaitForGPUEventHandle != INVALID_HANDLE_VALUE);
}

UnityGraphicsD3D12Impl::~UnityGraphicsD3D12Impl()
{
    IdleGPU();
    CloseHandle(m_WaitForGPUEventHandle);
}

static void GetHardwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1** ppAdapter)
{
	CComPtr<IDXGIAdapter1> adapter;
	*ppAdapter = nullptr;

	for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			// Don't select the Basic Render Driver adapter.
			// If you want a software adapter, pass in "/warp" on the command line.
			continue;
		}

		// Check to see if the adapter supports Direct3D 12, but don't create the
		// actual device yet.
		if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
		{
			break;
		}
	}

	*ppAdapter = adapter.Detach();
}


void UnityGraphicsD3D12Impl::CreateDeviceAndCommandQueue()
{
#if defined(_DEBUG)
	// Enable the D3D12 debug layer.
	{
		CComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(__uuidof(debugController), reinterpret_cast<void**>(static_cast<ID3D12Debug**>(&debugController)) )))
		{
			debugController->EnableDebugLayer();
		}
	}
#endif

	CComPtr<IDXGIFactory4> factory;
    HRESULT hr = CreateDXGIFactory1(__uuidof(factory), reinterpret_cast<void**>(static_cast<IDXGIFactory4**>(&factory)) );
    if(FAILED(hr)) LOG_ERROR_AND_THROW("Failed to create DXGI factory");

	CComPtr<IDXGIAdapter1> hardwareAdapter;
	GetHardwareAdapter(factory, &hardwareAdapter);
    
	hr = D3D12CreateDevice(hardwareAdapter, D3D_FEATURE_LEVEL_11_0, __uuidof(m_D3D12Device), reinterpret_cast<void**>(static_cast<ID3D12Device**>(&m_D3D12Device)) );
    if( FAILED(hr))
    {
        LOG_WARNING_MESSAGE("Failed to create hardware device. Attempting to create WARP device");

		CComPtr<IDXGIAdapter> warpAdapter;
		hr = factory->EnumWarpAdapter( __uuidof(warpAdapter),  reinterpret_cast<void**>(static_cast<IDXGIAdapter**>(&warpAdapter)) );
        if(FAILED(hr)) LOG_ERROR_AND_THROW("Failed to enum warp adapter");

		hr = D3D12CreateDevice( warpAdapter, D3D_FEATURE_LEVEL_11_0, __uuidof(m_D3D12Device), reinterpret_cast<void**>(static_cast<ID3D12Device**>(&m_D3D12Device)) );
        if(FAILED(hr)) LOG_ERROR_AND_THROW("Failed to crate warp device");
    }

#if _DEBUG
    {
	    CComPtr<ID3D12InfoQueue> pInfoQueue;
        hr = m_D3D12Device->QueryInterface(__uuidof(pInfoQueue), reinterpret_cast<void**>(static_cast<ID3D12InfoQueue**>(&pInfoQueue)));
	    if( SUCCEEDED(hr) )
	    {
		    // Suppress whole categories of messages
		    //D3D12_MESSAGE_CATEGORY Categories[] = {};

		    // Suppress messages based on their severity level
		    D3D12_MESSAGE_SEVERITY Severities[] = 
		    {
			    D3D12_MESSAGE_SEVERITY_INFO
		    };

		    // Suppress individual messages by their ID
		    //D3D12_MESSAGE_ID DenyIds[] = {};

		    D3D12_INFO_QUEUE_FILTER NewFilter = {};
		    //NewFilter.DenyList.NumCategories = _countof(Categories);
		    //NewFilter.DenyList.pCategoryList = Categories;
		    NewFilter.DenyList.NumSeverities = _countof(Severities);
		    NewFilter.DenyList.pSeverityList = Severities;
		    //NewFilter.DenyList.NumIDs = _countof(DenyIds);
		    //NewFilter.DenyList.pIDList = DenyIds;

		    hr = pInfoQueue->PushStorageFilter(&NewFilter);
            VERIFY(SUCCEEDED(hr), "Failed to push storage filter");
        }
    }
#endif

    {
        hr = m_D3D12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(m_D3D12FrameFence), reinterpret_cast<void**>(static_cast<ID3D12Fence**>(&m_D3D12FrameFence)));
        VERIFY(SUCCEEDED(hr), "Failed to create the fence");
	    m_D3D12FrameFence->SetName(L"Completed Frame Fence fence");
	    m_D3D12FrameFence->Signal(m_CompletedFenceValue); // 0 cmd lists are completed
    }

#ifndef RELEASE
	// Prevent the GPU from overclocking or underclocking to get consistent timings
	//d3d12Device->SetStablePowerState(TRUE);
#endif

	// Describe and create the command queue.
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    hr = m_D3D12Device->CreateCommandQueue(&queueDesc, __uuidof(m_D3D12CmdQueue), reinterpret_cast<void**>(static_cast<ID3D12CommandQueue**>(&m_D3D12CmdQueue)));
    if(FAILED(hr)) LOG_ERROR_AND_THROW("Failed to create command queue");

    hr = m_D3D12Device->SetName(L"Main Command Queue");
    VERIFY_EXPR(SUCCEEDED(hr));
}

void UnityGraphicsD3D12Impl::CreateSwapChain(void* pNativeWndHandle, unsigned int Width, unsigned int Height)
{
#if PLATFORM_WIN32
    auto hWnd = reinterpret_cast<HWND>(pNativeWndHandle);
    RECT rc;
    GetClientRect( hWnd, &rc );
    VERIFY_EXPR(static_cast<LONG>(Width)  == rc.right - rc.left);
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
    // We will create RGBA8_UNORM swap chain, but RGBA8_UNORM_SRGB render target view
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = m_BackBuffersCount;
    swapChainDesc.Scaling = DXGI_SCALING_NONE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED; // Not used
    swapChainDesc.Flags = 0;

    CComPtr<IDXGISwapChain1> pSwapChain1;
	CComPtr<IDXGIFactory4> factory;
    HRESULT hr = CreateDXGIFactory1(__uuidof(factory), reinterpret_cast<void**>(static_cast<IDXGIFactory4**>(&factory)) );
    if(FAILED(hr))LOG_ERROR_AND_THROW("Failed to create DXGI factory");

#if PLATFORM_WIN32
    hr = factory->CreateSwapChainForHwnd(m_D3D12CmdQueue, hWnd, &swapChainDesc, nullptr, nullptr, &pSwapChain1);
    if(FAILED(hr))LOG_ERROR_AND_THROW("Failed to create Swap Chain" );

	// This sample does not support fullscreen transitions.
	hr = factory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER);

#elif PLATFORM_UNIVERSAL_WINDOWS

    hr = factory->CreateSwapChainForCoreWindow(
		m_D3D12CmdQueue,
		reinterpret_cast<IUnknown*>(pNativeWndHandle),
		&swapChainDesc,
		nullptr,
		&pSwapChain1);
    if(FAILED(hr))LOG_ERROR_AND_THROW("Failed to create DXGI swap chain" );

	// Ensure that DXGI does not queue more than one frame at a time. This both reduces latency and
	// ensures that the application will only render after each VSync, minimizing power consumption.
    //pDXGIDevice->SetMaximumFrameLatency( 1 );
#endif

    pSwapChain1->QueryInterface(__uuidof(m_SwapChain), reinterpret_cast<void**>( static_cast<IDXGISwapChain3**>(&m_SwapChain) ));
        
    m_FrameIndex = m_SwapChain->GetCurrentBackBufferIndex();

    InitBuffersAndViews();
}

void UnityGraphicsD3D12Impl::InitBuffersAndViews()
{
    // Create RTV descriptor heap
    if (!m_RTVDescriptorHeap)
    {
        // Describe and create a render target view (RTV) descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = m_BackBuffersCount;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        auto hr = m_D3D12Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_RTVDescriptorHeap));
        if (FAILED(hr))LOG_ERROR_AND_THROW("Failed to create RTV descriptor heap");

        m_rtvDescriptorSize = m_D3D12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }

    // Create DSV descriptor heap
    if (!m_DSVDescriptorHeap)
    {
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = 1;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        auto hr = m_D3D12Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_DSVDescriptorHeap));
        if (FAILED(hr))LOG_ERROR_AND_THROW("Failed to create DSV descriptor heap");
    }

    {
        // Create render target views
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        // Create a RTV for each frame.
        for (UINT n = 0; n < m_BackBuffersCount; n++)
        {
            auto hr = m_SwapChain->GetBuffer(n, IID_PPV_ARGS(&m_RenderTargets[n]));
            VERIFY_EXPR(SUCCEEDED(hr));

            D3D12_RENDER_TARGET_VIEW_DESC RTVDesc = {};
            RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            RTVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
            RTVDesc.Texture2D.MipSlice = 0;
            RTVDesc.Texture2D.PlaneSlice = 0;
            m_D3D12Device->CreateRenderTargetView(m_RenderTargets[n], &RTVDesc, rtvHandle);
            rtvHandle.ptr += m_rtvDescriptorSize;
        }
    }

    // Create depth-stencil buffer
    {
	    D3D12_HEAP_PROPERTIES HeapProps;
	    HeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	    HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	    HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	    HeapProps.CreationNodeMask = 1;
	    HeapProps.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC Desc = {};
        Desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        Desc.Alignment = 0;
        Desc.Width = m_BackBufferWidth;
        Desc.Height = m_BackBufferHeight;
        Desc.DepthOrArraySize = 1;
        Desc.MipLevels = 1;
        Desc.Format = DXGI_FORMAT_D32_FLOAT;
        Desc.SampleDesc.Count = 1;
        Desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        Desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE ClearValue = {};
        ClearValue.DepthStencil.Depth = 0.f;
        ClearValue.Format = Desc.Format;
        auto hr = m_D3D12Device->CreateCommittedResource( &HeapProps, D3D12_HEAP_FLAG_NONE,
		    &Desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &ClearValue, __uuidof(m_DepthStencilBuffer), reinterpret_cast<void**>(static_cast<ID3D12Resource**>(&m_DepthStencilBuffer)) );
        if(FAILED(hr))LOG_ERROR_AND_THROW("Failed to create depth-stencil buffer");

        // Create depth-stencil view
        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_DSVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        m_D3D12Device->CreateDepthStencilView(m_DepthStencilBuffer, nullptr, dsvHandle);
    }
}

void UnityGraphicsD3D12Impl::Present()
{
    UINT SyncInterval = 0;
#if PLATFORM_UNIVERSAL_WINDOWS
    SyncInterval = 1; // Interval 0 is not supported on Windows Phone 
#endif

    //pImmediateCtxD3D12->Flush();

    auto hr = m_SwapChain->Present( SyncInterval, 0 );
    VERIFY(SUCCEEDED(hr), "Present failed");

    //auto *pDeviceD3D12 = ValidatedCast<RenderDeviceD3D12Impl>( pImmediateCtxD3D12->GetDevice() );
    //pDeviceD3D12->FinishFrame();
    m_FrameIndex = m_SwapChain->GetCurrentBackBufferIndex();
}

void UnityGraphicsD3D12Impl::ResizeSwapChain( UINT NewWidth, UINT NewHeight )
{
    if (NewWidth == 0 || NewHeight == 0)
        return;

    IdleGPU();

    for (auto &rtv : m_RenderTargets) rtv.Release();
    m_DepthStencilBuffer.Release();

    m_BackBufferWidth = NewWidth;
    m_BackBufferHeight = NewHeight;

    DXGI_SWAP_CHAIN_DESC SCDes;
    memset( &SCDes, 0, sizeof( SCDes ) );
    m_SwapChain->GetDesc( &SCDes );
    auto hr = m_SwapChain->ResizeBuffers(SCDes.BufferCount, NewWidth, NewHeight, SCDes.BufferDesc.Format, SCDes.Flags);
    if(FAILED(hr))
        LOG_ERROR_MESSAGE("Failed to resize swap chain");

    m_FrameIndex = m_SwapChain->GetCurrentBackBufferIndex();

    InitBuffersAndViews();
}
    
bool UnityGraphicsD3D12Emulator::SwapChainInitialized()
{
    return m_GraphicsImpl->GetSwapChain() != nullptr;
}

void* UnityGraphicsD3D12Emulator::GetD3D12Device()
{
    return m_GraphicsImpl->GetD3D12Device();
}

void* UnityGraphicsD3D12Emulator::GetDXGISwapChain()
{
    return m_GraphicsImpl->GetDXGISwapChain();
}

void UnityGraphicsD3D12Emulator::GetBackBufferSize(unsigned int &Width, unsigned int &Height)
{
    Width = m_GraphicsImpl->GetBackBufferWidth();
    Height = m_GraphicsImpl->GetBackBufferHeight();
}

CComPtr<ID3D12CommandAllocator> UnityGraphicsD3D12Impl::GetCommandAllocator()
{
    if (m_CmdAllocator)
        return m_CmdAllocator;

    if (!m_DiscardedAllocators.empty())
	{
		auto& AllocatorPair = m_DiscardedAllocators.front();
		if ( IsFenceCompleted(AllocatorPair.first) )
		{
			m_CmdAllocator = AllocatorPair.second;
			auto hr = m_CmdAllocator->Reset();
            VERIFY_EXPR(SUCCEEDED(hr));
			m_DiscardedAllocators.pop_front();
		}
	}

	// If no allocators were ready to be reused, create a new one
	if (!m_CmdAllocator)
	{
		auto hr = m_D3D12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(m_CmdAllocator), reinterpret_cast<void**>(&m_CmdAllocator));
        VERIFY(SUCCEEDED(hr), "Failed to create command allocator");
		m_CmdAllocator->SetName(L"UnityGraphicsD3D12Impl: cmd list allocator");
	}

    return m_CmdAllocator;
}

void UnityGraphicsD3D12Impl::DiscardCommandAllocator()
{
    VERIFY_EXPR(m_CmdAllocator);
	m_DiscardedAllocators.emplace_back( m_NextFenceValue, m_CmdAllocator );
    m_CmdAllocator.Release();
}

bool UnityGraphicsD3D12Impl::IsFenceCompleted(UINT64 FenceValue)
{
    m_CompletedFenceValue = m_D3D12FrameFence->GetCompletedValue();
    return FenceValue <= m_CompletedFenceValue;
}

void UnityGraphicsD3D12Impl::IdleGPU()
{
    auto SignaledValue = m_NextFenceValue;
	m_D3D12CmdQueue->Signal(m_D3D12FrameFence, SignaledValue);
    ++m_NextFenceValue;

    if (m_D3D12FrameFence->GetCompletedValue() < SignaledValue)
    {
        // Wait for the fence
        m_D3D12FrameFence->SetEventOnCompletion(SignaledValue, m_WaitForGPUEventHandle);
        WaitForSingleObject(m_WaitForGPUEventHandle, INFINITE);
    }
}

UINT64 UnityGraphicsD3D12Impl::ExecuteCommandList(ID3D12CommandList *pCmdList)
{
    ID3D12CommandList *CmdLists[] = { pCmdList };
    m_D3D12CmdQueue->ExecuteCommandLists(1, CmdLists);
    auto FenceValue = m_NextFenceValue;
    m_D3D12CmdQueue->Signal(m_D3D12FrameFence, m_NextFenceValue++);
    return FenceValue;
}
    
void UnityGraphicsD3D12Impl::TransitonResourceStates(int stateCount, UnityGraphicsD3D12ResourceState* states)
{
    if (stateCount >= 0)
    {
        VERIFY(m_pStateTransitionHandler != nullptr, "State transition handler is not set");
        m_pStateTransitionHandler->TransitionResources(stateCount, states);
    }
}


std::unique_ptr<UnityGraphicsD3D12Impl> UnityGraphicsD3D12Emulator::m_GraphicsImpl;

UnityGraphicsD3D12Emulator::UnityGraphicsD3D12Emulator()
{
    VERIFY(!m_GraphicsImpl, "Another emulator has already been initialized");
    m_GraphicsImpl.reset( new UnityGraphicsD3D12Impl );
    GeUnityInterfaces().RegisterInterface(IUnityGraphicsD3D12v2_GUID, GetUnityGraphicsAPIInterface());
}

UnityGraphicsD3D12Emulator& UnityGraphicsD3D12Emulator::GetInstance()
{
    static UnityGraphicsD3D12Emulator TheInstance;
    return TheInstance;
}

void UnityGraphicsD3D12Emulator::CreateD3D12DeviceAndCommandQueue()
{
    m_GraphicsImpl->CreateDeviceAndCommandQueue();
}

void UnityGraphicsD3D12Emulator::CreateSwapChain(void *pNativeWndHandle, unsigned int Width, unsigned int Height)
{
    m_GraphicsImpl->CreateSwapChain(pNativeWndHandle, Width, Height);
}

void UnityGraphicsD3D12Emulator::SetTransitionHandler(IResourceStateTransitionHandler *pTransitionHandler)
{
    m_GraphicsImpl->SetTransitionHandler(pTransitionHandler);
}

void UnityGraphicsD3D12Emulator::Present()
{
    m_GraphicsImpl->Present();
}

void UnityGraphicsD3D12Emulator::Release()
{
    m_GraphicsImpl.reset();
}

void UnityGraphicsD3D12Emulator::ResizeSwapChain(unsigned int Width, unsigned int Height)
{
    m_GraphicsImpl->ResizeSwapChain(Width, Height);
}

static ID3D12Device* UNITY_INTERFACE_API UnityGraphicsD3D12_GetDevice()
{
    auto *GraphicsImpl = UnityGraphicsD3D12Emulator::GetGraphicsImpl();
    return GraphicsImpl != nullptr ? GraphicsImpl->GetD3D12Device() : nullptr;
}

static UINT64 UNITY_INTERFACE_API UnityGraphicsD3D12_ExecuteCommandList(ID3D12GraphicsCommandList* commandList, int stateCount, UnityGraphicsD3D12ResourceState* states)
{
    auto *GraphicsImpl = UnityGraphicsD3D12Emulator::GetGraphicsImpl();
    if (GraphicsImpl != nullptr)
    {
        if (stateCount > 0)
            GraphicsImpl->TransitonResourceStates(stateCount, states);
        return GraphicsImpl->ExecuteCommandList(commandList);
    }
    return 0;
}

UINT64 UNITY_INTERFACE_API UnityGraphicsD3D12_GetNextFrameFenceValue()
{
    auto *GraphicsImpl = UnityGraphicsD3D12Emulator::GetGraphicsImpl();
    return GraphicsImpl != nullptr ? GraphicsImpl->GetNextFenceValue() : 0;
}

static ID3D12Fence* UNITY_INTERFACE_API UnityGraphicsD3D12_GetFrameFence()
{
    auto *GraphicsImpl = UnityGraphicsD3D12Emulator::GetGraphicsImpl();
    return GraphicsImpl != nullptr ? GraphicsImpl->GetFrameFence() : nullptr;
}

UnityGraphicsD3D12Impl* UnityGraphicsD3D12Emulator::GetGraphicsImpl()
{
    return m_GraphicsImpl.get();
}

IUnityInterface* UnityGraphicsD3D12Emulator::GetUnityGraphicsAPIInterface()
{
    static IUnityGraphicsD3D12v2 UnityGraphicsD3D12;
    UnityGraphicsD3D12.GetDevice = UnityGraphicsD3D12_GetDevice;
    UnityGraphicsD3D12.ExecuteCommandList = UnityGraphicsD3D12_ExecuteCommandList;
    UnityGraphicsD3D12.GetFrameFence = UnityGraphicsD3D12_GetFrameFence;
    UnityGraphicsD3D12.GetNextFrameFenceValue = UnityGraphicsD3D12_GetNextFrameFenceValue;
    return &UnityGraphicsD3D12;
}

UnityGfxRenderer UnityGraphicsD3D12Emulator::GetUnityGfxRenderer()
{
    return kUnityGfxRendererD3D12;
}

void UnityGraphicsD3D12Emulator::BeginFrame()
{
    auto Device = m_GraphicsImpl->GetD3D12Device();
    auto CmdListAllocator = m_GraphicsImpl->GetCommandAllocator();
    ID3D12GraphicsCommandList* CmdList;
	auto hr = Device->CreateCommandList(1, D3D12_COMMAND_LIST_TYPE_DIRECT, CmdListAllocator, nullptr, __uuidof(CmdList), reinterpret_cast<void**>(&CmdList) );
    VERIFY(SUCCEEDED(hr), "Failed to create command list");

    D3D12_CPU_DESCRIPTOR_HANDLE RTV = m_GraphicsImpl->GetRTV();
    D3D12_CPU_DESCRIPTOR_HANDLE DSV = m_GraphicsImpl->GetDSV();
    CmdList->OMSetRenderTargets(1, &RTV, FALSE, &DSV);
    D3D12_VIEWPORT Viewport;
    Viewport.TopLeftX = 0;
    Viewport.TopLeftY = 0;
    Viewport.Width = static_cast<float>( m_GraphicsImpl->GetBackBufferWidth() );
    Viewport.Height = static_cast<float>( m_GraphicsImpl->GetBackBufferHeight() );
    Viewport.MinDepth = 0;
    Viewport.MaxDepth = 1;
    CmdList->RSSetViewports(1, &Viewport);
    float ClearColor[] = { 0, 0, 0.5f, 1 };
    D3D12_RESOURCE_BARRIER RTVBarrier = {};
	RTVBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	RTVBarrier.Transition.pResource = m_GraphicsImpl->GetCurrentBackBuffer();
	RTVBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	RTVBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	RTVBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    CmdList->ResourceBarrier(1, &RTVBarrier);
    CmdList->ClearRenderTargetView(RTV, ClearColor, 0, nullptr);
    CmdList->ClearDepthStencilView(DSV, D3D12_CLEAR_FLAG_DEPTH, UsesReverseZ() ? 0.f : 1.f, 0, 0, nullptr);

    CmdList->Close();
    m_GraphicsImpl->ExecuteCommandList(CmdList);
    CmdList->Release();
}

void UnityGraphicsD3D12Emulator::EndFrame()
{
    auto CmdListAllocator = m_GraphicsImpl->GetCommandAllocator();
    ID3D12GraphicsCommandList* CmdList = nullptr;
    auto Device = m_GraphicsImpl->GetD3D12Device();
	auto hr = Device->CreateCommandList(1, D3D12_COMMAND_LIST_TYPE_DIRECT, CmdListAllocator, nullptr, __uuidof(CmdList), reinterpret_cast<void**>(&CmdList) );
    VERIFY(SUCCEEDED(hr), "Failed to create command list");

    D3D12_RESOURCE_BARRIER RTVBarrier = {};
	RTVBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	RTVBarrier.Transition.pResource = m_GraphicsImpl->GetCurrentBackBuffer();
	RTVBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	RTVBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	RTVBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    CmdList->ResourceBarrier(1, &RTVBarrier);
    CmdList->Close();
    m_GraphicsImpl->ExecuteCommandList(CmdList);
    CmdList->Release();

    m_GraphicsImpl->DiscardCommandAllocator();
}
