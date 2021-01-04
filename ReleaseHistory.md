
## v2.4.g

### API Changes

* Enabled ray tracing (API Version 240080)
* Added `IDeviceContext::GetFrameNumber` method (API Version 240079)
* Added `ShaderResourceQueries` device feature and `EngineGLCreateInfo::ForceNonSeparablePrograms` parameter (API Version 240078)

* Renamed `USAGE_STATIC` to `USAGE_IMMUTABLE` (API Version 240077)

* Renamed static samplers into immutable samplers (API Version 240076)
  * Renamed `StaticSamplerDesc` -> `ImmutableSamplerDesc`
  * Renamed `PipelineResourceLayoutDesc::NumStaticSamplers` -> `PipelineResourceLayoutDesc::NumImmutableSamplers`
  * Renamed `PipelineResourceLayoutDesc::StaticSamplers` -> `PipelineResourceLayoutDesc::ImmutableSamplers`

* Refactored pipeline state creation (API Version 240075)
  * Replaced `PipelineStateCreateInfo` with `GraphicsPipelineStateCreateInfo` and `ComputePipelineStateCreateInfo`
  * Replaced `IRenderDevice::CreatePipelineState` with `IRenderDevice::CreateGraphicsPipelineState` and `IRenderDevice::CreateComputePipelineState`
  * `pVS`, `pGS`, `pHS`, `pDS`, `pPS`, `pAS`, `pMS` were moved from `GraphicsPipelineDesc` to `GraphicsPipelineStateCreateInfo`
  * `GraphicsPipelineDesc GraphicsPipeline`  was moved from `PipelineStateDesc` to `GraphicsPipelineStateCreateInfo`
  * `pCS` is now a member of `ComputePipelineStateCreateInfo`, `ComputePipelineDesc` was removed
  * Added `IPipelineState::GetGraphicsPipelineDesc` method
  
  Old API for graphics pipeline initialization:
  ```cpp
  PipelineStateCreateInfo PSOCreateInfo;
  PipelineStateDesc&      PSODesc = PSOCreateInfo.PSODesc;

  PSODesc.GraphicsPipeline.pVS = pVS;
  PSODesc.GraphicsPipeline.pPS = pVS;
  // ...
  Device->CreatePipelineState(PSOCreateInfo, &pPSO);
  ```

  New API for graphics pipeline initialization:
  ```cpp
  GraphicsPipelineStateCreateInfo PSOCreateInfo;
  // ...
  PSOCreateInfo.pVS = pVS;
  PSOCreateInfo.pPS = pVS;
  Device->CreateGraphicsPipelineState(PSOCreateInfo, &pPSO);
  ```

  Old API for compute pipeline initialization:
  ```cpp
  PipelineStateCreateInfo PSOCreateInfo;
  PipelineStateDesc&      PSODesc = PSOCreateInfo.PSODesc;

  PSODesc.ComputePipeline.pCS = pCS;
  // ...
  Device->CreatePipelineState(PSOCreateInfo, &pPSO);
  ```

    New API for compute pipeline initialization:
  ```cpp
  ComputePipelineStateCreateInfo PSOCreateInfo;

  PSOCreateInfo.pCS = pCS;
  Device->CreateComputePipelineState(PSOCreateInfo, &pPSO);
  ```

* Added `ShaderInt8`, `ResourceBuffer8BitAccess`, and `UniformBuffer8BitAccess` device features. (API Version 240074)
* Added `ShaderFloat16`, `ResourceBuffer16BitAccess`, `UniformBuffer16BitAccess`, and `ShaderInputOutput16` device features. (API Version 240073)


### Samples and Tutorials

* Added [Tutorial21 - Ray Tracing](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial21_RayTracing)


## v2.4.f

### API Changes

* Added `UnifiedMemoryCPUAccess` member to `GraphicsAdapterInfo` struct (API Version 240072)
   * An application should check allowed unified memory access types before creating unified buffers
* Added GPU vendor and memory size detection (API Version 240071)
   * Added `ADAPTER_VENDOR` enum
   * Added `GraphicsAdapterInfo` struct
   * Added `GraphicsAdapterInfo AdapterInfo` member to `DeviceCaps` struct
   * Removed `ADAPTER_TYPE AdaterType` from `DeviceCaps` struct 
* Reworked texture format properties (API Version 240070)
   * Added `RESOURCE_DIMENSION_SUPPORT` enum
   * Reworked `TextureFormatInfoExt` struct
