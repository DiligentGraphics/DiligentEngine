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

//--------------------------------------------------------------------------------
// Include files
//--------------------------------------------------------------------------------
#include <jni.h>
#include <errno.h>

#include <android/sensor.h>
#include <android/log.h>
#include <android_native_app_glue.h>
#include <android/native_window_jni.h>
#include <cpu-features.h>
#include <memory>

#include "NDKHelper.h"

#include "Timer.h"

#include "IUnityInterface.h"
#include "UnityGraphicsGLESAndroid_Impl.h"
#include "UnityGraphicsGLCoreES_Emulator.h"
#include "DiligentGraphicsAdapterGL.h"
#include "UnitySceneBase.h"
#include "StringTools.h"
#include "Errors.h"

//-------------------------------------------------------------------------
//Preprocessor
//-------------------------------------------------------------------------
#define HELPER_CLASS_NAME "com/sample/helper/NDKHelper" //Class name of helper function
//-------------------------------------------------------------------------
//Shared state for our app.
//-------------------------------------------------------------------------

extern "C"
{
    void __attribute__((visibility("default"))) UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* unityInterfaces);
    void __attribute__((visibility("default"))) UNITY_INTERFACE_API UnityPluginUnload();
    UnityRenderingEvent __attribute__((visibility("default"))) UNITY_INTERFACE_API GetRenderEventFunc();
}

void* LoadPluginFunction(const char* FunctionName);

bool LoadPlugin()
{
    // Do nothing. Android automagically sets function pointers
    return true;
}


struct android_app;
class Engine
{
    bool initialized_resources_;
    bool has_focus_;

    ndk_helper::DoubletapDetector doubletap_detector_;
    ndk_helper::PinchDetector pinch_detector_;
    ndk_helper::DragDetector drag_detector_;
    ndk_helper::PerfMonitor monitor_;

    //ndk_helper::TapCamera tap_camera_;

    android_app* app_;

    ASensorManager* sensor_manager_;
    const ASensor* accelerometer_sensor_;
    ASensorEventQueue* sensor_event_queue_;

    void UpdateFPS( float fFPS );
    void ShowUI();
    //void TransformPosition( ndk_helper::Vec2& vec );

    std::unique_ptr<DiligentGraphicsAdapter> diligent_graphics_;
    std::unique_ptr<UnitySceneBase> scene_;
    UnityGraphicsGLCoreES_Emulator *unity_graphics_gles_emulator_ = nullptr;
    UnityRenderingEvent render_event_func_ = nullptr;

public:
    static void HandleCmd( struct android_app* app,
        int32_t cmd );
    static int32_t HandleInput( android_app* app,
        AInputEvent* event );

    Engine();
    ~Engine();
    void SetState( android_app* state );
    int InitDisplay();
    void LoadResources();
    void UnloadResources();
    void DrawFrame();
    void TermDisplay();
    void TrimMemory();
    bool IsReady();
    void UnloadPlugin();

    void UpdatePosition( AInputEvent* event,
        int32_t iIndex,
        float& fX,
        float& fY );

    void InitSensors();
    void ProcessSensors( int32_t id );
    void SuspendSensors();
    void ResumeSensors();
};

//-------------------------------------------------------------------------
//Ctor
//-------------------------------------------------------------------------
Engine::Engine() :
    initialized_resources_( false ),
    has_focus_( false ),
    app_( NULL ),
    sensor_manager_( NULL ),
    accelerometer_sensor_( NULL ),
    sensor_event_queue_( NULL ),
    scene_(CreateScene())
{
    if (!LoadPlugin())
    {
        LOG_ERROR_AND_THROW("Failed to load native plugin");
    }
    scene_->OnPluginLoad(LoadPluginFunction);
    render_event_func_ = GetRenderEventFunc();
}

void Engine::UnloadPlugin()
{
    unity_graphics_gles_emulator_->InvokeDeviceEventCallback(kUnityGfxDeviceEventShutdown);
    UnityPluginUnload();
}

//-------------------------------------------------------------------------
//Dtor
//-------------------------------------------------------------------------
Engine::~Engine()
{
    scene_->OnPluginUnload();
    scene_.reset();
    UnloadPlugin();
    diligent_graphics_.reset();
    unity_graphics_gles_emulator_->Release();
}

