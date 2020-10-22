// Copyright 2014 Intel Corporation All Rights Reserved
//
// Intel makes no representations about the suitability of this software for any purpose.  
// THIS SOFTWARE IS PROVIDED ""AS IS."" INTEL SPECIFICALLY DISCLAIMS ALL WARRANTIES,
// EXPRESS OR IMPLIED, AND ALL LIABILITY, INCLUDING CONSEQUENTIAL AND OTHER INDIRECT DAMAGES,
// FOR THE USE OF THIS SOFTWARE, INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PROPRIETARY
// RIGHTS, AND INCLUDING THE WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
// Intel does not assume any responsibility for any errors which may appear in this software
// nor any responsibility to update it.

#pragma once

#include <DirectXMath.h>
#include <interactioncontext.h>

class OrbitCamera
{
public:
    OrbitCamera();
    ~OrbitCamera();

    void View(DirectX::XMVECTOR center,
              float radius, float minRadius, float maxRadius,
              float longAngle, float latAngle);

    // Uses the provided fov for the larger dimension
    void Projection(float fov, float aspect);

    DirectX::XMVECTOR const& Eye() const { return mEye; }
    DirectX::XMMATRIX const& ViewProjection() const { return mViewProjection; }

    void AddPointer(UINT pointerId);
    void ProcessPointerFrames(UINT pointerId, const POINTER_INFO* pointerInfo);
    void ProcessInertia();
    void RemovePointer(UINT pointerId);

    void OrbitX(float angle);
    void OrbitY(float angle);
    void ZoomRadius(float delta);
    void ZoomRadiusScale(float delta);
    
private:
    void UpdateData();
    static VOID CALLBACK StaticInteractionOutputCallback(VOID *clientData, const INTERACTION_CONTEXT_OUTPUT *output);
    void InteractionOutputCallback(const INTERACTION_CONTEXT_OUTPUT *output);

    DirectX::XMVECTOR mCenter;
    DirectX::XMVECTOR mUp;
    float mMinRadius;
    float mMaxRadius;
    
    float mLatAngle;
    float mLongAngle;
    float mRadius;

    DirectX::XMVECTOR mEye;
    DirectX::XMMATRIX mView;
    DirectX::XMMATRIX mProjection;
    DirectX::XMMATRIX mViewProjection;

    HINTERACTIONCONTEXT mInteractionContext;
};
