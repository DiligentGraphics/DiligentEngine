/*     Copyright 2015-2018 Egor Yusov
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF ANY PROPRIETARY RIGHTS.
*
*  In no event and under no legal theory, whether in tort (including negligence),
*  contract, or otherwise, unless required by applicable law (such as deliberate
*  and grossly negligent acts) or agreed to in writing, shall any Contributor be
*  liable for any damages, including any direct, indirect, special, incidental,
*  or consequential damages of any character arising as a result of this License or
*  out of the use or inability to use the software (including but not limited to damages
*  for loss of goodwill, work stoppage, computer failure or malfunction, or any and
*  all other commercial damages or losses), even if such Contributor has been advised
*  of the possibility of such damages.
*/

#include "UnityAppBase.h"
#include "IUnityInterface.h"
#include "UnityGraphicsD3D11Emulator.h"
#include "UnityGraphicsD3D12Emulator.h"
#include "DiligentGraphicsAdapterD3D11.h"
#include "DiligentGraphicsAdapterD3D12.h"
#include "ValidatedCast.h"
#include "StringTools.h"
#include "Errors.h"

using namespace Diligent;

class UnityAppUWP final : public UnityAppBase
{
public:
    UnityAppUWP()
    {
        m_DeviceType = DeviceType::D3D12;
    }

    virtual void OnWindowSizeChanged()override final
    {
        InitWindowSizeDependentResources();

        if (m_SceneInitialized)
        {
            auto backBufferWidth = m_DeviceResources->GetBackBufferWidth();
            auto backBufferHeight = m_DeviceResources->GetBackBufferHeight();
            m_Scene->OnWindowResize(backBufferWidth, backBufferHeight);
        }
    }

    virtual void Render()override
    {
        // Don't try to render anything before the first Update.
        if (m_timer.GetFrameCount() == 0)
        {
            return;
        }
        UnityAppBase::Render();
        m_bFrameReady = true;
    }

    virtual void Present()override
    {
        m_GraphicsEmulator->Present();

        //// If the device was removed either by a disconnection or a driver upgrade, we 
        //// must recreate all device resources.
        //if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        //{
        //	m_deviceRemoved = true;
        //}
        //else
        //{
        //	DX::ThrowIfFailed(hr);
        //}
    }

    virtual std::shared_ptr<DX::DeviceResources> InitDeviceResources()override
    {
        InitGraphics(nullptr, 0, 0);

        ID3D12Device *pd3d12Device = nullptr;
        ID3D11Device *pd3d11Device = nullptr;
        if (m_DeviceType == DeviceType::D3D12)
        {
            auto &GraphicsD3D12Emulator = UnityGraphicsD3D12Emulator::GetInstance();
            pd3d12Device = reinterpret_cast<ID3D12Device*>(GraphicsD3D12Emulator.GetD3D12Device());
        }
        else if (m_DeviceType == DeviceType::D3D11)
        {
            auto &GraphicsD3D11Emulator = UnityGraphicsD3D11Emulator::GetInstance();
            pd3d11Device = reinterpret_cast<ID3D11Device*>(GraphicsD3D11Emulator.GetD3D11Device());
        }
        else
        {
            UNEXPECTED("Unexpected device type");
        }
        m_DeviceResources = std::make_shared<DX::DeviceResources>(pd3d11Device, pd3d12Device);

        return m_DeviceResources;
    }

    virtual void InitWindowSizeDependentResources()override
    {
        m_DeviceResources->UpdateRenderTargetSize();
        auto backBufferWidth = m_DeviceResources->GetBackBufferWidth();
        auto backBufferHeight = m_DeviceResources->GetBackBufferHeight();

        if (m_GraphicsEmulator->SwapChainInitialized())
        {
            m_DiligentGraphics->PreSwapChainResize();
            // If the swap chain already exists, resize it.
            m_GraphicsEmulator->ResizeSwapChain(backBufferWidth, backBufferHeight);

            m_DiligentGraphics->PostSwapChainResize();
#if 0
            if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
            {
                // If the device was removed for any reason, a new device and swap chain will need to be created.
                m_deviceRemoved = true;

                // Do not continue execution of this method. DeviceResources will be destroyed and re-created.
                return;
            }
            else
            {
                DX::ThrowIfFailed(hr);
            }
#endif
        }
        else
        {
            auto NativeWndHandle = reinterpret_cast<IUnknown*>(m_DeviceResources->GetWindow());
            switch (m_DeviceType)
            {
            case DeviceType::D3D11:
            {
                auto &GraphicsD3D11Emulator = UnityGraphicsD3D11Emulator::GetInstance();
                GraphicsD3D11Emulator.CreateSwapChain(NativeWndHandle, backBufferWidth, backBufferHeight);
                ValidatedCast<DiligentGraphicsAdapterD3D11>(m_DiligentGraphics.get())->InitProxySwapChain();
            }
            break;

            case DeviceType::D3D12:
            {
                auto &GraphicsD3D12Emulator = UnityGraphicsD3D12Emulator::GetInstance();
                GraphicsD3D12Emulator.CreateSwapChain(NativeWndHandle, backBufferWidth, backBufferHeight);
                ValidatedCast<DiligentGraphicsAdapterD3D12>(m_DiligentGraphics.get())->InitProxySwapChain();
            }
            break;

            default:
                UNEXPECTED("Unsupported device type");
            }
        }

        if (m_DeviceType == DeviceType::D3D12)
        {
            auto &GraphicsD3D12Emulator = UnityGraphicsD3D12Emulator::GetInstance();
            m_swapChain = reinterpret_cast<IDXGISwapChain3*>(GraphicsD3D12Emulator.GetDXGISwapChain());
        }
        else if (m_DeviceType == DeviceType::D3D11)
        {
            auto &GraphicsD3D11Emulator = UnityGraphicsD3D11Emulator::GetInstance();
            auto *pSwapChain1 = reinterpret_cast<IDXGISwapChain*>(GraphicsD3D11Emulator.GetDXGISwapChain());
            pSwapChain1->QueryInterface(__uuidof(m_swapChain), reinterpret_cast<void**>(static_cast<IDXGISwapChain3**>(&m_swapChain)));
        }
        else
            UNEXPECTED("Unexpected device type");
        m_DeviceResources->SetSwapChainRotation(m_swapChain.Get());
    }

