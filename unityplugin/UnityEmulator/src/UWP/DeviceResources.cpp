#include "pch2.h"
#include "DeviceResources.h"
#include "DirectXHelper.h"

#include "IUnityInterface.h"
#include "UnityGraphicsD3D11Emulator.h"
#include "UnityGraphicsD3D12Emulator.h"
#include "DiligentGraphicsAdapterD3D11.h"
#include "DiligentGraphicsAdapterD3D12.h"
#include "ValidatedCast.h"

#include "UnitySceneBase.h"
#include "StringTools.h"
#include "Errors.h"

using namespace DirectX;
using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml::Controls;
using namespace Platform;
using namespace Diligent;

namespace DisplayMetrics
{
	// High resolution displays can require a lot of GPU and battery power to render.
	// High resolution phones, for example, may suffer from poor battery life if
	// games attempt to render at 60 frames per second at full fidelity.
	// The decision to render at full fidelity across all platforms and form factors
	// should be deliberate.
	static const bool SupportHighResolutions = false;

	// The default thresholds that define a "high resolution" display. If the thresholds
	// are exceeded and SupportHighResolutions is false, the dimensions will be scaled
	// by 50%.
	static const float DpiThreshold = 192.0f;		// 200% of standard desktop display.
	static const float WidthThreshold = 1920.0f;	// 1080p width.
	static const float HeightThreshold = 1080.0f;	// 1080p height.
};

// Constants used to calculate screen rotations.
namespace ScreenRotation
{
	// 0-degree Z-rotation
	static const XMFLOAT4X4 Rotation0(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
		);

	// 90-degree Z-rotation
	static const XMFLOAT4X4 Rotation90(
		0.0f, 1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
		);

	// 180-degree Z-rotation
	static const XMFLOAT4X4 Rotation180(
		-1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
		);

	// 270-degree Z-rotation
	static const XMFLOAT4X4 Rotation270(
		0.0f, -1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
		);
};

// Constructor for DeviceResources.
DX::DeviceResources::DeviceResources() :
	m_d3dRenderTargetSize(),
	m_outputSize(),
	m_logicalSize(),
	m_nativeOrientation(DisplayOrientations::None),
	m_currentOrientation(DisplayOrientations::None),
	m_dpi(-1.0f),
	m_deviceRemoved(false)
{
	CreateDeviceResources();
}

// Configures the Direct3D device, and stores handles to it and the device context.
void DX::DeviceResources::CreateDeviceResources()
{
    switch (m_DeviceType)
    {
        case DeviceType::D3D11:
        {
            auto &GraphicsD3D11Emulator = UnityGraphicsD3D11Emulator::GetInstance();
            GraphicsD3D11Emulator.CreateD3D11DeviceAndContext();
            m_UnityGraphicsEmulator = &GraphicsD3D11Emulator;
            m_DiligentGraphics.reset(new DiligentGraphicsAdapterD3D11(GraphicsD3D11Emulator));
        }
        break;

        case DeviceType::D3D12:
        {
            auto &GraphicsD3D12Emulator = UnityGraphicsD3D12Emulator::GetInstance();
            GraphicsD3D12Emulator.CreateD3D12DeviceAndCommandQueue();
            m_UnityGraphicsEmulator = &GraphicsD3D12Emulator;
            m_DiligentGraphics.reset(new DiligentGraphicsAdapterD3D12(GraphicsD3D12Emulator));
        }
        break;

        default:
            LOG_ERROR_AND_THROW("Unsupported device type");
    }
}

DX::DeviceResources::~DeviceResources()
{
    m_DiligentGraphics.reset();
    m_UnityGraphicsEmulator->Release();
}

