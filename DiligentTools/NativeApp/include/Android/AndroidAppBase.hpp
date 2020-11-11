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

#include <android_native_app_glue.h>
#include "AppBase.hpp"

#include "NDKHelper.h"

struct android_app;

namespace Diligent
{

/// Base class for Android applications.
class AndroidAppBase : public AppBase
{
public:
    int            InitDisplay();
    void           SetState(android_app* state, const char* native_activity_class_name);
    void           InitSensors();
    void           ProcessSensors(int32_t id);
    virtual void   DrawFrame();
    bool           IsReady();
    virtual void   TrimMemory()  = 0;
    virtual void   TermDisplay() = 0;
    static int32_t HandleInput(android_app* app, AInputEvent* event);
    static void    HandleCmd(android_app* app, int32_t cmd);

protected:
    virtual void Initialize()
    {
    }

    virtual int Resume(ANativeWindow* window) = 0;

    virtual int32_t HandleInput(AInputEvent* event) { return 0; }

    virtual void LoadResources()
    {
        //renderer_.Init();
        //renderer_.Bind( &tap_camera_ );
    }

    virtual void UnloadResources()
    {
        //renderer_.Unload();
    }

    ndk_helper::DoubletapDetector doubletap_detector_;
    ndk_helper::PinchDetector     pinch_detector_;
    ndk_helper::DragDetector      drag_detector_;
    ndk_helper::PerfMonitor       monitor_;

    //ndk_helper::TapCamera tap_camera_;
    android_app* app_ = nullptr;
    std::string  native_activity_class_name_;

private:
    void UpdatePosition(AInputEvent* event, int32_t iIndex, float& fX, float& fY);
    void SuspendSensors();
    void ResumeSensors();
    void ShowUI();
    void UpdateFPS(float fFPS);

    bool initialized_resources_ = false;
    bool has_focus_             = false;

    ASensorManager*    sensor_manager_       = nullptr;
    const ASensor*     accelerometer_sensor_ = nullptr;
    ASensorEventQueue* sensor_event_queue_   = nullptr;
};

} // namespace Diligent
