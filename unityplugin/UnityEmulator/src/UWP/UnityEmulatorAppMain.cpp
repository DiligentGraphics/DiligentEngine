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

#include "pch2.h"
#include "UnityEmulatorAppMain.h"
#include "DirectXHelper.h"
#include "StringTools.h"
#include "Errors.h"
#include "FileSystem.h"

using namespace UnityEmulatorApp;
using namespace Windows::Foundation;
using namespace Windows::System::Threading;
using namespace Concurrency;
using namespace Diligent;

// The DirectX 12 Application template is documented at http://go.microsoft.com/fwlink/?LinkID=613670&clcid=0x409

HMODULE UnityEmulatorAppMain::m_DLLHandle;
UnityEmulatorAppMain::TUnityPluginLoad UnityEmulatorAppMain::UnityPluginLoad;
UnityEmulatorAppMain::TUnityPluginUnload UnityEmulatorAppMain::UnityPluginUnload;
UnityEmulatorAppMain::TGetRenderEventFunc UnityEmulatorAppMain::GetRenderEventFunc;
UnityRenderingEvent UnityEmulatorAppMain::RenderEventFunc;

void* UnityEmulatorAppMain::LoadPluginFunction(const char* FunctionName)
{
    auto Func = GetProcAddress(m_DLLHandle, FunctionName);
    VERIFY( Func != nullptr, "Failed to import plugin function \"", FunctionName, "\"." );
    return Func;
}


bool UnityEmulatorAppMain::LoadPlugin(const char* LibPath)
{
    auto wLibPath = WidenString(LibPath);
    m_DLLHandle = LoadPackagedLibrary( wLibPath.c_str(), 0);
    if( m_DLLHandle == NULL )
    {
        LOG_ERROR_MESSAGE( "Failed to load ", LibPath, " library." );
        return false;
    }

    UnityPluginLoad = reinterpret_cast<TUnityPluginLoad>( GetProcAddress(m_DLLHandle, "UnityPluginLoad") );
    UnityPluginUnload = reinterpret_cast<TUnityPluginUnload>( GetProcAddress(m_DLLHandle, "UnityPluginUnload") );
    GetRenderEventFunc = reinterpret_cast<TGetRenderEventFunc>( GetProcAddress(m_DLLHandle, "GetRenderEventFunc") );
    if( UnityPluginLoad == nullptr || UnityPluginUnload == nullptr || GetRenderEventFunc == nullptr )
    {
        LOG_ERROR_MESSAGE( "Failed to import plugin functions from ", LibPath, " library." );
        FreeLibrary( m_DLLHandle );
        return false;
    }

    return true;
}


// Loads and initializes application assets when the application is loaded.
UnityEmulatorAppMain::UnityEmulatorAppMain() :
    m_scene(CreateScene())
{
#if defined(PLATFORM_UNIVERSAL_WINDOWS)
    FileSystem::SetWorkingDirectory("assets");
#endif
    
    std::string LibName = "Assets\\";
    LibName.append(m_scene->GetPluginName());

#if _WIN64
#   if _M_ARM >= 7
        LibName += "_arm";
#   else
        LibName += "_64";
#   endif
#else
#   if _M_ARM >= 7
        LibName += "_arm";
#   else
        LibName += "_32";
#   endif
#endif

#ifdef _DEBUG
    LibName += "d";
#else
    LibName += "r";
#endif

    LibName.append(".dll");
    if (!LoadPlugin(LibName.c_str()))
        throw std::runtime_error("Failed to load plugin");

    m_scene->OnPluginLoad(LoadPluginFunction);
    
	// TODO: Change the timer settings if you want something other than the default variable timestep mode.
	// e.g. for 60 FPS fixed timestep update logic, call:
    m_timer.SetFixedTimeStep(true);
	m_timer.SetTargetElapsedSeconds(1.0 / 60);
}

void UnityEmulatorAppMain::UnloadPlugin()
{
    m_deviceResources->GetUnityGraphicsEmulator()->InvokeDeviceEventCallback(kUnityGfxDeviceEventShutdown);
    UnityPluginUnload();
    FreeLibrary(m_DLLHandle);
    m_DLLHandle = NULL;
}


UnityEmulatorAppMain::~UnityEmulatorAppMain()
{
    m_scene->OnPluginUnload();
    m_scene.reset();
    UnloadPlugin();
}

// Creates and initializes the renderers.
void UnityEmulatorAppMain::CreateRenderers(const std::shared_ptr<DX::DeviceResources>& deviceResources)
{
    m_deviceResources = deviceResources;
    m_scene->SetDiligentGraphicsAdapter(m_deviceResources->GetDiligentGraphicsAdapter());
    m_scene->OnGraphicsInitialized();
    m_deviceResources->SetResourceStateTransitionHandler(m_scene->GetStateTransitionHandler());
    UnityPluginLoad(&m_deviceResources->GetUnityGraphicsEmulator()->GeUnityInterfaces());
    RenderEventFunc = GetRenderEventFunc();

	OnWindowSizeChanged();
}

// Updates the application state once per frame.
void UnityEmulatorAppMain::Update()
{
	// Update scene objects.
	m_timer.Tick([&]()
	{

	});
}

// Renders the current frame according to the current application state.
// Returns true if the frame was rendered and is ready to be displayed.
bool UnityEmulatorAppMain::Render()
{
	// Don't try to render anything before the first Update.
	if (m_timer.GetFrameCount() == 0)
	{
		return false;
	}

    float CurrTime = static_cast<float>(m_timer.GetTotalSeconds());
    float ElapsedTime = static_cast<float>(m_timer.GetElapsedSeconds());
    m_scene->Render(RenderEventFunc, CurrTime, ElapsedTime);

    return true;
}

// Updates application state when the window's size changes (e.g. device orientation change)
void UnityEmulatorAppMain::OnWindowSizeChanged()
{
    auto Size = m_deviceResources->GetOutputSize();
    m_scene->OnWindowResize(static_cast<int>(Size.Width), static_cast<int>(Size.Height));
}

// Notifies the app that it is being suspended.
void UnityEmulatorAppMain::OnSuspending()
{
	// TODO: Replace this with your app's suspending logic.

	// Process lifetime management may terminate suspended apps at any time, so it is
	// good practice to save any state that will allow the app to restart where it left off.

	//m_sceneRenderer->SaveState();

	// If your application uses video memory allocations that are easy to re-create,
	// consider releasing that memory to make it available to other applications.
}

// Notifes the app that it is no longer suspended.
void UnityEmulatorAppMain::OnResuming()
{
	// TODO: Replace this with your app's resuming logic.
}

// Notifies renderers that device resources need to be released.
void UnityEmulatorAppMain::OnDeviceRemoved()
{
	// TODO: Save any necessary application or renderer state and release the renderer
	// and its resources which are no longer valid.
	//m_sceneRenderer->SaveState();
	//m_pSample = nullptr;
}