// These resources need to be recreated every time the window size is changed.
void DX::DeviceResources::CreateWindowSizeDependentResources()
{
	UpdateRenderTargetSize();

	// The width and height of the swap chain must be based on the window's
	// natively-oriented width and height. If the window is not in the native
	// orientation, the dimensions must be reversed.
	DXGI_MODE_ROTATION displayRotation = ComputeDisplayRotation();

	bool swapDimensions = displayRotation == DXGI_MODE_ROTATION_ROTATE90 || displayRotation == DXGI_MODE_ROTATION_ROTATE270;
	auto fWidth = swapDimensions ? m_outputSize.Height : m_outputSize.Width;
	auto fHeight = swapDimensions ? m_outputSize.Width : m_outputSize.Height;

	UINT backBufferWidth = lround(fWidth);
	UINT backBufferHeight = lround(fHeight);

    if (m_UnityGraphicsEmulator->SwapChainInitialized())
	{
        m_DiligentGraphics->PreSwapChainResize();
        // If the swap chain already exists, resize it.
        m_UnityGraphicsEmulator->ResizeSwapChain(backBufferWidth, backBufferHeight);

        m_DiligentGraphics->PostSwapChainResize();

#if 0
		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
		{
			// If the device was removed for any reason, a new device and swap chain will need to be created.
			m_deviceRemoved = true;

			// Do not continue execution of this method. DeviceResources will be destroyed and re-created.
			return;
		}
		else
		{
			DX::ThrowIfFailed(hr);
		}
#endif
	}
	else
	{
        auto NativeWndHandle = reinterpret_cast<IUnknown*>(m_window.Get());
        switch (m_DeviceType)
        {
            case DeviceType::D3D11:
            {
                auto &GraphicsD3D11Emulator = UnityGraphicsD3D11Emulator::GetInstance();
                GraphicsD3D11Emulator.CreateSwapChain(NativeWndHandle, backBufferWidth, backBufferHeight);
                ValidatedCast<DiligentGraphicsAdapterD3D11>(m_DiligentGraphics.get())->InitProxySwapChain();
            }
            break;

            case DeviceType::D3D12:
            {
                auto &GraphicsD3D12Emulator = UnityGraphicsD3D12Emulator::GetInstance();
                GraphicsD3D12Emulator.CreateSwapChain(NativeWndHandle, backBufferWidth, backBufferHeight);
                ValidatedCast<DiligentGraphicsAdapterD3D12>(m_DiligentGraphics.get())->InitProxySwapChain();
            }
            break;

            default:
                UNEXPECTED("Unsupported device type");
        }
    }

	// Set the proper orientation for the swap chain, and generate
	// 3D matrix transformations for rendering to the rotated swap chain.
	// The 3D matrix is specified explicitly to avoid rounding errors.

	switch (displayRotation)
	{
	case DXGI_MODE_ROTATION_IDENTITY:
		m_orientationTransform3D = ScreenRotation::Rotation0;
		break;

	case DXGI_MODE_ROTATION_ROTATE90:
		m_orientationTransform3D = ScreenRotation::Rotation270;
		break;

	case DXGI_MODE_ROTATION_ROTATE180:
		m_orientationTransform3D = ScreenRotation::Rotation180;
		break;

	case DXGI_MODE_ROTATION_ROTATE270:
		m_orientationTransform3D = ScreenRotation::Rotation90;
		break;

	default:
		throw ref new FailureException();
	}
#if 0
	DX::ThrowIfFailed(
		m_swapChain->SetRotation(displayRotation)
		);
#endif

	
}

// Determine the dimensions of the render target and whether it will be scaled down.
void DX::DeviceResources::UpdateRenderTargetSize()
{
	m_effectiveDpi = m_dpi;

	// To improve battery life on high resolution devices, render to a smaller render target
	// and allow the GPU to scale the output when it is presented.
	if (!DisplayMetrics::SupportHighResolutions && m_dpi > DisplayMetrics::DpiThreshold)
	{
		float width = DX::ConvertDipsToPixels(m_logicalSize.Width, m_dpi);
		float height = DX::ConvertDipsToPixels(m_logicalSize.Height, m_dpi);

		// When the device is in portrait orientation, height > width. Compare the
		// larger dimension against the width threshold and the smaller dimension
		// against the height threshold.
		if (max(width, height) > DisplayMetrics::WidthThreshold && min(width, height) > DisplayMetrics::HeightThreshold)
		{
			// To scale the app we change the effective DPI. Logical size does not change.
			m_effectiveDpi /= 2.0f;
		}
	}

	// Calculate the necessary render target size in pixels.
	m_outputSize.Width = DX::ConvertDipsToPixels(m_logicalSize.Width, m_effectiveDpi);
	m_outputSize.Height = DX::ConvertDipsToPixels(m_logicalSize.Height, m_effectiveDpi);

	// Prevent zero size DirectX content from being created.
	m_outputSize.Width = max(m_outputSize.Width, 1);
	m_outputSize.Height = max(m_outputSize.Height, 1);
}

