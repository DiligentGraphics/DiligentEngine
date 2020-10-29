#pragma once
#include <memory>
#include <iomanip>
#include <iostream>

#ifndef NOMINMAX
#    define NOMINMAX
#endif
#include <Windows.h>
#include <crtdbg.h>

#ifndef PLATFORM_WIN32
#    define PLATFORM_WIN32 1
#endif

#ifndef ENGINE_DLL
#    define ENGINE_DLL 1
#endif

#ifndef D3D11_SUPPORTED
#    define D3D11_SUPPORTED 1
#endif

#ifndef D3D12_SUPPORTED
#    define D3D12_SUPPORTED 1
#endif

#ifndef GL_SUPPORTED
#    define GL_SUPPORTED 1
#endif

#ifndef VULKAN_SUPPORTED
#    define VULKAN_SUPPORTED 1
#endif

#include "Graphics/GraphicsEngineD3D11/interface/EngineFactoryD3D11.h"
#include "Graphics/GraphicsEngineD3D12/interface/EngineFactoryD3D12.h"
#include "Graphics/GraphicsEngineOpenGL/interface/EngineFactoryOpenGL.h"
#include "Graphics/GraphicsEngineVulkan/interface/EngineFactoryVk.h"

#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/SwapChain.h"

#include "Common/interface/RefCntAutoPtr.hpp"

using namespace Diligent;

class Win32
{
public:
    Win32();

    ~Win32() { m_pImmediateContext->Flush(); }

    bool InitializeDiligentEngine(HWND hWnd);
    bool ProcessCommandLine(const char* CmdLine);

    void CreateResources();
    void Render();

    void Present() { m_pSwapChain->Present(); }

    void WindowResize(Uint32 Width, Uint32 Height) { if (m_pSwapChain)  m_pSwapChain->Resize(Width, Height); }

    RENDER_DEVICE_TYPE GetDeviceType() const { return m_DeviceType; }

private:
    RefCntAutoPtr<IRenderDevice>  m_pDevice;
    RefCntAutoPtr<IDeviceContext> m_pImmediateContext;
    RefCntAutoPtr<ISwapChain>     m_pSwapChain;
    RefCntAutoPtr<IPipelineState> m_pPSO;
    RENDER_DEVICE_TYPE            m_DeviceType = RENDER_DEVICE_TYPE_VULKAN;
};
