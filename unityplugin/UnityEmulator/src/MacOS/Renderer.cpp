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

#include "Renderer.h"
#include "Errors.h"
#include "RenderDeviceFactoryOpenGL.h"

#include "IUnityInterface.h"
#include "UnityGraphicsGLCoreES_Emulator.h"
#include "DiligentGraphicsAdapterGL.h"
#include "StringTools.h"

using namespace Diligent;

extern "C"
{
    void __attribute__((visibility("default"))) UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* unityInterfaces);
    void __attribute__((visibility("default"))) UNITY_INTERFACE_API UnityPluginUnload();
    UnityRenderingEvent __attribute__((visibility("default"))) UNITY_INTERFACE_API GetRenderEventFunc();
}

void* LoadPluginFunction(const char* FunctionName);

bool LoadPlugin()
{
    // Do nothing. Function pointers are set automagically
    return true;
}

void UnloadPlugin(UnityGraphicsEmulator *GraphicsEmulator)
{
    GraphicsEmulator->InvokeDeviceEventCallback(kUnityGfxDeviceEventShutdown);
    UnityPluginUnload();
}

Renderer::Renderer()
{
}

Renderer::~Renderer()
{

    pScene->OnPluginUnload();
    pScene.reset();
    UnloadPlugin(GraphicsEmulator);
    
    pDiligentGraphics.reset();
    GraphicsEmulator->Release();
}

void Renderer::Init()
{
    int major_version = 4;
    int minor_version = 1;
    auto &GraphicsGLCoreES_Emulator = UnityGraphicsGLCoreES_Emulator::GetInstance();
    GraphicsGLCoreES_Emulator.InitGLContext(nullptr, major_version, minor_version);
    
    pDiligentGraphics.reset(new DiligentGraphicsAdapterGL(GraphicsGLCoreES_Emulator));
    GraphicsEmulator = &GraphicsGLCoreES_Emulator;
    
    pScene.reset(CreateScene());
    std::string Title = pScene->GetSceneName();
    pScene->SetDiligentGraphicsAdapter(pDiligentGraphics.get());
    pScene->OnGraphicsInitialized();
    //if (DevType == DeviceType::D3D12)
    //{
    //    UnityGraphicsD3D12Emulator::GetInstance().SetTransitionHandler(g_pScene->GetStateTransitionHandler());
    //}
    
    if (!LoadPlugin())
    {
        LOG_ERROR_AND_THROW("Failed to load the plugin");
    }
    
    pScene->OnPluginLoad(LoadPluginFunction);
    UnityPluginLoad(&GraphicsEmulator->GeUnityInterfaces());
    
    RenderEventFunc = GetRenderEventFunc();

    PrevTime = timer.GetElapsedTime();
}

void Renderer::WindowResize(int width, int height)
{
    pDiligentGraphics->PreSwapChainResize();
    GraphicsEmulator->ResizeSwapChain(static_cast<Uint32>(width), static_cast<Uint32>(height));
    pDiligentGraphics->PostSwapChainResize();
    pScene->OnWindowResize(static_cast<Uint32>(width), static_cast<Uint32>(height));
}

void Renderer::Render()
{
    // Render the scene
    auto CurrTime = timer.GetElapsedTime();
    auto ElapsedTime = CurrTime - PrevTime;
    PrevTime = CurrTime;
    
    GraphicsEmulator->BeginFrame();
    pDiligentGraphics->BeginFrame();
    
    pScene->Render(RenderEventFunc, CurrTime, ElapsedTime);
    
    pDiligentGraphics->EndFrame();
    GraphicsEmulator->EndFrame();
}