/**
* Load resources
*/
void Engine::LoadResources()
{
    scene_->OnGraphicsInitialized();

    //renderer_.Init();
    //renderer_.Bind( &tap_camera_ );
}

/**
* Unload resources
*/
void Engine::UnloadResources()
{
    //renderer_.Unload();
}

/**
* Initialize an EGL context for the current display.
*/
int Engine::InitDisplay()
{
    if( !initialized_resources_ )
    {
        // Init graphics resources
        auto &GraphicsGLCoreES_Emulator = UnityGraphicsGLCoreES_Emulator::GetInstance();
        GraphicsGLCoreES_Emulator.InitGLContext(app_->window, 3, 1);
        unity_graphics_gles_emulator_ = &GraphicsGLCoreES_Emulator;
        diligent_graphics_.reset(new DiligentGraphicsAdapterGL(GraphicsGLCoreES_Emulator));
        
        scene_->SetDiligentGraphicsAdapter(diligent_graphics_.get());

        LoadResources();
        initialized_resources_ = true;
    }
    else
    {
        // initialize OpenGL ES and EGL
        if( EGL_SUCCESS != unity_graphics_gles_emulator_->GetGraphicsImpl()->Resume( app_->window ) )
        {
            UnloadResources();
            LoadResources();
        }
    }

    UnityPluginLoad(&unity_graphics_gles_emulator_->GeUnityInterfaces());

    ShowUI();


    auto *graphics_impl = unity_graphics_gles_emulator_->GetGraphicsImpl();
    auto width = graphics_impl->GetBackBufferWidth();
    auto height = graphics_impl->GetBackBufferHeight();

    scene_->OnWindowResize(width, height);

    //tap_camera_.SetFlip( 1.f, -1.f, -1.f );
    //tap_camera_.SetPinchTransformFactor( 2.f, 2.f, 8.f );

    return 0;
}

/**
* Just the current frame in the display.
*/
void Engine::DrawFrame()
{
    float fFPS;
    if( monitor_.Update( fFPS ) )
    {
        UpdateFPS( fFPS );
    }

    unity_graphics_gles_emulator_->BeginFrame();
    diligent_graphics_->BeginFrame();

    static Diligent::Timer timer;
    static double prev_time = timer.GetElapsedTime();
    auto curr_time = timer.GetElapsedTime();
    auto elapsed_time = curr_time - prev_time;
    prev_time = curr_time;

    scene_->Render(render_event_func_, curr_time, elapsed_time);

    diligent_graphics_->EndFrame();
    unity_graphics_gles_emulator_->EndFrame();
    
    unity_graphics_gles_emulator_->Present();
    //if( EGL_SUCCESS != pRenderDevice_->Present() )
    //{
    //    UnloadResources();
    //    LoadResources();
    //}
}

/**
* Tear down the EGL context currently associated with the display.
*/
void Engine::TermDisplay()
{
    unity_graphics_gles_emulator_->GetGraphicsImpl()->Suspend();
}

void Engine::TrimMemory()
{
    LOGI( "Trimming memory" );
    unity_graphics_gles_emulator_->GetGraphicsImpl()->Invalidate();
}

