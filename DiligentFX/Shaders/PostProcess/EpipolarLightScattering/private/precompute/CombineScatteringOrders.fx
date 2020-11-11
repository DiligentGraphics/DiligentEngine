
#ifndef THREAD_GROUP_SIZE
#   define THREAD_GROUP_SIZE 16
#endif

Texture3D<float3> g_tex3DSingleSctrLUT;
Texture3D<float3> g_tex3DHighOrderSctrLUT;

RWTexture3D</*format = rgba16f*/float3> g_rwtex3DMultipleSctr;

[numthreads(THREAD_GROUP_SIZE, THREAD_GROUP_SIZE, 1)]
void CombineScatteringOrdersCS(uint3 ThreadId  : SV_DispatchThreadID)
{
    // Combine single & higher order scattering into single look-up table
    g_rwtex3DMultipleSctr[ThreadId] = 
                     g_tex3DSingleSctrLUT.Load( int4(ThreadId, 0) ).xyz + 
                     g_tex3DHighOrderSctrLUT.Load( int4(ThreadId, 0) ).xyz;
}
