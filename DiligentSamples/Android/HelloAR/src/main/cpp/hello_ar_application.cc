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

// Modified by Diligent Graphics LLC

#include "hello_ar_application.h"

#include <GLES3/gl3.h>
#include <GLES3/gl31.h>

#include <android/asset_manager.h>

#include "EngineFactoryOpenGL.h"
#include "DeviceContextGL.h"
#include "AndroidFileSystem.hpp"
#include "BasicMath.hpp"

#include "util.h"

using namespace Diligent;

namespace hello_ar
{

namespace
{

constexpr size_t kMaxNumberOfObjectsToRender = 20;

const Diligent::float3 kWhite = {1.f, 1.f, 1.f};

} // namespace

HelloArApplication::HelloArApplication(AAssetManager* asset_manager) :
    asset_manager_(asset_manager)
{
    AndroidFileSystem::Init(nullptr, "", asset_manager_);
}

HelloArApplication::~HelloArApplication()
{
    if (ar_session_ != nullptr)
    {
        ArSession_destroy(ar_session_);
        ArFrame_destroy(ar_frame_);
    }
}

void HelloArApplication::OnPause()
{
    LOGI("OnPause()");

    if (ar_session_ != nullptr)
    {
        ArSession_pause(ar_session_);
    }
}

void HelloArApplication::OnResume(void* env, void* context, void* activity)
{
    LOGI("OnResume()");

    if (ar_session_ == nullptr)
    {
        ArInstallStatus install_status;
        // If install was not yet requested, that means that we are resuming the
        // activity first time because of explicit user interaction (such as
        // launching the application)
        bool user_requested_install = !install_requested_;

        // === ATTENTION!  ATTENTION!  ATTENTION! ===
        // This method can and will fail in user-facing situations.  Your
        // application must handle these cases at least somewhat gracefully.  See
        // HelloAR Java sample code for reasonable behavior.
        VERIFY_EXPR(ArCoreApk_requestInstall(env, activity, user_requested_install,
                                             &install_status) == AR_SUCCESS);

        switch (install_status)
        {
            case AR_INSTALL_STATUS_INSTALLED:
                break;
            case AR_INSTALL_STATUS_INSTALL_REQUESTED:
                install_requested_ = true;
                return;
        }

        // === ATTENTION!  ATTENTION!  ATTENTION! ===
        // This method can and will fail in user-facing situations.  Your
        // application must handle these cases at least somewhat gracefully.  See
        // HelloAR Java sample code for reasonable behavior.
        VERIFY_EXPR(ArSession_create(env, context, &ar_session_) == AR_SUCCESS);
        VERIFY_EXPR(ar_session_);

        ArFrame_create(ar_session_, &ar_frame_);
        VERIFY_EXPR(ar_frame_);

        ArSession_setDisplayGeometry(ar_session_, display_rotation_, width_,
                                     height_);
    }

    const ArStatus status = ArSession_resume(ar_session_);
    VERIFY_EXPR(status == AR_SUCCESS);
}

void HelloArApplication::OnSurfaceCreated()
{
    LOGI("OnSurfaceCreated()");

    // Attach Diligent Engine to existing OpenGLES context
    auto*              pFactory = GetEngineFactoryOpenGL();
    EngineGLCreateInfo EngineCI;
#if DILIGENT_DEVELOPMENT
    EngineCI.CreateDebugContext = true;
#endif
    pFactory->AttachToActiveGLContext(EngineCI, &render_device_, &device_context_);
    // Init Android file system so that we can use shader source stream factory to load shaders.
    pFactory->InitAndroidFileSystem(nullptr, "", asset_manager_);

    background_renderer_.Initialize(render_device_);
    point_cloud_renderer_.Initialize(render_device_);
    cube_renderer_.Initialize(render_device_);
    plane_renderer_.Initialize(render_device_);
}

void HelloArApplication::OnDisplayGeometryChanged(int display_rotation,
                                                  int width,
                                                  int height)
{
    LOGI("OnSurfaceChanged(%d, %d)", width, height);

    display_rotation_ = display_rotation;
    width_            = width;
    height_           = height;
    if (ar_session_ != nullptr)
    {
        ArSession_setDisplayGeometry(ar_session_, display_rotation, width, height);
    }
}

void HelloArApplication::OnDrawFrame()
{
    // Get current framebuffer

    GLint DrawFramebuffer = -1;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &DrawFramebuffer);
    if (glGetError() != GL_NO_ERROR)
    {
        LOGE("Failed to get current draw framebuffer binding");
        DrawFramebuffer = 0;
    }