/**
* Process the next input event.
*/
int32_t Engine::HandleInput( android_app* app,
    AInputEvent* event )
{
    Engine* eng = (Engine*)app->userData;
    if( AInputEvent_getType( event ) == AINPUT_EVENT_TYPE_MOTION )
    {
        ndk_helper::GESTURE_STATE doubleTapState = eng->doubletap_detector_.Detect( event );
        ndk_helper::GESTURE_STATE dragState = eng->drag_detector_.Detect( event );
        ndk_helper::GESTURE_STATE pinchState = eng->pinch_detector_.Detect( event );

        //Double tap detector has a priority over other detectors
        if( doubleTapState == ndk_helper::GESTURE_STATE_ACTION )
        {
            //Detect double tap
            //eng->tap_camera_.Reset( true );
        }
        else
        {
            //Handle drag state
            if( dragState & ndk_helper::GESTURE_STATE_START )
            {
                //Otherwise, start dragging
                ndk_helper::Vec2 v;
                eng->drag_detector_.GetPointer( v );
                float fX = 0, fY = 0;
                v.Value(fX, fY);

                //eng->TransformPosition( v );
                //eng->tap_camera_.BeginDrag( v );
            }
            else if( dragState & ndk_helper::GESTURE_STATE_MOVE )
            {
                ndk_helper::Vec2 v;
                eng->drag_detector_.GetPointer( v );
                float fX = 0, fY = 0;
                v.Value(fX, fY);

                //eng->TransformPosition( v );
                //eng->tap_camera_.Drag( v );
            }
            else if( dragState & ndk_helper::GESTURE_STATE_END )
            {
                //eng->tap_camera_.EndDrag();
            }

            //Handle pinch state
            if( pinchState & ndk_helper::GESTURE_STATE_START )
            {
                //Start new pinch
                ndk_helper::Vec2 v1;
                ndk_helper::Vec2 v2;
                eng->pinch_detector_.GetPointers( v1, v2 );
                //eng->TransformPosition( v1 );
                //eng->TransformPosition( v2 );
                //eng->tap_camera_.BeginPinch( v1, v2 );
            }
            else if( pinchState & ndk_helper::GESTURE_STATE_MOVE )
            {
                //Multi touch
                //Start new pinch
                ndk_helper::Vec2 v1;
                ndk_helper::Vec2 v2;
                eng->pinch_detector_.GetPointers( v1, v2 );
                //eng->TransformPosition( v1 );
                //eng->TransformPosition( v2 );
                //eng->tap_camera_.Pinch( v1, v2 );
            }
        }
        return 1;
    }
    return 0;
}

/**
* Process the next main command.
*/
void Engine::HandleCmd( struct android_app* app,
    int32_t cmd )
{
    Engine* eng = (Engine*)app->userData;
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
void Engine::InitSensors()
{
    sensor_manager_ = ASensorManager_getInstance();
    accelerometer_sensor_ = ASensorManager_getDefaultSensor( sensor_manager_,
        ASENSOR_TYPE_ACCELEROMETER );
    sensor_event_queue_ = ASensorManager_createEventQueue( sensor_manager_, app_->looper,
        LOOPER_ID_USER, NULL, NULL );
}

void Engine::ProcessSensors( int32_t id )
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

void Engine::ResumeSensors()
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

void Engine::SuspendSensors()
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
void Engine::SetState( android_app* state )
{
    app_ = state;
    doubletap_detector_.SetConfiguration( app_->config );
    drag_detector_.SetConfiguration( app_->config );
    pinch_detector_.SetConfiguration( app_->config );
}

bool Engine::IsReady()
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

void Engine::ShowUI()
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

void Engine::UpdateFPS( float fFPS )
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

Engine g_engine;

/**
* This is the main entry point of a native application that is using
* android_native_app_glue.  It runs in its own thread, with its own
* event loop for receiving input events and doing other things.
*/

// The actual android_main() must be defined in the application project
void android_main_impl( android_app* state )
{
    app_dummy();

    g_engine.SetState( state );

    //Init helper functions
    ndk_helper::JNIHelper::Init( state->activity, HELPER_CLASS_NAME );

    state->userData = &g_engine;
    state->onAppCmd = Engine::HandleCmd;
    state->onInputEvent = Engine::HandleInput;

#ifdef USE_NDK_PROFILER
    monstartup( "libEngineSandbox.so" );
#endif

    // Prepare to monitor accelerometer
    g_engine.InitSensors();

    // loop waiting for stuff to do.
    while( 1 )
    {
        // Read all pending events.
        int id;
        int events;
        android_poll_source* source;

        // If not animating, we will block forever waiting for events.
        // If animating, we loop until all events are read, then continue
        // to draw the next frame of animation.
        while( (id = ALooper_pollAll( g_engine.IsReady() ? 0 : -1, NULL, &events, (void**)&source )) >= 0 )
        {
            // Process this event.
            if( source != NULL )
                source->process( state, source );

            g_engine.ProcessSensors( id );

            // Check if we are exiting.
            if( state->destroyRequested != 0 )
            {
                g_engine.TermDisplay();
                return;
            }
        }

        if( g_engine.IsReady() )
        {
            // Drawing is throttled to the screen update rate, so there
            // is no need to do timing here.
            g_engine.DrawFrame();
        }
    }
}
