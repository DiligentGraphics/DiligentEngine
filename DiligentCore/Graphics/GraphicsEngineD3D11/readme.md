
# GraphicsEngineD3D11

Implementation of Direct3D11 back-end

# Initialization

The following code snippet shows how to initialize Diligent Engine in Direct3D11 mode.

```cpp
#include "EngineFactoryD3D11.h"
using namespace Diligent;

// ...

EngineD3D11CreateInfo EngineCI;
EngineCI.DebugFlags =
    D3D11_DEBUG_FLAG_VERIFY_COMMITTED_SHADER_RESOURCES;

// Get pointer to the function that returns the factory
#if ENGINE_DLL
    // Load the dll and import GetEngineFactoryD3D11() function
    auto GetEngineFactoryD3D11 = LoadGraphicsEngineD3D11();
#endif
auto* pFactoryD3D11 = GetEngineFactoryD3D11();

RefCntAutoPtr<IRenderDevice> pRenderDevice;
RefCntAutoPtr<IDeviceContext> pImmediateContext;
SwapChainDesc SwapChainDesc;
RefCntAutoPtr<ISwapChain> pSwapChain;
pFactoryD3D11->CreateDeviceAndContextsD3D11(EngineCI, &pRenderDevice, &pImmediateContext);
NativeWindow Window;
Window.hWnd = hWnd;
pFactoryD3D11->CreateSwapChainD3D11(pRenderDevice, pImmediateContext, SwapChainDesc, Window, &pSwapChain);
```

Alternatively, the engine can be initialized by attaching to existing D3D11 device and immediate context (see below).

# Interoperability with Direct3D11

Diligent Engine exposes methods to access internal D3D11 objects, is able to create diligent engine buffers
and textures from existing Direct3D11 buffers and textures, and can be initialized by attaching to existing D3D11
device and immediate context.

## Accessing Native Direct3D11 objects

Below are some of the methods that provide access to internal D3D11 objects:

|                              Function                                       |                              Description                                                                      |
|-----------------------------------------------------------------------------|---------------------------------------------------------------------------------------------------------------|
| `ID3D11Buffer* IBufferD3D11::GetD3D11Buffer()`                              | returns a pointer to the `ID3D11Buffer` interface of the internal Direct3D11 buffer object                      |
| `ID3D11Resource* ITextureD3D11::GetD3D11Texture()`                          | returns a pointer to the `ID3D11Resource` interface of the internal Direct3D11 texture object                   |
| `ID3D11View* IBufferViewD3D11()::GetD3D11View()`                            | returns a pointer to the `ID3D11View` interface of the internal Direct3D11 object representing the buffer view       |
| `ID3D11View* ITextureViewD3D11::GetD3D11View()`                             | returns a pointer to the `ID3D11View` interface of the internal Direct3D11 object representing the texture view      |
| `ID3D11Device* IRenderDeviceD3D11::GetD3D11Device()`                        | returns a pointer to the native Direct3D11 device object                                                           |
| `ID3D11DeviceContext* IDeviceContextD3D11::GetD3D11DeviceContext()`         | returns a pointer to the native `ID3D11DeviceContext` object                                                    |

## Creating Diligent Engine Objects from Direct3D11 Resources

* `void IRenderDeviceD3D11::CreateBufferFromD3DResource(ID3D11Buffer* pd3d11Buffer, const BufferDesc& BuffDesc, RESOURCE_STATE InitialState, IBuffer** ppBuffer)` -
   creates a Diligent Engine buffer object from the native Direct3D11 buffer
* `void IRenderDeviceD3D11::CreateTextureFromD3DResource(ID3D11Texture1D* pd3d11Texture, RESOURCE_STATE InitialState, ITexture** ppTexture)` -
   create a Diligent Engine texture object from the native Direct3D11 1D texture
* `void IRenderDeviceD3D11::CreateTextureFromD3DResource(ID3D11Texture2D* pd3d11Texture, RESOURCE_STATE InitialState, ITexture** ppTexture)` -
   create a Diligent Engine texture object from the native Direct3D11 2D texture
* `void IRenderDeviceD3D11::CreateTextureFromD3DResource(ID3D11Texture3D* pd3d11Texture, RESOURCE_STATE InitialState, ITexture** ppTexture)` -
   create a Diligent Engine texture object from the native Direct3D11 3D texture

## Initializing the Engine by Attaching to Existing Direct3D11 Device and Immediate Context

The code snippet below shows how diligent engine can be attached to Direct3D11 device returned by Unity

```cpp
IUnityGraphicsD3D11* d3d = interfaces->Get<IUnityGraphicsD3D11>();
ID3D11Device* d3d11NativeDevice = d3d->GetDevice();
CComPtr<ID3D11DeviceContext> d3d11ImmediateContext;
d3d11NativeDevice->GetImmediateContext(&d3d11ImmediateContext);
auto* pFactoryD3d11 = GetEngineFactoryD3D11();
EngineD3D11CreateInfo EngineCI;
pFactoryD3d11->AttachToD3D11Device(d3d11NativeDevice, d3d11ImmediateContext, EngineCI, &m_Device, &m_Context, 0);
```

For more information about interoperability with Direct3D11, please visit [Diligent Engine web site](http://diligentgraphics.com/diligent-engine/native-api-interoperability/direct3d11-interoperability/)

# References

[Interoperability with Direct3D11](http://diligentgraphics.com/diligent-engine/native-api-interoperability/direct3d11-interoperability/)

[Architecture of D3D11-based implementation](http://diligentgraphics.com/diligent-engine/architecture/d3d11)

-------------------

[diligentgraphics.com](http://diligentgraphics.com)

[![Diligent Engine on Twitter](https://github.com/DiligentGraphics/DiligentCore/blob/master/media/twitter.png)](https://twitter.com/diligentengine)
[![Diligent Engine on Facebook](https://github.com/DiligentGraphics/DiligentCore/blob/master/media/facebook.png)](https://www.facebook.com/DiligentGraphics/)
