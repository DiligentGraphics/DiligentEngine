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

#include "UnityAppBase.h"
#include "IUnityInterface.h"

#if D3D11_SUPPORTED
#   include "UnityGraphicsD3D11Emulator.h"
#   include "DiligentGraphicsAdapterD3D11.h"
#endif

#if D3D12_SUPPORTED
#   include "UnityGraphicsD3D12Emulator.h"
#   include "DiligentGraphicsAdapterD3D12.h"
#endif

#if OPENGL_SUPPORTED
#   include "UnityGraphicsGLCoreES_Emulator.h"
#   include "DiligentGraphicsAdapterGL.h"
#endif

#include "UnityAppBase.h"
#include "StringTools.h"
#include "Errors.h"

using namespace Diligent;

UnityAppBase::UnityAppBase() : 
    m_Scene(CreateScene())
{
    m_AppTitle = m_Scene->GetSceneName();
}

UnityAppBase::~UnityAppBase()
{
    m_Scene->OnPluginUnload();
    m_Scene.reset();
    UnloadPlugin();

    m_DiligentGraphics.reset();
    m_GraphicsEmulator->Release();
}



void UnityAppBase::ProcessCommandLine(const char *CmdLine)
{
    const auto* Key = "mode=";
    const auto *pos = strstr(CmdLine, Key);
    if (pos != nullptr)
    {
        pos += strlen(Key);
        if (_stricmp(pos, "D3D11") == 0)
        {
            m_DeviceType = DeviceType::D3D11;
        }
        else if (_stricmp(pos, "D3D12") == 0)
        {
            m_DeviceType = DeviceType::D3D12;
        }
        else if (_stricmp(pos, "GL") == 0)
        {
            m_DeviceType = DeviceType::OpenGL;
        }
        else
        {
            LOG_ERROR_AND_THROW("Unknown device type. Only the following types are supported: D3D11, D3D12, GL");
        }
    }
    else
    {
        LOG_INFO_MESSAGE("Device type is not specified. Using D3D11 device");
        m_DeviceType = DeviceType::D3D11;
    }

    switch (m_DeviceType)
    {
        case DeviceType::D3D11: m_AppTitle.append(" (D3D11)"); break;
        case DeviceType::D3D12: m_AppTitle.append(" (D3D12)"); break;
        case DeviceType::OpenGL: m_AppTitle.append(" (OpenGL)"); break;
        default: UNEXPECTED("Unknown device type");
    }
}

