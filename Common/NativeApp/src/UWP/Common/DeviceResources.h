#pragma once

#include <dxgi1_4.h>
#include <d3d12.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <agile.h>

namespace DX
{
	// Controls all the DirectX device resources.
	class DeviceResources
	{
	public:
		DeviceResources(ID3D11Device *d3d11Device, ID3D12Device *d3d12Device);

		void SetWindow(Windows::UI::Core::CoreWindow^ window);
		void SetLogicalSize(Windows::Foundation::Size logicalSize);
		void SetCurrentOrientation(Windows::Graphics::Display::DisplayOrientations currentOrientation);
		void SetDpi(float dpi);
		void ValidateDevice();
        void SetSwapChainRotation(IDXGISwapChain3 *swapChain);

		// The size of the render target, in pixels.
		Windows::Foundation::Size	GetOutputSize() const				{ return m_outputSize; }

		// The size of the render target, in dips.
		Windows::Foundation::Size	GetLogicalSize() const				{ return m_logicalSize; }

		float						GetDpi() const						{ return m_effectiveDpi; }
		bool						IsDeviceRemoved() const				{ return m_deviceRemoved; }

		// D3D Accessors.
		DirectX::XMFLOAT4X4			GetOrientationTransform3D() const	{ return m_orientationTransform3D; }

        void UpdateRenderTargetSize();
        UINT GetBackBufferWidth()  {return m_backBufferWidth;}
        UINT GetBackBufferHeight() {return m_backBufferHeight;}

        Windows::UI::Core::CoreWindow^ GetWindow(){return m_window.Get();}

	private:
		
		
		DXGI_MODE_ROTATION ComputeDisplayRotation();

        bool											m_deviceRemoved;

		// Direct3D objects.
		Microsoft::WRL::ComPtr<ID3D12Device>			m_d3d12Device;
        Microsoft::WRL::ComPtr<ID3D11Device>			m_d3d11Device;
		
		// Cached reference to the Window.
		Platform::Agile<Windows::UI::Core::CoreWindow>	m_window;

		// Cached device properties.
		Windows::Foundation::Size						m_d3dRenderTargetSize;
		Windows::Foundation::Size						m_outputSize;
		Windows::Foundation::Size						m_logicalSize;
		Windows::Graphics::Display::DisplayOrientations	m_nativeOrientation;
		Windows::Graphics::Display::DisplayOrientations	m_currentOrientation;
		float											m_dpi;
        UINT                                            m_backBufferWidth  = 0;
        UINT                                            m_backBufferHeight = 0;

		// This is the DPI that will be reported back to the app. It takes into account whether the app supports high resolution screens or not.
		float											m_effectiveDpi;

		// Transforms used for display orientation.
		DirectX::XMFLOAT4X4								m_orientationTransform3D;
	};
}
