/*     Copyright 2015-2016 Egor Yusov
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF ANY PROPRIETARY RIGHTS.
 *
 *  In no event and under no legal theory, whether in tort (including negligence), 
 *  contract, or otherwise, unless required by applicable law (such as deliberate 
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental, 
 *  or consequential damages of any character arising as a result of this License or 
 *  out of the use or inability to use the software (including but not limited to damages 
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and 
 *  all other commercial damages or losses), even if such Contributor has been advised 
 *  of the possibility of such damages.
 */

#define NOMINIMAX
#include <wrl.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <agile.h>

#if defined(_DEBUG)
#   include <dxgidebug.h>
#endif

#include "App.h"
#include "StringTools.h"

#include <ppltasks.h>

using namespace SampleApp;
using namespace Diligent;

using namespace concurrency;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;

using Microsoft::WRL::ComPtr;


ref class ApplicationSource sealed : IFrameworkViewSource
{
public:
    virtual IFrameworkView^ CreateView()
    {
        return ref new App();
    }
};


// The main function is only used to initialize our IFrameworkView class.
[Platform::MTAThread]
int main(Platform::Array<Platform::String^>^)
{
	auto direct3DApplicationSource = ref new ApplicationSource();
	CoreApplication::Run(direct3DApplicationSource);
	return 0;
}

App::App() :
	m_windowClosed(false),
	m_windowVisible(true)
{
}


// The first method called when the IFrameworkView is being created.
void App::Initialize(CoreApplicationView^ applicationView)
{
	// Register event handlers for app lifecycle. This example includes Activated, so that we
	// can make the CoreWindow active and start rendering on the window.
	applicationView->Activated +=
		ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &App::OnActivated);
    
	CoreApplication::Suspending +=
		ref new EventHandler<SuspendingEventArgs^>(this, &App::OnSuspending);

	CoreApplication::Resuming +=
		ref new EventHandler<Platform::Object^>(this, &App::OnResuming);
}

// Called when the CoreWindow object is created (or re-created).
void App::SetWindow(CoreWindow^ window)
{
	window->SizeChanged += 
		ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(this, &App::OnWindowSizeChanged);

	window->VisibilityChanged +=
		ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &App::OnVisibilityChanged);

	window->Closed += 
		ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &App::OnWindowClosed);

    window->KeyDown += 
        ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &App::OnKeyDown);

    window->KeyUp +=
        ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &App::OnKeyUp);

	DisplayInformation^ currentDisplayInformation = DisplayInformation::GetForCurrentView();

	currentDisplayInformation->DpiChanged +=
		ref new TypedEventHandler<DisplayInformation^, Object^>(this, &App::OnDpiChanged);

	currentDisplayInformation->OrientationChanged +=
		ref new TypedEventHandler<DisplayInformation^, Object^>(this, &App::OnOrientationChanged);

	DisplayInformation::DisplayContentsInvalidated +=
		ref new TypedEventHandler<DisplayInformation^, Object^>(this, &App::OnDisplayContentsInvalidated);

    if (m_Main)
    {
        m_Main->OnSetWindow(window);
    }
}

// Initializes scene resources, or loads a previously saved app state.
void App::Load(Platform::String^ entryPoint)
{
	if (m_Main == nullptr)
	{
        m_Main.reset(CreateApplication());
        m_Main->OnSetWindow(CoreWindow::GetForCurrentThread());
	}
}

// This method is called after the window becomes active.
void App::Run()
{
	while (!m_windowClosed)
	{
		if (m_windowVisible)
		{
			CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);

            auto commandQueue = GetDeviceResources();//->GetCommandQueue();
			//PIXBeginEvent(commandQueue, 0, L"Update");
			{
				m_Main->Update();
			}
			//PIXEndEvent(commandQueue);

			//PIXBeginEvent(commandQueue, 0, L"Render");
			{
                m_Main->Render();
				if (m_Main->IsFrameReady())
				{
                    m_Main->Present();
				}
			}
			//PIXEndEvent(commandQueue);
		}
		else
		{
			CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
		}
	}
}

// Required for IFrameworkView.
// Terminate events do not cause Uninitialize to be called. It will be called if your IFrameworkView
// class is torn down while the app is in the foreground.
void App::Uninitialize()
{
}

// Application lifecycle event handlers.

