/*
 *  Copyright 2019-2020 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *  
 *      http://www.apache.org/licenses/LICENSE-2.0
 *  
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
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

#pragma once

#include <float.h>

#include "../../Platforms/interface/PlatformDefinitions.h"
#include "../../Primitives/interface/FlagEnum.h"

#include "BasicMath.hpp"

namespace Diligent
{

// Structure describing a plane
struct Plane3D
{
    float3 Normal;
    float  Distance = 0; //Distance from the coordinate system origin to the plane along the normal direction

    operator float4&()
    {
        return *reinterpret_cast<float4*>(this);
    }
    operator const float4&() const
    {
        return *reinterpret_cast<const float4*>(this);
    }
};

struct ViewFrustum
{
    enum PLANE_IDX : Uint32
    {
        LEFT_PLANE_IDX   = 0,
        RIGHT_PLANE_IDX  = 1,
        BOTTOM_PLANE_IDX = 2,
        TOP_PLANE_IDX    = 3,
        NEAR_PLANE_IDX   = 4,
        FAR_PLANE_IDX    = 5,
        NUM_PLANES       = 6
    };

    Plane3D LeftPlane;
    Plane3D RightPlane;
    Plane3D BottomPlane;
    Plane3D TopPlane;
    Plane3D NearPlane;
    Plane3D FarPlane;

    const Plane3D& GetPlane(PLANE_IDX Idx) const
    {
        VERIFY_EXPR(Idx < NUM_PLANES);
        const Plane3D* Planes = reinterpret_cast<const Plane3D*>(this);
        return Planes[static_cast<size_t>(Idx)];
    }
};

struct ViewFrustumExt : public ViewFrustum
{
    float3 FrustumCorners[8];
};

// For OpenGL, matrix is still considered row-major. The only difference is that
// near clip plane is at -1, not 0.
//
// Note that returned plane normal vectors are not normalized, which does not make a difference
// when testing a point against the plane:
//
//      IsPointInsidePlane = dot(Plane.Normal, Point) < Plane.Distance
//
// However, to use the planes with other distances (e.g. for testing a sphere against the plane),
// the normal vectors must be normalized and the distances scaled accordingly.
inline void ExtractViewFrustumPlanesFromMatrix(const float4x4& Matrix, ViewFrustum& Frustum, bool bIsOpenGL)
{
    // For more details, see Gribb G., Hartmann K., "Fast Extraction of Viewing Frustum Planes from the
    // World-View-Projection Matrix" (the paper is available at
    // http://gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf)

    // Left clipping plane
    Frustum.LeftPlane.Normal.x = Matrix._14 + Matrix._11;
    Frustum.LeftPlane.Normal.y = Matrix._24 + Matrix._21;
    Frustum.LeftPlane.Normal.z = Matrix._34 + Matrix._31;
    Frustum.LeftPlane.Distance = Matrix._44 + Matrix._41;

    // Right clipping plane
    Frustum.RightPlane.Normal.x = Matrix._14 - Matrix._11;
    Frustum.RightPlane.Normal.y = Matrix._24 - Matrix._21;
    Frustum.RightPlane.Normal.z = Matrix._34 - Matrix._31;
    Frustum.RightPlane.Distance = Matrix._44 - Matrix._41;

    // Top clipping plane
    Frustum.TopPlane.Normal.x = Matrix._14 - Matrix._12;
    Frustum.TopPlane.Normal.y = Matrix._24 - Matrix._22;
    Frustum.TopPlane.Normal.z = Matrix._34 - Matrix._32;
    Frustum.TopPlane.Distance = Matrix._44 - Matrix._42;

    // Bottom clipping plane
    Frustum.BottomPlane.Normal.x = Matrix._14 + Matrix._12;
    Frustum.BottomPlane.Normal.y = Matrix._24 + Matrix._22;
    Frustum.BottomPlane.Normal.z = Matrix._34 + Matrix._32;
    Frustum.BottomPlane.Distance = Matrix._44 + Matrix._42;

    // Near clipping plane
    if (bIsOpenGL)
    {
        // -w <= z <= w
        Frustum.NearPlane.Normal.x = Matrix._14 + Matrix._13;
        Frustum.NearPlane.Normal.y = Matrix._24 + Matrix._23;
        Frustum.NearPlane.Normal.z = Matrix._34 + Matrix._33;
        Frustum.NearPlane.Distance = Matrix._44 + Matrix._43;
    }
    else
    {
        // 0 <= z <= w
        Frustum.NearPlane.Normal.x = Matrix._13;
        Frustum.NearPlane.Normal.y = Matrix._23;
        Frustum.NearPlane.Normal.z = Matrix._33;
        Frustum.NearPlane.Distance = Matrix._43;
    }

    // Far clipping plane
    Frustum.FarPlane.Normal.x = Matrix._14 - Matrix._13;
    Frustum.FarPlane.Normal.y = Matrix._24 - Matrix._23;
    Frustum.FarPlane.Normal.z = Matrix._34 - Matrix._33;
    Frustum.FarPlane.Distance = Matrix._44 - Matrix._43;
}

inline void ExtractViewFrustumPlanesFromMatrix(const float4x4& Matrix, ViewFrustumExt& FrustumExt, bool bIsOpenGL)
{
    ExtractViewFrustumPlanesFromMatrix(Matrix, static_cast<ViewFrustum&>(FrustumExt), bIsOpenGL);

    // Compute frustum corners
    float4x4 InvMatrix = Matrix.Inverse();

    float nearClipZ = bIsOpenGL ? -1.f : 0.f;

    static const float3 ProjSpaceCorners[] =
        {
            // clang-format off
            float3(-1, -1, nearClipZ),
            float3( 1, -1, nearClipZ),
            float3(-1,  1, nearClipZ),
            float3( 1,  1, nearClipZ),

            float3(-1, -1, 1),
            float3( 1, -1, 1),
            float3(-1,  1, 1),
            float3( 1,  1, 1),
            // clang-format on
        };

    for (int i = 0; i < 8; ++i)
        FrustumExt.FrustumCorners[i] = ProjSpaceCorners[i] * InvMatrix;
}

struct BoundBox
{
    float3 Min;
    float3 Max;

    // Computes new bounding box by applying transform matrix m to the box
    BoundBox Transform(const float4x4& m) const
    {
        BoundBox NewBB;
        NewBB.Min = float3::MakeVector(m[3]);
        NewBB.Max = NewBB.Min;
        float3 v0, v1;

        float3 right = float3::MakeVector(m[0]);

        v0 = right * Min.x;
        v1 = right * Max.x;
        NewBB.Min += std::min(v0, v1);
        NewBB.Max += std::max(v0, v1);

        float3 up = float3::MakeVector(m[1]);

        v0 = up * Min.y;
        v1 = up * Max.y;
        NewBB.Min += std::min(v0, v1);
        NewBB.Max += std::max(v0, v1);

        float3 back = float3::MakeVector(m[2]);

        v0 = back * Min.z;
        v1 = back * Max.z;
        NewBB.Min += std::min(v0, v1);
        NewBB.Max += std::max(v0, v1);

        return NewBB;
    }
};

enum class BoxVisibility
{
    //  Bounding box is guaranteed to be outside of the view frustum
    //                 .
    //             . ' |
    //         . '     |
    //       |         |
    //         .       |
    //       ___ ' .   |
    //      |   |    ' .
    //      |___|
    //
    Invisible,

    //  Bounding box intersects the frustum
    //                 .
    //             . ' |
    //         . '     |
    //       |         |
    //        _.__     |
    //       |   '|.   |
    //       |____|  ' .
    //
    Intersecting,

    //  Bounding box is fully inside the view frustum
    //                 .
    //             . ' |
    //         . '___  |
    //       |   |   | |
    //         . |___| |
    //           ' .   |
    //               ' .
    //
    FullyVisible
};

/// Returns the nearest bounding box corner along the given direction
inline float3 GetBoxNearestCorner(const float3& Direction, const BoundBox& Box)
{
    return float3 //
        {
            (Direction.x > 0) ? Box.Min.x : Box.Max.x,
            (Direction.y > 0) ? Box.Min.y : Box.Max.y,
            (Direction.z > 0) ? Box.Min.z : Box.Max.z //
        };
}

/// Returns the farthest bounding box corner along the given direction
inline float3 GetBoxFarthestCorner(const float3& Direction, const BoundBox& Box)
{
    return float3 //
        {
            (Direction.x > 0) ? Box.Max.x : Box.Min.x,
            (Direction.y > 0) ? Box.Max.y : Box.Min.y,
            (Direction.z > 0) ? Box.Max.z : Box.Min.z //
        };
}

inline BoxVisibility GetBoxVisibilityAgainstPlane(const Plane3D& Plane, const BoundBox& Box)
{
    const float3& Normal = Plane.Normal;

    float3 MaxPoint //
        {
            (Normal.x > 0) ? Box.Max.x : Box.Min.x,
            (Normal.y > 0) ? Box.Max.y : Box.Min.y,
            (Normal.z > 0) ? Box.Max.z : Box.Min.z //
        };
    float DMax = dot(MaxPoint, Normal) + Plane.Distance;
    if (DMax < 0)
        return BoxVisibility::Invisible;

    float3 MinPoint //
        {
            (Normal.x > 0) ? Box.Min.x : Box.Max.x,
            (Normal.y > 0) ? Box.Min.y : Box.Max.y,
            (Normal.z > 0) ? Box.Min.z : Box.Max.z //
        };
    float DMin = dot(MinPoint, Normal) + Plane.Distance;
    if (DMin > 0)
        return BoxVisibility::FullyVisible;

    return BoxVisibility::Intersecting;
}

// Flags must be listed in the same order as planes in the ViewFrustum struct:
// LeftPlane, RightPlane, BottomPlane, TopPlane, NearPlane, FarPlane
enum FRUSTUM_PLANE_FLAGS : Uint32
{
    FRUSTUM_PLANE_FLAG_NONE         = 0x00,
    FRUSTUM_PLANE_FLAG_LEFT_PLANE   = 1 << ViewFrustum::LEFT_PLANE_IDX,
    FRUSTUM_PLANE_FLAG_RIGHT_PLANE  = 1 << ViewFrustum::RIGHT_PLANE_IDX,
    FRUSTUM_PLANE_FLAG_BOTTOM_PLANE = 1 << ViewFrustum::BOTTOM_PLANE_IDX,
    FRUSTUM_PLANE_FLAG_TOP_PLANE    = 1 << ViewFrustum::TOP_PLANE_IDX,
    FRUSTUM_PLANE_FLAG_NEAR_PLANE   = 1 << ViewFrustum::NEAR_PLANE_IDX,
    FRUSTUM_PLANE_FLAG_FAR_PLANE    = 1 << ViewFrustum::FAR_PLANE_IDX,

    FRUSTUM_PLANE_FLAG_FULL_FRUSTUM = FRUSTUM_PLANE_FLAG_LEFT_PLANE |
        FRUSTUM_PLANE_FLAG_RIGHT_PLANE |
        FRUSTUM_PLANE_FLAG_BOTTOM_PLANE |
        FRUSTUM_PLANE_FLAG_TOP_PLANE |
        FRUSTUM_PLANE_FLAG_NEAR_PLANE |
        FRUSTUM_PLANE_FLAG_FAR_PLANE,

    FRUSTUM_PLANE_FLAG_OPEN_NEAR = FRUSTUM_PLANE_FLAG_LEFT_PLANE |
        FRUSTUM_PLANE_FLAG_RIGHT_PLANE |
        FRUSTUM_PLANE_FLAG_BOTTOM_PLANE |
        FRUSTUM_PLANE_FLAG_TOP_PLANE |
        FRUSTUM_PLANE_FLAG_FAR_PLANE
};
DEFINE_FLAG_ENUM_OPERATORS(FRUSTUM_PLANE_FLAGS);

// Tests if bounding box is visible by the camera
inline BoxVisibility GetBoxVisibility(const ViewFrustum&  ViewFrustum,
                                      const BoundBox&     Box,
                                      FRUSTUM_PLANE_FLAGS PlaneFlags = FRUSTUM_PLANE_FLAG_FULL_FRUSTUM)
{
    int NumPlanesInside = 0;
    int TotalPlanes     = 0;
    for (Uint32 plane_idx = 0; plane_idx < ViewFrustum::NUM_PLANES; ++plane_idx)
    {
        if ((PlaneFlags & (1 << plane_idx)) == 0)
            continue;

        const Plane3D& CurrPlane = ViewFrustum.GetPlane(static_cast<ViewFrustum::PLANE_IDX>(plane_idx));

        auto VisibilityAgainstPlane = GetBoxVisibilityAgainstPlane(CurrPlane, Box);

        // If bounding box is "behind" one of the planes, it is definitely invisible
        if (VisibilityAgainstPlane == BoxVisibility::Invisible)
            return BoxVisibility::Invisible;

        // Count total number of planes the bound box is inside
        if (VisibilityAgainstPlane == BoxVisibility::FullyVisible)
            ++NumPlanesInside;

        ++TotalPlanes;
    }

    return (NumPlanesInside == TotalPlanes) ? BoxVisibility::FullyVisible : BoxVisibility::Intersecting;
}

inline BoxVisibility GetBoxVisibility(const ViewFrustumExt& ViewFrustumExt,
                                      const BoundBox&       Box,
                                      FRUSTUM_PLANE_FLAGS   PlaneFlags = FRUSTUM_PLANE_FLAG_FULL_FRUSTUM)
{
    auto Visibility = GetBoxVisibility(static_cast<const ViewFrustum&>(ViewFrustumExt), Box, PlaneFlags);
    if (Visibility == BoxVisibility::FullyVisible || Visibility == BoxVisibility::Invisible)
        return Visibility;

    if ((PlaneFlags & FRUSTUM_PLANE_FLAG_FULL_FRUSTUM) == FRUSTUM_PLANE_FLAG_FULL_FRUSTUM)
    {
        // Additionally test if the whole frustum is outside one of
        // the the bounding box planes. This helps in the following situation:
        //
        //
        //       .
        //      /   '  .       .
        //     / AABB  /   . ' |
        //    /       /. '     |
        //       ' . / |       |
        //       * .   |       |
        //           ' .       |
        //               ' .   |
        //                   ' .

        // Test all frustum corners against every bound box plane
        for (int iBoundBoxPlane = 0; iBoundBoxPlane < 6; ++iBoundBoxPlane)
        {
            // struct BoundBox
            // {
            //     float3 Min;
            //     float3 Max;
            // };
            float CurrPlaneCoord = reinterpret_cast<const float*>(&Box)[iBoundBoxPlane];
            // Bound box normal is one of the axis, so we just need to pick the right coordinate
            int iCoordOrder = iBoundBoxPlane % 3; // 0, 1, 2, 0, 1, 2
            // Since plane normal is directed along one of the axis, we only need to select
            // if it is pointing in the positive (max planes) or negative (min planes) direction
            float fSign              = (iBoundBoxPlane >= 3) ? +1.f : -1.f;
            bool  bAllCornersOutside = true;
            for (int iCorner = 0; iCorner < 8; iCorner++)
            {
                // Pick the frustum corner coordinate
                float CurrCornerCoord = ViewFrustumExt.FrustumCorners[iCorner][iCoordOrder];
                // Dot product is simply the coordinate difference multiplied by the sign
                if (fSign * (CurrPlaneCoord - CurrCornerCoord) > 0)
                {
                    bAllCornersOutside = false;
                    break;
                }
            }
            if (bAllCornersOutside)
                return BoxVisibility::Invisible;
        }
    }

    return BoxVisibility::Intersecting;
}

inline float GetPointToBoxDistance(const BoundBox& BndBox, const float3& Pos)
{
    VERIFY_EXPR(BndBox.Max.x >= BndBox.Min.x &&
                BndBox.Max.y >= BndBox.Min.y &&
                BndBox.Max.z >= BndBox.Min.z);
    float fdX = (Pos.x > BndBox.Max.x) ? (Pos.x - BndBox.Max.x) : ((Pos.x < BndBox.Min.x) ? (BndBox.Min.x - Pos.x) : 0.f);
    float fdY = (Pos.y > BndBox.Max.y) ? (Pos.y - BndBox.Max.y) : ((Pos.y < BndBox.Min.y) ? (BndBox.Min.y - Pos.y) : 0.f);
    float fdZ = (Pos.z > BndBox.Max.z) ? (Pos.z - BndBox.Max.z) : ((Pos.z < BndBox.Min.z) ? (BndBox.Min.z - Pos.z) : 0.f);
    VERIFY_EXPR(fdX >= 0 && fdY >= 0 && fdZ >= 0);

    float3 RangeVec(fdX, fdY, fdZ);
    return length(RangeVec);
}

inline bool operator==(const Plane3D& p1, const Plane3D& p2)
{
    return p1.Normal == p2.Normal &&
        p1.Distance == p2.Distance;
}

inline bool operator==(const ViewFrustum& f1, const ViewFrustum& f2)
{
    // clang-format off
    return f1.LeftPlane   == f2.LeftPlane   &&
           f1.RightPlane  == f2.RightPlane  &&
           f1.BottomPlane == f2.BottomPlane &&
           f1.TopPlane    == f2.TopPlane    &&
           f1.NearPlane   == f2.NearPlane   &&
           f1.FarPlane    == f2.FarPlane;
    // clang-format on
}

inline bool operator==(const ViewFrustumExt& f1, const ViewFrustumExt& f2)
{
    if (!(static_cast<const ViewFrustum&>(f1) == static_cast<const ViewFrustum&>(f2)))
        return false;

    for (int c = 0; c < _countof(f1.FrustumCorners); ++c)
        if (f1.FrustumCorners[c] != f2.FrustumCorners[c])
            return false;

    return true;
}

template <typename T, typename Y>
T HermiteSpline(T f0, // F(0)
                T f1, // F(1)
                T t0, // F'(0)
                T t1, // F'(1)
                Y x)
{
    // https://en.wikipedia.org/wiki/Cubic_Hermite_spline
    auto x2 = x * x;
    auto x3 = x2 * x;
    return (2 * x3 - 3 * x2 + 1) * f0 + (x3 - 2 * x2 + x) * t0 + (-2 * x3 + 3 * x2) * f1 + (x3 - x2) * t1;
}

// Retuns the minimum bounding sphere of a view frustum
inline void GetFrustumMinimumBoundingSphere(float   Proj_00,   // cot(HorzFOV / 2)
                                            float   Proj_11,   // cot(VertFOV / 2) == proj_00 / AspectRatio
                                            float   NearPlane, // Near clip plane
                                            float   FarPlane,  // Far clip plane
                                            float3& Center,    // Sphere center == (0, 0, c)
                                            float&  Radius     // Sphere radius
)
{
    // https://lxjk.github.io/2017/04/15/Calculate-Minimal-Bounding-Sphere-of-Frustum.html
    VERIFY_EXPR(FarPlane >= NearPlane);
    auto k2 = 1.f / (Proj_00 * Proj_00) + 1.f / (Proj_11 * Proj_11);
    if (k2 > (FarPlane - NearPlane) / (FarPlane + NearPlane))
    {
        Center = float3(0, 0, FarPlane);
        Radius = FarPlane * std::sqrt(k2);
    }
    else
    {
        Center = float3(0, 0, 0.5f * (FarPlane + NearPlane) * (1 + k2));
        Radius = 0.5f * std::sqrt((FarPlane - NearPlane) * (FarPlane - NearPlane) + 2 * (FarPlane * FarPlane + NearPlane * NearPlane) * k2 + (FarPlane + NearPlane) * (FarPlane + NearPlane) * k2 * k2);
    }
}

/// Intersects a ray with 3D box and computes distances to intersections
inline bool IntersectRayBox3D(const float3& RayOrigin,
                              const float3& RayDirection,
                              float3        BoxMin,
                              float3        BoxMax,
                              float&        EnterDist,
                              float&        ExitDist)
{
    VERIFY_EXPR(RayDirection != float3(0, 0, 0));

    BoxMin -= RayOrigin;
    BoxMax -= RayOrigin;

    static constexpr float Epsilon = 1e-20f;

    float3 AbsRayDir = abs(RayDirection);
    float3 t_min //
        {
            AbsRayDir.x > Epsilon ? BoxMin.x / RayDirection.x : +FLT_MAX,
            AbsRayDir.y > Epsilon ? BoxMin.y / RayDirection.y : +FLT_MAX,
            AbsRayDir.z > Epsilon ? BoxMin.z / RayDirection.z : +FLT_MAX //
        };
    float3 t_max //
        {
            AbsRayDir.x > Epsilon ? BoxMax.x / RayDirection.x : -FLT_MAX,
            AbsRayDir.y > Epsilon ? BoxMax.y / RayDirection.y : -FLT_MAX,
            AbsRayDir.z > Epsilon ? BoxMax.z / RayDirection.z : -FLT_MAX //
        };

    EnterDist = max3(std::min(t_min.x, t_max.x), std::min(t_min.y, t_max.y), std::min(t_min.z, t_max.z));
    ExitDist  = min3(std::max(t_min.x, t_max.x), std::max(t_min.y, t_max.y), std::max(t_min.z, t_max.z));

    // if ExitDist < 0, the ray intersects AABB, but the whole AABB is behind it
    // if EnterDist > ExitDist, the ray doesn't intersect AABB
    return ExitDist >= 0 && EnterDist <= ExitDist;
}

/// Intersects a ray with the axis-aligned bounding box and computes distances to intersections
inline bool IntersectRayAABB(const float3&   RayOrigin,
                             const float3&   RayDirection,
                             const BoundBox& AABB,
                             float&          EnterDist,
                             float&          ExitDist)
{
    return IntersectRayBox3D(RayOrigin, RayDirection, AABB.Min, AABB.Max, EnterDist, ExitDist);
}

/// Intersects a 2D ray with the 2D axis-aligned bounding box and computes distances to intersections
inline bool IntersectRayBox2D(const float2& RayOrigin,
                              const float2& RayDirection,
                              float2        BoxMin,
                              float2        BoxMax,
                              float&        EnterDist,
                              float&        ExitDist)
{
    VERIFY_EXPR(RayDirection != float2(0, 0));

    BoxMin -= RayOrigin;
    BoxMax -= RayOrigin;

    static constexpr float Epsilon = 1e-20f;

    float2 AbsRayDir = abs(RayDirection);
    float2 t_min //
        {
            AbsRayDir.x > Epsilon ? BoxMin.x / RayDirection.x : +FLT_MAX,
            AbsRayDir.y > Epsilon ? BoxMin.y / RayDirection.y : +FLT_MAX //
        };
    float2 t_max //
        {
            AbsRayDir.x > Epsilon ? BoxMax.x / RayDirection.x : -FLT_MAX,
            AbsRayDir.y > Epsilon ? BoxMax.y / RayDirection.y : -FLT_MAX //
        };

    EnterDist = std::max(std::min(t_min.x, t_max.x), std::min(t_min.y, t_max.y));
    ExitDist  = std::min(std::max(t_min.x, t_max.x), std::max(t_min.y, t_max.y));

    // if ExitDist < 0, the ray intersects AABB, but the whole AABB is behind it
    // if EnterDist > ExitDist, the ray doesn't intersect AABB
    return ExitDist >= 0 && EnterDist <= ExitDist;
}


/// Intersects a ray with the trianlge using Moller-Trumbore algorithm and returns
/// the distance along the ray to the intesrsection point.
/// If the intersection point is behind the ray origin, the distance will be negative.
/// If there is no intersection, returns +FLT_MAX.
inline float IntersectRayTriangle(const float3& V0,
                                  const float3& V1,
                                  const float3& V2,
                                  const float3& RayOrigin,
                                  const float3& RayDirection,
                                  bool          CullBackFace = false)
{
    float3 V0_V1 = V1 - V0;
    float3 V0_V2 = V2 - V0;

    float3 PVec = cross(RayDirection, V0_V2);

    float Det = dot(V0_V1, PVec);

    float t = +FLT_MAX;

    static constexpr float Epsilon = 1e-10f;
    // If determinant is near zero, the ray lies in the triangle plane
    if (Det > Epsilon || (!CullBackFace && Det < -Epsilon))
    {
        float3 V0_RO = RayOrigin - V0;

        // calculate U parameter and test bounds
        float u = dot(V0_RO, PVec) / Det;
        if (u >= 0 && u <= 1)
        {
            float3 QVec = cross(V0_RO, V0_V1);

            // calculate V parameter and test bounds
            float v = dot(RayDirection, QVec) / Det;
            if (v >= 0 && u + v <= 1)
            {
                // calculate t, ray intersects triangle
                t = dot(V0_V2, QVec) / Det;
            }
        }
    }

    return t;
}


/// Traces a 2D line through the square cell grid and enumerates all cells the line touches.

/// \tparam TCallback - Type of the callback function.
/// \param f2Start    - Line start point.
/// \param f2End      - Line end point.
/// \param i2GridSize - Grid dimensions.
/// \param Callback   - Callback function that will be caled with the argument of type int2
///                     for every cell visited. The function should return true to continue
///                     tracing and false to stop it.
///
/// \remarks The algorithm clips the line against the grid boundaries [0 .. i2GridSize.x] x [0 .. i2GridSize.y]
///
///          When one of the end points falls exactly on a vertical cell boundary, cell to the right is enumerated.
///          When one of the end points falls exactly on a horizontal cell boundary, cell above is enumerated.
///
/// For example, for the line below on a 2x2 grid, the algorithm will trace the following cells: (0,0), (0,1), (1,1)
///
///                End
///                /
///   __________ _/________  2
///  |          |/         |
///  |          /          |
///  |         /|          |
///  |________/_|__________| 1
///  |       /  |          |
///  |      /   |          |
///  |    Start |          |
///  |__________|__________| 0
/// 0           1          2
///
template <typename TCallback>
void TraceLineThroughGrid(float2    f2Start,
                          float2    f2End,
                          int2      i2GridSize,
                          TCallback Callback)
{
    VERIFY_EXPR(i2GridSize.x > 0 && i2GridSize.y > 0);
    const auto f2GridSize = i2GridSize.Recast<float>();

    if (f2Start == f2End)
    {
        if (f2Start.x >= 0 && f2Start.x < f2GridSize.x &&
            f2Start.y >= 0 && f2Start.y < f2GridSize.y)
        {
            Callback(f2Start.Recast<int>());
        }
        return;
    }

    float2 f2Direction = f2End - f2Start;

    float EnterDist, ExitDist;
    if (IntersectRayBox2D(f2Start, f2Direction, float2{0, 0}, f2GridSize, EnterDist, ExitDist))
    {
        f2End   = f2Start + f2Direction * std::min(ExitDist, 1.f);
        f2Start = f2Start + f2Direction * std::max(EnterDist, 0.f);
        // Clamp start and end points to avoid FP precision issues
        f2Start = clamp(f2Start, float2{0, 0}, f2GridSize);
        f2End   = clamp(f2End, float2{0, 0}, f2GridSize);

        const int   dh = f2Direction.x > 0 ? 1 : -1;
        const int   dv = f2Direction.y > 0 ? 1 : -1;
        const float p  = f2Direction.y * f2Start.x - f2Direction.x * f2Start.y;
        const float tx = p - f2Direction.y * static_cast<float>(dh);
        const float ty = p + f2Direction.x * static_cast<float>(dv);

        const int2 i2End = f2End.Recast<int>();
        VERIFY_EXPR(i2End.x >= 0 && i2End.y >= 0 && i2End.x <= i2GridSize.x && i2End.y <= i2GridSize.y);

        int2 i2Pos = f2Start.Recast<int>();
        VERIFY_EXPR(i2Pos.x >= 0 && i2Pos.y >= 0 && i2Pos.x <= i2GridSize.x && i2Pos.y <= i2GridSize.y);

        // Loop condition checks if we missed the end point of the line due to
        // floating point precision issues.
        // Normally we exit the loop when i2Pos == i2End.
        while ((i2End.x - i2Pos.x) * dh >= 0 &&
               (i2End.y - i2Pos.y) * dv >= 0)
        {
            if (i2Pos.x < i2GridSize.x && i2Pos.y < i2GridSize.y)
            {
                if (!Callback(i2Pos))
                    break;
            }

            if (i2Pos == i2End)
            {
                // End of the line
                break;
            }
            else
            {
                // step to the next cell
                float t = f2Direction.x * (static_cast<float>(i2Pos.y) + 0.5f) - f2Direction.y * (static_cast<float>(i2Pos.x) + 0.5f);
                if (std::abs(t + tx) < std::abs(t + ty))
                    i2Pos.x += dh;
                else
                    i2Pos.y += dv;
            }
        }
    }
}


/// Tests if a point is inside triangle.

/// \tparam T                - Vector component type
/// \tparam IntermediateType - Intermediate type used in calculations
///
/// \param [in] V0         - First triangle vertex
/// \param [in] V1         - Second triangle vertex
/// \param [in] V2         - Third triangle vertex
/// \param [in] Point      - Point to test
/// \param [in] AllowEdges - Whether to accept points lying on triangle edges
/// \return     true if the point lies inside the triangle,
///             and false otherwise.
template <typename T, typename IntermediateType>
bool IsPointInsideTriangle(const Vector2<T>& V0,
                           const Vector2<T>& V1,
                           const Vector2<T>& V2,
                           const Vector2<T>& Point,
                           bool              AllowEdges)
{
    using DirType = Vector2<IntermediateType>;

    const DirType Rib[3] = //
        {
            DirType //
            {
                IntermediateType{V1.x} - IntermediateType{V0.x},
                IntermediateType{V1.y} - IntermediateType{V0.y},
            },
            DirType //
            {
                IntermediateType{V2.x} - IntermediateType{V1.x},
                IntermediateType{V2.y} - IntermediateType{V1.y},
            },
            DirType //
            {
                IntermediateType{V0.x} - IntermediateType{V2.x},
                IntermediateType{V0.y} - IntermediateType{V2.y},
            } //
        };

    const DirType VertToPoint[3] = //
        {
            DirType //
            {
                IntermediateType{Point.x} - IntermediateType{V0.x},
                IntermediateType{Point.y} - IntermediateType{V0.y} //
            },
            DirType //
            {
                IntermediateType{Point.x} - IntermediateType{V1.x},
                IntermediateType{Point.y} - IntermediateType{V1.y} //
            },
            DirType //
            {
                IntermediateType{Point.x} - IntermediateType{V2.x},
                IntermediateType{Point.y} - IntermediateType{V2.y} //
            }                                                      //
        };

    IntermediateType NormalZ[3];
    for (int i = 0; i < 3; i++)
    {
        NormalZ[i] = Rib[i].x * VertToPoint[i].y - Rib[i].y * VertToPoint[i].x;
    }

    // clang-format off
    if (AllowEdges)
    {
        return (NormalZ[0] >= 0 && NormalZ[1] >= 0 && NormalZ[2] >= 0) ||
               (NormalZ[0] <= 0 && NormalZ[1] <= 0 && NormalZ[2] <= 0);
    }
    else
    {
        return (NormalZ[0] > 0 && NormalZ[1] > 0 && NormalZ[2] > 0) ||
               (NormalZ[0] < 0 && NormalZ[1] < 0 && NormalZ[2] < 0);
    }
    // clang-format on
}

template <typename T>
bool IsPointInsideTriangle(const Vector2<T>& V0,
                           const Vector2<T>& V1,
                           const Vector2<T>& V2,
                           const Vector2<T>& Point,
                           bool              AllowEdges)
{
    return IsPointInsideTriangle<T, T>(V0, V1, V2, Point, AllowEdges);
}


/// Rasterizes a triangle and calls the callback function for every sample covered.

/// The samples are assumed to be located at integer coordinates. Samples located on
/// edges are always enumerated. Samples are enumerated row by row, bottom to top,
/// left to right. For example, for triangle (1, 1)-(1, 3)-(3, 1),
/// the following locations will be enumerated:
/// (1, 1), (2, 1), (3, 1), (1, 2), (2, 2), (1, 3).
///
///  3 *   *.  *   *
///        | '.
///  2 *   *   *.  *
///        |     '.
///  1 *   *---*---*
///
///  0 *   *   *   *
///    0   1   2   3
///
/// \tparam [in] T    - Vertex component type.
/// \tparam TCallback - Type of the callback function.
///
/// \param [in] V0         - First triangle vertex.
/// \param [in] V1         - Second triangle vertex.
/// \param [in] V2         - Third triangle vertex.
/// \param [in] Callback   - Callback function that will be caled with the argument of type int2
///                          for every sample covered.
template <typename T,
          class TCallback>
void RasterizeTriangle(Vector2<T> V0,
                       Vector2<T> V1,
                       Vector2<T> V2,
                       TCallback  Callback)
{
    if (V1.y < V0.y)
        std::swap(V1, V0);
    if (V2.y < V0.y)
        std::swap(V2, V0);
    if (V2.y < V1.y)
        std::swap(V2, V1);

    VERIFY_EXPR(V0.y <= V1.y && V1.y <= V2.y);

    const int iStartRow = static_cast<int>(FastCeil(V0.y));
    const int iEndRow   = static_cast<int>(FastFloor(V2.y));

    if (iStartRow == iEndRow)
    {
        auto iStartCol = static_cast<int>(FastCeil(min3(V0.x, V1.x, V2.x)));
        auto iEndCol   = static_cast<int>(FastFloor(max3(V0.x, V1.x, V2.x)));
        for (int iCol = iStartCol; iCol <= iEndCol; ++iCol)
        {
            Callback(int2{iCol, iStartRow});
        }
        return;
    }

    auto LerpCol = [](T StartCol, T EndCol, T StartRow, T EndRow, int CurrRow) //
    {
        return StartCol +
            ((EndCol - StartCol) * (static_cast<T>(CurrRow) - StartRow)) / (EndRow - StartRow);
    };

    for (int iRow = iStartRow; iRow <= iEndRow; ++iRow)
    {
        auto dStartCol = LerpCol(V0.x, V2.x, V0.y, V2.y, iRow);

        T dEndCol;
        if (static_cast<T>(iRow) < V1.y)
        {
            //                          V2.
            //    V2-------V1              \' .
            //     |     .'   <-            \   ' . V1
            //     |   .'     <-             \    /      <-
            //     | .'       <-              \  /       <-
            //     .'         <-               \/        <-
            //    V0          <-               V0        <-
            dEndCol = LerpCol(V0.x, V1.x, V0.y, V1.y, iRow);
        }
        else
        {
            if (V1.y < V2.y)
            {
                //                            V2.             <-
                //    V2            <-           \' .         <-
                //     |'.          <-            \   ' . V1  <-
                //     |  '.        <-             \    /
                //     |    '.      <-              \  /
                //     |      '.    <-               \/
                //    V0-------V1   <-               V0
                dEndCol = LerpCol(V1.x, V2.x, V1.y, V2.y, iRow);
            }
            else
            {
                //    V2-------V1   <-
                //     |     .'
                //     |   .'
                //     | .'
                //     .'
                //    V0
                dEndCol = V1.x;
            }
        }
        if (dStartCol > dEndCol)
            std::swap(dStartCol, dEndCol);

        int iStartCol = static_cast<int>(FastCeil(dStartCol));
        int iEndCol   = static_cast<int>(FastFloor(dEndCol));

        for (int iCol = iStartCol; iCol <= iEndCol; ++iCol)
        {
            Callback(int2{iCol, iRow});
        }
    }
}

} // namespace Diligent

namespace std
{

template <>
struct hash<Diligent::Plane3D>
{
    size_t operator()(const Diligent::Plane3D& Plane) const
    {
        return Diligent::ComputeHash(Plane.Normal, Plane.Distance);
    }
};

template <>
struct hash<Diligent::ViewFrustum>
{
    size_t operator()(const Diligent::ViewFrustum& Frustum) const
    {
        return Diligent::ComputeHash(Frustum.LeftPlane, Frustum.RightPlane, Frustum.BottomPlane, Frustum.TopPlane, Frustum.NearPlane, Frustum.FarPlane);
    }
};

template <>
struct hash<Diligent::ViewFrustumExt>
{
    size_t operator()(const Diligent::ViewFrustumExt& Frustum) const
    {
        auto Seed = Diligent::ComputeHash(static_cast<const Diligent::ViewFrustum&>(Frustum));
        for (int Corner = 0; Corner < _countof(Frustum.FrustumCorners); ++Corner)
            Diligent::HashCombine(Seed, Frustum.FrustumCorners[Corner]);
        return Seed;
    }
};

} // namespace std