    GLint ReadFramebuffer = -1;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &ReadFramebuffer);
    if (glGetError() != GL_NO_ERROR)
    {
        LOGE("Failed to get current read framebuffer binding");
        ReadFramebuffer = 0;
    }

    glViewport(0, 0, width_, height_);
    glScissor(0, 0, width_, height_);

    // The pixel ownership test, the scissor test, dithering, and the buffer writemasks affect the
    // operation of glClear.
    // Alpha function, blend function, logical operation, stenciling, texture mapping, and
    // depth-buffering are ignored by glClear.
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClearDepthf(1.f);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    if (ar_session_ == nullptr) return;

    ArSession_setCameraTextureName(ar_session_,
                                   background_renderer_.GetTextureId());

    // Update session to get current frame and render camera background.
    if (ArSession_update(ar_session_, ar_frame_) != AR_SUCCESS)
    {
        LOGE("HelloArApplication::OnDrawFrame ArSession_update error");
    }

    ArCamera* ar_camera;
    ArFrame_acquireCamera(ar_session_, ar_frame_, &ar_camera);

    Diligent::float4x4 view_mat;
    Diligent::float4x4 projection_mat;
    ArCamera_getViewMatrix(ar_session_, ar_camera, view_mat.Data());
    ArCamera_getProjectionMatrix(ar_session_, ar_camera,
                                 /*near=*/0.1f, /*far=*/100.f,
                                 projection_mat.Data());
    // Transpose the matrices because Diligent uses row-major storage as GL-style column-major storage
    view_mat       = view_mat.Transpose();
    projection_mat = projection_mat.Transpose();

    ArTrackingState camera_tracking_state;
    ArCamera_getTrackingState(ar_session_, ar_camera, &camera_tracking_state);
    ArCamera_release(ar_camera);

    // Invalidate state
    device_context_->InvalidateState();
    // Restore original framebuffer that was unbound by InvalidateState()
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, DrawFramebuffer);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, ReadFramebuffer);

    background_renderer_.Draw(ar_session_, ar_frame_, device_context_);

    // If the camera isn't tracking don't bother rendering other objects.
    if (camera_tracking_state != AR_TRACKING_STATE_TRACKING)
    {
        return;
    }

    // Get light estimation value.
    ArLightEstimate*     ar_light_estimate;
    ArLightEstimateState ar_light_estimate_state;
    ArLightEstimate_create(ar_session_, &ar_light_estimate);

    ArFrame_getLightEstimate(ar_session_, ar_frame_, ar_light_estimate);
    ArLightEstimate_getState(ar_session_, ar_light_estimate,
                             &ar_light_estimate_state);

    // Set light intensity to default. Intensity value ranges from 0.0f to 1.0f.
    // The first three components are color scaling factors.
    // The last one is the average pixel intensity in gamma space.
    float color_correction[4] = {1.f, 1.f, 1.f, 1.f};
    if (ar_light_estimate_state == AR_LIGHT_ESTIMATE_STATE_VALID)
    {
        ArLightEstimate_getColorCorrection(ar_session_, ar_light_estimate,
                                           color_correction);
    }

    ArLightEstimate_destroy(ar_light_estimate);
    ar_light_estimate = nullptr;

    // Render cubes.
    for (const auto& colored_anchor : anchors_)
    {
        ArTrackingState tracking_state = AR_TRACKING_STATE_STOPPED;
        ArAnchor_getTrackingState(ar_session_, colored_anchor.anchor,
                                  &tracking_state);
        if (tracking_state == AR_TRACKING_STATE_TRACKING)
        {
            // Render object only if the tracking state is AR_TRACKING_STATE_TRACKING.
            Diligent::float4x4 model_mat = Diligent::float4x4::Identity();
            util::GetTransformMatrixFromAnchor(*colored_anchor.anchor, ar_session_, &model_mat);
            model_mat = model_mat.Transpose();
            cube_renderer_.Draw(device_context_, projection_mat, view_mat, model_mat, color_correction,
                                colored_anchor.color);
        }
    }

    // Update and render planes.
    ArTrackableList* plane_list = nullptr;
    ArTrackableList_create(ar_session_, &plane_list);
    VERIFY_EXPR(plane_list != nullptr);

    ArTrackableType plane_tracked_type = AR_TRACKABLE_PLANE;
    ArSession_getAllTrackables(ar_session_, plane_tracked_type, plane_list);

    int32_t plane_list_size = 0;
    ArTrackableList_getSize(ar_session_, plane_list, &plane_list_size);
    plane_count_ = plane_list_size;

    for (int i = 0; i < plane_list_size; ++i)
    {
        ArTrackable* ar_trackable = nullptr;
        ArTrackableList_acquireItem(ar_session_, plane_list, i, &ar_trackable);
        ArPlane*        ar_plane = ArAsPlane(ar_trackable);
        ArTrackingState out_tracking_state;
        ArTrackable_getTrackingState(ar_session_, ar_trackable,
                                     &out_tracking_state);

        ArPlane* subsume_plane;
        ArPlane_acquireSubsumedBy(ar_session_, ar_plane, &subsume_plane);
        if (subsume_plane != nullptr)
        {
            ArTrackable_release(ArAsTrackable(subsume_plane));
            continue;
        }

        if (ArTrackingState::AR_TRACKING_STATE_TRACKING != out_tracking_state)
        {
            continue;
        }

        ArTrackingState plane_tracking_state;
        ArTrackable_getTrackingState(ar_session_, ArAsTrackable(ar_plane),
                                     &plane_tracking_state);
        if (plane_tracking_state == AR_TRACKING_STATE_TRACKING)
        {
            plane_renderer_.Draw(device_context_, projection_mat, view_mat, *ar_session_, *ar_plane, kWhite);
            ArTrackable_release(ar_trackable);
        }
    }

    ArTrackableList_destroy(plane_list);
    plane_list = nullptr;

    // Update and render point cloud.
    ArPointCloud* ar_point_cloud = nullptr;
    ArStatus      point_cloud_status =
        ArFrame_acquirePointCloud(ar_session_, ar_frame_, &ar_point_cloud);
    if (point_cloud_status == AR_SUCCESS)
    {
        point_cloud_renderer_.Draw(device_context_, projection_mat * view_mat, ar_session_, ar_point_cloud);
        ArPointCloud_release(ar_point_cloud);
    }

    // We never call Present, so explicitly call FinishFrame()
    device_context_->FinishFrame();

    glDepthMask(GL_TRUE);
    glUseProgram(0);
    glBindProgramPipeline(0);
    glBindVertexArray(0);
}