void App::OnActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args)
{
    if (args->Kind == ActivationKind::Launch)
    {
        LaunchActivatedEventArgs^ launchArgs = (LaunchActivatedEventArgs^)args;
        auto CmdLine = Diligent::NarrowString(launchArgs->Arguments->Data());
        m_Main->ProcessCommandLine(CmdLine.c_str());
    }

    auto Title = Diligent::WidenString(m_Main->GetAppTitle());
    Windows::UI::ViewManagement::ApplicationView::GetForCurrentView()->Title = ref new Platform::String(Title.c_str());

    // Run() won't start until the CoreWindow is activated.
    CoreWindow::GetForCurrentThread()->Activate();
}

void App::OnSuspending(Platform::Object^ sender, SuspendingEventArgs^ args)
{
	// Save app state asynchronously after requesting a deferral. Holding a deferral
	// indicates that the application is busy performing suspending operations. Be
	// aware that a deferral may not be held indefinitely. After about five seconds,
	// the app will be forced to exit.
	SuspendingDeferral^ deferral = args->SuspendingOperation->GetDeferral();

	create_task([this, deferral]()
	{
		m_Main->OnSuspending();
		deferral->Complete();
	});
}

void App::OnResuming(Platform::Object^ sender, Platform::Object^ args)
{
	// Restore any data or state that was unloaded on suspend. By default, data
	// and state are persisted when resuming from suspend. Note that this event
	// does not occur if the app was previously terminated.

	m_Main->OnResuming();
}

// Window event handlers.

void App::OnWindowSizeChanged(CoreWindow^ sender, WindowSizeChangedEventArgs^ args)
{
	GetDeviceResources()->SetLogicalSize(Size(sender->Bounds.Width, sender->Bounds.Height));
	m_Main->OnWindowSizeChanged();
}

void App::OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
{
	m_windowVisible = args->Visible;
}

void App::OnWindowClosed(CoreWindow^ sender, CoreWindowEventArgs^ args)
{
	m_windowClosed = true;
}

// DisplayInformation event handlers.

void App::OnDpiChanged(DisplayInformation^ sender, Object^ args)
{
	// Note: The value for LogicalDpi retrieved here may not match the effective DPI of the app
	// if it is being scaled for high resolution devices. Once the DPI is set on DeviceResources,
	// you should always retrieve it using the GetDpi method.
	// See DeviceResources.cpp for more details.
	GetDeviceResources()->SetDpi(sender->LogicalDpi);
	m_Main->OnWindowSizeChanged();
}

void App::OnOrientationChanged(DisplayInformation^ sender, Object^ args)
{
	GetDeviceResources()->SetCurrentOrientation(sender->CurrentOrientation);
	m_Main->OnWindowSizeChanged();
}

void App::OnDisplayContentsInvalidated(DisplayInformation^ sender, Object^ args)
{
	GetDeviceResources()->ValidateDevice();
}

void App::OnKeyDown(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args)
{
    auto Key = args->VirtualKey;
    switch(Key)
    {
        case VirtualKey::Escape:
            CoreApplication::Exit();
        break;

        case VirtualKey::Enter:
            if(m_bShiftPressed)
            {
                auto applicationView = Windows::UI::ViewManagement::ApplicationView::GetForCurrentView();
                if (applicationView->IsFullScreenMode)
                {
                    applicationView->ExitFullScreenMode();
                }
                else
                {
                    applicationView->TryEnterFullScreenMode();
                }
            }
        break;

        case VirtualKey::Shift:
            m_bShiftPressed = true;
            break;
    }
}

void App::OnKeyUp(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args)
{
    auto Key = args->VirtualKey;
    switch (Key)
    {
        case VirtualKey::Shift:
            m_bShiftPressed = false;
            break;
    }
}

std::shared_ptr<DX::DeviceResources> App::GetDeviceResources()
{
	if (m_deviceResources != nullptr && m_deviceResources->IsDeviceRemoved())
	{
		// All references to the existing D3D device must be released before a new device
		// can be created.

		m_deviceResources = nullptr;
		m_Main->OnDeviceRemoved();

#if defined(_DEBUG)
		ComPtr<IDXGIDebug1> dxgiDebug;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
		{
			dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
		}
#endif
	}

	if (m_deviceResources == nullptr)
	{
		m_deviceResources = m_Main->InitDeviceResources();
		m_deviceResources->SetWindow(CoreWindow::GetForCurrentThread());
        m_Main->OnWindowSizeChanged();
		m_Main->CreateRenderers();
	}
	return m_deviceResources;
}
