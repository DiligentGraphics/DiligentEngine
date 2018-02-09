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


#pragma once 

#include <memory>

#define NOMINIMAX
#include <wrl.h>
#include <wrl/client.h>

#include "AppBase.h"
#include "Common/StepTimer.h"
#include "Common/DeviceResources.h"

class UWPAppBase : public AppBase
{
public:
    UWPAppBase();

    virtual void OnSetWindow(Windows::UI::Core::CoreWindow^ window) {}
    virtual void OnWindowSizeChanged() = 0;

    using AppBase::Update;
    virtual void Update();

    // Notifies the app that it is being suspended.
    virtual void OnSuspending() {}
    
    // Notifes the app that it is no longer suspended.
    virtual void OnResuming() {}

    // Notifies renderers that device resources need to be released.
    virtual void OnDeviceRemoved() {}

    bool IsFrameReady()const { return m_bFrameReady; }

    virtual std::shared_ptr<DX::DeviceResources> InitDeviceResources() = 0;

    virtual void InitWindowSizeDependentResources() = 0;
    
    virtual void CreateRenderers() = 0;

protected:
    std::shared_ptr<DX::DeviceResources> m_DeviceResources;

    // Rendering loop timer.
    DX::StepTimer m_timer;
    bool m_bFrameReady = false;
};