    virtual void CreateRenderers()override
    {
        InitScene();
        m_SceneInitialized = true;
    }


    // Notifies the app that it is being suspended.
    virtual void OnSuspending()override final
    {
        // TODO: Replace this with your app's suspending logic.

        // Process lifetime management may terminate suspended apps at any time, so it is
        // good practice to save any state that will allow the app to restart where it left off.

        //m_sceneRenderer->SaveState();

        // If your application uses video memory allocations that are easy to re-create,
        // consider releasing that memory to make it available to other applications.
    }

    // Notifes the app that it is no longer suspended.
    virtual void OnResuming()override final
    {
        // TODO: Replace this with your app's resuming logic.
    }

    // Notifies renderers that device resources need to be released.
    virtual void OnDeviceRemoved()override final
    {
        // TODO: Save any necessary application or renderer state and release the renderer
        // and its resources which are no longer valid.
        //m_sceneRenderer->SaveState();
        m_Scene.reset();
    }

private:
    Microsoft::WRL::ComPtr<IDXGISwapChain3>	m_swapChain;
    bool m_SceneInitialized = false;
};

namespace Diligent
{

NativeAppBase* CreateApplication()
{
    return new UnityAppUWP;
}

}


HMODULE g_DLLHandle;
void* UnityAppBase::LoadPluginFunction(const char* FunctionName)
{
    auto Func = GetProcAddress(g_DLLHandle, FunctionName);
    VERIFY(Func != nullptr, "Failed to import plugin function \"", FunctionName, "\".");
    return Func;
}

bool UnityAppBase::LoadPlugin()
{
    std::string LibName = m_Scene->GetPluginName();

#if _WIN64
#   if _M_ARM >= 7
        LibName += "_arm";
#   else
        LibName += "_64";
#   endif
#else
#   if _M_ARM >= 7
        LibName += "_arm";
#   else
        LibName += "_32";
#   endif
#endif

#ifdef _DEBUG
    LibName += "d";
#else
    LibName += "r";
#endif

    LibName.append(".dll");

    auto wLibPath = WidenString(LibName);
    g_DLLHandle = LoadPackagedLibrary( wLibPath.c_str(), 0);
    if( g_DLLHandle == NULL )
    {
        LOG_ERROR_MESSAGE( "Failed to load ", LibName, " library." );
        return false;
    }

    UnityPluginLoad = reinterpret_cast<TUnityPluginLoad>( GetProcAddress(g_DLLHandle, "UnityPluginLoad") );
    UnityPluginUnload = reinterpret_cast<TUnityPluginUnload>( GetProcAddress(g_DLLHandle, "UnityPluginUnload") );
    GetRenderEventFunc = reinterpret_cast<TGetRenderEventFunc>( GetProcAddress(g_DLLHandle, "GetRenderEventFunc") );
    if( UnityPluginLoad == nullptr || UnityPluginUnload == nullptr || GetRenderEventFunc == nullptr )
    {
        LOG_ERROR_MESSAGE( "Failed to import plugin functions from ", LibName, " library." );
        FreeLibrary( g_DLLHandle );
        return false;
    }

    return true;
}

void UnityAppBase::UnloadPlugin()
{
    m_GraphicsEmulator->InvokeDeviceEventCallback(kUnityGfxDeviceEventShutdown);
    UnityPluginUnload();
    FreeLibrary(g_DLLHandle);
    g_DLLHandle = NULL;
}
