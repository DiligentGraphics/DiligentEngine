#include "structures.fxh"

// Draw task arguments
StructuredBuffer<DrawTask> DrawTasks;

cbuffer cbConstants
{
    Constants g_Constants;
}

cbuffer cbCubeData
{
    CubeData g_CubeData;
}

// Statistics buffer contains the global counter of visible objects
RWByteAddressBuffer Statistics;

// Payload will be used in the mesh shader.
groupshared Payload s_Payload;

// The sphere is visible when the distance from each plane is greater than or
// equal to the radius of the sphere.
bool IsVisible(float3 cubeCenter, float radius)
{
    float4 center = float4(cubeCenter, 1.0);

    for (int i = 0; i < 6; ++i)
    {
        if (dot(g_Constants.Frustum[i], center) < -radius)
            return false;
    }
    return true;
}

float CalcDetailLevel(float3 cubeCenter, float radius)
{
    // cubeCenter - the center of the sphere 
    // radius     - the radius of circumscribed sphere
    
    // Get the position in the view space
    float3 pos   = mul(float4(cubeCenter, 1.0), g_Constants.ViewMat).xyz;
    
    // Square of distance from camera to circumscribed sphere
    float  dist2 = dot(pos, pos);
    
    // Calculate the sphere size in screen space
    float  size  = g_Constants.CoTanHalfFov * radius / sqrt(dist2 - radius * radius);
    
    // Calculate detail level
    float  level = clamp(1.0 - size, 0.0, 1.0);
    return level;
}

// The number of cubes that are visible by the camera,
// computed by every thread group
groupshared uint s_TaskCount;

[numthreads(GROUP_SIZE, 1, 1)]
void main(in uint I  : SV_GroupIndex,
          in uint wg : SV_GroupID)
{
    // Reset the counter from the first thread in the group
    if (I == 0)
    {
        s_TaskCount = 0;
    }

    // Flush the cache and synchronize
    GroupMemoryBarrierWithGroupSync();

    // Read the task arguments
    const uint gid   = wg * GROUP_SIZE + I;
    DrawTask   task  = DrawTasks[gid];
    float3     pos   = float3(task.BasePos, 0.0).xzy;
    float      scale = task.Scale;
    float      timeOffset  = task.TimeOffset;

    // Simple animation
    pos.y = sin(g_Constants.CurrTime + timeOffset);

    // Frustum culling
    if (g_Constants.FrustumCulling == 0 || IsVisible(pos, g_CubeData.SphereRadius.x * scale))
    {
        // Acquire an index that will be used to safely access the payload.
        // Each thread gets a unique index.
        uint index = 0;
        InterlockedAdd(s_TaskCount, 1, index);
        
        s_Payload.PosX[index]  = pos.x;
        s_Payload.PosY[index]  = pos.y;
        s_Payload.PosZ[index]  = pos.z;
        s_Payload.Scale[index] = scale;
        s_Payload.LODs[index]  = CalcDetailLevel(pos, g_CubeData.SphereRadius.x * scale);
    }
    
    // All threads must complete their work so that we can read s_TaskCount
    GroupMemoryBarrierWithGroupSync();

    if (I == 0)
    {
        // Update statistics from the first thread
        uint orig_value;
        Statistics.InterlockedAdd(0, s_TaskCount, orig_value);
    }
    
    // This function must be called exactly once per amplification shader.
    // The DispatchMesh call implies a GroupMemoryBarrierWithGroupSync(), and ends the amplification shader group's execution.
    DispatchMesh(s_TaskCount, 1, 1, s_Payload);
}
