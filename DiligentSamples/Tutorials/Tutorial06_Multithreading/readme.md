# Tutorial06 - Multithreading

This tutorial shows how to record command lists in parallel from multiple threads.

![](Animation_Large.gif)

This tutorial generates the same output as Tutorial05, but renders every cube using individual draw call.
It shows how recording commands can be split between multiple threads. Note that this tutorial illustrates
the API usage and for this specific rendering problem, instancing is a more efficient solution.
However, multithreading in a real application can be implemented in the same way as shown in this
tutorial.

## Shaders

This tutorial uses shaders from Tutorial03. While pixel shader is exactly the same, the vertex shader
applies rotation and instance-specific transformation before the global view-projection transform. The
instance transform matrix resides in its own constant buffer that is updated every time a new instance is
rendered.

```hlsl
cbuffer Constants
{
    float4x4 g_ViewProj;
    float4x4 g_Rotation;
};

cbuffer InstanceData
{
    float4x4 g_InstanceMatr;
};

struct VSInput
{
    float3 Pos : ATTRIB0; 
    float2 UV  : ATTRIB1,
};

struct PSInput 
{ 
    float4 Pos : SV_POSITION; 
    float2 uv : TEX_COORD; 
};

void main(in  VSInput VSIn,
          out PSInput PSIn) 
{
    // Apply rotation
    float4 TransformedPos = mul( float4(VSIn.Pos,1.0),g_Rotation);
    // Apply instance-specific transformation
    TransformedPos = mul(TransformedPos, g_InstanceMatr);
    // Apply view-projection matrix
    PSIn.Pos = mul( TransformedPos, g_ViewProj);
    PSIn.UV  = VSIn.UV;
}
```

## Resource initialization

Pipeline state, shaders, vertex and index buffers are initialized in the same way as in 
previous tutorials. What is different is that this time we load every texture
individually, and then bind the texture to its own shader resource binding object:

```cpp
for(int tex=0; tex < NumTextures; ++tex)
{
    // Create one Shader Resource Binding for every texture
    m_pPSO->CreateShaderResourceBinding(&m_SRB[tex]);
    m_SRB[tex]->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture")->Set(m_TextureSRV[tex]);
}
```

This example illustrates the expected usage of mutable shader resources: the app creates 
several SRB objects encompassing different resource bindings.

## Explicit state transitoins

This tutorial explicitly transitions all resources to required states using
`IDeviceContext::TransitionResourceStates()` method. The method takes an array
of `StateTransitionDesc` structures. The structure defines resource to transition,
as well old state and new states. Old state can be set to `RESOURCE_STATE_UNKNOWN` in 
which case the engine will use the internal resource state. For a texture, the structure
also defines the range of array slices and mip levels to transition.
For example, transitioning vertex and index buffers to required states can be performed as follows:

```cpp
StateTransitionDesc Barriers[2];
Barriers[0].pBuffer  = m_CubeVertexBuffer;
Barriers[0].OldState = RESOURCE_STATE_UNKNOWN; // Use the internal buffer state
Barriers[0].NewState = RESOURCE_STATE_VERTEX_BUFFER;
Barriers[0].UpdateResourceState = true;

Barriers[1].pBuffer  = m_CubeIndexBuffer;
Barriers[1].OldState = RESOURCE_STATE_UNKNOWN; // Use the internal buffer state
Barriers[1].NewState = RESOURCE_STATE_INDEX_BUFFER;
Barriers[1].UpdateResourceState = true;

m_pImmediateContext->TransitionResourceStates(2, Barriers);
```

When resources are explicitly transitioned to correct states, the engine does not need to check
the states at every draw command which greately reduces the overhead. 

## Multithreaded Rendering

