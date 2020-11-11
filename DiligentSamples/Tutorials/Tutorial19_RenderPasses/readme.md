# Tutorial19 - Render Passes

This tutorial demonstrates how to use the render passes API to implement simple deferred shading.

![](Animation_Large.gif)

Render passes is a feature of the next-generation APIs that allows applications to define rendering commands in a way
that better maps to tiled-deferred rendering architectures used by virtually all mobile platforms. Unlike immediate
rendering architectures typical for desktop platforms, tiled-deferred renderers split the screen into small tiles (e.g. 64x64 pixels,
the actual size depends on multiple factors including render target format, fast memory size, GPU vendor, etc.)
and perform rendering operations tile after tile. This allows GPU to keep all data in a fast GPU-local cache, which is both
faster and more power-efficient. When GPU is done processing one tile, it flushes all the data to the main memory and
moves to the next tile.

Render passes were introduced to give applications explicit control over tile operations. A good metal model of a render pass
is a set of operations that the GPU performs in a local tile cache before flushing the data to the main memory and moving to the
next tile. 


A render pass is defined by the following key components:

- *Render pass attachments*, which are the set of texture views used within the render pass.
  Every attachment defines how its contents shoud be treated at the beginning of the render pass (load operations)
  as well as at the end of the render pass (store operation). The attachments can be used as outputs (render target or depth-stencil) 
  in one subpass as well as inputs to other subpasses. A render pass can also perform multisample resolve operations at the end of the subpass.

- *Subpasses*. A render pass has one more subpasses. Every subpass defines a subset of render pass attachments that are used as output
  attachments, input attachments and resolve attachments. 

- *Subpass dependencies* that define subpass attachment state transitions (e.g. from render target to input attachment).

Diligent Engine enables applications to use and intermix render target API and render passes API.
While the former one is a more implicit way, the latter is a more explicit approach and requires more effort from the application
developers. Most importantly, **no state transitions are allowed within the render pass**. As a result, an application must not use 
`RESOURCE_STATE_TRANSITION_MODE_TRANSITION` with any command while a render pass is active.



## Deferred Shading Using Render Passes

This tutorial demonstrates a simple deferred shading renderer implemented using render passes API.
The render pass consists of two subpasses. The first subpass is a G-buffer pass: it renders the scene
and populates two buffers - color and depth. The second pass is a lighting pass. It renders
light volumes and applies simple distance-based lighting to the G-buffer.
Using the render passes API lets the driver reorder the operations and fuse G-buffer pass and
lighting pass into a single tile operation thus avoiding the need to store intermediate G-buffer
data to the main memory and reading it back.

## Creating Render Pass

To create a render pass we need to prepare an instance of `RenderPassDesc` struct.
But first we need to define some auxiliary data.

### Render Pass Attachments

The first piece of the information we need to define is the render pass attachments.
In this tutorial we will be using 4 attachments:

1. Color buffer
2. Depth Z
3. Depth buffer
4. Final color buffer

```cpp
constexpr Uint32 NumAttachments = 4;
RenderPassAttachmentDesc Attachments[NumAttachments];
```

The first attachment is the color G-Buffer:

```cpp
Attachments[0].Format       = TEX_FORMAT_RGBA8_UNORM;
Attachments[0].InitialState = RESOURCE_STATE_RENDER_TARGET;
Attachments[0].FinalState   = RESOURCE_STATE_INPUT_ATTACHMENT;
Attachments[0].LoadOp       = ATTACHMENT_LOAD_OP_CLEAR;
Attachments[0].StoreOp      = ATTACHMENT_STORE_OP_DISCARD;
```

Notice that we must specify the initial attachment state that the corresponding texture will be in
before the render pass begins as well as the final state it will be in after the render pass ends.
Also notice that as the load operation, we specify `ATTACHMENT_LOAD_OP_CLEAR`. This will tell
the driver that old contents of the texture is not needed and should not be loaded from the main
memory. Also note that as the store operation we use `ATTACHMENT_STORE_OP_DISCARD` that instructs
the driver to discard all the data after the end of the render pass thus avoiding the need to
write it back to the main memory.

The second attachment is the normalized device Z coordinate. Note that we can't extract this from the
depth buffer (attacment 3), as we can't use it as both depth-stencil and input attachment during the second
lighting subpass.

