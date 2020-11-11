
# GraphicsEngineD3D12

Implementation of Direct3D12 back-end

# Initialization

The following code snippet shows how to initialize Diligent Engine in Direct3D12 mode.

```cpp
#include "EngineFactoryD3D12.h"
using namespace Diligent;

// ...
#if ENGINE_DLL
    // Load the dll and import GetEngineFactoryD3D12() function
    auto GetEngineFactoryD3D12 = LoadGraphicsEngineD3D12();
#endif
auto* pFactoryD3D12 = GetEngineFactoryD3D12();
EngineD3D12CreateInfo EngineCI;
EngineCI.CPUDescriptorHeapAllocationSize[0] = 1024;
EngineCI.CPUDescriptorHeapAllocationSize[1] = 32;
EngineCI.CPUDescriptorHeapAllocationSize[2] = 16;
EngineCI.CPUDescriptorHeapAllocationSize[3] = 16;
EngineCI.NumCommandsToFlushCmdList = 64;
RefCntAutoPtr<IRenderDevice>  pRenderDevice;
RefCntAutoPtr<IDeviceContext> pImmediateContext;
SwapChainDesc SwapChainDesc;
RefCntAutoPtr<ISwapChain> pSwapChain;
pFactoryD3D12->CreateDeviceAndContextsD3D12(EngineCI, &pRenderDevice, &pImmediateContext);
NativeWindow Window;
Window.hWnd = hWnd;
pFactoryD3D12->CreateSwapChainD3D12(pRenderDevice, pImmediateContext, SwapChainDesc, Window, &pSwapChain);
```

Alternatively, the engine can be initialized by attaching to existing Direct3D12 device (see below).

# Interoperability with Direct3D12

Diligent Engine exposes methods to access internal Direct3D12 objects, is able to create diligent engine buffers
and textures from existing Direct3D12 resources, and can be initialized by attaching to existing Direct3D12
device and provide synchronization tools.

## Accessing Native Direct3D12 Resources

Below are some of the methods that provide access to internal Direct3D12 objects:

|                              Function                                       |                              Description                                                                      |
|-----------------------------------------------------------------------------|---------------------------------------------------------------------------------------------------------------|
| `ID3D12Resource* IBufferD3D12::GetD3D12Buffer(size_t& DataStartByteOffset, Uint32 ContextId)` | returns a pointer to the `ID3D12Resource` interface of the internal Direct3D12 buffer object. Note that dynamic buffers are suballocated from dynamic heap, and every context has its own dynamic heap. Offset from the beginning of the dynamic heap for a context identified by `ContextId` is returned in `DataStartByteOffset` parameter |
| `void IBufferD3D12::SetD3D12ResourceState(D3D12_RESOURCE_STATES state)`                       | sets the buffer usage state. This method should be used when an application transitions the buffer to inform diligent engine about the current usage state |
| `D3D12_CPU_DESCRIPTOR_HANDLE IBufferViewD3D12::GetCPUDescriptorHandle()`                      | returns CPU descriptor handle of the buffer view |
| `ID3D12Resource* ITextureD3D12::GetD3D12Texture()`                                            |  returns a pointer to the `ID3D12Resource` interface of the internal Direct3D12 texture object |
| `void ITextureD3D12::SetD3D12ResourceState(D3D12_RESOURCE_STATES state)`                      | sets the texture usage state. This method should be used when an application transitions the texture to inform diligent engine about the current usage state |
| `D3D12_CPU_DESCRIPTOR_HANDLE ITextureViewD3D12::GetCPUDescriptorHandle()`                     | returns CPU descriptor handle of the texture view |
| `void IDeviceContextD3D12::TransitionTextureState(ITexture* pTexture, D3D12_RESOURCE_STATES State)` | transitions internal Direct3D12 texture object to a specified state |
| `void IDeviceContextD3D12::TransitionBufferState(IBuffer* pBuffer, D3D12_RESOURCE_STATES State)`    | transitions internal Direct3D12 buffer object to a specified state |
| `ID3D12PipelineState* IPipelineStateD3D12::GetD3D12PipelineState()`                           | returns ID3D12PipelineState interface of the internal Direct3D12 pipeline state object object |
| `ID3D12RootSignature* IPipelineStateD3D12::GetD3D12RootSignature()`                           | returns a pointer to the root signature object associated with this pipeline state |
| `D3D12_CPU_DESCRIPTOR_HANDLE ISamplerD3D12::GetCPUDescriptorHandle()`                         | returns a CPU descriptor handle of the Direct3D12 sampler object |
| `ID3D12Device* IRenderDeviceD3D12::GetD3D12Device()`                                          | returns ID3D12Device interface of the internal Direct3D12 device object |

