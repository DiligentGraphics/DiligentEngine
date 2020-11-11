#ifndef THREAD_GROUP_SIZE
#   define THREAD_GROUP_SIZE 16
#endif

Texture3D<float3> g_tex3DCurrentOrderScattering;

RWTexture3D</*format = rgba16f*/float3> g_rwtex3DHighOrderSctr;

[numthreads(THREAD_GROUP_SIZE, THREAD_GROUP_SIZE, 1)]
void InitHighOrderScatteringCS(uint3 ThreadId  : SV_DispatchThreadID)
{
    // Accumulate in-scattering using alpha-blending
    g_rwtex3DHighOrderSctr[ThreadId] = g_tex3DCurrentOrderScattering.Load( int4(ThreadId, 0) );
}
