#pragma once

#include "UnityGraphicsEmulator.h"
#include "DiligentGraphicsAdapter.h"
#include "ResourceStateTransitionHandler.h"

namespace DX
{
	// Controls all the DirectX device resources.
	class DeviceResources
	{
	public:
		DeviceResources();
        ~DeviceResources();
		void SetWindow(Windows::UI::Core::CoreWindow^ window);
		void SetLogicalSize(Windows::Foundation::Size logicalSize);
		void SetCurrentOrientation(Windows::Graphics::Display::DisplayOrientations currentOrientation);
		void SetDpi(float dpi);
		void ValidateDevice();
        void BeginFrame();
		void Present();
        void EndFrame();
        DiligentGraphicsAdapter* GetDiligentGraphicsAdapter() { return m_DiligentGraphics.get(); }
        UnityGraphicsEmulator*   GetUnityGraphicsEmulator()   { return m_UnityGraphicsEmulator;}

        Diligent::DeviceType        GetDeviceType()const                {return m_DeviceType;}

		// The size of the render target, in pixels.
		Windows::Foundation::Size	GetOutputSize() const				{ return m_outputSize; }

		// The size of the render target, in dips.
		Windows::Foundation::Size	GetLogicalSize() const				{ return m_logicalSize; }

		float						GetDpi() const						{ return m_effectiveDpi; }
		bool						IsDeviceRemoved() const				{ return m_deviceRemoved; }
        void                        SetResourceStateTransitionHandler(IResourceStateTransitionHandler *pHandler);
		// D3D Accessors.
		DirectX::XMFLOAT4X4			GetOrientationTransform3D() const	{ return m_orientationTransform3D; }

	private:
		void CreateDeviceResources();
		void CreateWindowSizeDependentResources();
		void UpdateRenderTargetSize();
		DXGI_MODE_ROTATION ComputeDisplayRotation();

        bool											m_deviceRemoved = false;

        UnityGraphicsEmulator*                          m_UnityGraphicsEmulator = nullptr;

        std::unique_ptr<DiligentGraphicsAdapter>        m_DiligentGraphics;

		// Cached reference to the Window.
		Platform::Agile<Windows::UI::Core::CoreWindow>	m_window;

		// Cached device properties.
		Windows::Foundation::Size						m_d3dRenderTargetSize;
		Windows::Foundation::Size						m_outputSize;
		Windows::Foundation::Size						m_logicalSize;
		Windows::Graphics::Display::DisplayOrientations	m_nativeOrientation;
		Windows::Graphics::Display::DisplayOrientations	m_currentOrientation;
		float											m_dpi;

		// This is the DPI that will be reported back to the app. It takes into account whether the app supports high resolution screens or not.
		float											m_effectiveDpi;

		// Transforms used for display orientation.
		DirectX::XMFLOAT4X4								m_orientationTransform3D;

        Diligent::DeviceType m_DeviceType = Diligent::DeviceType::D3D12;
	};
}