// This method is called when the CoreWindow is created (or re-created).
void DX::DeviceResources::SetWindow(CoreWindow^ window)
{
	DisplayInformation^ currentDisplayInformation = DisplayInformation::GetForCurrentView();

	m_window = window;
	m_logicalSize = Windows::Foundation::Size(window->Bounds.Width, window->Bounds.Height);
	m_nativeOrientation = currentDisplayInformation->NativeOrientation;
	m_currentOrientation = currentDisplayInformation->CurrentOrientation;
	m_dpi = currentDisplayInformation->LogicalDpi;

	CreateWindowSizeDependentResources();
}

// This method is called in the event handler for the SizeChanged event.
void DX::DeviceResources::SetLogicalSize(Windows::Foundation::Size logicalSize)
{
	if (m_logicalSize != logicalSize)
	{
		m_logicalSize = logicalSize;
		CreateWindowSizeDependentResources();
	}
}

// This method is called in the event handler for the DpiChanged event.
void DX::DeviceResources::SetDpi(float dpi)
{
	if (dpi != m_dpi)
	{
		m_dpi = dpi;

		// When the display DPI changes, the logical size of the window (measured in Dips) also changes and needs to be updated.
		m_logicalSize = Windows::Foundation::Size(m_window->Bounds.Width, m_window->Bounds.Height);

		CreateWindowSizeDependentResources();
	}
}

// This method is called in the event handler for the OrientationChanged event.
void DX::DeviceResources::SetCurrentOrientation(DisplayOrientations currentOrientation)
{
	if (m_currentOrientation != currentOrientation)
	{
		m_currentOrientation = currentOrientation;
		CreateWindowSizeDependentResources();
	}
}

// This method is called in the event handler for the DisplayContentsInvalidated event.
void DX::DeviceResources::ValidateDevice()
{
	// The D3D Device is no longer valid if the default adapter changed since the device
	// was created or if the device has been removed.

	ComPtr<IDXGIDevice3> dxgiDevice;
    ComPtr<ID3D11Device> d3d11Device;
    ComPtr<ID3D12Device> d3d12Device;

    if (m_DeviceType == DeviceType::D3D11)
    {
        auto &GraphicsD3D11Emulator = UnityGraphicsD3D11Emulator::GetInstance();
        d3d11Device = reinterpret_cast<ID3D11Device*>(GraphicsD3D11Emulator.GetGraphicsImpl());
        DX::ThrowIfFailed(d3d11Device.As(&dxgiDevice));
    }
    else if (m_DeviceType == DeviceType::D3D12)
    {
        auto &GraphicsD3D12Emulator = UnityGraphicsD3D12Emulator::GetInstance();
        d3d12Device = reinterpret_cast<ID3D12Device*>(GraphicsD3D12Emulator.GetGraphicsImpl());
        DX::ThrowIfFailed(d3d12Device.As(&dxgiDevice));
    }

	ComPtr<IDXGIAdapter> deviceAdapter;
	DX::ThrowIfFailed(dxgiDevice->GetAdapter(&deviceAdapter));

	ComPtr<IDXGIFactory2> deviceFactory;
	DX::ThrowIfFailed(deviceAdapter->GetParent(IID_PPV_ARGS(&deviceFactory)));

	// First, get the LUID for the default adapter from when the device was created.

	DXGI_ADAPTER_DESC previousDesc;
	{
		ComPtr<IDXGIAdapter1> previousDefaultAdapter;
		DX::ThrowIfFailed(deviceFactory->EnumAdapters1(0, &previousDefaultAdapter));

		DX::ThrowIfFailed(previousDefaultAdapter->GetDesc(&previousDesc));
	}

	// Next, get the information for the current default adapter.

	DXGI_ADAPTER_DESC currentDesc;
	{
		ComPtr<IDXGIFactory4> currentDxgiFactory;
		DX::ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&currentDxgiFactory)));

		ComPtr<IDXGIAdapter1> currentDefaultAdapter;
		DX::ThrowIfFailed(currentDxgiFactory->EnumAdapters1(0, &currentDefaultAdapter));

		DX::ThrowIfFailed(currentDefaultAdapter->GetDesc(&currentDesc));
	}

	// If the adapter LUIDs don't match, or if the device reports that it has been removed,
	// a new D3D device must be created.

	if (previousDesc.AdapterLuid.LowPart != currentDesc.AdapterLuid.LowPart ||
		previousDesc.AdapterLuid.HighPart != currentDesc.AdapterLuid.HighPart ||
		d3d11Device && FAILED(d3d11Device->GetDeviceRemovedReason()) || 
        d3d12Device && FAILED(d3d12Device->GetDeviceRemovedReason()))
	{
		m_deviceRemoved = true;
	}
}