## Synchronization Tools

|                              Function                         |                              Description                                                                      |
|---------------------------------------------------------------|---------------------------------------------------------------------------------------------------------------|
| `Uint64 IRenderDeviceD3D12::GetNextFenceValue()`              | returns the fence value that will be signaled by the GPU command queue when the next command list is submitted for execution |
| `Bool IRenderDeviceD3D12::IsFenceSignaled(Uint64 FenceValue)` | checks if the fence value has been signaled by the GPU. True means that all associated work has been finished |
| `void IRenderDeviceD3D12::FinishFrame()`                      |  this method should be called at the end of the frame when attached to existing Direct3D12 device. Otherwise the method is automatically called before present |

## Creating Diligent Engine Objects from D3D12 Resources

* `void IRenderDeviceD3D12::CreateTextureFromD3DResource(ID3D12Resource* pd3d12Texture, RESOURCE_STATE InitialState, ITexture** ppTexture)` -
   creates a Diligent Engine texture object from native Direct3D12 resource.
* `void IRenderDeviceD3D12::CreateBufferFromD3DResource(ID3D12Resource* pd3d12Buffer, const BufferDesc& BuffDesc, RESOURCE_STATE InitialState, IBuffer** ppBuffer)` -
   creates a Diligent Engine buffer object from native Direct3D12 resource.
   The method takes a pointer to the native Direct3D12 resiyrce `pd3d12Buffer`, buffer description `BuffDesc` and writes a pointer to the `IBuffer`
   interface at the memory location pointed to by `ppBuffer`. The system can recover buffer size, but the rest of the fields of
   BuffDesc structure need to be populated by the client as they cannot be recovered from Direct3D12 resource description.


## Initializing the Engine by Attaching to Existing D3D12 Device

To attach diligent engine to existing D3D12 device, use the following factory function:

```cpp
void IEngineFactoryD3D12::AttachToD3D12Device(void*							pd3d12NativeDevice,
                                              class ICommandQueueD3D12*		pCommandQueue,
                                              const EngineD3D12CreateInfo&  EngineCI,
                                              IRenderDevice**				ppDevice,
                                              IDeviceContext**				ppContexts);
```

The method takes a pointer to the native D3D12 device `pd3d12NativeDevice`, initialization parameters `EngineCI`,
and returns diligent engine device interface in `ppDevice`, and diligent engine contexts in `ppContexts`. Pointer to the
immediate goes at position 0. If `EngineCI.NumDeferredContexts` > 0, pointers to deferred contexts go afterwards.
The function also takes a pointer to the command queue object `pCommandQueue`, which needs to implement
`ICommandQueueD3D12` interface.

For more information about interoperability with Direct3D12, please visit [Diligent Engine web site](http://diligentgraphics.com/diligent-engine/native-api-interoperability/direct3d12-interoperability/)

# References

[Interoperability with Direct3D12](http://diligentgraphics.com/diligent-engine/native-api-interoperability/direct3d12-interoperability/)

[Architecture of D3D12-based implementation](http://diligentgraphics.com/diligent-engine/architecture/D3D12)

-------------------

[diligentgraphics.com](http://diligentgraphics.com)

[![Diligent Engine on Twitter](https://github.com/DiligentGraphics/DiligentCore/blob/master/media/twitter.png)](https://twitter.com/diligentengine)
[![Diligent Engine on Facebook](https://github.com/DiligentGraphics/DiligentCore/blob/master/media/facebook.png)](https://www.facebook.com/DiligentGraphics/)
