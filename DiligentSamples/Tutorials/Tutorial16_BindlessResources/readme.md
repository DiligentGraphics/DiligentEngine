# Tutorial16 - Bindless Resources

This tutorial shows how to implement bindless resources, a technique that leverages
dynamic shader resource indexing feature enabled by the next-gen APIs to significantly
improve rendering performance.

![](Animation_Large.gif)

In old APIs (Direct3D11, OpenGL/GLES), when an application needed to render multiple objects
using different shader resources, it had to run the following loop:

1. Bind required resources (textures, constant buffers, etc.)
2. Issue draw commands
3. Go to step 1 and repeat operations for the next object.

There are multiple rechinques to make this loop more efficient such as instancing (see 
[Tutorial 4](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial04_Instancing)),
texture arrays (see [Tutorial 5](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial05_TextureArray)),
etc. All these methods, however, are very limited.

Next-generation APIs (Direct3D12, Vulkan, Metal) enable a more efficient way: instead of binding new resources
every time next object is rendered, all required resources can be bound once, while shaders can dynamically
access required resources using the draw call information. 

## Shaders

This tutorial is based on [Tutorial 5](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial05_TextureArray).
However, while original tutorial used a texture array object that requried all array slices to be identical (same size, format,
number of mip levels, etc.), this tutorial binds textures as an array of shader resources. Unlike texture array object, all resources
in an array of resources may have completely different parameters. The pixel shader is able to dynamically select the texture to sample
using the texture index it receives from the vertex shader (that reads it from the instance data buffer):

```hlsl
#define NUM_TEXTURES 4
Texture2D     g_Texture[NUM_TEXTURES];
SamplerState  g_Texture_sampler;

struct PSInput 
{ 
    float4 Pos      : SV_POSITION; 
    float2 UV       : TEX_COORD; 
    uint   TexIndex : TEX_ARRAY_INDEX;
};

struct PSOutput
{
    float4 Color : SV_TARGET;
};

void main(in  PSInput  PSIn,
          out PSOutput PSOut)
{
    PSOut.Color = g_Texture[PSIn.TexIndex].Sample(g_Texture_sampler, PSIn.UV);
}
```

## Pipeline State and Shader Resource Binding

There are no differences in pipeline state initialization for bindless shaders.
Shader resource binding objects are also initialized the same way, with the only difference
that we bind an array of objects using `SetArray` method:

```cpp
m_pBindlessPSO->CreateShaderResourceBinding(&m_BindlessSRB, true);
m_BindlessSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture")->SetArray(pTexSRVs, 0, NumTextures);
```

## Rendering

As we discussed earlier, to render all objects, we bind all resources once: 

```cpp
m_pImmediateContext->SetPipelineState(m_pBindlessPSO);
m_pImmediateContext->CommitShaderResources(m_BindlessSRB, RESOURCE_STATE_TRANSITION_MODE_VERIFY);
```

And then render each object using its geometry properties. Texture index will be fetched
from the instance data buffer and passed over to the pixel shader.

```cpp
for (int i=0; i < NumObjects; ++i)
{
    const auto& Geometry = m_Geometries[m_GeometryType[i]];

    DrawIndexedAttribs DrawAttrs;
    DrawAttrs.IndexType             = VT_UINT32;
    DrawAttrs.NumIndices            = Geometry.NumIndices;
    DrawAttrs.FirstIndexLocation    = Geometry.FirstIndex;
    DrawAttrs.FirstInstanceLocation = static_cast<Uint32>(i);
    DrawAttrs.Flags = DRAW_FLAG_VERIFY_ALL | DRAW_FLAG_DYNAMIC_RESOURCE_BUFFERS_INTACT;
    m_pImmediateContext->DrawIndexed(DrawAttrs);
}
```

Notice that we use `DRAW_FLAG_DYNAMIC_RESOURCE_BUFFERS_INTACT` flag. This flag informs the engine
that none of the dynamic buffers have been modified since the last draw command, which saves extra work
the engine would have to perform otherwise.
