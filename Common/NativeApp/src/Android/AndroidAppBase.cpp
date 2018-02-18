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

#include "AndroidAppBase.h"
#include "Timer.h"

#include <android/sensor.h>
//#include <android/log.h>
//#include <android_native_app_glue.h>
#include <android/native_window_jni.h>
//#include <cpu-features.h>

int AndroidAppBase::InitDisplay()
{
    if( !initialized_resources_ )
    {
        Initialize(app_->window);

        LoadResources();
        initialized_resources_ = true;
    }
    else
    {
        // initialize OpenGL ES and EGL
        if( EGL_SUCCESS != Resume( app_->window ) )
        {
            UnloadResources();
            LoadResources();
        }
    }
    
    ShowUI();

    //tap_camera_.SetFlip( 1.f, -1.f, -1.f );
    //tap_camera_.SetPinchTransformFactor( 2.f, 2.f, 8.f );

    return 0;    
}

void AndroidAppBase::InitSensors()
{
    sensor_manager_ = ASensorManager_getInstance();
    accelerometer_sensor_ = ASensorManager_getDefaultSensor( sensor_manager_, ASENSOR_TYPE_ACCELEROMETER );
    sensor_event_queue_ = ASensorManager_createEventQueue( sensor_manager_, app_->looper, LOOPER_ID_USER, NULL, NULL );
}

//
// Just the current frame in the display.
//
void AndroidAppBase::DrawFrame()
{
    // APP_CMD_CONFIG_CHANGED event is generated seveal frames
    // before the screen is actually resized. The only robust way
    // to detect window resize is to check it very frame
    if(CheckWindowSizeChanged())
        WindowResize(0,0);

    float fFPS;
    if( monitor_.Update( fFPS ) )
    {
        UpdateFPS( fFPS );
    }

    static Diligent::Timer Timer;
    static double PrevTime = Timer.GetElapsedTime();
    auto CurrTime = Timer.GetElapsedTime();
    auto ElapsedTime = CurrTime - PrevTime;
    PrevTime = CurrTime;
    
    Update(CurrTime, ElapsedTime);

    Render();

    Present();
    //if( EGL_SUCCESS != pRenderDevice_->Present() )
    //{
    //    UnloadResources();
    //    LoadResources();
    //}
}


//
// Process the next input event.
//
int32_t AndroidAppBase::HandleInput( android_app* app, AInputEvent* event )
{
    AndroidAppBase* eng = (AndroidAppBase*)app->userData;
    return eng->HandleInput( event );
}

//
// Process the next main command.
//
void AndroidAppBase::HandleCmd( struct android_app* app, int32_t cmd )
{
    AndroidAppBase* eng = (AndroidAppBase*)app->userData;
    switch( cmd )
    {
    case APP_CMD_SAVE_STATE:
        break;

    case APP_CMD_INIT_WINDOW:
        // The window is being shown, get it ready.
        if( app->window != NULL )
        {
            eng->InitDisplay();
            eng->DrawFrame();
        }
        break;

    case APP_CMD_CONFIG_CHANGED:
    case APP_CMD_WINDOW_RESIZED:
        // This does not work as the screen resizes few frames
        // after the event has been received
        // eng->WindowResize(0,0);
        break;

    case APP_CMD_TERM_WINDOW:
        // The window is being hidden or closed, clean it up.
        eng->TermDisplay();
        eng->has_focus_ = false;
        break;

    case APP_CMD_STOP:
        break;

    case APP_CMD_GAINED_FOCUS:
        eng->ResumeSensors();
        //Start animation
        eng->has_focus_ = true;
        break;

    case APP_CMD_LOST_FOCUS:
        eng->SuspendSensors();
        // Also stop animating.
        eng->has_focus_ = false;
        eng->DrawFrame();
        break;

    case APP_CMD_LOW_MEMORY:
        //Free up GL resources
        eng->TrimMemory();
        break;
    }
}

//-------------------------------------------------------------------------
//Sensor handlers
//-------------------------------------------------------------------------
void AndroidAppBase::ProcessSensors( int32_t id )
{
    // If a sensor has data, process it now.
    if( id == LOOPER_ID_USER )
    {
        if( accelerometer_sensor_ != NULL )
        {
            ASensorEvent event;
            while( ASensorEventQueue_getEvents( sensor_event_queue_, &event, 1 ) > 0 )
            {
            }
        }
    }
}

void AndroidAppBase::ResumeSensors()
{
    // When our app gains focus, we start monitoring the accelerometer.
    if( accelerometer_sensor_ != NULL )
    {
        ASensorEventQueue_enableSensor( sensor_event_queue_, accelerometer_sensor_ );
        // We'd like to get 60 events per second (in us).
        ASensorEventQueue_setEventRate( sensor_event_queue_, accelerometer_sensor_,
            (1000L / 60) * 1000 );
    }
}

void AndroidAppBase::SuspendSensors()
{
    // When our app loses focus, we stop monitoring the accelerometer.
    // This is to avoid consuming battery while not being used.
    if( accelerometer_sensor_ != NULL )
    {
        ASensorEventQueue_disableSensor( sensor_event_queue_, accelerometer_sensor_ );
    }
}

//-------------------------------------------------------------------------
//Misc
//-------------------------------------------------------------------------
void AndroidAppBase::SetState( android_app* state )
{
    app_ = state;
    doubletap_detector_.SetConfiguration( app_->config );
    drag_detector_.SetConfiguration( app_->config );
    pinch_detector_.SetConfiguration( app_->config );
}

bool AndroidAppBase::IsReady()
{
    if( has_focus_ )
        return true;

    return false;
}

//void Engine::TransformPosition( ndk_helper::Vec2& vec )
//{
//    vec = ndk_helper::Vec2( 2.0f, 2.0f ) * vec
//        / ndk_helper::Vec2( pDeviceContext_->GetMainBackBufferDesc().Width, pDeviceContext_->GetMainBackBufferDesc().Height )
//        - ndk_helper::Vec2( 1.f, 1.f );
//}

void AndroidAppBase::ShowUI()
{
    JNIEnv *jni;
    app_->activity->vm->AttachCurrentThread( &jni, NULL );

    //Default class retrieval
    jclass clazz = jni->GetObjectClass( app_->activity->clazz );
    jmethodID methodID = jni->GetMethodID( clazz, "showUI", "()V" );
    jni->CallVoidMethod( app_->activity->clazz, methodID );

    app_->activity->vm->DetachCurrentThread();
    return;
}

void AndroidAppBase::UpdateFPS( float fFPS )
{
    JNIEnv *jni;
    app_->activity->vm->AttachCurrentThread( &jni, NULL );

    //Default class retrieval
    jclass clazz = jni->GetObjectClass( app_->activity->clazz );
    jmethodID methodID = jni->GetMethodID( clazz, "updateFPS", "(F)V" );
    jni->CallVoidMethod( app_->activity->clazz, methodID, fFPS );

    app_->activity->vm->DetachCurrentThread();
    return;
}