void UnityAppBase::InitGraphics(
#if PLATFORM_LINUX
        void *display,
#endif    
        void *NativeWindowHandle, 
        int WindowWidth, 
        int WindowHeight
    )
{
   switch (m_DeviceType)
   {
#if D3D11_SUPPORTED
        case DeviceType::D3D11:
        {
            auto &GraphicsD3D11Emulator = UnityGraphicsD3D11Emulator::GetInstance();
            GraphicsD3D11Emulator.CreateD3D11DeviceAndContext();
            m_GraphicsEmulator = &GraphicsD3D11Emulator;
            auto *pDiligentAdapterD3D11 = new DiligentGraphicsAdapterD3D11(GraphicsD3D11Emulator);
            m_DiligentGraphics.reset(pDiligentAdapterD3D11);
            if (NativeWindowHandle != nullptr)
            {
                GraphicsD3D11Emulator.CreateSwapChain(NativeWindowHandle, WindowWidth, WindowHeight);
                pDiligentAdapterD3D11->InitProxySwapChain();
            }
        }
        break;
#endif

#if D3D12_SUPPORTED
        case DeviceType::D3D12:
        {
            auto &GraphicsD3D12Emulator = UnityGraphicsD3D12Emulator::GetInstance();
            GraphicsD3D12Emulator.CreateD3D12DeviceAndCommandQueue();
            m_GraphicsEmulator = &GraphicsD3D12Emulator;
            auto *pDiligentAdapterD3D12 = new DiligentGraphicsAdapterD3D12(GraphicsD3D12Emulator);
            m_DiligentGraphics.reset(pDiligentAdapterD3D12);
            if (NativeWindowHandle != nullptr)
            {
                GraphicsD3D12Emulator.CreateSwapChain(NativeWindowHandle, WindowWidth, WindowHeight);
                pDiligentAdapterD3D12->InitProxySwapChain();
            }
        }
        break;
#endif

#if OPENGL_SUPPORTED
        case DeviceType::OpenGL:
        case DeviceType::OpenGLES:
        {
#if !PLATFORM_MACOS
            VERIFY_EXPR(NativeWindowHandle != nullptr);
#endif
            auto &GraphicsGLCoreES_Emulator = UnityGraphicsGLCoreES_Emulator::GetInstance();

#           if PLATFORM_WIN32 || PLATFORM_LINUX
                int major_version = 4;
                int minor_version = 4;
#           elif PLATFORM_MACOS
                int major_version = 4;
                int minor_version = 1;
#           elif PLATFORM_ANDROID
                int major_version = 3;
                int minor_version = 1;
#           elif PLATFORM_IOS
                int major_version = 3;
                int minor_version = 0;
#           else
#               error Unknown platform
#           endif

            GraphicsGLCoreES_Emulator.InitGLContext(NativeWindowHandle,
            #if PLATFORM_LINUX
                display,
            #endif
                major_version, minor_version
            );
            m_GraphicsEmulator = &GraphicsGLCoreES_Emulator;
            m_DiligentGraphics.reset(new DiligentGraphicsAdapterGL(GraphicsGLCoreES_Emulator));
        }
        break;
#endif

        default:
            LOG_ERROR_AND_THROW("Unsupported device type");
    }
}

void UnityAppBase::InitScene()
{
   m_Scene->SetDiligentGraphicsAdapter(m_DiligentGraphics.get());
   m_Scene->OnGraphicsInitialized();
#if D3D12_SUPPORTED
   if (m_DeviceType == DeviceType::D3D12)
   {
       UnityGraphicsD3D12Emulator::GetInstance().SetTransitionHandler(m_Scene->GetStateTransitionHandler());
   }
#endif
   if (!LoadPlugin())
   {
       LOG_ERROR_AND_THROW("Failed to load plugin");
   }

   m_Scene->OnPluginLoad(LoadPluginFunction);
   UnityPluginLoad(&m_GraphicsEmulator->GeUnityInterfaces());

   RenderEventFunc = GetRenderEventFunc();

   unsigned int SCWidth = 0;
   unsigned int SCHeight = 0;
   m_GraphicsEmulator->GetBackBufferSize(SCWidth, SCHeight);
   m_Scene->OnWindowResize(SCWidth, SCHeight);
}

void UnityAppBase::Update(double CurrTime, double ElapsedTime)
{
    m_Scene->Update(CurrTime, ElapsedTime);
}

void UnityAppBase::Render()
{
    m_GraphicsEmulator->BeginFrame();
    m_DiligentGraphics->BeginFrame();

    m_Scene->Render(RenderEventFunc);

    m_DiligentGraphics->EndFrame();
    m_GraphicsEmulator->EndFrame();
}

void UnityAppBase::Present()
{
    m_GraphicsEmulator->Present();
}

void UnityAppBase::WindowResize(int width, int height)
{
    if (m_GraphicsEmulator)
    {
        m_DiligentGraphics->PreSwapChainResize();
        m_GraphicsEmulator->ResizeSwapChain(width, height);
        m_DiligentGraphics->PostSwapChainResize();
        unsigned int SCWidth = 0;
        unsigned int SCHeight = 0;
        m_GraphicsEmulator->GetBackBufferSize(SCWidth, SCHeight);
        m_Scene->OnWindowResize(SCWidth, SCHeight);
    }
}