```cpp
Attachments[1].Format       = TEX_FORMAT_R32_FLOAT;
Attachments[1].InitialState = RESOURCE_STATE_RENDER_TARGET;
Attachments[1].FinalState   = RESOURCE_STATE_INPUT_ATTACHMENT;
Attachments[1].LoadOp       = ATTACHMENT_LOAD_OP_CLEAR;
Attachments[1].StoreOp      = ATTACHMENT_STORE_OP_DISCARD;
```
Note again that we use `ATTACHMENT_LOAD_OP_CLEAR` and `ATTACHMENT_STORE_OP_DISCARD` as load and store
operations.

The third attachment is the depth buffer:

```cpp
Attachments[2].Format       = DepthBufferFormat;
Attachments[2].InitialState = RESOURCE_STATE_DEPTH_WRITE;
Attachments[2].FinalState   = RESOURCE_STATE_DEPTH_WRITE;
Attachments[2].LoadOp       = ATTACHMENT_LOAD_OP_CLEAR;
Attachments[2].StoreOp      = ATTACHMENT_STORE_OP_DISCARD;
```

The last attachment is the final buffer where the shaded result will be written to:

```cpp
Attachments[3].Format       = m_pSwapChain->GetDesc().ColorBufferFormat;
Attachments[3].InitialState = RESOURCE_STATE_RENDER_TARGET;
Attachments[3].FinalState   = RESOURCE_STATE_RENDER_TARGET;
Attachments[3].LoadOp       = ATTACHMENT_LOAD_OP_CLEAR;
Attachments[3].StoreOp      = ATTACHMENT_STORE_OP_STORE;
```

Note that unlike previous attachments, this time we use `ATTACHMENT_STORE_OP_STORE` because
we will need to keep the final image to display it on the screen. 

### Subpasses

As discussed above, the render pass will have two subpasses.
The first subpass is the G-buffer pass, the second one is the
lighting pass:

```cpp
constexpr Uint32 NumSubpasses = 2;
SubpassDesc Subpasses[NumSubpasses];
```

The first subpass uses attachments 0 and 1 as render targets,
and attachment 2 as depth-stencil buffer.

```cpp
AttachmentReference RTAttachmentRefs0[] =
{
    {0, RESOURCE_STATE_RENDER_TARGET},
    {1, RESOURCE_STATE_RENDER_TARGET}
};

AttachmentReference DepthAttachmentRef0 = {2, RESOURCE_STATE_DEPTH_WRITE};

Subpasses[0].RenderTargetAttachmentCount = _countof(RTAttachmentRefs0);
Subpasses[0].pRenderTargetAttachments    = RTAttachmentRefs0;
Subpasses[0].pDepthStencilAttachment     = &DepthAttachmentRef0;
```

The `AttachmentReference` struct defines the attachment number
as well as its state during the subpass. 

The second subpass uses attachments 0 and 1 as input attachments,
attachment 2 as depth-stencil buffer, and attachment 3 as render target:

```cpp
AttachmentReference RTAttachmentRefs1[] =
{
    {3, RESOURCE_STATE_RENDER_TARGET}
};

AttachmentReference DepthAttachmentRef1 = {2, RESOURCE_STATE_DEPTH_WRITE};

AttachmentReference InputAttachmentRefs1[] =
{
    {0, RESOURCE_STATE_INPUT_ATTACHMENT},
    {1, RESOURCE_STATE_INPUT_ATTACHMENT}
};

Subpasses[1].RenderTargetAttachmentCount = _countof(RTAttachmentRefs1);
Subpasses[1].pRenderTargetAttachments    = RTAttachmentRefs1;
Subpasses[1].pDepthStencilAttachment     = &DepthAttachmentRef1;
Subpasses[1].InputAttachmentCount        = _countof(InputAttachmentRefs1);
Subpasses[1].pInputAttachments           = InputAttachmentRefs1;
```

### Dependencies