void HelloArApplication::OnTouched(float x, float y)
{
    if (ar_frame_ != nullptr && ar_session_ != nullptr)
    {
        ArHitResultList* hit_result_list = nullptr;
        ArHitResultList_create(ar_session_, &hit_result_list);
        VERIFY_EXPR(hit_result_list);
        ArFrame_hitTest(ar_session_, ar_frame_, x, y, hit_result_list);

        int32_t hit_result_list_size = 0;
        ArHitResultList_getSize(ar_session_, hit_result_list,
                                &hit_result_list_size);

        // The hitTest method sorts the resulting list by distance from the camera,
        // increasing.  The first hit result will usually be the most relevant when
        // responding to user input.

        ArHitResult*    ar_hit_result  = nullptr;
        ArTrackableType trackable_type = AR_TRACKABLE_NOT_VALID;
        for (int32_t i = 0; i < hit_result_list_size; ++i)
        {
            ArHitResult* ar_hit = nullptr;
            ArHitResult_create(ar_session_, &ar_hit);
            ArHitResultList_getItem(ar_session_, hit_result_list, i, ar_hit);

            if (ar_hit == nullptr)
            {
                LOGE("HelloArApplication::OnTouched ArHitResultList_getItem error");
                return;
            }

            ArTrackable* ar_trackable = nullptr;
            ArHitResult_acquireTrackable(ar_session_, ar_hit, &ar_trackable);
            ArTrackableType ar_trackable_type = AR_TRACKABLE_NOT_VALID;
            ArTrackable_getType(ar_session_, ar_trackable, &ar_trackable_type);
            // Creates an anchor if a plane or an oriented point was hit.
            if (AR_TRACKABLE_PLANE == ar_trackable_type)
            {
                ArPose* hit_pose = nullptr;
                ArPose_create(ar_session_, nullptr, &hit_pose);
                ArHitResult_getHitPose(ar_session_, ar_hit, hit_pose);
                int32_t  in_polygon = 0;
                ArPlane* ar_plane   = ArAsPlane(ar_trackable);
                ArPlane_isPoseInPolygon(ar_session_, ar_plane, hit_pose, &in_polygon);

                // Use hit pose and camera pose to VERIFY_EXPR if hittest is from the
                // back of the plane, if it is, no need to create the anchor.
                ArPose* camera_pose = nullptr;
                ArPose_create(ar_session_, nullptr, &camera_pose);
                ArCamera* ar_camera;
                ArFrame_acquireCamera(ar_session_, ar_frame_, &ar_camera);
                ArCamera_getPose(ar_session_, ar_camera, camera_pose);
                ArCamera_release(ar_camera);
                float normal_distance_to_plane = util::CalculateDistanceToPlane(
                    *ar_session_, *hit_pose, *camera_pose);

                ArPose_destroy(hit_pose);
                ArPose_destroy(camera_pose);

                if (!in_polygon || normal_distance_to_plane < 0)
                {
                    continue;
                }

                ar_hit_result  = ar_hit;
                trackable_type = ar_trackable_type;
                break;
            }
            else if (AR_TRACKABLE_POINT == ar_trackable_type)
            {
                ArPoint*               ar_point = ArAsPoint(ar_trackable);
                ArPointOrientationMode mode;
                ArPoint_getOrientationMode(ar_session_, ar_point, &mode);
                if (AR_POINT_ORIENTATION_ESTIMATED_SURFACE_NORMAL == mode)
                {
                    ar_hit_result  = ar_hit;
                    trackable_type = ar_trackable_type;
                    break;
                }
            }
        }

        if (ar_hit_result)
        {
            // Note that the application is responsible for releasing the anchor
            // pointer after using it. Call ArAnchor_release(anchor) to release.
            ArAnchor* anchor = nullptr;
            if (ArHitResult_acquireNewAnchor(ar_session_, ar_hit_result, &anchor) !=
                AR_SUCCESS)
            {
                LOGE(
                    "HelloArApplication::OnTouched ArHitResult_acquireNewAnchor error");
                return;
            }

            ArTrackingState tracking_state = AR_TRACKING_STATE_STOPPED;
            ArAnchor_getTrackingState(ar_session_, anchor, &tracking_state);
            if (tracking_state != AR_TRACKING_STATE_TRACKING)
            {
                ArAnchor_release(anchor);
                return;
            }

            if (anchors_.size() >= kMaxNumberOfObjectsToRender)
            {
                ArAnchor_release(anchors_[0].anchor);
                anchors_.erase(anchors_.begin());
            }

            // Assign a color to the object for rendering based on the trackable type
            // this anchor attached to. For AR_TRACKABLE_POINT, it's blue color, and
            // for AR_TRACKABLE_PLANE, it's green color.
            ColoredAnchor colored_anchor;
            colored_anchor.anchor = anchor;
            switch (trackable_type)
            {
                case AR_TRACKABLE_POINT:
                    SetColor(66.0f, 133.0f, 244.0f, 255.0f, colored_anchor.color);
                    break;
                case AR_TRACKABLE_PLANE:
                    SetColor(139.0f, 195.0f, 74.0f, 255.0f, colored_anchor.color);
                    break;
                default:
                    SetColor(0.0f, 0.0f, 0.0f, 0.0f, colored_anchor.color);
                    break;
            }
            anchors_.push_back(colored_anchor);

            ArHitResult_destroy(ar_hit_result);
            ar_hit_result = nullptr;

            ArHitResultList_destroy(hit_result_list);
            hit_result_list = nullptr;
        }
    }
}

void HelloArApplication::SetColor(float r, float g, float b, float a, float* color4f)
{
    *(color4f)     = r;
    *(color4f + 1) = g;
    *(color4f + 2) = b;
    *(color4f + 3) = a;
}

} // namespace hello_ar
