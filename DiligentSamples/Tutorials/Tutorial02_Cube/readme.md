# Tutorial02 - Cube

This tutorial builds on top of Tutorial01 and demonstrates how to render an actual 3D object, a cube. 
It shows how to load shaders from files, create and use vertex, index and uniform buffers.

![](Animation_Large.gif)

## Shaders

This tutorial uses a little bit more complicated vertex shader that reads two attributes from the
input vertex buffer, a `float3` position and a `float4` color:

```hlsl
struct VSInput
{
    float3 Pos   : ATTRIB0;
    float4 Color : ATTRIB1;
};

void main(in  VSInput VSIn,
          out PSInput PSIn) 
```

By convention, **vertex shader inputs should be labeled as ATTRIBn, where n is the attribute number.** The 
attributes must match the input layout defined in the pipeline state object.
Note that if separate shader objects are not supported (this is only the case for old GLES3.0 devices), vertex 
shader output variable name must match exactly the name of the pixel shader input variable. If the variable has 
structure type (like in this example), the structure declarations must also be indentical.
The shader also uses a world-view-projection matrix defined in a constant (uniform) buffer called `Constants` to
transform vertex positions:

```hlsl
cbuffer Constants
{
    float4x4 g_WorldViewProj;
};
```

The full vertex shader source code is as follows:

```hlsl
cbuffer Constants
{
    float4x4 g_WorldViewProj;
};

struct VSInput
{
    float3 Pos   : ATTRIB0;
    float4 Color : ATTRIB1;
};

struct PSInput 
{ 
    float4 Pos   : SV_POSITION; 
    float4 Color : COLOR0; 
};

void main(in  VSInput VSIn,
          out PSInput PSIn) 
{
    PSIn.Pos   = mul( float4(VSIn.Pos,1.0), g_WorldViewProj);
    PSIn.Color = VSIn.Color;
}
```

Similar to Tutorial01, pixel (fragment) shader simply interpolates vertex colors.

## Initializing the Pipeline State

In this tutorial, we will be using depth buffer, so besides color output, we need to specify
the format of the depth output in the `PSOCreateInfo`:

```cpp
PSOCreateInfo.GraphicsPipeline.NumRenderTargets             = 1;
PSOCreateInfo.GraphicsPipeline.RTVFormats[0]                = pSwapChain->GetDesc().ColorBufferFormat;
PSOCreateInfo.GraphicsPipeline.DSVFormat                    = pSwapChain->GetDesc().DepthBufferFormat;
PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = True;
```

Also, we will enable back-face culling:

```cpp
PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_BACK;
```

In this tutorial, we create shaders from files rather than from the source code strings. Diligent Engine
accesses platform-specific files through `IShaderSourceInputStreamFactory` interface. The engine
provides default implementation for the interface that should be sufficient in most cases.

```cpp
RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
m_pEngineFactory->CreateDefaultShaderSourceStreamFactory(nullptr, &pShaderSourceFactory);
ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;
```

`CreateDefaultShaderSourceStreamFactory` method optionally takes a semicolon-separated list of
directories where source files will be looked up. 

Other than using the shader source factory instead of the source code string, vertex shader
initialization is the same as in Tutorial01:

```cpp
ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
ShaderCI.EntryPoint      = "main";
ShaderCI.Desc.Name       = "Cube VS";
ShaderCI.FilePath        = "cube.vsh";
pDevice->CreateShader(ShaderCI, &pVS);
```

This time our shader uses a resource - a uniform buffer. So the first step is to create the buffer
that will hold the transformation matrix. To create a buffer, populate `BufferDesc` structure:

```cpp
BufferDesc CBDesc;
CBDesc.Name           = "VS constants CB";
CBDesc.uiSizeInBytes  = sizeof(float4x4);
CBDesc.Usage          = USAGE_DYNAMIC;
CBDesc.BindFlags      = BIND_UNIFORM_BUFFER;
CBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
pDevice->CreateBuffer( CBDesc, nullptr, &m_VSConstants );
```