Each subpass defines the states of all its attachments, and the
attachments are transitioned between the states when going from one subpasspass to the
next. However, besides attachment states, a render pass must also specify
execution dependencies.
In our specific example, attachments 0 and 1 are used as render targets in the
first subpass and as input attachments in the second. So we need to specify
a dependency from `ACCESS_FLAG_RENDER_TARGET_WRITE` access type performed by
`PIPELINE_STAGE_FLAG_RENDER_TARGET` pipeline stage of subass 0 to
`ACCESS_FLAG_SHADER_READ` access type from `PIPELINE_STAGE_FLAG_PIXEL_SHADER`
pipeline stage of subpass 1.

```cpp
SubpassDependencyDesc Dependencies[1];
Dependencies[0].SrcSubpass    = 0;
Dependencies[0].DstSubpass    = 1;
Dependencies[0].SrcStageMask  = PIPELINE_STAGE_FLAG_RENDER_TARGET;
Dependencies[0].DstStageMask  = PIPELINE_STAGE_FLAG_PIXEL_SHADER;
Dependencies[0].SrcAccessMask = ACCESS_FLAG_RENDER_TARGET_WRITE;
Dependencies[0].DstAccessMask = ACCESS_FLAG_SHADER_READ;
```

Execution dependencies is a very complicated topic and is beyond the scope of this tutorial.

### Creating the Render Pass Object

Finally, when we have all pieces that describe the render pass,
we can populate the `RenderPassDesc` structure and create the render
pass object:

```cpp
RenderPassDesc RPDesc;
RPDesc.Name            = "Deferred shading render pass desc";
RPDesc.AttachmentCount = _countof(Attachments);
RPDesc.pAttachments    = Attachments;
RPDesc.SubpassCount    = _countof(Subpasses);
RPDesc.pSubpasses      = Subpasses;
RPDesc.DependencyCount = _countof(Dependencies);
RPDesc.pDependencies   = Dependencies;

m_pDevice->CreateRenderPass(RPDesc, &m_pRenderPass);
```

## Creating the PSO

Creating a pipeline state object that uses explicit render pass is
mostly the same as creating a PSO that uses render targets, with
one difference: the PSO description structure should use the `pRenderPass`
and `SubpassIndex` members:

```cpp
PSOCreateInfo.GraphicsPipeline.pRenderPass  = m_pRenderPass;
PSOCreateInfo.GraphicsPipeline.SubpassIndex = 0;
```

Note that when `pRenderPass` is not null, all render target
formats as well as depth-stencil format must be `TEX_FORMAT_UNKNOWN`,
and the number of render targets must be 0.

### Using Subpass Attachments in the Shader

The only backend that currently natively supports input attachments is Vulkan,
and subpass attachments are only supported in GLSL. To define subpass inputs in
the shader, use the following syntax:

```glsl
layout(input_attachment_index = 0, binding = 0) uniform highp subpassInput g_SubpassInputColor;
layout(input_attachment_index = 1, binding = 1) uniform highp subpassInput g_SubpassInputDepthZ;
```

In the shader, use `subpassLoad` function to load the subpass data:

```glsl
float Depth = subpassLoad(g_SubpassInputDepthZ).r;
vec3  Color = subpassLoad(g_SubpassInputColor).rgb;
```

Note that `subpassLoad` function does not take the position because it is implicitly defined by
the position of the current fragment.


In all other backends input attachments should be defined as regular textures and accessed
appropriately:

```hlsl
Texture2D<float4> g_SubpassInputColor;
SamplerState      g_SubpassInputColor_sampler;

Texture2D<float4> g_SubpassInputDepthZ;
SamplerState      g_SubpassInputDepthZ_sampler;

...

float  Depth = g_SubpassInputDepthZ.Load(int3(PSIn.Pos.xy, 0)).r
float3 Color = g_SubpassInputColor.Load(int3(PSIn.Pos.xy, 0)).rgb;
```

## Creating the Framebuffer

The final part of the render passes API is the framebuffer. The framebuffer encapsulates
the actual textures that will be used as attachments in the render pass. The framebuffer
must use exactly same number of attachments as the render pass, and the the texture view
formats must match exactly the corresponding render pass attachment formats.
To create a framebuffer, prepare `FramebufferDesc` structure and call
`IRenderDevice::CreateFramebuffer` method:

