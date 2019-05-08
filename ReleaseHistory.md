## Current Progress

* Enabled Vulkan on iOS
* Added [GLTF2.0 loader](https://github.com/DiligentGraphics/DiligentTools/tree/master/AssetLoader)
  and [PBR renderer](https://github.com/DiligentGraphics/DiligentFX/tree/master/GLTF_PBR_Renderer)
* Added [GLTF Viewer](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Samples/GLTFViewer)

## v2.4.b

### General

* Added cmake options to disable specific back-ends and glslang
* Improved engine support of GLES3.0 devices
* Added new module - [DiligentFX](https://github.com/DiligentGraphics/DiligentFX), a high-level rendering framework
  * Reworked light scattering post-processing effect to be ready-to-use component

### API changes

* Updated `IRenderDevice::CreateTexture()` and `IRenderDevice::CreateBuffer()` to take pointer
  to initialization data rather than references.
* Added `LayoutElement::AutoOffset` and `LayoutElement::AutoOffset` values to use instead of 0 when
  automatically computing input layout elements offset and strides.
* Renamed factory interfaces and headers:
  * `IRenderDeviceFactoryD3D11` -> `IEngineFactoryD3D11`, RenderDeviceFactoryD3D11.h -> EngineFactoryD3D11.h
  * `IRenderDeviceFactoryD3D12` -> `IEngineFactoryD3D12`, RenderDeviceFactoryD3D12.h -> EngineFactoryD3D12.h
  * `IRenderDeviceFactoryOpenGL` -> `IEngineFactoryOpenGL`, RenderDeviceFactoryOpenGL.h -> EngineFactoryOpenGL.h
  * `IRenderDeviceFactoryVk` -> `IEngineFactoryVk`, RenderDeviceFactoryVk.h -> EngineFactoryVk.h
  * `IRenderDeviceFactoryMtl` -> `IEngineFactoryMtl`, RenderDeviceFactoryMtl.h -> EngineFactoryMtl.h
* Renamed `IShaderVariable` -> `IShaderResourceVariable`
* Renamed `SHADER_VARIABLE_TYPE` -> `SHADER_RESOURCE_VARIABLE_TYPE`
* Renamed `ShaderVariableDesc` -> `ShaderResourceVariableDesc`
* Added `SHADER_RESOURCE_TYPE` enum
* Moved shader variable type and static sampler definition from shader creation to PSO creation stage:
  * Removed `IShader::GetVariable`, `IShader::GetVariableCount`, and `IShader::BindResources` methods
  * Added `IPipelineState::BindStaticResoruces`, `IPipelineState::GetStaticVariableCount`,
    and `IPipelineState::GetStaticShaderVariable` methods
  * Added `PipelineResourceLayoutDesc` structure and `ResourceLayout` member to `PipelineStateDesc`
* Added `ShaderResourceDesc` structure
* Added `IShader::GetResourceCount` and `IShader::GetResource` methods
* Replaced `IShaderVariable::GetArraySize` and `IShaderVariable::GetName` methods with `IShaderResourceVariable::GetResourceDesc` method
* Added `HLSLShaderResourceDesc` structure as well as `IShaderResourceVariableD3D` and `IShaderResourceVariableD3D` interfaces 
  to query HLSL-specific shader resource description (shader register)

With the new API, shader initialization and pipeline state creation changed as shown below.

Old API:

```cpp
RefCntAutoPtr<IShader> pVS;
{
    CreationAttribs.Desc.ShaderType = SHADER_TYPE_VERTEX;
    CreationAttribs.EntryPoint = "main";
    CreationAttribs.Desc.Name  = "Cube VS";
    CreationAttribs.FilePath   = "cube.vsh";
    pDevice->CreateShader(CreationAttribs, &pVS);
    pVS->GetShaderVariable("Constants")->Set(m_VSConstants);
}
RefCntAutoPtr<IShader> pPS;
{
    CreationAttribs.Desc.ShaderType = SHADER_TYPE_PIXEL;
    CreationAttribs.EntryPoint = "main";
    CreationAttribs.Desc.Name  = "Cube PS";
    CreationAttribs.FilePath   = "cube.psh";
    ShaderVariableDesc Vars[] = 
    {
        {"g_Texture", SHADER_VARIABLE_TYPE_MUTABLE}
    };
    CreationAttribs.Desc.VariableDesc = Vars;
    CreationAttribs.Desc.NumVariables = _countof(Vars);

    SamplerDesc SamLinearClampDesc( FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, 
                                    TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP);
    StaticSamplerDesc StaticSamplers[] = 
    {
        {"g_Texture", SamLinearClampDesc}
    };
    CreationAttribs.Desc.StaticSamplers = StaticSamplers;
    CreationAttribs.Desc.NumStaticSamplers = _countof(StaticSamplers);

    pDevice->CreateShader(CreationAttribs, &pPS);
}
// ...
pDevice->CreatePipelineState(PSODesc, &m_pPSO);
```

New API:

```cpp
RefCntAutoPtr<IShader> pVS;
{
    ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
    ShaderCI.EntryPoint      = "main";
    ShaderCI.Desc.Name       = "Cube VS";
    ShaderCI.FilePath        = "cube.vsh";
    pDevice->CreateShader(ShaderCI, &pVS);
}
RefCntAutoPtr<IShader> pVS;
{
    ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
    ShaderCI.EntryPoint      = "main";
    ShaderCI.Desc.Name       = "Cube VS";
    ShaderCI.FilePath        = "cube.vsh";
    pDevice->CreateShader(ShaderCI, &pVS);
}
// ...
ShaderResourceVariableDesc Vars[] = 
{
    {SHADER_TYPE_PIXEL, "g_Texture", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE}
};
PSODesc.ResourceLayout.Variables    = Vars;
PSODesc.ResourceLayout.NumVariables = _countof(Vars);

// Define static sampler for g_Texture. Static samplers should be used whenever possible
SamplerDesc SamLinearClampDesc( FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, 
                                TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP);
StaticSamplerDesc StaticSamplers[] = 
{
    {SHADER_TYPE_PIXEL, "g_Texture", SamLinearClampDesc}
};
PSODesc.ResourceLayout.StaticSamplers    = StaticSamplers;
PSODesc.ResourceLayout.NumStaticSamplers = _countof(StaticSamplers);

pDevice->CreatePipelineState(PSODesc, &m_pPSO);
m_pPSO->GetStaticShaderVariable(SHADER_TYPE_VERTEX, "Constants")->Set(m_VSConstants);
```

### Samples and Tutorials

* Added [Tutorial12 - Render target](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial12_RenderTarget)
  (credits to @dolphineye for contribution)

## v2.4.a

* Enabled MinGW build
* Enabled Vulkan on MacOS
* Implemented split barriers (https://github.com/DiligentGraphics/DiligentCore/issues/43)
  * Added `STATE_TRANSITION_TYPE` enum and `STATE_TRANSITION_TYPE TransitionType` member to `StateTransitionDesc` structure
* Added Metal backend stub
* Samples:
  * Added rendering backend selection dialog on Win32 and Mac

## v2.4

Core:

* Implemented explicit resource state transitions
* API Changes
  * Added `RESOURCE_STATE` enum that defines the resource state
  * Added `RESOURCE_STATE_TRANSITION_MODE` enum that controls resource state transition mode
  * Added `DRAW_FLAGS` enum that controls state validation performed by Draw command
  * Added `Flags` member to `DrawAttribs` structure (values from `DRAW_FLAGS`)
  * Added `IndirectAttribsBufferStateTransitionMode` member to `DrawAttribs` and `DispatchComputeAttribs` structures (values from `RESOURCE_STATE_TRANSITION_MODE`)
  * Added `StateTransitionDesc` structure that describes resource state transition barrier
  * Added `IDeviceContext::TransitionResourceStates(Uint32 BarrierCount, StateTransitionDesc* pResourceBarriers)` method
  * Added `IBuffer::SetState()`, `IBuffer::GetState()`, `ITexture::SetState()`, `ITexture::GetState()` methods
  * Added `IShaderResourceBinding::InitializeStaticResources()` to explicitly initialize static resources and
    avoid problems in multi-threaded environments
  * Added `InitStaticResources` parameter to `IPipelineState::CreateShaderResourceBinding()` method to allow
    immediate initialization of static resources in a SRB
  * Removed default SRB object
  * Renamed/moved `IBuffer::UpdateData()` to `IDeviceContext::UpdateBuffer()`
  * Renamed/moved `IBuffer::CopyData()` to `IDeviceContext::CopyBuffer()`
  * Renamed/moved `IBuffer::Map()` to `IDeviceContext::MapBuffer()`
  * Renamed/moved `IBuffer::Unmap()` to `IDeviceContext::UnmapBuffer()`
    * Removed MapFlags parameter
  * Renamed/moved `ITexture::UpdateData()` to `IDeviceContext::UpdateTexture()`
  * Renamed/moved `ITexture::CopyData()` to `IDeviceContext::CopyTexture()`
  * Renamed/moved `ITexture::Map()` to `IDeviceContext::MapTextureSubresource()`
  * Renamed/moved `ITexture::Unmap()` to `IDeviceContext::UnmapTextureSubresource()`
  * Moved `ITextureView::GenerateMips()` to `IDeviceContext::GenerateMips()`
  * Added state transition mode parameters to `IDeviceContext::UpdateBuffer()`, `IDeviceContext::UpdateTexture()`,
    `IDeviceContext::CopyBuffer()`, `IDeviceContext::CopyTexture()`, `IDeviceContext::SetVertexBuffers()`, 
	`IDeviceContext::SetIndexBuffers()`, `IDeviceContext::ClearRenderTargets()`, and `IDeviceContext::ClearDepthStencil()` methods
  * Replaced `COMMIT_SHADER_RESOURCES_FLAGS` enum with `RESOURCE_STATE_TRANSITION_MODE`
  * Added `ITextureD3D12::GetD3D12ResourceState()`, `IBufferD3D12::GetD3D12ResourceState()`,
	`IBufferVk::GetAccessFlags()`, and `ITextureVk::GetLayout()` methods
  * Added `CopyTextureAttribs` structure that combines all paramters of `IDeviceContext::CopyTexture()` method

## v2.3.b

* Core
  * Enabled Vulkan backend on Linux
  * API Changes
    * Implemented separate texture samplers: 
      * Added `UseCombinedTextureSamplers` and `CombinedSamplerSuffix` members to `ShaderCreationAttribs` structure
      * When separate samplers are used (`UseCombinedTextureSamplers == false`), samplers are set in the same way as other shader variables
        via shader or SRB objects
    * Removed `BIND_SHADER_RESOURCES_RESET_BINDINGS` flag, renamed `BIND_SHADER_RESOURCES_KEEP_EXISTING` to `BIND_SHADER_RESOURCES_KEEP_EXISTING`.
	  Added `BIND_SHADER_RESOURCES_UPDATE_STATIC`, `BIND_SHADER_RESOURCES_UPDATE_MUTABLE`, `BIND_SHADER_RESOURCES_UPDATE_DYNAMIC`, and
	  `BIND_SHADER_RESOURCES_UPDATE_ALL` flags
  * Using glslang to compile HLSL to SPIRV in Vulkan backend instead of relying on HLSL->GLSL converter


## v2.3.a

* Core
  * Added `IFence` interface and `IDeviceContext::SignalFence()` method to enable CPU-GPU synchronization
  * Added `BUFFER_MODE_RAW` mode allowing raw buffer views in D3D11/D3D12.
  * Moved `Format` member from `BufferDesc` to `BufferViewDesc`
  * Removed `IsIndirect` member from `DrawAttrbis` as setting `pIndirectDrawAttribs` to a non-null buffer already indicates indirect rendering

* Samples:
  * Added Tutorial 10 - Data Streaming
  * Added Tutorial 11 - Resource Updates

## v2.3

* Core:
  * **Implemented Vulkan backend**
  * Implemented hardware adapter & display mode enumeration in D3D11 and D3D12 modes
  * Implemented initialization in fullscreen mode as well as toggling between fullscreen and windowed modes in run time
  * Added sync interval parameter to ISwapChain::Present()
  * API Changes
    * Added `NumViewports` member to `GraphicsPipelineDesc` struct
    * Removed `PRIMITIVE_TOPOLOGY_TYPE` type
	* Replaced `PRIMITIVE_TOPOLOGY_TYPE GraphicsPipelineDesc::PrimitiveTopologyType` 
      with `PRIMITIVE_TOPOLOGY GraphicsPipelineDesc::PrimitiveTopology`
	* Removed `DrawAttribs::Topology`
    * Removed `pStrides` parameter from `IDeviceContext::SetVertexBuffers()`. Strides are now defined
      through vertex layout.
* API Changes:
  * Math library functions `SetNearFarClipPlanes()`, `GetNearFarPlaneFromProjMatrix()`, `Projection()`,
    `OrthoOffCenter()`, and `Ortho()` take `bIsGL` flag instead of `bIsDirectX`
  * Vertex buffer strides are now defined by the pipeline state as part of the input layout description (`LayoutElement::Stride`)
  * Added `COMMIT_SHADER_RESOURCES_FLAG_VERIFY_STATES` flag
  * Added `NumViewports` member to `GraphicsPipelineDesc` structure
* Samples:
  * Added fullscreen mode selection dialog box
  * Implemented fullscreen mode toggle on UWP with shift + enter
  * Implemented fullscreen window toggle on Win32 with alt + enter
  * Added Tutorial 09 - Quads
* Fixed the following issues:
  * [Add option to redirect diligent error messages](https://github.com/DiligentGraphics/DiligentEngine/issues/9)
  * [Add ability to run in exclusive fullscreen/vsync mode](https://github.com/DiligentGraphics/DiligentEngine/issues/10)

## v2.2.a

* Enabled Win32 build targeting Windows 8.1 SDK
* Enabled build customization through custom build config file
* Implemented PSO compatibility
* Fixed the following issues: 
  * [Messy #include structure?](https://github.com/DiligentGraphics/DiligentEngine/issues/3) 
  * [Move GetEngineFactoryXXXType and LoadGraphicsEngineXXX to Diligent namespace](https://github.com/DiligentGraphics/DiligentEngine/issues/5)
  * [Customizable build scripts](https://github.com/DiligentGraphics/DiligentEngine/issues/6)
  * [Win32FileSystem related functions should use wchar_t (UTF-16)](https://github.com/DiligentGraphics/DiligentEngine/issues/7)

## v2.2

* Added MacOS  and iOS support

## v2.1.b

* Removed legacy Visual Studio solution and project files
* Added API reference
* Added tutorials 1-8

## v2.1.a

* Refactored build system to use CMake and Gradle for Android
* Added support for Linux platform

## v2.1

### New Features

#### Core

* Interoperability with native API
  * Accessing internal objects and handles
  * Creating diligent engine buffers/textures from native resources
  * Attaching to existing D3D11/D3D12 device or GL context
  * Resource state and command queue synchronization for D3D12
* Integraion with Unity
* Geometry shader support
* Tessellation support
* Performance optimizations

#### HLSL->GLSL converter
* Support for structured buffers
* HLSL->GLSL conversion is now a two-stage process:
  * Creating conversion stream
  * Creating GLSL source from the stream
* Geometry shader support
* Tessellation control and tessellation evaluation shader support
* Support for non-void shader functions
* Allowing structs as input parameters for shader functions


## v2.0 (alpha)

Alpha release of Diligent Engine 2.0. The engine has been updated to take advantages of Direct3D12:

* Pipeline State Object encompasses all coarse-grain state objects like Depth-Stencil State, Blend State, Rasterizer State, shader states etc.
* New shader resource binding model implemented to leverage Direct3D12

* OpenGL and Direct3D11 backends
* Alpha release is only available on Windows platform
* Direct3D11 backend is very thoroughly optimized and has very low overhead compared to native D3D11 implementation
* Direct3D12 implementation is preliminary and not yet optimized

### v1.0.0

Initial release