All rendering commands in Diligent Engine are issued through device contexts.
Similar to [Direct3D11](https://msdn.microsoft.com/en-us/library/windows/desktop/ff476880(v=vs.85).aspx), 
there are two types of contexts: immediate and deferred. An immediate context records
rendering commands and implicitly submits them for execution. Deferred contexts can only record
commands to a command list that can later be executed through the immediate context.
Deferred contexts should be created for every worker thread that records rendering commands.

### Main Thread

Main thread coordinates the execution of worker threads and handles recorded command lists.
It starts by signaling all worker threads to start:

```cpp
m_NumThreadsCompleted = 0;
m_RenderSubsetSignal.Trigger(true);
```

and renders its own subset:

```cpp
RenderSubset(m_pImmediateContext, 0);
```

It then waits until worker threads signal that all command lists are ready
and executes them:

```cpp
m_ExecuteCommandListsSignal.Wait(true, 1);

for (auto& cmdList : m_CmdLists)
{
    m_pImmediateContext->ExecuteCommandList(cmdList);
}
```

Finally, it tells the worker threads to proceed to the next frame:

```cpp
m_NumThreadsReady = 0;
m_GotoNextFrameSignal.Trigger(true);
```

### Worker Threads

Every worker thread starts by waiting for the signal from the main thread (a negative
value is an exit signal):

```cpp
auto SignaledValue = pThis->m_RenderSubsetSignal.Wait(true, pThis->m_NumWorkerThreads);
if (SignaledValue < 0)
    return;
```

The thread then renders the allotted subset using its own deferred context:

```cpp
IDeviceContext* pDeferredCtx = pThis->m_pDeferredContexts[ThreadNum];
pThis->RenderSubset(pDeferredCtx, 1+ThreadNum);
```

When all commands are recorded, a command list is requested from the deferred context
that is later executed by the main thread:

```cpp
RefCntAutoPtr<ICommandList> pCmdList;
pDeferredCtx->FinishCommandList(&pCmdList);
pThis->m_CmdLists[ThreadNum] = pCmdList;
```

When all threads are done recording the commands, the last thread
signals the main thread that it can start executing the command lists.
The threads then wait for the signal from the main thread to proceed to the
next frame. After the signal is received, every thread calls FinishFrame() to
release all dynamic resources allocted by its deferred context. This must be done
after the command lists have been submitted for execution.

```cpp
pThis->m_GotoNextFrameSignal.Wait(true, pThis->m_NumWorkerThreads);

pDeferredCtx->FinishFrame();
```

### Rendering Subsets

Subset rendering procedure is generally the same as in previous tutorials. Few details are worth mentioning.
1. Deferred contexts start in default state (no render target, viewports, pipeline state etc. are bound),
so every context should set the default render target:

```cpp
auto* pRTV = m_pSwapChain->GetCurrentBackBufferRTV();
auto* pDSV = m_pSwapChain->GetDepthBufferDSV();
pCtx->SetRenderTargets(1, &pRTV, pDSV, RESOURCE_STATE_TRANSITION_MODE_VERIFY);
```

Note that render targets are set and transitioned to correct states by the main thread, so we use
`RESOURCE_STATE_TRANSITION_MODE_VERIFY` flag to double-check the states are correct.

2. The rendering procedure iterates through all the instances in the allotted subset, and for every instance
does the following:

* Commits SRB object corresponding to the texture index, no RESOURCE_STATE_TRANSITION_MODE_TRANSITION
  is specified since we already transitioned all resources to correct states.

* Updates the constant buffer with the transformation matrix for this instance

* Issues the draw call


```cpp
DrawIndexedAttribs DrawAttrs;
DrawAttrs.IndexType  = VT_UINT32;
DrawAttrs.NumIndices = 36;
DrawAttrs.Flags      = DRAW_FLAG_VERIFY_ALL;

pCtx->SetPipelineState(m_pPSO);
for (size_t inst = StartInst; inst < EndInst; ++inst)
{
    const auto& CurrInstData = m_InstanceData[inst];
    // Shader resources have been explicitly transitioned to correct states, so
    // RESOURCE_STATE_TRANSITION_MODE_TRANSITION mode is not needed.
    // Instead, we use RESOURCE_STATE_TRANSITION_MODE_VERIFY mode to
    // verify that all resources are in correct states. This mode only has effect
    // in debug and development builds
    pCtx->CommitShaderResources(m_SRB[CurrInstData.TextureInd], RESOURCE_STATE_TRANSITION_MODE_VERIFY);

    {
        MapHelper<float4x4> InstData(pCtx, m_InstanceConstants, MAP_WRITE, MAP_FLAG_DISCARD);
        *InstData = transposeMatrix(CurrInstData.Matrix);
    }

    pCtx->DrawIndexed(DrawAttrs);
}
```