* Added option to disable/enable device features during initialization (API Version 240069)
   * Added `DEVICE_FEATURE_STATE` enum
   * Changed the types of members of `DeviceFeatures` struct from bool to `DEVICE_FEATURE_STATE`
   * Added `DeviceFeatures Features` member to `EngineCreateInfo` struct
* Enabled mesh shaders (API Version 240068)
   * Added `PIPELINE_TYPE` enum
   * Replaced `IsComputePipline` member of `PipelineStateDesc` struct with `PIPELINE_TYPE PipelineType`
   * Added new mesh shader types
   * Added mesh shader draw commands
* Added `QUERY_TYPE_DURATION` query type (API Version 240067)
* Added `USAGE_UNIFIED` usage type (API Version 240066)
* Added render passes (API Version 240065)
* Added `CREATE_SHADER_SOURCE_INPUT_STREAM_FLAGS` enum and `IShaderSourceInputStreamFactory::CreateInputStream2` method (API Version 240064)
* Added `ISwapChain::SetMaximumFrameLatency` function (API Version 240061)
* Added `EngineGLCreateInfo::CreateDebugContext` member (API Version 240060)
* Added `SHADER_SOURCE_LANGUAGE_GLSL_VERBATIM` value (API Version 240059).
* Added `GLBindTarget` parameter to `IRenderDeviceGL::CreateTextureFromGLHandle` method (API Version 240058).

### Samples and Tutorials

* Added [HelloAR Android sample](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Android/HelloAR)
* Added [Tutorial19 - Render Passes](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial19_RenderPasses)
* Added [Tutorial20 - Mesh Shader](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial20_MeshShader)


## v2.4.e

### General

* Enabled Vulkan on Android
* Added C Interface (API Version 240052)

### API Changes

* Added `PreTransform` parameter to swap chain description (API Version 240057).
* Added `PipelineStateCreateInfo` struct that is now taken by `IRenderDevice::CreatePipelineState` instead of
  `PipelineStateDesc` struct. Added `PSO_CREATE_FLAGS` enum (API Version 240056).

  Old API:
  ```cpp
  PipelineStateDesc PSODesc;
  // ...
  pRenderDevice->CreatePipelineState(PSODesc, &pPSO);
  ```

  New API:
  ```cpp
  PipelineStateCreateInfo PSOCreateInfo;
  PipelineStateDesc&      PSODesc = PSOCreateInfo.PSODesc;
  // ...
  pRenderDevice->CreatePipelineState(PSOCreateInfo, &pPSO);
  ```

* Added `PRIMITIVE_TOPOLOGY_LINE_STRIP` topology (API Version 240055)
* Updated swap chain creation functions to use `NativeWindow` (API Version 240054)
* Added `NativeWindow` wrapper and replaced `pNativeWndHandle` and `pDisplay` members with it in `EngineGLCreateInfo` (API Version 240053)


## v2.4.d

### API Changes

* Added Queries (API Version 240051)
* Added `AdapterType` member to `DeviceCaps` struct (API Version 240048)
* Added `IDeviceContextGL::SetSwapChain` and `IRenderDeviceGL::CreateDummyTexture` methods (API Version 240047)
* Removed `IDeviceContext::SetSwapChain` method (API Version 240046)
* Renamed `MAP_FLAG_DO_NOT_SYNCHRONIZE` flag to `MAP_FLAG_NO_OVERWRITE` (API Version 240045)
* Added `GetVkInstance` and `GetVkPhysicalDevice` methods to `IRenderDeviceVk` interface (API Version 240044)
* Added `HLSLSemantic` member to `LayoutElement` struct (API Version 240042)
* Added `ResolveTextureSubresource` device context command, removed `SamplesCount` member of the
  `SwapChainDesc` (API Version 240041)
* Added `APIVersion` member to `EngineCreateInfo` struct (API Version 240040)
* Added `IDeviceObject::GetUniqueID` method (API Version 240039)
* Added `IDeviceContextD3D12::LockCommandQueue`, `IDeviceContextD3D12::UnlockCommandQueue`,
  `IDeviceContextVk::LockCommandQueue`, and `IDeviceContextVk::UnlockCommandQueue` methods (API Version 240038)
* Added `EnableGPUBasedValidation` member to `EngineD3D12CreateInfo` struct (API Version 240037)
* Added `DRAW_FLAG_RESOURCE_BUFFERS_INTACT` flag (API Version 240036)
* Added `HLSLVersion`, `GLSLVersion` and `GLESSLVersion` to `ShaderCreateInfo` struct (API Version 240035)
* Renamed `EngineD3D11DebugFlags` to `D3D11_DEBUG_FLAGS` (API Version 240034)
* Split up `Draw` command into `Draw`, `DrawIndexed`, `DrawIndirect` and `DrawIndexedIndirect`.
  Split up `DispatchCompute` command into `DispatchCompute` and `DispatchComputeInidrect` (API Version 240033).
