// Copyright 2014 Intel Corporation All Rights Reserved
//
// Intel makes no representations about the suitability of this software for any purpose.  
// THIS SOFTWARE IS PROVIDED ""AS IS."" INTEL SPECIFICALLY DISCLAIMS ALL WARRANTIES,
// EXPRESS OR IMPLIED, AND ALL LIABILITY, INCLUDING CONSEQUENTIAL AND OTHER INDIRECT DAMAGES,
// FOR THE USE OF THIS SOFTWARE, INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PROPRIETARY
// RIGHTS, AND INCLUDING THE WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
// Intel does not assume any responsibility for any errors which may appear in this software
// nor any responsibility to update it.

#include <algorithm>
#include <cmath>

#include "camera.h"
#include "util.h"

using namespace DirectX;


OrbitCamera::OrbitCamera()
{
    // Defaults
    mCenter = XMVectorSet(0, 0, 0, 0);
    mUp = XMVectorSet(0, 1, 0, 0);
    mRadius = 1.0f;
    mMinRadius = 1.0f;
    mMaxRadius = 1.0f;
    mLongAngle = 0.0f;
    mLatAngle = 0.0f;

    // Set up interaction context (i.e. touch input processing, etc)
    ThrowIfFailed(CreateInteractionContext(&mInteractionContext));
    ThrowIfFailed(SetPropertyInteractionContext(mInteractionContext, INTERACTION_CONTEXT_PROPERTY_FILTER_POINTERS, TRUE));

    {
        INTERACTION_CONTEXT_CONFIGURATION config[] = 
        {
            {INTERACTION_ID_MANIPULATION,
                INTERACTION_CONFIGURATION_FLAG_MANIPULATION |
                INTERACTION_CONFIGURATION_FLAG_MANIPULATION_TRANSLATION_X |
                INTERACTION_CONFIGURATION_FLAG_MANIPULATION_TRANSLATION_Y |
                INTERACTION_CONFIGURATION_FLAG_MANIPULATION_SCALING |
                INTERACTION_CONFIGURATION_FLAG_MANIPULATION_TRANSLATION_INERTIA |
                INTERACTION_CONFIGURATION_FLAG_MANIPULATION_SCALING_INERTIA |
                INTERACTION_CONFIGURATION_FLAG_MANIPULATION_MULTIPLE_FINGER_PANNING
            },
        };

        ThrowIfFailed(SetInteractionConfigurationInteractionContext(mInteractionContext, ARRAYSIZE(config), config));
    }

    ThrowIfFailed(RegisterOutputCallbackInteractionContext(mInteractionContext, OrbitCamera::StaticInteractionOutputCallback, this));
}


OrbitCamera::~OrbitCamera()
{
    DestroyInteractionContext(mInteractionContext);
}


void OrbitCamera::View(
    DirectX::XMVECTOR center, float radius, float minRadius, float maxRadius,
    float longAngle, float latAngle)
{
    mCenter = center;
    mRadius = radius;
    mMinRadius = minRadius;
    mMaxRadius = maxRadius;
    mLongAngle = longAngle;
    mLatAngle = latAngle;
    UpdateData();
}


void OrbitCamera::Projection(float fov, float aspect)
{
    float fovY = (aspect <= 1.0 ? fov : fov / aspect);
    mProjection = XMMatrixPerspectiveFovRH(fovY, aspect, 10000.0f, 0.1f);
    UpdateData();
}


void OrbitCamera::UpdateData()
{
    mEye = XMVectorSet(
        mRadius * std::sin(mLatAngle) * std::cos(mLongAngle),
        mRadius * std::cos(mLatAngle),
        mRadius * std::sin(mLatAngle) * std::sin(mLongAngle),
        0.0f);

    mView = XMMatrixLookAtRH(mEye, mCenter, mUp);
    mViewProjection = XMMatrixMultiply(mView, mProjection);
}


void OrbitCamera::OrbitX(float angle)
{
    mLongAngle += angle;
    UpdateData();
}


void OrbitCamera::OrbitY(float angle)
{
    float limit = XM_PI * 0.01f;
    mLatAngle = std::max(limit, std::min(XM_PI-limit, mLatAngle + angle));
    UpdateData();
}


void OrbitCamera::ZoomRadius(float delta)
{
    mRadius = std::max(mMinRadius, std::min(mMaxRadius, mRadius + delta));
    UpdateData();
}

void OrbitCamera::ZoomRadiusScale(float delta)
{
    mRadius = std::max(mMinRadius, std::min(mMaxRadius, mRadius * delta));
    UpdateData();
}


void OrbitCamera::AddPointer(UINT pointerId)
{
    AddPointerInteractionContext(mInteractionContext, pointerId);
}


void OrbitCamera::ProcessPointerFrames(UINT pointerId, const POINTER_INFO* pointerInfo)
{
    ProcessPointerFramesInteractionContext(mInteractionContext, 1, 1, pointerInfo);
}


void OrbitCamera::RemovePointer(UINT pointerId)
{
    RemovePointerInteractionContext(mInteractionContext, pointerId);
}


void OrbitCamera::ProcessInertia()
{
    ProcessInertiaInteractionContext(mInteractionContext);
}


VOID CALLBACK OrbitCamera::StaticInteractionOutputCallback(VOID *clientData, const INTERACTION_CONTEXT_OUTPUT *output)
{
    auto camera = reinterpret_cast<OrbitCamera*>(clientData);
    camera->InteractionOutputCallback(output);
}


void OrbitCamera::InteractionOutputCallback(const INTERACTION_CONTEXT_OUTPUT *output)
{
    switch(output->interactionId)
    {
    case INTERACTION_ID_MANIPULATION: {
        auto delta = &output->arguments.manipulation.delta;
        OrbitX(delta->translationX * 0.0007f);
        OrbitY(-delta->translationY * 0.0007f);
        ZoomRadiusScale(1.0f / delta->scale);

        break;
    }

    default:
        break;
    }
}