```cpp
ITextureView* pAttachments[] =
{
    pColorBuffer->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET),
    pDepthZBuffer->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET),
    pDepthBuffer->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL),
    pDstRenderTarget
};

FramebufferDesc FBDesc;
FBDesc.Name            = "G-buffer framebuffer";
FBDesc.pRenderPass     = m_pRenderPass;
FBDesc.AttachmentCount = _countof(pAttachments);
FBDesc.ppAttachments   = pAttachments;

RefCntAutoPtr<IFramebuffer> pFramebuffer;
m_pDevice->CreateFramebuffer(FBDesc, &pFramebuffer);
```

## Using Render Passes

There are three main subpass commands: `BeginRenderPass`, `NextSubpass`,
and `EndRenderPass`.

`BeginRenderPass` as the name suggests begins a render pass and starts
the first subpass. To begin a render pass, besides the render pass itself
we also need to specify a framebuffer, as well as clear values for all attachments
that use `ATTACHMENT_LOAD_OP_CLEAR` load operation:

```cpp
BeginRenderPassAttribs RPBeginInfo;
RPBeginInfo.pRenderPass  = m_pRenderPass;
RPBeginInfo.pFramebuffer = pFramebuffer;

OptimizedClearValue ClearValues[4];

// Color
ClearValues[0].Color[0] = 0.f;
ClearValues[0].Color[1] = 0.f;
ClearValues[0].Color[2] = 0.f;
ClearValues[0].Color[3] = 0.f;

// Depth Z
ClearValues[1].Color[0] = 1.f;
ClearValues[1].Color[1] = 1.f;
ClearValues[1].Color[2] = 1.f;
ClearValues[1].Color[3] = 1.f;

// Depth buffer
ClearValues[2].DepthStencil.Depth = 1.f;

// Final color buffer
ClearValues[3].Color[0] = 0.0625f;
ClearValues[3].Color[1] = 0.0625f;
ClearValues[3].Color[2] = 0.0625f;
ClearValues[3].Color[3] = 0.f;

RPBeginInfo.pClearValues        = ClearValues;
RPBeginInfo.ClearValueCount     = _countof(ClearValues);
RPBeginInfo.StateTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
m_pImmediateContext->BeginRenderPass(RPBeginInfo);
```

In the first subpass of our render pass, we render the scene. Then
we call `NextSubpass` to move to the lighting subpass and draw
the lights. Finally, we call `EndRenderPass` to finish the render pass:

```cpp
m_pImmediateContext->BeginRenderPass(RPBeginInfo);
DrawScene();
m_pImmediateContext->NextSubpass();
ApplyLighting();
m_pImmediateContext->EndRenderPass();
```

A very important aspect of render passes that needs to be mentioned again is that
state transitions are not allowed between `BeginRenderPass` and `EndRenderPass` calls.
The tutorial explicitly transitions all resources it uses to correct state during the initialization:

```cpp
StateTransitionDesc Barriers[] =
{
    {m_pShaderConstantsCB, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_CONSTANT_BUFFER, true},
    {m_CubeVertexBuffer, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_VERTEX_BUFFER, true},
    {m_CubeIndexBuffer, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_INDEX_BUFFER, true},
    {m_pLightsBuffer, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_VERTEX_BUFFER, true},
    {m_CubeTextureSRV->GetTexture(), RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_SHADER_RESOURCE, true} //
};
m_pImmediateContext->TransitionResourceStates(_countof(Barriers), Barriers);
```

and then uses `RESOURCE_STATE_TRANSITION_MODE_VERIFY` mode with every call that requires state transition mode.

## Further Reading

Diligent Engine's render passes API largely resembles Vulkan, so
[Vulkan spec](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#renderpass)
will provide the most comprehensive description.
ARM software maintains a list of [Vulkan best practices for mobile developers](https://github.com/ARM-software/vulkan_best_practice_for_mobile_developers)
that include 
[attachment load/store operations](https://github.com/ARM-software/vulkan_best_practice_for_mobile_developers/blob/master/samples/performance/render_passes/render_passes_tutorial.md),
[attachment layouts transitions](https://github.com/ARM-software/vulkan_best_practice_for_mobile_developers/blob/master/samples/performance/layout_transitions/layout_transitions_tutorial.md),
and [subpasses](https://github.com/ARM-software/vulkan_best_practice_for_mobile_developers/blob/master/samples/performance/render_subpasses/render_subpasses_tutorial.md).
