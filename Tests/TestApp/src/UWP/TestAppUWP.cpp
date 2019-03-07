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

#include "TestApp.h"
#include "RenderDeviceD3D12.h"
#include "RenderDeviceD3D11.h"
#include "SwapChainD3D12.h"
#include "SwapChainD3D11.h"
#include "EngineFactoryD3D11.h"
#include "EngineFactoryD3D12.h"

namespace Diligent
{

class TestAppUWP final : public TestApp
{
public:
    TestAppUWP()
    {
        m_DeviceType = DeviceType::D3D12;
    }

    virtual void OnWindowSizeChanged()override final
    {
        InitWindowSizeDependentResources();
    }

    virtual void Render()override
    {
        // Don't try to render anything before the first Update.
        if (m_timer.GetFrameCount() == 0)
        {
            return;
        }
        TestApp::Render();
        m_bFrameReady = true;
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
    }

    virtual void Present()override
    {
        m_pSwapChain->Present();

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
        InitializeDiligentEngine(nullptr);

        ID3D12Device *pd3d12Device = nullptr;
        ID3D11Device *pd3d11Device = nullptr;
        if (m_DeviceType == DeviceType::D3D12)
        {
            // Store pointers to the Direct3D 11.1 API device and immediate context.
            RefCntAutoPtr<IRenderDeviceD3D12> pRenderDeviceD3D12(m_pDevice, IID_RenderDeviceD3D12);
            pd3d12Device = pRenderDeviceD3D12->GetD3D12Device();
        }
        else if (m_DeviceType == DeviceType::D3D11)
        {
            RefCntAutoPtr<IRenderDeviceD3D11> pRenderDeviceD3D11(m_pDevice, IID_RenderDeviceD3D11);
            pd3d11Device = pRenderDeviceD3D11->GetD3D11Device();
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

        if (m_swapChain != nullptr)
        {
            m_swapChain.Reset();
        
            // If the swap chain already exists, resize it.
            m_pSwapChain->Resize(backBufferWidth, backBufferHeight);
        
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
        	//DXGI_SCALING scaling = DisplayMetrics::SupportHighResolutions ? DXGI_SCALING_NONE : DXGI_SCALING_STRETCH;
            SwapChainDesc  SCDesc;
            SCDesc.Width  = backBufferWidth;
            SCDesc.Height = backBufferHeight;
            SCDesc.ColorBufferFormat = TEX_FORMAT_RGBA8_UNORM_SRGB;
            SCDesc.DepthBufferFormat = TEX_FORMAT_D32_FLOAT;
            auto window = m_DeviceResources->GetWindow();
            IDXGISwapChain3 *pDXGISwapChain3 = nullptr;
            if (m_DeviceType == DeviceType::D3D12)
            {
                GetEngineFactoryD3D12()->CreateSwapChainD3D12(m_pDevice, m_pImmediateContext, SCDesc, FullScreenModeDesc{}, reinterpret_cast<IUnknown*>(window), &m_pSwapChain);
            }
            else if (m_DeviceType == DeviceType::D3D11)
            {
                GetEngineFactoryD3D11()->CreateSwapChainD3D11(m_pDevice, m_pImmediateContext, SCDesc, FullScreenModeDesc{}, reinterpret_cast<IUnknown*>(window), &m_pSwapChain);
            }
            else
                UNEXPECTED("Unexpected device type");
        }

        IDXGISwapChain3 *pDXGISwapChain3 = nullptr;
        if (m_DeviceType == DeviceType::D3D12)
        {
            RefCntAutoPtr<ISwapChainD3D12> pSwapChainD3D12(m_pSwapChain, IID_SwapChainD3D12);
            pSwapChainD3D12->GetDXGISwapChain()->QueryInterface(__uuidof(pDXGISwapChain3), reinterpret_cast<void**>(&pDXGISwapChain3));
        }
        else if (m_DeviceType == DeviceType::D3D11)
        {
            RefCntAutoPtr<ISwapChainD3D11> pSwapChainD3D11(m_pSwapChain, IID_SwapChainD3D11);
            pSwapChainD3D11->GetDXGISwapChain()->QueryInterface(__uuidof(pDXGISwapChain3), reinterpret_cast<void**>(&pDXGISwapChain3));
        }
        else
            UNEXPECTED("Unexpected device type");
        m_swapChain.Attach(pDXGISwapChain3);

        m_DeviceResources->SetSwapChainRotation(m_swapChain.Get());
    }

    virtual void CreateRenderers()override
    {
        InitializeRenderers();
    }

private:
    Microsoft::WRL::ComPtr<IDXGISwapChain3>	m_swapChain;
};


NativeAppBase* CreateApplication()
{
    return new TestAppUWP;
}

}