Usage and Bind flags are designed after [D3D11 Usage](https://msdn.microsoft.com/en-us/library/windows/desktop/ff476259(v=vs.85).aspx) 
and [D3D11 Bind Flags](https://msdn.microsoft.com/en-us/library/windows/desktop/ff476085(v=vs.85).aspx).

Since our vertex shader reads attributes from the vertex buffer, the pipeline state 
must define how the attributes are fetched from the buffer. The two attributes are defined
as follows:

```cpp
LayoutElement LayoutElems[] =
{
    // Attribute 0 - vertex position
    LayoutElement{0, 0, 3, VT_FLOAT32, False},
    // Attribute 1 - vertex color
    LayoutElement{1, 0, 4, VT_FLOAT32, False}
};
PSOCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = LayoutElems;
PSOCreateInfo.GraphicsPipeline.InputLayout.NumElements    = _countof(LayoutElems);
```

Our shader has one variable that needs to be bound by the application, a uniform buffer `Constants`.
Shader variables can be assigned one of three types, static, dynamic, or mutable. Please read
[this post](http://diligentgraphics.com/2016/03/23/resource-binding-model-in-diligent-engine-2-0/)
for details. If no explicit type is provided for a variable, default type will be used:

```cpp
PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
```

`Constants` uniform buffer is a static resource variable. Static variables are bound directly to the pipeline state and 
cannot be changed once bound:

```cpp
m_pPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "Constants")->Set(m_VSConstants);
```

Notice that only the binding cannot be changed. The contents of the buffer is modifiable.

## Creating Vertex and Index Buffers

To create a vertex buffer, we first prepare the data to fill the buffer with. Our
vertex layout corresponds to the following structure:

```cpp
struct Vertex
{
    float3 pos;
    float4 color;
};
```

Our vertex buffer will contain 8 vertices. Every vertex will have position and color:

```cpp
//      (-1,+1,+1)________________(+1,+1,+1) 
//               /|              /|
//              / |             / |
//             /  |            /  |
//            /   |           /   |
//(-1,-1,+1) /____|__________/(+1,-1,+1)
//           |    |__________|____| 
//           |   /(-1,+1,-1) |    /(+1,+1,-1)
//           |  /            |   /
//           | /             |  /
//           |/              | /
//           /_______________|/ 
//        (-1,-1,-1)       (+1,-1,-1)
// 

Vertex CubeVerts[8] =
{
    {float3(-1,-1,-1), float4(1,0,0,1)},
    {float3(-1,+1,-1), float4(0,1,0,1)},
    {float3(+1,+1,-1), float4(0,0,1,1)},
    {float3(+1,-1,-1), float4(1,1,1,1)},

    {float3(-1,-1,+1), float4(1,1,0,1)},
    {float3(-1,+1,+1), float4(0,1,1,1)},
    {float3(+1,+1,+1), float4(1,0,1,1)},
    {float3(+1,-1,+1), float4(0.2f,0.2f,0.2f,1)},
};
```

Similar to unifrom buffer, to create a vertex buffer, we populate `BufferDesc` structure. Since
the data in the buffer will never change, we create the buffer with immutable usage (`USAGE_IMMUTABLE`)
and provide initial data to `CreateBuffer()`:

```cpp
BufferDesc VertBuffDesc;
VertBuffDesc.Name          = "Cube vertex buffer";
VertBuffDesc.Usage         = USAGE_IMMUTABLE;
VertBuffDesc.BindFlags     = BIND_VERTEX_BUFFER;
VertBuffDesc.uiSizeInBytes = sizeof(CubeVerts);
BufferData VBData;
VBData.pData    = CubeVerts;
VBData.DataSize = sizeof(CubeVerts);
pDevice->CreateBuffer(VertBuffDesc, &VBData, &m_CubeVertexBuffer);
```

Index buffer is initialized in a very similar fashion.

## Creating Shader Resource Binding Object

Since our fragment shader uses shader resources (constant buffer), we need to create
a shader resource binding object that will manage all requried resource bindings:

```cpp
m_pPSO->CreateShaderResourceBinding(&m_pSRB, true);
```

The second parameter tells the engine to initialize all static resources bindings
in the created SRB object.


## Rendering

There are few changes that we need to make to our rendering procedure compared to Tutorial01.
First, we need to update our transformation matrix. Since we created our constant buffer
as dynamic buffer, it can be mapped. Diligent Engine provides `MapHelper` template class
that facilitates buffer mapping:

```cpp
{
    // Map the buffer and write current world-view-projection matrix
    MapHelper<float4x4> CBConstants(m_pImmediateContext, m_VSConstants, MAP_WRITE, MAP_FLAG_DISCARD);
    *CBConstants = transposeMatrix(m_WorldViewProjMatrix);
}
```

Second, we need to bind vertex and index buffer to the GPU pipeline.
We use `RESOURCE_STATE_TRANSITION_MODE_TRANSITION` to let the engine automatically
transition the buffers to required states.

```cpp
Uint32 offset = 0;
IBuffer *pBuffs[] = {m_CubeVertexBuffer};
m_pImmediateContext->SetVertexBuffers(0, 1, pBuffs, &offset, RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
                                      SET_VERTEX_BUFFERS_FLAG_RESET);
m_pImmediateContext->SetIndexBuffer(m_CubeIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
```

Next step is very important: we need to commit shader resources:

```cpp
m_pImmediateContext->CommitShaderResources(m_pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
```

The first argument of `CommitShaderResources()` is the shader resource binding object. 
The `RESOURCE_STATE_TRANSITION_MODE_TRANSITION` tells the system that resources need to be
transitioned to correct states by the engine. Transitioning resources introduces some overhead
and can be avoided when it is known that resources are already in correct states.


Finally, this time the draw call is an indexed one:

```cpp
DrawIndexedAttribs DrawAttrs; // This is an indexed draw call
DrawAttrs.IndexType  = VT_UINT32; // Index type
DrawAttrs.NumIndices = 36;
// Verify the state of vertex and index buffers as well as consistence of 
// render targets and correctness of draw command arguments
DrawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;
m_pImmediateContext->DrawIndexed(DrawAttrs);
```

We want the engine to verify that the states are correct,
so we use `DRAW_FLAG_VERIFY_ALL` flag.
