#include "structures.fxh"
#include "particles.fxh"

cbuffer Constants
{
    GlobalConstants g_Constants;
};

#ifndef THREAD_GROUP_SIZE
#   define THREAD_GROUP_SIZE 64
#endif

#ifndef UPDATE_SPEED
#   define UPDATE_SPEED 0
#endif

RWStructuredBuffer<ParticleAttribs> g_Particles;
Buffer<int>                         g_ParticleListHead;
Buffer<int>                         g_ParticleLists;

// https://en.wikipedia.org/wiki/Elastic_collision
void CollideParticles(inout ParticleAttribs P0, in ParticleAttribs P1)
{
    float2 R01 = (P1.f2Pos.xy - P0.f2Pos.xy) / g_Constants.f2Scale.xy;
    float d01 = length(R01);
    R01 /= d01;
    if (d01 < P0.fSize + P1.fSize)
    {
#if UPDATE_SPEED
        // The math for speed update is only valid for two-particle collisions.
        if (P0.iNumCollisions == 1 && P1.iNumCollisions == 1)
        {
            float v0 = dot(P0.f2Speed.xy, R01);
            float v1 = dot(P1.f2Speed.xy, R01);

            float m0 = P0.fSize * P0.fSize;
            float m1 = P1.fSize * P1.fSize;

            float new_v0 = ((m0 - m1) * v0 + 2.0 * m1 * v1) / (m0 + m1);
            P0.f2NewSpeed += (new_v0 - v0) * R01;
        }
#else
        {
            // Move the particle away
            P0.f2NewPos += -R01 * (P0.fSize + P1.fSize - d01) * g_Constants.f2Scale.xy * 0.51;

            // Set our fake temperature to 1 to indicate collision
            P0.fTemperature = 1.0;

            // Count the number of collisions
            P0.iNumCollisions += 1;
        }
#endif
    }
}

[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void main(uint3 Gid  : SV_GroupID,
          uint3 GTid : SV_GroupThreadID)
{
    uint uiGlobalThreadIdx = Gid.x * uint(THREAD_GROUP_SIZE) + GTid.x;
    if (uiGlobalThreadIdx >= g_Constants.uiNumParticles)
        return;

    int iParticleIdx = int(uiGlobalThreadIdx);
    ParticleAttribs Particle = g_Particles[iParticleIdx];
    
    int2 i2GridPos = GetGridLocation(Particle.f2Pos, g_Constants.i2ParticleGridSize).xy;
    int GridWidth  = g_Constants.i2ParticleGridSize.x;
    int GridHeight = g_Constants.i2ParticleGridSize.y;

#if !UPDATE_SPEED
    Particle.f2NewPos       = Particle.f2Pos;
    Particle.iNumCollisions = 0;
#else
    Particle.f2NewSpeed     = Particle.f2Speed;
    // Only update speed when there is single collision with another particle.
    if (Particle.iNumCollisions == 1)
    {
#endif
        for (int y = max(i2GridPos.y - 1, 0); y <= min(i2GridPos.y + 1, GridHeight-1); ++y)
        {
            for (int x = max(i2GridPos.x - 1, 0); x <= min(i2GridPos.x + 1, GridWidth-1); ++x)
            {
                int AnotherParticleIdx = g_ParticleListHead.Load(x + y * GridWidth);
                while (AnotherParticleIdx >= 0)
                {
                    if (iParticleIdx != AnotherParticleIdx)
                    {
                        ParticleAttribs AnotherParticle = g_Particles[AnotherParticleIdx];
                        CollideParticles(Particle, AnotherParticle);
                    }

                    AnotherParticleIdx = g_ParticleLists.Load(AnotherParticleIdx);
                }
            }
        }
#if UPDATE_SPEED
    }
    else if (Particle.iNumCollisions > 1)
    {
        // If there are multiple collisions, reverse the particle move direction to
        // avoid particle crowding.
        Particle.f2NewSpeed = -Particle.f2Speed;
    }
#else
    ClampParticlePosition(Particle.f2NewPos, Particle.f2Speed, Particle.fSize, g_Constants.f2Scale);
#endif

    g_Particles[iParticleIdx] = Particle;
}
