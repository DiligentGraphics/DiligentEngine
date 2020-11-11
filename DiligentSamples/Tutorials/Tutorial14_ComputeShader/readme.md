# Tutorial14 - Compute Shader

This tutorial shows how to implement a simple particle simulation system using compute shaders.

![](Animation_Large.gif)

The particle system consists of a number of spherical particles moving in random directions that
encounter [elastic collisions](https://en.wikipedia.org/wiki/Elastic_collision). The simulation
and collision detection is performed on the GPU by compute shaders. To accelerate collision detection,
the shader subdivides the screen into bins and for every bin creates a list of particles residing in the bin.
The number of bins is the same as the number of particles and the bins are distributed evenly on the screen,
thus every bin on average contains one particle. The size of the particle does not exceed the bin size, so
a particle should only be tested for collision against particles residing in its own or eight neighboring bins, 
resulting in O(n) overall algorithmic complexity.

## Buffers

The tutorial uses a number of buffers described below.

## Particle Attributes Buffer

This buffer contains the particle attributes organized into the following structure:

```cpp
struct ParticleAttribs
{
    float2 f2Pos;
    float2 f2NewPos;

    float2 f2Speed;
    float2 f2NewSpeed;

    float  fSize;
    float  fTemperature;
    int    iNumCollisions;
    float  fPadding0;
};
```

Notice the padding element that is required to make the struct size `float4`-aligned. Note also that the struct contains
current and new values of position and speed. This is required because they can't be updated in place due to
unspecified execution order of GPU threads. The buffer initialization is pretty standard except for the fact that we use
`BIND_UNORDERED_ACCESS` bind flag to make the buffer available for unordered read/write operations in the shader.
Another important thing is that we use `BUFFER_MODE_STRUCTURED` to allow the buffer be accessed as a structured
buffer in the shader. When `BUFFER_MODE_STRUCTURED` mode is used, `ElementByteStride` must define the element
stride, in bytes, which in our case is `sizeof(ParticleAttribs)`.

```cpp
BufferDesc BuffDesc;
BuffDesc.Name              = "Particle attribs buffer";
BuffDesc.Usage             = USAGE_DEFAULT;
BuffDesc.BindFlags         = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
BuffDesc.Mode              = BUFFER_MODE_STRUCTURED;
BuffDesc.ElementByteStride = sizeof(ParticleAttribs);
BuffDesc.uiSizeInBytes     = sizeof(ParticleAttribs) * m_NumParticles;
m_pDevice->CreateBuffer(BuffDesc, &VBData, &m_pParticleAttribsBuffer);
```

Structured buffers can be defined as both read-only or read-write buffers in the shader:

```hlsl
StructuredBuffer<ParticleAttribs> g_Particles;
```

or

```hlsl
RWStructuredBuffer<ParticleAttribs> g_Particles;
```

The buffers can be accessed using array-style indexing in the shaders:

```hlsl
ParticleAttribs Particle = g_Particles[iParticleIdx];

// Update the particle attribs

g_Particles[iParticleIdx] = Particle;
```

To bind structured buffers to shader variables, we can request default shader resource and unordered access views and
set them in the shader resource binding like any other variable:

```cpp
IBufferView* pParticleAttribsBufferSRV = m_pParticleAttribsBuffer->GetDefaultView(BUFFER_VIEW_SHADER_RESOURCE);
IBufferView* pParticleAttribsBufferUAV = m_pParticleAttribsBuffer->GetDefaultView(BUFFER_VIEW_UNORDERED_ACCESS);

m_pRenderParticleSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_Particles")
                    ->Set(pParticleAttribsBufferSRV);
m_pMoveParticlesSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_Particles")
                   ->Set(pParticleAttribsBufferUAV);
```

## Particle List Heads and Particle List Buffer

The second buffer that we will use is going to contain the index of the first
particle in the list, for every bin. Finally, the last buffer will contain
the index of the next particle in the list, for every particle. Both buffers will store
one integer per particle and will be initialized as formatted buffers:

```cpp
BuffDesc.ElementByteStride = sizeof(int);
BuffDesc.Mode              = BUFFER_MODE_FORMATTED;
BuffDesc.uiSizeInBytes     = BuffDesc.ElementByteStride * m_NumParticles;
BuffDesc.BindFlags         = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
m_pDevice->CreateBuffer(BuffDesc, nullptr, &m_pParticleListHeadsBuffer);
m_pDevice->CreateBuffer(BuffDesc, nullptr, &m_pParticleListsBuffer);
```

Formatted buffer allows format conversions when accessing elements of a buffer
(such as float->UNORM). We will not use this though and access buffers as raw
integer values. Formatted buffers do not provide default views because default
format is ambiguous, so we have to explicitly define the format (one 32-bit
integer) and manually create the views:

```cpp
RefCntAutoPtr<IBufferView> pParticleListHeadsBufferUAV;
RefCntAutoPtr<IBufferView> pParticleListsBufferUAV;
RefCntAutoPtr<IBufferView> pParticleListHeadsBufferSRV;
RefCntAutoPtr<IBufferView> pParticleListsBufferSRV;

BufferViewDesc ViewDesc;
ViewDesc.ViewType = BUFFER_VIEW_UNORDERED_ACCESS;
ViewDesc.Format.ValueType     = VT_INT32;
ViewDesc.Format.NumComponents = 1;
m_pParticleListHeadsBuffer->CreateView(ViewDesc, &pParticleListHeadsBufferUAV);
m_pParticleListsBuffer->CreateView(ViewDesc, &pParticleListsBufferUAV);

ViewDesc.ViewType = BUFFER_VIEW_SHADER_RESOURCE;
m_pParticleListHeadsBuffer->CreateView(ViewDesc, &pParticleListHeadsBufferSRV);
m_pParticleListsBuffer->CreateView(ViewDesc, &pParticleListsBufferSRV);
```

The buffer views are bound to shader resource binding objects in a typical way:

```cpp
m_pMoveParticlesSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_ParticleListHead")
                   ->Set(pParticleListHeadsBufferUAV);
m_pMoveParticlesSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_ParticleLists")
                   ->Set(pParticleListsBufferUAV);
```

In the shaders, formatted buffers are declared similar to structured buffers, but they can only use
basic types (`int`, `float4`, `uint2`, etc.):

```hlsl
RWBuffer<int /*format=r32i*/> g_ParticleListHead;
```

Notice the special comment in the type declaration. This comment is used by the HLSL->GLSL converter to
make the shader work in GLSL backend.

Access to a formatted buffer is performed similar to a structured buffer using the array notation:

```hlsl
g_ParticleListHead[uiGlobalThreadIdx] = -1;
```

## Compute Shaders

Particle update pipeline consists of the following steps described in details below:

1. Reset particle lists
2. Move particles and perform binning
3. Particle collision - position update
4. Particle collision - speed update

### Resetting the Particle Lists

The first shader in our particle simulation pipeline resets the particle list heads
for every bin by writing -1:

```hlsl
cbuffer Constants
{
    GlobalConstants g_Constants;
};

RWBuffer<int /*format=r32i*/> g_ParticleListHead;

[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void main(uint3 Gid  : SV_GroupID,
          uint3 GTid : SV_GroupThreadID)
{
    uint uiGlobalThreadIdx = Gid.x * uint(THREAD_GROUP_SIZE) + GTid.x;
    if (uiGlobalThreadIdx < uint(g_Constants.i2ParticleGridSize.x * g_Constants.i2ParticleGridSize.y))
        g_ParticleListHead[uiGlobalThreadIdx] = -1;
}
```

Notice, again, the usage of special comment `/*format=r32i*/`. The `THREAD_GROUP_SIZE` macro is defined
by the host and defines the size of the compute shader thread group. 

Please refer to [this page](https://docs.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-dispatch)
for explanation of group index, group thread index and other compute shader specific elements.

### Moving Particles

The second compute shader in the pipeline moves every particle, updates the speed calculated
by the collision shader previously and performs particle binning. The full source is given below:

```hlsl
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

```

The shader starts by loading the particle attributes and updating the position and temperature.
The temperature is not a real temperature but rather indicates if the particle has been hit recently.
It then clamps the particle position against the screen boundaries and writes the updated particle back
to the structured buffer:

```hlsl
ParticleAttribs Particle = g_Particles[iParticleIdx];
Particle.f2Pos   = Particle.f2NewPos;
Particle.f2Speed = Particle.f2NewSpeed;
Particle.f2Pos  += Particle.f2Speed * g_Constants.f2Scale * g_Constants.fDeltaTime;
Particle.fTemperature -= Particle.fTemperature * g_Constants.fDeltaTime * 2.0;

ClampParticlePosition(Particle.f2Pos, Particle.f2Speed, Particle.fSize, g_Constants.f2Scale);
g_Particles[iParticleIdx] = Particle;
```

The most interesting part of this shader is particle binning that is performed by the
following code:

```hlsl
int GridIdx = GetGridLocation(Particle.f2Pos, g_Constants.i2ParticleGridSize).z;
int OriginalListIdx;
InterlockedExchange(g_ParticleListHead[GridIdx], iParticleIdx, OriginalListIdx);
g_ParticleLists[iParticleIdx] = OriginalListIdx;
```

The code relies on an interlocked exchange operation that atomically writes a value
to the buffer and returns an old value. Using interlocked operation is crucial here
as multiple GPU threads may potentially attempt to access the same memory when more
than one particle resides in a bin.

The update process is illustrated below:

1. Atomically write particle Id to the buffer and get original list head index.

```
           GridId
... |     |  N  |     |     | ...  List heads
   
                ||
                \/

           GridId
... |     |PatId|     |     | ...  List heads

OriginalListIdx == N
```

2. Write original list head to the particle list buffer:

```
           GridId
... |     |PatId|     |     | ...  List heads
             |
             |_____
                   |
                   V
       N         PatId
... |     |     |  N  |     | ...  Particle lists
       ^          |
       |__________|
```

The lists are thus grown from the start by replacing the head.
Given this linked list structure defined by these two buffers,
the particles can be iterated as shown in the collision shader.


### Particle Collision

Particle collision is performed in two steps. On the first step we update the particle
position to make sure it does not intersect with other particles. On the second step,
we update the particle speed. The reasons the steps are separated is because the math we
use for speed updates is only valid for two-particle collisions, so we count the number 
of collisions on the first step and use this number at the second step

Both steps are implemented by the same shader. Whether we perform position or speed
update is controlled by the value of `UPDATE_SPEED` macro. The shader uses all three
particle-related buffers:

```hlsl
RWStructuredBuffer<ParticleAttribs> g_Particles;
Buffer<int>                         g_ParticleListHead;
Buffer<int>                         g_ParticleLists;
```

The shader starts by reading the current particle attributes
and computing the grid location:

```hlsl
int iParticleIdx = int(uiGlobalThreadIdx);
ParticleAttribs Particle = g_Particles[iParticleIdx];
    
int2 i2GridPos = GetGridLocation(Particle.f2Pos, g_Constants.i2ParticleGridSize).xy;
int GridWidth  = g_Constants.i2ParticleGridSize.x;
int GridHeight = g_Constants.i2ParticleGridSize.y;

```

The shader then goes through all bins in a 3x3 neighborhood, and for
every bin it iterates through the list of particles assigned to that bin:

```hlsl
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
```

Notice how iteration through the list is performed: we first read the index of the first
particle:

```hlsl
int AnotherParticleIdx = g_ParticleListHead.Load(x + y * GridWidth);
```

And then run the loop until we reach the end of the list (`AnotherParticleIdx == -1`).

Inside the loop we go to the next particle by reading the next index from the 
particle's list buffer:

```hlsl
AnotherParticleIdx = g_ParticleLists.Load(AnotherParticleIdx);
```

`CollideParticles` function implements the crux of particle collision. Please refer to the shader's
[full source code](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial14_ComputeShader/assets/collide_particles.csh)
for details.

## Particle Rendering Shader

Particle rendering shader is pretty straightforward. The only thing worth mentioning is the usage of the
particle attributes structured buffer:

```hlsl
StructuredBuffer<ParticleAttribs> g_Particles;

// ...

ParticleAttribs Attribs = g_Particles[VSIn.InstID];
```

## Initializing Compute Pipeline States

Initialization of the compute pipeline is performed very similar to a graphics pipeline
except that there is way fewer states to describe. Resource layout is pretty
much everything you need to specify except for the compute shader itself:

```cpp
ComputePipelineStateCreateInfo PSOCreateInfo;
PipelineStateDesc&             PSODesc = PSOCreateInfo.PSODesc;

// Pipeline state name is used by the engine to report issues.
PSODesc.Name = "Reset particle lists PSO";

// This is a compute pipeline
PSODesc.PipelineType = PIPELINE_TYPE_COMPUTE;

PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;
ShaderResourceVariableDesc Vars[] = 
{
    {SHADER_TYPE_COMPUTE, "Constants", SHADER_RESOURCE_VARIABLE_TYPE_STATIC}
};
PSODesc.ResourceLayout.Variables    = Vars;
PSODesc.ResourceLayout.NumVariables = _countof(Vars);
    
PSOCreateInfo.pCS = pResetParticleListsCS;
m_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pResetParticleListsPSO);
m_pResetParticleListsPSO->GetStaticVariableByName(SHADER_TYPE_COMPUTE, "Constants")->Set(m_Constants);
```

## Dispatching Compute Commands

Compute commands are executed by the `DispatchCompute()` method of the device context. 
The method takes `DispatchComputeAttribs` argument that describes the compute grid size.
Note that we round the grid x dimension up to make sure that all particles are processed.
Every compute shader thread checks if the particle id is in the range and exits early if
it is not.

```cpp
DispatchComputeAttribs DispatAttribs;
DispatAttribs.ThreadGroupCountX = (m_NumParticles + m_ThreadGroupSize-1) / m_ThreadGroupSize;

m_pImmediateContext->SetPipelineState(m_pResetParticleListsPSO);
m_pImmediateContext->CommitShaderResources(m_pResetParticleListsSRB,
                                            RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
m_pImmediateContext->DispatchCompute(DispatAttribs);

m_pImmediateContext->SetPipelineState(m_pMoveParticlesPSO);
m_pImmediateContext->CommitShaderResources(m_pMoveParticlesSRB,
                                           RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
m_pImmediateContext->DispatchCompute(DispatAttribs);

m_pImmediateContext->SetPipelineState(m_pCollideParticlesPSO);
m_pImmediateContext->CommitShaderResources(m_pCollideParticlesSRB,
                                           RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
m_pImmediateContext->DispatchCompute(DispatAttribs);

m_pImmediateContext->SetPipelineState(m_pUpdateParticleSpeedPSO);
// Use the same SRB
m_pImmediateContext->CommitShaderResources(m_pCollideParticlesSRB,
                                           RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
m_pImmediateContext->DispatchCompute(DispatAttribs);
```

## Rendering

Particle rendering is pretty typical: we draw one instance per particle and the vertex shader reads
the particle attributes from the structured buffer updated by the compute shaders.

```cpp
m_pImmediateContext->SetPipelineState(m_pRenderParticlePSO);
m_pImmediateContext->CommitShaderResources(m_pRenderParticleSRB[m_BufferIdx],
                                           RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
DrawAttribs drawAttrs;
drawAttrs.NumVertices  = 4;
drawAttrs.NumInstances = m_NumParticles;
m_pImmediateContext->Draw(drawAttrs);
```
