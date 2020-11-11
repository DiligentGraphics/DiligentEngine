# Diligent Core [![Tweet](https://img.shields.io/twitter/url/http/shields.io.svg?style=social)](https://twitter.com/intent/tweet?text=An%20easy-to-use%20cross-platform%20graphics%20library%20that%20takes%20full%20advantage%20of%20%23Direct3D12%20and%20%23VulkanAPI&url=https://github.com/DiligentGraphics/DiligentEngine) <img src="media/diligentgraphics-logo.png" height=64 align="right" valign="middle">

This module implements [Diligent Engine](https://github.com/DiligentGraphics/DiligentEngine)'s core functionality: Direct3D11, Direct3D12,
OpenGL, OpenGLES, and Vulkan rendering backends as well as basic platform-specific utilities. It is self-contained and can be built by its own.
The module's cmake script defines a number of variables that are required to generate build files for other modules,
so it must always be handled first.

| Platform             | Build Status  |
| ---------------------| ------------- |
|<img src="media/windows-logo.png" width=24 valign="middle"> Win32               | [![Build Status](https://ci.appveyor.com/api/projects/status/github/DiligentGraphics/DiligentCore?svg=true)](https://ci.appveyor.com/project/DiligentGraphics/diligentcore) |
|<img src="media/uwindows-logo.png" width=24 valign="middle"> Universal Windows  | [![Build Status](https://ci.appveyor.com/api/projects/status/github/DiligentGraphics/DiligentCore?svg=true)](https://ci.appveyor.com/project/DiligentGraphics/diligentcore) |
|<img src="media/linux-logo.png" width=24 valign="middle"> Linux                 | [![Build Status](https://travis-ci.org/DiligentGraphics/DiligentCore.svg?branch=master)](https://travis-ci.org/DiligentGraphics/DiligentCore)      |
|<img src="media/macos-logo.png" width=24 valign="middle"> MacOS                 | [![Build Status](https://travis-ci.org/DiligentGraphics/DiligentCore.svg?branch=master)](https://travis-ci.org/DiligentGraphics/DiligentCore)      |
|<img src="media/apple-logo.png" width=24 valign="middle"> iOS                   | [![Build Status](https://travis-ci.org/DiligentGraphics/DiligentCore.svg?branch=master)](https://travis-ci.org/DiligentGraphics/DiligentCore)      |

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](License.txt)
[![Chat on gitter](https://badges.gitter.im/gitterHQ/gitter.png)](https://gitter.im/diligent-engine)
[![Chat on Discord](https://img.shields.io/discord/730091778081947680?logo=discord)](https://discord.gg/t7HGBK7)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/bb1c00eacb1740d68339d3a45f4c5756)](https://www.codacy.com/manual/DiligentGraphics/DiligentCore?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=DiligentGraphics/DiligentCore&amp;utm_campaign=Badge_Grade)

# Table of Contents

- [Clonning the Repository](#clonning)
- [API Basics](#api_basics)
  - [Initializing the Engine](#initialization)
    - [Win32](#initialization_win32)
    - [Universal Windows Platform](#initialization_uwp)
    - [Linux](#initialization_linux)
    - [MacOS](#initialization_macos)
    - [iOS](#initialization_ios)
    - [Attaching to Already Initialized Graphics API](#initialization_attaching)
    - [Destroying the Engine](#initialization_destroying)
  - [Creating Resources](#creating_resources)
  - [Creating Shaders](#creating_shaders)
  - [Initializing Pipeline State](#initializing_pso)
    - [Pipeline Resource Layout](#pipeline_resource_layout)
  - [Binding Shader Resources](#binding_resources)
  - [Setting the Pipeline State and Invoking Draw Command](#draw_command)
- [Low-level API interoperability](#low_level_api_interoperability)
- [Repository structure](#repository_structure)
- [Contributing](#contributing)
- [License](https://github.com/DiligentGraphics/DiligentCore#license)
- [References](#references)
- [Release History](#release_history)


<a name="clonning"></a>
# Clonning the Repository

To get the repository and all submodules, use the following command:

```
git clone --recursive https://github.com/DiligentGraphics/DiligentCore.git
```

To build the module, see 
[build instructions](https://github.com/DiligentGraphics/DiligentEngine/blob/master/README.md#build-and-run-instructions) 
in the master repository.
 
<a name="api_basics"></a>
# API Basics

<a name="initialization"></a>
## Initializing the Engine

Before you can use any functionality provided by the engine, you need to create a render device, an immediate context and a swap chain.

<a name="initialization_win32"></a>
### Win32
On Win32 platform, you can create OpenGL, Direct3D11, Direct3D12 or Vulkan device as shown below:

```cpp
void InitializeDiligentEngine(HWND NativeWindowHandle)
{
    SwapChainDesc SCDesc;
    SCDesc.SamplesCount = 1;
    switch (m_DeviceType)
    {
        case DeviceType::D3D11:
        {
            EngineD3D11CreateInfo EngineCI;
#if ENGINE_DLL
            GetEngineFactoryD3D11Type GetEngineFactoryD3D11 = nullptr;
            // Load the dll and import GetEngineFactoryD3D11() function
            LoadGraphicsEngineD3D11(GetEngineFactoryD3D11);
#endif
            auto* pFactoryD3D11 = GetEngineFactoryD3D11();
            pFactoryD3D11->CreateDeviceAndContextsD3D11(EngineCI, &m_pDevice, &m_pImmediateContext);
            pFactoryD3D11->CreateSwapChainD3D11(m_pDevice, m_pImmediateContext,
			                        SCDesc, NativeWindowHandle, &m_pSwapChain);
        }
        break;

        case DeviceType::D3D12:
        {
#if ENGINE_DLL
            GetEngineFactoryD3D12Type GetEngineFactoryD3D12 = nullptr;
            // Load the dll and import GetEngineFactoryD3D12() function
            LoadGraphicsEngineD3D12(GetEngineFactoryD3D12);
#endif
            EngineD3D12CreateInfo EngineCI;
            auto* pFactoryD3D12 = GetEngineFactoryD3D12();
            pFactoryD3D12->CreateDeviceAndContextsD3D12(EngineCI, &m_pDevice, &m_pImmediateContext);
            pFactoryD3D12->CreateSwapChainD3D12(m_pDevice, m_pImmediateContext,
			                        SCDesc, NativeWindowHandle, &m_pSwapChain);
        }
        break;

    case DeviceType::OpenGL:
    {

#if ENGINE_DLL
        // Declare function pointer
        GetEngineFactoryOpenGLType GetEngineFactoryOpenGL = nullptr;
        // Load the dll and import GetEngineFactoryOpenGL() function
        LoadGraphicsEngineOpenGL(GetEngineFactoryOpenGL);
#endif
        auto* pFactoryOpenGL = GetEngineFactoryOpenGL();
        EngineGLCreateInfo EngineCI;
        EngineCI.Window.hWnd = NativeWindowHandle;
        pFactoryOpenGL->CreateDeviceAndSwapChainGL(
            EngineCI, &m_pDevice, &m_pImmediateContext, SCDesc, &m_pSwapChain);
    }
    break;

    case DeviceType::Vulkan:
    {
#if ENGINE_DLL
        GetEngineFactoryVkType GetEngineFactoryVk = nullptr;
        // Load the dll and import GetEngineFactoryVk() function
        LoadGraphicsEngineVk(GetEngineFactoryVk);
#endif
        EngineVkCreateInfo EngineCI;
        auto* pFactoryVk = GetEngineFactoryVk();
        pFactoryVk->CreateDeviceAndContextsVk(EngineCI, &m_pDevice, &m_pImmediateContext);
        pFactoryVk->CreateSwapChainVk(m_pDevice, m_pImmediateContext,
                                      SCDesc, NativeWindowHandle, &m_pSwapChain);
    }
    break;

    default:
        std::cerr << "Unknown device type";
    }
}
```

On Windows, the engine can be statically linked to the application or built as a separate DLL. In the former case,
factory functions `GetEngineFactoryOpenGL()`, `GetEngineFactoryD3D11()`, `GetEngineFactoryD3D12()`, and `GetEngineFactoryVk()`
can be called directly. In the latter case, you need to load the DLL into the process's address space using `LoadGraphicsEngineOpenGL()`,
`LoadGraphicsEngineD3D11()`, `LoadGraphicsEngineD3D12()`, or `LoadGraphicsEngineVk()` function. Each function loads appropriate
dynamic library and imports the functions required to initialize the engine. You need to include the following headers:

```cpp
#include "EngineFactoryD3D11.h"
#include "EngineFactoryD3D12.h"
#include "EngineFactoryOpenGL.h"
#include "EngineFactoryVk.h"

```

You also need to add the following directories to the include search paths:

* DiligentCore/Graphics/GraphicsEngineD3D11/interface
* DiligentCore/Graphics/GraphicsEngineD3D12/interface
* DiligentCore/Graphics/GraphicsEngineOpenGL/interface
* DiligentCore/Graphics/GraphicsEngineVulkan/interface

Also, enable Diligent namespace:

```cpp
using namespace Diligent;
```

`IEngineFactoryD3D11::CreateDeviceAndContextsD3D11()`, `IEngineFactoryD3D12::CreateDeviceAndContextsD3D12()`, and 
`IEngineFactoryVk::CreateDeviceAndContextsVk()` functions can also create a specified number of deferred contexts, which 
can be used for multi-threaded command recording. Deferred contexts can only be created during the initialization of the 
engine. The function populates an array of pointers to the contexts, where the immediate context goes at position 0,
followed by all deferred contexts.

For more details, take a look at 
[Tutorial00_HelloWin32.cpp](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial00_HelloWin32/src/Tutorial00_HelloWin32.cpp) 
file.

<a name="initialization_uwp"></a>
### Universal Windows Platform

On Universal Windows Platform, you can create Direct3D11 or Direct3D12 device. Only static linking is
currently supported, but dynamic linking can also be implemented. Initialization is performed the same
way as on Win32 Platform. The difference is that you first create the render device and device contexts by
calling `IEngineFactoryD3D11::CreateDeviceAndContextsD3D11()` or `IEngineFactoryD3D12::CreateDeviceAndContextsD3D12()`.
The swap chain is created later by a call to `IEngineFactoryD3D11::CreateSwapChainD3D11()` or `IEngineFactoryD3D12::CreateSwapChainD3D12()`.
Please look at
[SampleAppUWP.cpp](https://github.com/DiligentGraphics/DiligentSamples/blob/master/SampleBase/src/UWP/SampleAppUWP.cpp) 
file for more details.

<a name="initialization_linux"></a>
### Linux

On Linux platform, the engine supports OpenGL and Vulkan backends. Initialization of GL context on Linux is tightly
coupled with window creation. As a result, Diligent Engine does not initialize the context, but
attaches to the one initialized by the app. An example of the engine initialization on Linux can be found in
[Tutorial00_HelloLinux.cpp](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial00_HelloLinux/src/Tutorial00_HelloLinux.cpp).

<a name="initialization_macos"></a>
### MacOS

On MacOS, Diligent Engine supports OpenGL and Vulkan backends. Initialization of GL context on MacOS is
performed by the application, and the engine attaches to the context created by the app; see
[GLView.m](https://github.com/DiligentGraphics/DiligentTools/blob/master/NativeApp/Apple/Source/Classes/OSX/GLView.m)
for details. Vulkan backend is initialized similar to other platforms. See 
[MetalView.m](https://github.com/DiligentGraphics/DiligentTools/blob/master/NativeApp/Apple/Source/Classes/OSX/MetalView.m).

<a name="initialization_android"></a>
### Android

On Android, you can only create OpenGLES device. The following code snippet shows an example:

```cpp
auto* pFactoryOpenGL = GetEngineFactoryOpenGL();
EngineGLCreateInfo EngineCI;
EngineCI.Window.pAWindow = NativeWindowHandle;
pFactoryOpenGL->CreateDeviceAndSwapChainGL(
    EngineCI, &m_pDevice, &m_pContext, SCDesc, &m_pSwapChain);
IRenderDeviceGLES *pRenderDeviceOpenGLES;
pRenderDevice->QueryInterface(IID_RenderDeviceGLES, reinterpret_cast<IObject**>(&pRenderDeviceOpenGLES));
```

If engine is built as dynamic library, the library needs to be loaded by the native activity. The following code shows one possible way:

```java
static
{
    try{
        System.loadLibrary("GraphicsEngineOpenGL");
    } catch (UnsatisfiedLinkError e) {
        Log.e("native-activity", "Failed to load GraphicsEngineOpenGL library.\n" + e);
    }
}
```

<a name="initialization_ios"></a>
### iOS

iOS implementation only supports OpenGLES backend. Initialization of GL context on iOS is
performed by the application, and the engine attaches to the context initialized by the app; see
[EAGLView.m](https://github.com/DiligentGraphics/DiligentTools/blob/master/NativeApp/Apple/Source/Classes/iOS/EAGLView.m)
for details.

<a name="initialization_attaching"></a>
### Attaching to Already Initialized Graphics API

An alternative way to initialize the engine is to attach to existing D3D11/D3D12 device or OpenGL/GLES context.
Refer to [Native API interoperability](http://diligentgraphics.com/diligent-engine/native-api-interoperability/) for more details.

<a name="initialization_destroying"></a>
### Destroying the Engine

The engine performs automatic reference counting and shuts down when the last reference to an engine object is released.

<a name="creating_resources"></a>
## Creating Resources

Device resources are created by the render device. The two main resource types are buffers,
which represent linear memory, and textures, which use memory layouts optimized for fast filtering.
To create a buffer, you need to populate `BufferDesc` structure and call `IRenderDevice::CreateBuffer()`.
The following code creates a uniform (constant) buffer:

```cpp
BufferDesc BuffDesc;
BuffDesc.Name           = "Uniform buffer";
BuffDesc.BindFlags      = BIND_UNIFORM_BUFFER;
BuffDesc.Usage          = USAGE_DYNAMIC;
BuffDesc.uiSizeInBytes  = sizeof(ShaderConstants);
BuffDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
m_pDevice->CreateBuffer(BuffDesc, nullptr, &m_pConstantBuffer);
```

Similar, to create a texture, populate `TextureDesc` structure and call `IRenderDevice::CreateTexture()` as in the following example:

```cpp
TextureDesc TexDesc;
TexDesc.Name      = "My texture 2D";
TexDesc.Type      = TEXTURE_TYPE_2D;
TexDesc.Width     = 1024;
TexDesc.Height    = 1024;
TexDesc.Format    = TEX_FORMAT_RGBA8_UNORM;
TexDesc.Usage     = USAGE_DEFAULT;
TexDesc.BindFlags = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET | BIND_UNORDERED_ACCESS;
TexDesc.Name = "Sample 2D Texture";
m_pRenderDevice->CreateTexture(TexDesc, nullptr, &m_pTestTex);
```

There is only one function `CreateTexture()` that is capable of creating all types of textures. Type, format,
array size and all other parameters are specified by the members of the `TextureDesc` structure.

For every bind flag specified during the texture creation time, the texture object creates a default view.
Default shader resource view addresses the entire texture, default render target and depth stencil views reference
all array slices in the most detailed mip level, and unordered access view references the entire texture. To get a
default view from the texture, use `ITexture::GetDefaultView()` function. Note that this function does not increment
the reference counter on the returned interface. You can create additional texture views using `ITexture::CreateView()`.
Use `IBuffer::CreateView()` to create additional views of a buffer.

<a name="creating_shaders"></a>
## Creating Shaders

To create a shader, populate `ShaderCreateInfo` structure:

```cpp
ShaderCreateInfo ShaderCI;
```

There are two ways to create a shader. The first way is to provide a pointer to the shader source code through 
`ShaderCreateInfo::Source` member. The second way is to provide a file name. Graphics Engine is entirely decoupled
from the platform. Since the host file system is platform-dependent, the structure exposes
`ShaderCreateInfo::pShaderSourceStreamFactory` member that is intended to give the engine access to the file system.
If you provided the source file name, you must also provide a non-null pointer to the shader source stream factory.
If the shader source contains any `#include` directives, the source stream factory will also be used to load these
files. The engine provides default implementation for every supported platform that should be sufficient in most cases.
You can however define your own implementation.

An important member is `ShaderCreateInfo::SourceLanguage`. The following are valid values for this member:

* `SHADER_SOURCE_LANGUAGE_DEFAULT` - The shader source format matches the underlying graphics API: HLSL for D3D11 or D3D12 mode, and GLSL for OpenGL, OpenGLES, and Vulkan modes.
* `SHADER_SOURCE_LANGUAGE_HLSL`    - The shader source is in HLSL. For OpenGL and OpenGLES modes, the source code will be 
                                     converted to GLSL. In Vulkan back-end, the code will be compiled to SPIRV directly.
* `SHADER_SOURCE_LANGUAGE_GLSL`    - The shader source is in GLSL.

Other members of the `ShaderCreateInfo` structure define shader include search directories, shader macro definitions,
shader entry point and other parameters.

```cpp
ShaderMacroHelper Macros;
Macros.AddShaderMacro("USE_SHADOWS", 1);
Macros.AddShaderMacro("NUM_SHADOW_SAMPLES", 4);
Macros.Finalize();
ShaderCI.Macros = Macros;
```

When everything is ready, call `IRenderDevice::CreateShader()` to create the shader object:

```cpp
ShaderCreateInfo ShaderCI;
ShaderCI.Desc.Name         = "MyPixelShader";
ShaderCI.FilePath          = "MyShaderFile.fx";
ShaderCI.EntryPoint        = "MyPixelShader";
ShaderCI.Desc.ShaderType   = SHADER_TYPE_PIXEL;
ShaderCI.SourceLanguage    = SHADER_SOURCE_LANGUAGE_HLSL;
const auto* SearchDirectories = "shaders;shaders\\inc;";
RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
m_pEngineFactory->CreateDefaultShaderSourceStreamFactory(SearchDirectories, &pShaderSourceFactory);
ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;
RefCntAutoPtr<IShader> pShader;
m_pDevice->CreateShader(ShaderCI, &pShader);
```

<a name="initializing_pso"></a>
## Initializing Pipeline State

Diligent Engine follows Direct3D12/Vulkan style to configure the graphics/compute pipeline. One monolithic Pipelines State Object (PSO)
encompasses all required states (all shader stages, input layout description, depth stencil, rasterizer and blend state
descriptions etc.). To create a pipeline state object, define an instance of `GraphicsPipelineStateCreateInfo` structure:

```cpp
GraphicsPipelineStateCreateInfo PSOCreateInfo;
PipelineStateDesc&              PSODesc = PSOCreateInfo.PSODesc;

PSODesc.Name = "My pipeline state";
```

Describe the pipeline specifics such as if the pipeline is a compute pipeline, number and format
of render targets as well as depth-stencil format:

```cpp
// This is a graphics pipeline
PSODesc.PipelineType                            = PIPELINE_TYPE_GRAPHICS;
PSOCreateInfo.GraphicsPipeline.NumRenderTargets = 1;
PSOCreateInfo.GraphicsPipeline.RTVFormats[0]    = TEX_FORMAT_RGBA8_UNORM_SRGB;
PSOCreateInfo.GraphicsPipeline.DSVFormat        = TEX_FORMAT_D32_FLOAT;
```

Initialize depth-stencil state description structure DepthStencilStateDesc. Note that the constructor initializes
the members with default values and you may only set the ones that are different from default.

```cpp
// Init depth-stencil state
DepthStencilStateDesc& DepthStencilDesc = PSOCreateInfo.GraphicsPipeline.DepthStencilDesc;
DepthStencilDesc.DepthEnable            = true;
DepthStencilDesc.DepthWriteEnable       = true;
```

Initialize blend state description structure `BlendStateDesc`:

```cpp
// Init blend state
BlendStateDesc& BSDesc = PSOCreateInfo.GraphicsPipeline.BlendDesc;
BSDesc.IndependentBlendEnable = False;
auto &RT0 = BSDesc.RenderTargets[0];
RT0.BlendEnable           = True;
RT0.RenderTargetWriteMask = COLOR_MASK_ALL;
RT0.SrcBlend              = BLEND_FACTOR_SRC_ALPHA;
RT0.DestBlend             = BLEND_FACTOR_INV_SRC_ALPHA;
RT0.BlendOp               = BLEND_OPERATION_ADD;
RT0.SrcBlendAlpha         = BLEND_FACTOR_SRC_ALPHA;
RT0.DestBlendAlpha        = BLEND_FACTOR_INV_SRC_ALPHA;
RT0.BlendOpAlpha          = BLEND_OPERATION_ADD;
```

Initialize rasterizer state description structure `RasterizerStateDesc`:

```cpp
// Init rasterizer state
RasterizerStateDesc& RasterizerDesc = PSOCreateInfo.GraphicsPipeline.RasterizerDesc;
RasterizerDesc.FillMode              = FILL_MODE_SOLID;
RasterizerDesc.CullMode              = CULL_MODE_NONE;
RasterizerDesc.FrontCounterClockwise = True;
RasterizerDesc.ScissorEnable         = True;
RasterizerDesc.AntialiasedLineEnable = False;
```

Initialize input layout description structure `InputLayoutDesc`:

```cpp
// Define input layout
InputLayoutDesc& Layout = PSOCreateInfo.GraphicsPipeline.InputLayout;
LayoutElement LayoutElems[] =
{
    LayoutElement( 0, 0, 3, VT_FLOAT32, False ),
    LayoutElement( 1, 0, 4, VT_UINT8,   True ),
    LayoutElement( 2, 0, 2, VT_FLOAT32, False ),
};
Layout.LayoutElements = LayoutElems;
Layout.NumElements    = _countof(LayoutElems);
```

Define primitive topology and set shader pointers:

```cpp
// Define shader and primitive topology
PSOCreateInfo.GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
PSOCreateInfo.pVS = m_pVS;
PSOCreateInfo.pPS = m_pPS;
```

<a name="pipeline_resource_layout"></a>
### Pipeline Resource Layout

Pipeline resource layout informs the engine how the application is going to use different shader resource variables.
To allow grouping of resources based on the frequency of expected change, Diligent Engine introduces
classification of shader variables:

* **Static variables** (`SHADER_RESOURCE_VARIABLE_TYPE_STATIC`) are variables that are expected to be set only once. They may not be changed once a resource is bound to the variable. Such variables are intended to hold global constants such as camera attributes or global light attributes constant buffers.
* **Mutable variables** (`SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE`) define resources that are expected to change on a per-material frequency. Examples may include diffuse textures, normal maps etc.
* **Dynamic variables** (`SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC`) are expected to change frequently and randomly.

To define variable types, prepare an array of `ShaderResourceVariableDesc` structures and
initialize `PSODesc.ResourceLayout.Variables` and `PSODesc.ResourceLayout.NumVariables` members. Also
`PSODesc.ResourceLayout.DefaultVariableType` can be used to set the type that will be used if a variable name is not provided.

```cpp
ShaderResourceVariableDesc ShaderVars[] =
{
    {SHADER_TYPE_PIXEL, "g_StaticTexture",  SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
    {SHADER_TYPE_PIXEL, "g_MutableTexture", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
    {SHADER_TYPE_PIXEL, "g_DynamicTexture", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
};
PSODesc.ResourceLayout.Variables           = ShaderVars;
PSODesc.ResourceLayout.NumVariables        = _countof(ShaderVars);
PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
```

When creating a pipeline state, textures can be permanently assigned static samplers. If a static sampler is assigned to a texture,
it will always be used instead of the one initialized in the texture shader resource view. To define static samplers,
prepare an array of `StaticSamplerDesc` structures and intialize `PSODesc.ResourceLayout.StaticSamplers` and
`PSODesc.ResourceLayout.NumStaticSamplers` members. Notice that static samplers can be assigned to a texture variable of any type,
not necessarily static, so that the texture binding can be changed at run-time, while the sampler will stay immutable.
It is highly recommended to use static samplers whenever possible.

```cpp
StaticSamplerDesc StaticSampler;
StaticSampler.ShaderStages   = SHADER_TYPE_PIXEL;
StaticSampler.Desc.MinFilter = FILTER_TYPE_LINEAR;
StaticSampler.Desc.MagFilter = FILTER_TYPE_LINEAR;
StaticSampler.Desc.MipFilter = FILTER_TYPE_LINEAR;
StaticSampler.TextureName    = "g_MutableTexture";
PSODesc.ResourceLayout.NumStaticSamplers = 1;
PSODesc.ResourceLayout.StaticSamplers    = &StaticSampler;
```

When all required fields of PSO description structure are set, call `IRenderDevice::CreateGraphicsPipelineState()`
to create the PSO object:

```cpp
m_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pPSO);
```

<a name="binding_resources"></a>
## Binding Shader Resources

As mentioned above, [shader resource binding in Diligent Engine](http://diligentgraphics.com/2016/03/23/resource-binding-model-in-diligent-engine-2-0/)
is based on grouping variables in 3 different groups (static, mutable and dynamic). Static variables are variables that are
expected to be set only once. They may not be changed once a resource is bound to the variable. Such variables are intended
to hold global constants such as camera attributes or global light attributes constant buffers. They are bound directly to the
Pipeline State Object:

```cpp
m_pPSO->GetStaticShaderVariable(SHADER_TYPE_PIXEL, "g_tex2DShadowMap")->Set(pShadowMapSRV);
```

Mutable and dynamic variables are bound via a new object called Shader Resource Binding (SRB), which is created by the pipeline state
(`IPipelineState::CreateShaderResourceBinding()`):

```cpp
m_pPSO->CreateShaderResourceBinding(&m_pSRB, true);
```

The second parameter tells the system to initialize internal structures in the SRB object
that reference static variables in the PSO.

Dynamic and mutable resources are then bound through SRB object:

```cpp
m_pSRB->GetVariable(SHADER_TYPE_PIXEL,  "tex2DDiffuse")->Set(pDiffuseTexSRV);
m_pSRB->GetVariable(SHADER_TYPE_VERTEX, "cbRandomAttribs")->Set(pRandomAttrsCB);
```

The difference between mutable and dynamic resources is that mutable ones can only be set once for every instance
of a shader resource binding. Dynamic resources can be set multiple times. It is important to properly set the variable type as
this affects performance. Static variables are generally most efficient, followed by mutable. Dynamic variables are most expensive
from performance point of view.

An alternative way to bind shader resources is to create `IResourceMapping` interface that maps resource literal names to the
actual resources:

```cpp
ResourceMappingEntry Entries[] =
{
    {"g_Texture", pTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE)},
    ResourceMappingEntry{}
};
ResourceMappingDesc ResMappingDesc;
ResMappingDesc.pEntries= Entries;
RefCntAutoPtr<IResourceMapping> pResMapping;
pRenderDevice->CreateResourceMapping( ResMappingDesc, &pResMapping );
```

The resource mapping can then be used to bind all static resources in a pipeline state (`IPipelineState::BindStaticResources()`):

```cpp
m_pPSO->BindStaticResources(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, pResMapping, BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED);
```

or all mutable and dynamic resources in a shader resource binding (`IShaderResourceBinding::BindResources()`):

```cpp
m_pSRB->BindResources(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, pResMapping, BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED);
```

The last parameter to all `BindResources()` functions defines how resources should be resolved:

* `BIND_SHADER_RESOURCES_UPDATE_STATIC` - Indicates that static variable bindings are to be updated.
* `BIND_SHADER_RESOURCES_UPDATE_MUTABLE` - Indicates that mutable variable bindings are to be updated.
* `BIND_SHADER_RESOURCES_UPDATE_DYNAMIC` -Indicates that dynamic variable bindings are to be updated.
* `BIND_SHADER_RESOURCES_UPDATE_ALL` - Indicates that all variable types (static, mutable and dynamic) are to be updated.
   Note that if none of `BIND_SHADER_RESOURCES_UPDATE_STATIC`, `BIND_SHADER_RESOURCES_UPDATE_MUTABLE`, and 
   `BIND_SHADER_RESOURCES_UPDATE_DYNAMIC` flags are set, all variable types are updated as if `BIND_SHADER_RESOURCES_UPDATE_ALL` was specified.
* `BIND_SHADER_RESOURCES_KEEP_EXISTING` - If this flag is specified, only unresolved bindings will be updated. All existing bindings will keep their original values. If this flag is not specified, every shader variable will be updated if the mapping contains corresponding resource.
* `BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED` - If this flag is specified, all shader bindings are expected be resolved after the call. If this is not the case, an error will be reported.

`BindResources()` may be called several times with different resource mappings to bind resources.
However, it is recommended to use one large resource mapping as the size of the mapping does not affect element search time.

The engine performs run-time checks to verify that correct resources are being bound. For example, if you try to bind
a constant buffer to a shader resource view variable, an error will be output to the debug console.

<a name="draw_command"></a>
## Setting the Pipeline State and Invoking Draw Command

Before any draw command can be invoked, all required vertex and index buffers as well as the pipeline state should
be bound to the device context:

```cpp
// Set render targets before issuing any draw command.
auto* pRTV = m_pSwapChain->GetCurrentBackBufferRTV();
auto* pDSV = m_pSwapChain->GetDepthBufferDSV();
m_pContext->SetRenderTargets(1, &pRTV, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

// Clear render target and depth-stencil
const float zero[4] = {0, 0, 0, 0};
m_pContext->ClearRenderTarget(pRTV, ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
m_pContext->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

// Set vertex and index buffers
IBuffer* buffer[] = {m_pVertexBuffer};
Uint32 offsets[] = {0};
m_pContext->SetVertexBuffers(0, 1, buffer, offsets, SET_VERTEX_BUFFERS_FLAG_RESET,
                             RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
m_pContext->SetIndexBuffer(m_pIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

m_pContext->SetPipelineState(m_pPSO);
```

All methods that may need to perform resource state transitions take `RESOURCE_STATE_TRANSITION_MODE` enum
as parameter. The enum defines the following modes:

* `RESOURCE_STATE_TRANSITION_MODE_NONE` - Perform no resource state transitions.
* `RESOURCE_STATE_TRANSITION_MODE_TRANSITION` - Transition resources to the states required by the command.
* `RESOURCE_STATE_TRANSITION_MODE_VERIFY` - Do not transition, but verify that states are correct.

The final step is to committ shader resources to the device context. This is accomplished by
the `IDeviceContext::CommitShaderResources()` method:

```cpp
m_pContext->CommitShaderResources(m_pSRB, COMMIT_SHADER_RESOURCES_FLAG_TRANSITION_RESOURCES);
```

If the method is not called, the engine will detect that resources are not committed and output
debug message. Note that `CommitShaderResources()` must be called after the right pipeline state has been
bound to the context. Note that the last parameter tells the system to transition resources to correct states.
If this flag is not specified, the resources must be explicitly transitioned to required states by a call to
`IDeviceContext::TransitionShaderResources()`:

```cpp
m_pContext->TransitionShaderResources(m_pPSO, m_pSRB);
```

Note that the method requires pointer to the pipeline state that created the shader resource binding.

When all required states and resources are bound, `IDeviceContext::Draw()` can be used to execute draw
command or `IDeviceContext::DispatchCompute()` can be used to execute compute command. Note that for a draw command,
graphics pipeline must be bound, and for dispatch command, compute pipeline must be bound. `Draw()` takes
`DrawAttribs` structure as an argument. The structure members define all attributes required to perform the command
(number of vertices or indices, if draw call is indexed or not, if draw call is instanced or not,
if draw call is indirect or not, etc.). For example:

```cpp
DrawAttribs attrs;
attrs.IndexType  = VT_UINT16;
attrs.NumIndices = 36;
attrs.Flags      = DRAW_FLAG_VERIFY_STATES;
pContext->Draw(attrs);
```

`DRAW_FLAG_VERIFY_STATES` flag instructs the engine to verify that vertex and index buffers used by the
draw command are transitioned to proper states.

`DispatchCompute()` takes DispatchComputeAttribs structure that defines compute grid dimensions:

```cpp
m_pContext->SetPipelineState(m_pComputePSO);
m_pContext->CommitShaderResources(m_pComputeSRB, COMMIT_SHADER_RESOURCES_FLAG_TRANSITION_RESOURCES);
DispatchComputeAttribs DispatchAttrs(64, 64, 8);
m_pContext->DispatchCompute(DispatchAttrs);
```

You can learn more about the engine API by looking at the [engine samples' source code](https://github.com/DiligentGraphics/DiligentSamples) and the [API Reference][1].


<a name="low_level_api_interoperability"></a>
# Low-level API interoperability

Diligent Engine extensively supports interoperability with underlying low-level APIs. The engine can be initialized
by attaching to existing D3D11/D3D12 device or OpenGL/GLES context and provides access to the underlying native API
objects. Refer to the following pages for more information:

[Direct3D11 Interoperability](http://diligentgraphics.com/diligent-engine/native-api-interoperability/direct3d11-interoperability/)

[Direct3D12 Interoperability](http://diligentgraphics.com/diligent-engine/native-api-interoperability/direct3d12-interoperability/)

[OpenGL/GLES Interoperability](http://diligentgraphics.com/diligent-engine/native-api-interoperability/openglgles-interoperability/)


<a name="repository_structure"></a>
# Repository structure

 The repository contains the following projects:

 | Project                                                          | Description       |
 |------------------------------------------------------------------|-------------------|
 | [Primitives](https://github.com/DiligentGraphics/DiligentCore/tree/master/Primitives)                                        | Definitions of basic types (Int32, Int16, Uint32, etc.) and interfaces (IObject, IReferenceCounters, etc.) |
 | [Common](https://github.com/DiligentGraphics/DiligentCore/tree/master/Common)                                                | Common functionality such as file wrapper, logging, debug utilities, etc. |
 | [Graphics/GraphicsAccessories](https://github.com/DiligentGraphics/DiligentCore/tree/master/Graphics/GraphicsAccessories)    | Basic graphics accessories used by all implementations  |
 | [Graphics/GraphicsEngine](https://github.com/DiligentGraphics/DiligentCore/tree/master/Graphics/GraphicsEngine)              | Platform-independent base functionality |
 | [Graphics/GraphicsEngineD3DBase](https://github.com/DiligentGraphics/DiligentCore/tree/master/Graphics/GraphicsEngineD3DBase)| Base functionality for D3D11/D3D12 implementations |
 | [Graphics/GraphicsEngineD3D11](https://github.com/DiligentGraphics/DiligentCore/tree/master/Graphics/GraphicsEngineD3D11)     | Implementation of Direct3D11 rendering backend |
 | [Graphics/GraphicsEngineD3D12](https://github.com/DiligentGraphics/DiligentCore/tree/master/Graphics/GraphicsEngineD3D12)     | Implementation of Direct3D12 rendering backend |
 | [Graphics/GraphicsEngineOpenGL](https://github.com/DiligentGraphics/DiligentCore/tree/master/Graphics/GraphicsEngineOpenGL)   | Implementation of OpenGL/GLES rendering backend |
 | [Graphics/GraphicsEngineVulkan](https://github.com/DiligentGraphics/DiligentCore/tree/master/Graphics/GraphicsEngineVulkan)   | Implementation of Vulkan rendering backend |
 | [Graphics/GraphicsEngineMetal](https://github.com/DiligentGraphics/DiligentCore/tree/master/Graphics/GraphicsEngineMetal)     | Implementation of Metal rendering backend |
 | [Graphics/GraphicsTools](https://github.com/DiligentGraphics/DiligentCore/tree/master/Graphics/GraphicsTools)                 | Graphics utilities build on top of core interfaces (definitions of commonly used states, texture uploaders, etc.) |
 | [Graphics/HLSL2GLSLConverterLib](https://github.com/DiligentGraphics/DiligentCore/tree/master/Graphics/HLSL2GLSLConverterLib) | HLSL to GLSL source code converter library |
 | [Platforms/Basic](https://github.com/DiligentGraphics/DiligentCore/tree/master/Platforms/Basic)      | Interface for platform-specific routines and implementation of some common functionality |
 | [Platforms/Android](https://github.com/DiligentGraphics/DiligentCore/tree/master/Platforms/Android)  | Implementation of platform-specific routines on Android |
 | [Platforms/Apple](https://github.com/DiligentGraphics/DiligentCore/tree/master/Platforms/Apple)      | Implementation of platform-specific routines on Apple platforms (MacOS, iOS)|
 | [Platforms/UWP](https://github.com/DiligentGraphics/DiligentCore/tree/master/Platforms/UWP)          | Implementation of platform-specific routines on Universal Windows platform |
 | [Platforms/Win32](https://github.com/DiligentGraphics/DiligentCore/tree/master/Platforms/Win32)      | Implementation of platform-specific routines on Win32 platform |
 | [Platforms/Linux](https://github.com/DiligentGraphics/DiligentCore/tree/master/Platforms/Linux)      | Implementation of platform-specific routines on Linux platform |
 | External | Third-party libraries and modules |

# License

See [Apache 2.0 license](License.txt).

This project has some third-party dependencies, each of which may have independent licensing:

* [Vulkan-Headers](https://github.com/KhronosGroup/Vulkan-Headers): Vulkan Header files and API registry ([Apache License 2.0](https://github.com/DiligentGraphics/Vulkan-Headers/blob/master/LICENSE.txt)).
* [SPIRV-Cross](https://github.com/KhronosGroup/SPIRV-Cross): SPIRV parsing and cross-compilation tools ([Apache License 2.0](https://github.com/DiligentGraphics/SPIRV-Cross/blob/master/LICENSE)).
* [SPIRV-Headers](https://github.com/KhronosGroup/SPIRV-Headers): SPIRV header files ([Khronos MIT-like license](https://github.com/DiligentGraphics/SPIRV-Headers/blob/master/LICENSE)).
* [SPIRV-Tools](https://github.com/KhronosGroup/SPIRV-Tools): SPIRV optimization and validation tools ([Apache License 2.0](https://github.com/DiligentGraphics/SPIRV-Tools/blob/master/LICENSE)).
* [glslang](https://github.com/KhronosGroup/glslang): Khronos reference compiler and validator for GLSL, ESSL, and HLSL ([3-Clause BSD License, 2-Clause BSD License, MIT, Apache License 2.0](https://github.com/DiligentGraphics/glslang/blob/master/LICENSE.txt)).
* [glew](http://glew.sourceforge.net/): OpenGL Extension Wrangler Library ([Mesa 3-D graphics library, Khronos MIT-like license](https://github.com/DiligentGraphics/DiligentCore/blob/master/ThirdParty/glew/LICENSE.txt)).
* [volk](https://github.com/zeux/volk): Meta loader for Vulkan API ([Arseny Kapoulkine MIT-like license](https://github.com/DiligentGraphics/volk/blob/master/LICENSE.md)).
* [stb](https://github.com/nothings/stb): stb single-file public domain libraries for C/C++ ([MIT License or public domain](https://github.com/DiligentGraphics/DiligentCore/blob/master/ThirdParty/stb/stb_image_write.h#L1581)).
* [googletest](https://github.com/google/googletest): Google Testing and Mocking Framework ([BSD 3-Clause "New" or "Revised" License](https://github.com/DiligentGraphics/googletest/blob/master/LICENSE)).
* [DirectXShaderCompiler](https://github.com/microsoft/DirectXShaderCompiler): LLVM/Clang-based DirectX Shader Compiler ([LLVM Release License](https://github.com/DiligentGraphics/DiligentCore/blob/master/ThirdParty/DirectXShaderCompiler/LICENSE.TXT)).

<a name="contributing"></a>
# Contributing

To contribute your code, submit a [Pull Request](https://github.com/DiligentGraphics/DiligentCore/pulls) 
to this repository. **Diligent Engine** is distributed under the [Apache 2.0 license](License.txt) that guarantees 
that code in the **DiligentCore** repository is free of Intellectual Property encumbrances. In submitting code to
this repository, you are agreeing that the code is free of any Intellectual Property claims.

Diligent Engine uses [clang-format](https://clang.llvm.org/docs/ClangFormat.html) to ensure
consistent source code style throught the code base. The format is validated by appveyor and travis
for each commit and pull request, and the build will fail if any code formatting issue is found. Please refer
to [this page](https://github.com/DiligentGraphics/DiligentCore/blob/master/doc/code_formatting.md) for instructions
on how to set up clang-format and automatic code formatting.

<a name="references"></a>
# References

[Diligent Engine Architecture](http://diligentgraphics.com/diligent-engine/architecture/)

[API Basics](http://diligentgraphics.com/diligent-engine/api-basics/)

[API Reference][1]

<a name="release_history"></a>
# Release History

See [Release History](ReleaseHistory.md)

-------------------

[diligentgraphics.com](http://diligentgraphics.com)

[![Diligent Engine on Twitter](https://github.com/DiligentGraphics/DiligentCore/blob/master/media/twitter.png)](https://twitter.com/diligentengine)
[![Diligent Engine on Facebook](https://github.com/DiligentGraphics/DiligentCore/blob/master/media/facebook.png)](https://www.facebook.com/DiligentGraphics/)


[1]: https://cdn.rawgit.com/DiligentGraphics/DiligentCore/4949ec8a/doc/html/index.html
