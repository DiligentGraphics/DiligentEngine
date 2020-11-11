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

#ifndef C_ARCORE_HELLOE_AR_UTIL_H_
#define C_ARCORE_HELLOE_AR_UTIL_H_

#include <android/log.h>
#include "arcore_c_api.h"

#include "BasicMath.hpp"

#ifndef LOGI
#    define LOGI(...) \
        __android_log_print(ANDROID_LOG_INFO, "hello_ar_example_c", __VA_ARGS__)
#endif // LOGI

#ifndef LOGE
#    define LOGE(...) \
        __android_log_print(ANDROID_LOG_ERROR, "hello_ar_example_c", __VA_ARGS__)
#endif // LOGE

#ifndef CHECK
#    define CHECK(condition)                                                       \
        if (!(condition))                                                          \
        {                                                                          \
            LOGE("*** CHECK FAILED at %s:%d: %s", __FILE__, __LINE__, #condition); \
            abort();                                                               \
        }
#endif // CHECK

namespace hello_ar
{

// Utilities for C hello AR project.
namespace util
{

// Provides a scoped allocated instance of Anchor.
// Can be treated as an ArAnchor*.
class ScopedArPose
{
public:
    explicit ScopedArPose(const ArSession* session)
    {
        ArPose_create(session, nullptr, &pose_);
    }
    ~ScopedArPose() { ArPose_destroy(pose_); }
    ArPose* GetArPose() { return pose_; }
    // Delete copy constructors.
    ScopedArPose(const ScopedArPose&) = delete;
    void operator=(const ScopedArPose&) = delete;

private:
    ArPose* pose_;
};

// Get transformation matrix from ArAnchor.
void GetTransformMatrixFromAnchor(const ArAnchor&     ar_anchor,
                                  ArSession*          ar_session,
                                  Diligent::float4x4* out_model_mat);

// Get the plane's normal from center pose.
Diligent::float3 GetPlaneNormal(const ArSession& ar_session, const ArPose& plane_pose);

// Calculate the normal distance to plane from cameraPose, the given planePose
// should have y axis parallel to plane's normal, for example plane's center
// pose or hit test pose.
float CalculateDistanceToPlane(const ArSession& ar_session,
                               const ArPose&    plane_pose,
                               const ArPose&    camera_pose);
} // namespace util

} // namespace hello_ar

#endif // C_ARCORE_HELLOE_AR_UTIL_H_