void DX::DeviceResources::SetResourceStateTransitionHandler(IResourceStateTransitionHandler *pHandler)
{
    if (GetDeviceType() == DeviceType::D3D12)
    {
        UnityGraphicsD3D12Emulator::GetInstance().SetTransitionHandler(pHandler);
    }
}

void DX::DeviceResources::BeginFrame()
{
    m_UnityGraphicsEmulator->BeginFrame();
    m_DiligentGraphics->BeginFrame();
}

void DX::DeviceResources::EndFrame()
{
    m_DiligentGraphics->EndFrame();
    m_UnityGraphicsEmulator->EndFrame();
}

// Present the contents of the swap chain to the screen.
void DX::DeviceResources::Present()
{
    m_UnityGraphicsEmulator->Present();

	//// If the device was removed either by a disconnection or a driver upgrade, we 
	//// must recreate all device resources.
	//if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
	//{
	//	m_deviceRemoved = true;
	//}
	//else
	//{
	//	DX::ThrowIfFailed(hr);
	//}
}


// This method determines the rotation between the display device's native Orientation and the
// current display orientation.
DXGI_MODE_ROTATION DX::DeviceResources::ComputeDisplayRotation()
{
	DXGI_MODE_ROTATION rotation = DXGI_MODE_ROTATION_UNSPECIFIED;

	// Note: NativeOrientation can only be Landscape or Portrait even though
	// the DisplayOrientations enum has other values.
	switch (m_nativeOrientation)
	{
	case DisplayOrientations::Landscape:
		switch (m_currentOrientation)
		{
		case DisplayOrientations::Landscape:
			rotation = DXGI_MODE_ROTATION_IDENTITY;
			break;

		case DisplayOrientations::Portrait:
			rotation = DXGI_MODE_ROTATION_ROTATE270;
			break;

		case DisplayOrientations::LandscapeFlipped:
			rotation = DXGI_MODE_ROTATION_ROTATE180;
			break;

		case DisplayOrientations::PortraitFlipped:
			rotation = DXGI_MODE_ROTATION_ROTATE90;
			break;
		}
		break;

	case DisplayOrientations::Portrait:
		switch (m_currentOrientation)
		{
		case DisplayOrientations::Landscape:
			rotation = DXGI_MODE_ROTATION_ROTATE90;
			break;

		case DisplayOrientations::Portrait:
			rotation = DXGI_MODE_ROTATION_IDENTITY;
			break;

		case DisplayOrientations::LandscapeFlipped:
			rotation = DXGI_MODE_ROTATION_ROTATE270;
			break;

		case DisplayOrientations::PortraitFlipped:
			rotation = DXGI_MODE_ROTATION_ROTATE180;
			break;
		}
		break;
	}
	return rotation;
}