* Enabled bindless resources
* Removed `SHADER_PROFILE` enum (API Version 240032)
* Added `DIRECT3D_FEATURE_LEVEL` and `DIRECT3D_FEATURE_LEVEL MinimumFeatureLevel` member to 
  `EngineD3D11CreateInfo` and `EngineD3D12CreateInfo` structs (API Version 240032)
* Updated `IEngineFactoryD3D11::EnumerateHardwareAdapters`, `IEngineFactoryD3D11::EnumerateDisplayModes`,
  `IEngineFactoryD3D12::EnumerateHardwareAdapters`, `IEngineFactoryD3D12::EnumerateDisplayModes` 
  to take minimum feature level. (API Version 240032)
* Added `bBindlessSupported` member to `DeviceCaps` struct. (API Version 240032)

### General

* Enabled automated unit testing, format validation and static code analysis
* Added [Tutorial16 - Bindless Resources](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial16_BindlessResources)
* Added [Tutorial17 - MSAA](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial17_MSAA)
* Added [Tutorial18 - Queries](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial18_Queries)
* Removed RenderScript and Lua

## v2.4.c

### General

* Enabled Vulkan on iOS
* Replaced AntTweakBar UI library with dear imgui
* Added [GLTF2.0 loader](https://github.com/DiligentGraphics/DiligentTools/tree/master/AssetLoader)
  and [PBR renderer](https://github.com/DiligentGraphics/DiligentFX/tree/master/GLTF_PBR_Renderer)
* Added [GLTF Viewer](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Samples/GLTFViewer)
* Added [Shadowing Component](https://github.com/DiligentGraphics/DiligentFX/tree/master/Components#shadows)
  and [Shadows Sample](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Samples/Shadows)
* Added [Dear Imgui demo](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Samples/ImguiDemo)
* Added [Tutorial13 - Shadow Map](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial13_ShadowMap)
* Added [Tutorial14 - Compute Shader](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial14_ComputeShader)
* Added [Tutorial15 - Multiple Windows](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial15_MultipleWindows)
* Removed AntTweakBar sample

### API changes

* Moved `NumDeferredContexts` parameter from factory functions `IEngineFactoryD3D11::CreateDeviceAndContextsD3D11`,
  `IEngineFactoryD3D12::CreateDeviceAndContextsD3D12` and `IEngineFactoryVk::CreateDeviceAndContextsVk` to
  `EngineCreateInfo` struct.
* Renamed `USAGE_CPU_ACCESSIBLE` -> `USAGE_STAGING`
* Added `SWAP_CHAIN_USAGE_FLAGS` enum
* Replaced overloaded `IPipelineState::GetStaticShaderVariable()` with `IPipelineState::GetStaticVariableByName()` and `IPipelineState::GetStaticVariableByIndex()`
* Replaced overloaded `IShaderResourceBinding::GetVariable()` with `IShaderResourceBinding::GetVariableByName()` and `IShaderResourceBinding::GetVariableByIndex()`
* Made `IShaderSourceInputStreamFactory` derived from `IObject`;
  added `IEngineFactory::CreateDefaultShaderSourceStreamFactory()` method;
  added `IRenderDevice::GetEngineFactory()` method (API Version 240021)
* Added `DRAW_FLAG_VERIFY_DRAW_ATTRIBS`, `DRAW_FLAG_VERIFY_RENDER_TARGETS`, and `DRAW_FLAG_VERIFY_ALL` flags (API Version 240022)
* `TEXTURE_VIEW_FLAGS` enum and `Flags` member to `TextureViewDesc` structure (API Version 240023)
* Added `IShaderResourceVariable::IsBound()` method (API Version 240024)
* Added `Diligent-` prefix to project names to avoid name conflicts.
* Added `IDeviceContextD3D12::GetD3D12CommandList` method
* Added `IDeviceContext::WaitForFence()` method (API Version 240027)
* Added `IDeviceContext::WaitForIdle()` method (API Version 240028)
* Added `IRenderDevice::IdleGPU()` method (API Version 240029)
* Added `EngineD3D12CreateInfo::EnableDebugLayer` member (API Version 240030)
* Added `EngineD3D12CreateInfo::BreakOnError` and `EngineD3D12CreateInfo::BreakOnCorruption` members (API Version 240031)


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