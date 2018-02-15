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

#include "SampleApp.h"
#include "RenderDeviceGLES.h"
#include "AntTweakBar.h"

using namespace Diligent;

class SampleAppAndroid final : public SampleApp
{
public:
    SampleAppAndroid()
    {
        m_DeviceType = DeviceType::OpenGLES;
    }

    virtual void Initialize(ANativeWindow* window)override final
    {
        InitializeDiligentEngine(window);
        m_RenderDeviceGLES = RefCntAutoPtr<IRenderDeviceGLES>(m_pDevice, IID_RenderDeviceGLES);
        m_TheSample->SetUIScale(3);
        InitializeSample();
    }

    virtual int Resume(ANativeWindow* window)override final
    {
        return m_RenderDeviceGLES->Resume(window);
    }

    virtual void TermDisplay()override final
    {
        // Tear down the EGL context currently associated with the display.
        m_RenderDeviceGLES->Suspend();
    }

    virtual void TrimMemory()override final
    {
        LOGI( "Trimming memory" );
        m_RenderDeviceGLES->Invalidate();
    }
        
    virtual int32_t HandleInput( AInputEvent* event )override final
    {
        if( AInputEvent_getType( event ) == AINPUT_EVENT_TYPE_MOTION )
        {
            ndk_helper::GESTURE_STATE doubleTapState = doubletap_detector_.Detect( event );
            ndk_helper::GESTURE_STATE dragState = drag_detector_.Detect( event );
            ndk_helper::GESTURE_STATE pinchState = pinch_detector_.Detect( event );

            //Double tap detector has a priority over other detectors
            if( doubleTapState == ndk_helper::GESTURE_STATE_ACTION )
            {
                //Detect double tap
                //tap_camera_.Reset( true );
            }
            else
            {
                //Handle drag state
                if( dragState & ndk_helper::GESTURE_STATE_START )
                {
                    //Otherwise, start dragging
                    ndk_helper::Vec2 v;
                    drag_detector_.GetPointer( v );
                    float fX = 0, fY = 0;
                    v.Value(fX, fY);
                    TwMouseMotion((short)fX, (short)fY);
                    int Handled = TwMouseButton( TW_MOUSE_PRESSED, TW_MOUSE_LEFT );

                    //TransformPosition( v );
                    //tap_camera_.BeginDrag( v );
                }
                else if( dragState & ndk_helper::GESTURE_STATE_MOVE )
                {
                    ndk_helper::Vec2 v;
                    drag_detector_.GetPointer( v );
                    float fX = 0, fY = 0;
                    v.Value(fX, fY);
                    int Handled = TwMouseMotion((short)fX, (short)fY);
                    //TransformPosition( v );
                    //tap_camera_.Drag( v );
                }
                else if( dragState & ndk_helper::GESTURE_STATE_END )
                {
                    int Handled = TwMouseButton(TW_MOUSE_RELEASED, TW_MOUSE_LEFT);

                    //tap_camera_.EndDrag();
                }

                //Handle pinch state
                if( pinchState & ndk_helper::GESTURE_STATE_START )
                {
                    //Start new pinch
                    ndk_helper::Vec2 v1;
                    ndk_helper::Vec2 v2;
                    pinch_detector_.GetPointers( v1, v2 );
                    //TransformPosition( v1 );
                    //TransformPosition( v2 );
                    //tap_camera_.BeginPinch( v1, v2 );
                }
                else if( pinchState & ndk_helper::GESTURE_STATE_MOVE )
                {
                    //Multi touch
                    //Start new pinch
                    ndk_helper::Vec2 v1;
                    ndk_helper::Vec2 v2;
                    pinch_detector_.GetPointers( v1, v2 );
                    //TransformPosition( v1 );
                    //TransformPosition( v2 );
                    //tap_camera_.Pinch( v1, v2 );
                }
            }
            return 1;
        }
        return 0;
    }
    
private:
    RefCntAutoPtr<IRenderDeviceGLES> m_RenderDeviceGLES;
};

NativeAppBase* CreateApplication()
{
    return new SampleAppAndroid;
}
