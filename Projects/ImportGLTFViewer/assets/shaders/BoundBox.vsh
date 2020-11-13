#include "BasicStructures.fxh"

cbuffer cbCameraAttribs
{
    CameraAttribs g_CameraAttribs;
}

float4 BoundBoxVS(uint id : SV_VertexID) : SV_Position
{
    float3 BoundBoxMinXYZ = float3(0.0, 0.0, 0.0);
    float3 BoundBoxMaxXYZ = float3(1.0, 1.0, 1.0);
    
    float4x4 BBTransform = MatrixFromRows(
        g_CameraAttribs.f4ExtraData[0],
        g_CameraAttribs.f4ExtraData[1],
        g_CameraAttribs.f4ExtraData[2],
        g_CameraAttribs.f4ExtraData[3]);

    BBTransform = mul(BBTransform, g_CameraAttribs.mViewProj);

    float4 BoxCorners[8];
    
    BoxCorners[0] = float4(BoundBoxMinXYZ.x, BoundBoxMinXYZ.y, BoundBoxMinXYZ.z, 1.0);
    BoxCorners[1] = float4(BoundBoxMinXYZ.x, BoundBoxMaxXYZ.y, BoundBoxMinXYZ.z, 1.0);
    BoxCorners[2] = float4(BoundBoxMaxXYZ.x, BoundBoxMaxXYZ.y, BoundBoxMinXYZ.z, 1.0);
    BoxCorners[3] = float4(BoundBoxMaxXYZ.x, BoundBoxMinXYZ.y, BoundBoxMinXYZ.z, 1.0);

    BoxCorners[4] = float4(BoundBoxMinXYZ.x, BoundBoxMinXYZ.y, BoundBoxMaxXYZ.z, 1.0);
    BoxCorners[5] = float4(BoundBoxMinXYZ.x, BoundBoxMaxXYZ.y, BoundBoxMaxXYZ.z, 1.0);
    BoxCorners[6] = float4(BoundBoxMaxXYZ.x, BoundBoxMaxXYZ.y, BoundBoxMaxXYZ.z, 1.0);
    BoxCorners[7] = float4(BoundBoxMaxXYZ.x, BoundBoxMinXYZ.y, BoundBoxMaxXYZ.z, 1.0);

    if (id < 8u)
    {
        // 0,1, 1,2, 2,3, 3,0
        id = ((id + 1u) / 2u) % 4u;
    }
    else if (id < 16u)
    {
        // 4,5, 5,6, 6,7, 7,4
        id = 4u + ((id + 1u) / 2u) % 4u;
    }
    else
    {
        // 0,4, 1,5, 2,6, 3,7
        id = (id - 16u) / 2u + (id & 0x01u) * 4u;
    }

    return mul(BoxCorners[id], BBTransform);
}
