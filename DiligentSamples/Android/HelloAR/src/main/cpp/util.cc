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

#include "util.h"

namespace hello_ar
{

namespace util
{

void GetTransformMatrixFromAnchor(const ArAnchor&     ar_anchor,
                                  ArSession*          ar_session,
                                  Diligent::float4x4* out_model_mat)
{
    if (out_model_mat == nullptr)
    {
        LOGE("util::GetTransformMatrixFromAnchor model_mat is null.");
        return;
    }
    util::ScopedArPose pose(ar_session);
    ArAnchor_getPose(ar_session, &ar_anchor, pose.GetArPose());
    ArPose_getMatrix(ar_session, pose.GetArPose(), out_model_mat->Data());
}

Diligent::float3 GetPlaneNormal(const ArSession& ar_session,
                                const ArPose&    plane_pose)
{
    float plane_pose_raw[7] = {0.f};
    ArPose_getPoseRaw(&ar_session, &plane_pose, plane_pose_raw);
    Diligent::Quaternion plane_quaternion //
        {
            plane_pose_raw[0],
            plane_pose_raw[1],
            plane_pose_raw[2],
            plane_pose_raw[3] //
        };
    // Get normal vector, normal is defined to be positive Y-position in local
    // frame.
    return plane_quaternion.RotateVector(Diligent::float3(0.f, 1.f, 0.f));
}

float CalculateDistanceToPlane(const ArSession& ar_session,
                               const ArPose&    plane_pose,
                               const ArPose&    camera_pose)
{
    float plane_pose_raw[7] = {0.f};
    ArPose_getPoseRaw(&ar_session, &plane_pose, plane_pose_raw);
    Diligent::float3 plane_position //
        {
            plane_pose_raw[4],
            plane_pose_raw[5],
            plane_pose_raw[6] //
        };
    Diligent::float3 normal = GetPlaneNormal(ar_session, plane_pose);

    float camera_pose_raw[7] = {0.f};
    ArPose_getPoseRaw(&ar_session, &camera_pose, camera_pose_raw);
    Diligent::float3 camera_P_plane //
        {
            camera_pose_raw[4] - plane_position.x,
            camera_pose_raw[5] - plane_position.y,
            camera_pose_raw[6] - plane_position.z //
        };
    return Diligent::dot(normal, camera_P_plane);
}

} // namespace util

} // namespace hello_ar
