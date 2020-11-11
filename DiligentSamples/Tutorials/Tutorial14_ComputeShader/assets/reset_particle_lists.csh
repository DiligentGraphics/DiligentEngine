#include "structures.fxh"

cbuffer Constants
{
    GlobalConstants g_Constants;
};

#ifndef THREAD_GROUP_SIZE
#   define THREAD_GROUP_SIZE 64
#endif

RWBuffer<int /*format=r32i*/> g_ParticleListHead;

[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void main(uint3 Gid  : SV_GroupID,
          uint3 GTid : SV_GroupThreadID)
{
    uint uiGlobalThreadIdx = Gid.x * uint(THREAD_GROUP_SIZE) + GTid.x;
    if (uiGlobalThreadIdx < uint(g_Constants.i2ParticleGridSize.x * g_Constants.i2ParticleGridSize.y))
        g_ParticleListHead[uiGlobalThreadIdx] = -1;
}
