/*     Copyright 2015-2018 Egor Yusov
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

#include <memory>
#include <iomanip>

#define NOMINMAX
#include <Windows.h>

#include "DeviceCaps.h"
#include "Errors.h"
#include "Timer.h"

#include "IUnityInterface.h"
#include "UnityGraphicsD3D11Emulator.h"
#include "UnityGraphicsD3D12Emulator.h"
#include "UnityGraphicsGLCoreES_Emulator.h"
#include "DiligentGraphicsAdapterD3D11.h"
#include "DiligentGraphicsAdapterD3D12.h"
#include "DiligentGraphicsAdapterGL.h"
#include "UnitySceneBase.h"
#include "StringTools.h"

using namespace Diligent;

LRESULT CALLBACK MessageProc(HWND, UINT, WPARAM, LPARAM);

UnityGraphicsEmulator *g_GraphicsEmulator = nullptr;

typedef void (UNITY_INTERFACE_API *TUnityPluginLoad)(IUnityInterfaces* unityInterfaces);
typedef void (UNITY_INTERFACE_API *TUnityPluginUnload)();
typedef UnityRenderingEvent(UNITY_INTERFACE_API *TGetRenderEventFunc)();

TUnityPluginLoad UnityPluginLoad;
TUnityPluginUnload UnityPluginUnload;
TGetRenderEventFunc GetRenderEventFunc;

std::unique_ptr<DiligentGraphicsAdapter> g_pDiligentGraphics;
std::unique_ptr<UnitySceneBase> g_pScene;

HMODULE g_DLLHandle;

static UINT g_WindowWidth = 1024;
static UINT g_WindowHeight = 768;

static void* LoadPluginFunction(const char* FunctionName)
{
    auto Func = GetProcAddress(g_DLLHandle, FunctionName);
    VERIFY( Func != nullptr, "Failed to import plugin function \"", FunctionName, "\"." );
    return Func;
}

bool LoadPlugin()
{
    std::string LibName = g_pScene->GetPluginName();
#if _WIN64
    LibName += "_64";
#else
    LibName += "_32";
#endif

#ifdef _DEBUG
    LibName += "d";
#else
    LibName += "r";
#endif

    LibName += ".dll";
    g_DLLHandle = LoadLibraryA( LibName.c_str() );
    if( g_DLLHandle == NULL )
    {
        LOG_ERROR_MESSAGE( "Failed to load ", LibName, " library." );
        return false;
    }

    UnityPluginLoad = reinterpret_cast<TUnityPluginLoad>( GetProcAddress(g_DLLHandle, "UnityPluginLoad") );
    UnityPluginUnload = reinterpret_cast<TUnityPluginUnload>( GetProcAddress(g_DLLHandle, "UnityPluginUnload") );
    GetRenderEventFunc = reinterpret_cast<TGetRenderEventFunc>( GetProcAddress(g_DLLHandle, "GetRenderEventFunc") );
    if( UnityPluginLoad == nullptr || UnityPluginUnload == nullptr || GetRenderEventFunc == nullptr )
    {
        LOG_ERROR_MESSAGE( "Failed to import plugin functions from ", LibName, " library." );
        FreeLibrary( g_DLLHandle );
        return false;
    }

    return true;
}


void UnloadPlugin()
{
    g_GraphicsEmulator->InvokeDeviceEventCallback(kUnityGfxDeviceEventShutdown);
    UnityPluginUnload();
    FreeLibrary(g_DLLHandle);
}

// Main
int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int cmdShow)
{
#if defined(_DEBUG) || defined(DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    g_pScene.reset(CreateScene());

    std::wstring Title = WidenString(g_pScene->GetSceneName());

    DeviceType DevType = DeviceType::Undefined;
    std::wstring CmdLine = GetCommandLine();
    std::wstring Key = L"mode=";
    auto pos = CmdLine.find( Key );
    if( pos != std::string::npos )
    {
        pos += Key.length();
        auto Val = CmdLine.substr( pos );
        if(Val == L"D3D11")
        {
            DevType = DeviceType::D3D11;
            Title.append( L" (D3D11)" );
        }
        else if(Val == L"D3D12")
        {
            DevType = DeviceType::D3D12;
            Title.append( L" (D3D12)" );
        }
        else if(Val == L"GL")
        {
            DevType = DeviceType::OpenGL;
            Title.append( L" (OpenGL)" );
        }
        else
        {
            LOG_ERROR("Unknown device type. Only the following types are supported: D3D11, D3D12, GL");
            return -1;
        }
    }
    else
    {
        LOG_INFO_MESSAGE("Device type is not specified. Using D3D11 device");
        DevType = DeviceType::D3D11;
        Title.append( L" (D3D11)" );
    }
    // Register our window class
    WNDCLASSEX wcex = { sizeof(WNDCLASSEX), CS_HREDRAW|CS_VREDRAW, MessageProc,
                        0L, 0L, instance, NULL, NULL, NULL, NULL, L"SampleApp", NULL };
    RegisterClassEx(&wcex);

    // Create a window
    UINT ClientAreaWidth = 1280;
    UINT ClientAreaHeight = 1024;
    RECT rc = { 0, 0, static_cast<LONG>(ClientAreaWidth), static_cast<LONG>(ClientAreaHeight) };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    HWND wnd = CreateWindow(L"SampleApp", Title.c_str(), 
                            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 
                            rc.right-rc.left, rc.bottom-rc.top, NULL, NULL, instance, NULL);
    if (!wnd)
    {
        MessageBox(NULL, L"Cannot create window", L"Error", MB_OK|MB_ICONERROR);
        return -1;
    }
    ShowWindow(wnd, cmdShow);
    UpdateWindow(wnd);

    bool IsDX = false;
    try
    {
        switch (DevType)
        {
            case DeviceType::D3D11:
            {
                auto &GraphicsD3D11Emulator = UnityGraphicsD3D11Emulator::GetInstance();
                GraphicsD3D11Emulator.CreateD3D11DeviceAndContext();
                GraphicsD3D11Emulator.CreateSwapChain(wnd, ClientAreaWidth, ClientAreaHeight);
                g_GraphicsEmulator = &GraphicsD3D11Emulator;
                IsDX = true;
                auto *pDiligentAdapterD3D11 = new DiligentGraphicsAdapterD3D11(GraphicsD3D11Emulator);
                g_pDiligentGraphics.reset(pDiligentAdapterD3D11);
                pDiligentAdapterD3D11->InitProxySwapChain();
            }
            break;

            case DeviceType::D3D12:
            {
                auto &GraphicsD3D12Emulator = UnityGraphicsD3D12Emulator::GetInstance();
                GraphicsD3D12Emulator.CreateD3D12DeviceAndCommandQueue();
                GraphicsD3D12Emulator.CreateSwapChain(wnd, ClientAreaWidth, ClientAreaHeight);
                g_GraphicsEmulator = &GraphicsD3D12Emulator;
                IsDX = true;
                auto *pDiligentAdapterD3D12 = new DiligentGraphicsAdapterD3D12(GraphicsD3D12Emulator);
                g_pDiligentGraphics.reset(pDiligentAdapterD3D12);
                pDiligentAdapterD3D12->InitProxySwapChain();
            }
            break;

            case DeviceType::OpenGL:
            {
                auto &GraphicsGLCoreES_Emulator = UnityGraphicsGLCoreES_Emulator::GetInstance();
                GraphicsGLCoreES_Emulator.InitGLContext(wnd, 4, 4);
                g_GraphicsEmulator = &GraphicsGLCoreES_Emulator;
                g_pDiligentGraphics.reset(new DiligentGraphicsAdapterGL(GraphicsGLCoreES_Emulator));
            }
            break;

            default:
                LOG_ERROR("Unsupported device type");
                return -1;
        }
    }
    catch (std::runtime_error &err)
    {
        LOG_ERROR("Failed to initialize unity graphics emulator: ", err.what());
        return -1;
    }

    g_pScene->SetDiligentGraphicsAdapter(g_pDiligentGraphics.get());
    g_pScene->OnGraphicsInitialized();
    if (DevType == DeviceType::D3D12)
    {
        UnityGraphicsD3D12Emulator::GetInstance().SetTransitionHandler(g_pScene->GetStateTransitionHandler());
    }

    if (!LoadPlugin())
    {
        return -1;
    }

    g_pScene->OnPluginLoad(LoadPluginFunction);
    UnityPluginLoad(&g_GraphicsEmulator->GeUnityInterfaces());

    auto RenderEventFunc = GetRenderEventFunc();

    Timer timer;
    auto PrevTime = timer.GetElapsedTime();
    double filteredFrameTime = 0.0;

    // Main message loop
    MSG msg = {0};
    while (WM_QUIT != msg.message)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            g_GraphicsEmulator->BeginFrame();
            g_pDiligentGraphics->BeginFrame();

            auto CurrTime = timer.GetElapsedTime();
            auto ElapsedTime = CurrTime - PrevTime;
            PrevTime = CurrTime;

            g_pScene->Render(RenderEventFunc, CurrTime, ElapsedTime);

            g_pDiligentGraphics->EndFrame();
            g_GraphicsEmulator->EndFrame();

            g_GraphicsEmulator->Present();

            double filterScale = 0.2;
            filteredFrameTime = filteredFrameTime * (1.0 - filterScale) + filterScale * ElapsedTime;
            std::wstringstream fpsCounterSS;
            fpsCounterSS << " - " << std::fixed << std::setprecision(1) << filteredFrameTime * 1000;
            fpsCounterSS << " ms (" << 1.0 / filteredFrameTime << " fps)";
            SetWindowText(wnd, (Title + fpsCounterSS.str()).c_str());
        }
    }
    
    g_pScene->OnPluginUnload();
    g_pScene.reset();
    UnloadPlugin();
 
    g_pDiligentGraphics.reset();
    g_GraphicsEmulator->Release();

    return (int)msg.wParam;
}

// Called every time the application receives a message
LRESULT CALLBACK MessageProc(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // Send event message to AntTweakBar
    switch (message) 
    {
        case WM_PAINT:
            {
                PAINTSTRUCT ps;
                BeginPaint(wnd, &ps);
                EndPaint(wnd, &ps);
                return 0;
            }
        case WM_SIZE: // Window size has been changed
            if(g_GraphicsEmulator)
            {
                g_WindowWidth = LOWORD(lParam);
                g_WindowHeight = HIWORD(lParam);
                g_pDiligentGraphics->PreSwapChainResize();
                g_GraphicsEmulator->ResizeSwapChain(g_WindowWidth, g_WindowHeight);
                g_pDiligentGraphics->PostSwapChainResize();
                g_pScene->OnWindowResize(g_WindowWidth, g_WindowHeight);
            }
            return 0;
        case WM_CHAR:
            if (wParam == VK_ESCAPE)
                PostQuitMessage(0);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
        {
            struct WindowMessageData
            {
                HWND hWnd;
                UINT message;
                WPARAM wParam;
                LPARAM lParam;
            }msg{wnd, message, wParam, lParam};
            return DefWindowProc(wnd, message, wParam, lParam);
        }
    }
}
