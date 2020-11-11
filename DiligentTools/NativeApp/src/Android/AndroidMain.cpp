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
#include <jni.h>
#include <errno.h>

#include "PlatformDefinitions.h"
#include "NativeAppBase.hpp"


#define HELPER_CLASS_NAME         "com/android/helper/NDKHelper" //Class name of helper function
#define NATIVEACTIVITY_CLASS_NAME "android/app/NativeActivity"

using namespace Diligent;

// This is the main entry point of a native application that is using
// android_native_app_glue.  It runs in its own thread, with its own
// event loop for receiving input events and doing other things.
void android_main(android_app* state)
{
    std::unique_ptr<AndroidAppBase> theApp(CreateApplication());
    theApp->SetState(state, NATIVEACTIVITY_CLASS_NAME);

    //Init helper functions
    ndk_helper::JNIHelper::Init(state->activity, HELPER_CLASS_NAME);
    auto mode = ndk_helper::JNIHelper::GetInstance()->GetIntentStringExtra("mode");

    if (!mode.empty())
    {
        std::string cmd_line = "-mode " + mode;
        theApp->ProcessCommandLine(cmd_line.c_str());
    }

    state->userData     = theApp.get();
    state->onAppCmd     = AndroidAppBase::HandleCmd;
    state->onInputEvent = AndroidAppBase::HandleInput;

    // Prepare to monitor accelerometer
    theApp->InitSensors();

    // loop waiting for stuff to do.
    while (true)
    {
        // Read all pending events.
        int                  id;
        int                  events;
        android_poll_source* source;

        // If not animating, we will block forever waiting for events.
        // If animating, we loop until all events are read, then continue
        // to draw the next frame of animation.
        while ((id = ALooper_pollAll(theApp->IsReady() ? 0 : -1, NULL, &events, (void**)&source)) >= 0)
        {
            // Process this event.
            if (source != NULL)
                source->process(state, source);

            theApp->ProcessSensors(id);

            // Check if we are exiting.
            if (state->destroyRequested != 0)
            {
                theApp->TermDisplay();
                return;
            }
        }

        if (theApp->IsReady())
        {
            // Drawing is throttled to the screen update rate, so there
            // is no need to do timing here.
            theApp->DrawFrame();
        }
    }
}
