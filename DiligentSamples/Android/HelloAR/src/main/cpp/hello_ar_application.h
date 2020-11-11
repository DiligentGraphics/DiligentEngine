/*
 * Copyright 2017 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Modified by Diligent Graphics LLC.

#ifndef C_ARCORE_HELLOE_AR_HELLO_AR_APPLICATION_H_
#define C_ARCORE_HELLOE_AR_HELLO_AR_APPLICATION_H_

#include <android/asset_manager.h>
#include <jni.h>

#include "RefCntAutoPtr.hpp"
#include "RenderDevice.h"
#include "DeviceContext.h"

#include "arcore_c_api.h"
#include "BackgroundRenderer.h"


#include "ObjectRenderer.h"
#include "PlaneRenderer.h"
#include "PointCloudRenderer.h"
#include "util.h"

namespace hello_ar
{

// HelloArApplication handles all application logics.
class HelloArApplication
{
public:
    // Constructor and deconstructor.
    explicit HelloArApplication(AAssetManager* asset_manager);
    ~HelloArApplication();

    // OnPause is called on the UI thread from the Activity's onPause method.
    void OnPause();

    // OnResume is called on the UI thread from the Activity's onResume method.
    void OnResume(void* env, void* context, void* activity);

    // OnSurfaceCreated is called on the OpenGL thread when GLSurfaceView
    // is created.
    void OnSurfaceCreated();

    // OnDisplayGeometryChanged is called on the OpenGL thread when the
    // render surface size or display rotation changes.
    //
    // @param display_rotation: current display rotation.
    // @param width: width of the changed surface view.
    // @param height: height of the changed surface view.
    void OnDisplayGeometryChanged(int display_rotation, int width, int height);

    // OnDrawFrame is called on the OpenGL thread to render the next frame.
    void OnDrawFrame();

    // OnTouched is called on the OpenGL thread after the user touches the screen.
    // @param x: x position on the screen (pixels).
    // @param y: y position on the screen (pixels).
    void OnTouched(float x, float y);

    // Returns true if any planes have been detected.  Used for hiding the
    // "searching for planes" snackbar.
    bool HasDetectedPlanes() const { return plane_count_ > 0; }

private:
    ArSession* ar_session_ = nullptr;
    ArFrame*   ar_frame_   = nullptr;

    Diligent::RefCntAutoPtr<Diligent::IRenderDevice>  render_device_;
    Diligent::RefCntAutoPtr<Diligent::IDeviceContext> device_context_;

    bool install_requested_ = false;
    int  width_             = 1;
    int  height_            = 1;
    int  display_rotation_  = 0;

    AAssetManager* const asset_manager_;

    // The anchors at which we are drawing android models using given colors.
    struct ColoredAnchor
    {
        ArAnchor* anchor;
        float     color[4];
    };

    std::vector<ColoredAnchor> anchors_;

    PointCloudRenderer point_cloud_renderer_;
    BackgroundRenderer background_renderer_;
    PlaneRenderer      plane_renderer_;
    ObjRenderer        cube_renderer_;

    int32_t plane_count_ = 0;

    void SetColor(float r, float g, float b, float a, float* color4f);
};

} // namespace hello_ar

#endif // C_ARCORE_HELLOE_AR_HELLO_AR_APPLICATION_H_
