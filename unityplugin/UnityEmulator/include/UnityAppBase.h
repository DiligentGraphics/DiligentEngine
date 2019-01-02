/*     Copyright 2015-2019 Egor Yusov
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
#pragma once

#include <memory>

#include "NativeAppBase.h"
#include "RenderDevice.h"
#include "DeviceContext.h"
#include "RefCntAutoPtr.h"
#include "UnitySceneBase.h"
#include "IUnityGraphics.h"
#include "DiligentGraphicsAdapter.h"
#include "ResourceStateTransitionHandler.h"

typedef void* (*TLoadPluginFunction)(const char *FunctionName);

class UnityAppBase : public NativeAppBase
{
public:
    UnityAppBase();
    virtual ~UnityAppBase()override;

    virtual void ProcessCommandLine(const char *CmdLine)override;
    virtual const char* GetAppTitle()const override { return m_AppTitle.c_str(); }
    virtual void Render()override;
    virtual void Present()override;
    virtual void WindowResize(int width, int height)override;
    virtual void Update(double CurrTime, double ElapsedTime)override;

    bool LoadPlugin();

protected:
    virtual void InitGraphics(
#if PLATFORM_LINUX
        void *display,
#endif
        void *NativeWindowHandle, 
        int WindowWidth, 
        int WindowHeight
    );
    virtual void InitScene();

    std::unique_ptr<UnitySceneBase> m_Scene;
    Diligent::DeviceType m_DeviceType = Diligent::DeviceType::Undefined;
    std::string m_AppTitle;

    class UnityGraphicsEmulator *m_GraphicsEmulator = nullptr;
    std::unique_ptr<class DiligentGraphicsAdapter> m_DiligentGraphics;
    std::unique_ptr<IResourceStateTransitionHandler> m_pStateTransitionHandler;

    typedef void (UNITY_INTERFACE_API *TUnityPluginLoad)(IUnityInterfaces* unityInterfaces);
    typedef void (UNITY_INTERFACE_API *TUnityPluginUnload)();
    typedef UnityRenderingEvent(UNITY_INTERFACE_API *TGetRenderEventFunc)();

    TUnityPluginLoad UnityPluginLoad = nullptr;
    TUnityPluginUnload UnityPluginUnload = nullptr;
    TGetRenderEventFunc GetRenderEventFunc = nullptr;
    UnityRenderingEvent RenderEventFunc = nullptr;

    void UnloadPlugin();
    static void* LoadPluginFunction(const char* FunctionName);
};

