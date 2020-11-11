#version 460
#extension GL_NV_mesh_shader : require
#extension GL_GOOGLE_include_directive : require

#include "structures.fxh"

layout(local_size_x = GROUP_SIZE) in;

layout(std140) readonly buffer DrawTasks
{
    DrawTask g_DrawTasks[];
};

// Use row major matrices for compatibility with DirectX
layout(std140, row_major) uniform cbConstants
{
    Constants g_Constants;
};

layout(std140, row_major) uniform cbCubeData
{
    CubeData g_CubeData;
};

layout(std140) buffer Statistics
{
    uint g_VisibleCubes; // atomic
};


// The sphere is visible when the distance from each plane is greater than or equal to the radius of the sphere.
bool IsVisible(vec3 cubeCenter, float radius)
{
    vec4 center = vec4(cubeCenter, 1.0);

    for (int i = 0; i < 6; ++i)
    {
        if (dot(g_Constants.Frustum[i], center) < -radius)
            return false;
    }
    return true;
}

float CalcDetailLevel(vec3 cubeCenter, float radius)
{
    // cubeCenter is also the center of the sphere. 
    // radius - radius of circumscribed sphere

    // get position in view space
    vec3  pos   = (g_Constants.ViewMat * vec4(cubeCenter, 1.0)).xyz;

    // square of distance from camera to circumscribed sphere
    float dist2 = dot(pos, pos);

    // calculate sphere size in screen space
    float size  = g_Constants.CoTanHalfFov * radius / sqrt(dist2 - radius * radius);

    // calculate detail level
    float level = clamp(1.0 - size, 0.0, 1.0);
    return level;
}

// Task shader output data. Must be less than 16Kb.
taskNV out Task {
    Payload payload;
} Output;

// This value used to calculate the number of cubes that will be rendered after the frustum culling
shared uint s_TaskCount;


void main()
{
    // Initialize shared variable
    if (gl_LocalInvocationIndex == 0)
    {
        s_TaskCount = 0;
    }

    // Flush the cache and synchronize
    memoryBarrierShared();
    barrier();

    uint     gid   = gl_WorkGroupID.x * gl_WorkGroupSize.x + gl_LocalInvocationIndex;
    DrawTask task  = g_DrawTasks[gid];
    vec3     pos   = vec3(task.BasePos, 0.0).xzy;
    float    scale = task.Scale;
    float    timeOffset  = task.TimeOffset;

    // Simple animation
    pos.y = sin(g_Constants.CurrTime + timeOffset);
    
    // Frustum culling
    if (g_Constants.FrustumCulling == 0u || IsVisible(pos, g_CubeData.SphereRadius.x * scale))
    {
        // Acquire index that will be used to safely access shader output.
        // Each thread has unique index.
        uint index = atomicAdd(s_TaskCount, 1);

        Output.payload.PosX[index] = pos.x;
        Output.payload.PosY[index] = pos.y;
        Output.payload.PosZ[index] = pos.z;
        Output.payload.Scale[index] = scale;
        Output.payload.LODs[index] = CalcDetailLevel(pos, g_CubeData.SphereRadius.x * scale);
    }

    // all threads must complete their work so that we can read s_TaskCount
    barrier();

    // invalidate cache to read actual value from s_TaskCount
    memoryBarrierShared();

    if (gl_LocalInvocationIndex == 0)
    {
        // Set the number of mesh shader workgroups.
        gl_TaskCountNV = s_TaskCount;
        
        // update statistics
        atomicAdd(g_VisibleCubes, s_TaskCount);
    }
}
