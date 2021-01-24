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

#include "UnityGraphicsEmulator.h"
#include "UnityAppBase.h"
#include "IUnityInterface.h"
#include "Errors.hpp"

class UnityAppMacOS : public UnityAppBase
{
public:
    UnityAppMacOS()
    {
        m_DeviceType = Diligent::RENDER_DEVICE_TYPE_GL;
    }

    virtual void Initialize(void* view, RenderMode Mode)override final
    {
        if (Mode != RenderMode::OpenGL)
            LOG_ERROR_AND_THROW("This app only supports OpenGL mode");
        InitGraphics(nullptr, 0/*Unused*/, 0/*Unused*/);
        InitScene();
    }
};

namespace Diligent
{

NativeAppBase* CreateApplication()
{
    return new UnityAppMacOS();
}

}

// The function must be defined in the plugin
extern void *LoadPluginFunction(const char *name);
void* UnityAppBase::LoadPluginFunction(const char* FunctionName)
{
    return ::LoadPluginFunction(FunctionName);
}

extern "C"
{
    void __attribute__((visibility("default"))) UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* unityInterfaces);
    void __attribute__((visibility("default"))) UNITY_INTERFACE_API UnityPluginUnload();
    UnityRenderingEvent __attribute__((visibility("default"))) UNITY_INTERFACE_API GetRenderEventFunc();
}

bool UnityAppBase::LoadPlugin()
{
    // Linux automagically sets function pointers
    this->UnityPluginLoad = ::UnityPluginLoad;
    this->UnityPluginUnload = ::UnityPluginUnload;
    this->GetRenderEventFunc = ::GetRenderEventFunc;
    return true;
}

void UnityAppBase::UnloadPlugin()
{
    if (m_GraphicsEmulator)
        m_GraphicsEmulator->InvokeDeviceEventCallback(kUnityGfxDeviceEventShutdown);
    if (UnityPluginUnload)
        UnityPluginUnload();
}

