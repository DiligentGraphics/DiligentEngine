#include "structures.fxh"
#include "particles.fxh"

cbuffer Constants
{
    GlobalConstants g_Constants;
};

#ifndef THREAD_GROUP_SIZE
#   define THREAD_GROUP_SIZE 64
#endif

RWStructuredBuffer<ParticleAttribs> g_Particles;
RWBuffer<int /*format=r32i*/>       g_ParticleListHead;
RWBuffer<int /*format=r32i*/>       g_ParticleLists;

[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void main(uint3 Gid  : SV_GroupID,
          uint3 GTid : SV_GroupThreadID)
{
    uint uiGlobalThreadIdx = Gid.x * uint(THREAD_GROUP_SIZE) + GTid.x;
    if (uiGlobalThreadIdx >= g_Constants.uiNumParticles)
        return;

    int iParticleIdx = int(uiGlobalThreadIdx);

    ParticleAttribs Particle = g_Particles[iParticleIdx];
    Particle.f2Pos   = Particle.f2NewPos;
    Particle.f2Speed = Particle.f2NewSpeed;
    Particle.f2Pos  += Particle.f2Speed * g_Constants.f2Scale * g_Constants.fDeltaTime;
    Particle.fTemperature -= Particle.fTemperature * min(g_Constants.fDeltaTime * 2.0, 1.0);

    ClampParticlePosition(Particle.f2Pos, Particle.f2Speed, Particle.fSize, g_Constants.f2Scale);
    g_Particles[iParticleIdx] = Particle;

    // Bin particles
    int GridIdx = GetGridLocation(Particle.f2Pos, g_Constants.i2ParticleGridSize).z;
    int OriginalListIdx;
    InterlockedExchange(g_ParticleListHead[GridIdx], iParticleIdx, OriginalListIdx);
    g_ParticleLists[iParticleIdx] = OriginalListIdx;
}
