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

#include "RenderDevice.h"
#include "DeviceContext.h"
#include "RefCntAutoPtr.h"
#include "IUnityGraphics.h"
#include "DiligentGraphicsAdapter.h"
#include "ResourceStateTransitionHandler.h"

typedef void* (*TLoadPluginFunction)(const char *FunctionName);

class UnitySceneBase
{
public:
    virtual ~UnitySceneBase() = 0;

    virtual void OnPluginLoad(TLoadPluginFunction LoadPluginFunctionCallback) = 0;

    virtual void OnPluginUnload() = 0;

    virtual void OnGraphicsInitialized() = 0;

    virtual void OnWindowResize(int NewWidth, int NewHeight)
    {
        m_WindowWidth = NewWidth;
        m_WindowHeight = NewHeight;
    }

    virtual const char* GetSceneName()const = 0;

    virtual const char* GetPluginName()const = 0;

    virtual void Update(double CurrTime, double ElapsedTime) = 0;
    virtual void Render(UnityRenderingEvent RenderEventFunc) = 0;

    void SetDiligentGraphicsAdapter(DiligentGraphicsAdapter *DiligentGraphics)
    {
        m_DiligentGraphics = DiligentGraphics;
    }

    IResourceStateTransitionHandler* GetStateTransitionHandler() { return m_pStateTransitionHandler.get(); }

protected:
    DiligentGraphicsAdapter* m_DiligentGraphics;
    std::unique_ptr<IResourceStateTransitionHandler> m_pStateTransitionHandler;

    int m_WindowWidth = 640;
    int m_WindowHeight = 480;
};

inline UnitySceneBase::~UnitySceneBase() {}

extern UnitySceneBase* CreateScene();
