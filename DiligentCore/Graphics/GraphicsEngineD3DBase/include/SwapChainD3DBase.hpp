/*
 *  Copyright 2019-2020 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *  
 *      http://www.apache.org/licenses/LICENSE-2.0
 *  
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
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

#pragma once

#include <VersionHelpers.h>
#include "SwapChainBase.hpp"

/// \file
/// Base implementation of a D3D swap chain

namespace Diligent
{

/// Base implementation of a D3D swap chain
template <class BaseInterface, typename DXGISwapChainType>
class SwapChainD3DBase : public SwapChainBase<BaseInterface>
{
public:
    using TBase = SwapChainBase<BaseInterface>;
    SwapChainD3DBase(IReferenceCounters*       pRefCounters,
                     IRenderDevice*            pDevice,
                     IDeviceContext*           pDeviceContext,
                     const SwapChainDesc&      SCDesc,
                     const FullScreenModeDesc& FSDesc,
                     const NativeWindow&       Window) :
        // clang-format off
        TBase{pRefCounters, pDevice, pDeviceContext, SCDesc},
        m_FSDesc           {FSDesc},
        m_Window           {Window},
        m_MaxFrameLatency  {SCDesc.BufferCount}
    // clang-format on
    {
        if (m_DesiredPreTransform != SURFACE_TRANSFORM_OPTIMAL &&
            m_DesiredPreTransform != SURFACE_TRANSFORM_IDENTITY)
        {
            LOG_WARNING_MESSAGE(GetSurfaceTransformString(m_DesiredPreTransform),
                                " is not an allowed pretransform because Direct3D swap chains only support identity transform. "
                                "Use SURFACE_TRANSFORM_OPTIMAL (recommended) or SURFACE_TRANSFORM_IDENTITY.");
        }
        m_DesiredPreTransform        = SURFACE_TRANSFORM_OPTIMAL;
        m_SwapChainDesc.PreTransform = SURFACE_TRANSFORM_IDENTITY;
    }

    ~SwapChainD3DBase()
    {
        if (m_pSwapChain)
        {
            // Swap chain must be in windowed mode when it is destroyed
            // https://msdn.microsoft.com/en-us/library/windows/desktop/bb205075(v=vs.85).aspx#Destroying
            BOOL IsFullScreen = FALSE;
            m_pSwapChain->GetFullscreenState(&IsFullScreen, nullptr);
            if (IsFullScreen)
                m_pSwapChain->SetFullscreenState(FALSE, nullptr);
        }
    }

protected:
    virtual void UpdateSwapChain(bool CreateNew) = 0;

    bool Resize(Uint32 NewWidth, Uint32 NewHeight, SURFACE_TRANSFORM NewPreTransform, Int32 Dummy = 0 /*To be different from virtual function*/)
    {
        if (NewPreTransform != SURFACE_TRANSFORM_OPTIMAL &&
            NewPreTransform != SURFACE_TRANSFORM_IDENTITY)
        {
            LOG_WARNING_MESSAGE(GetSurfaceTransformString(NewPreTransform),
                                " is not an allowed pretransform because Direct3D swap chains only support identity transform. "
                                "Use SURFACE_TRANSFORM_OPTIMAL (recommended) or SURFACE_TRANSFORM_IDENTITY.");
        }
        NewPreTransform = SURFACE_TRANSFORM_OPTIMAL;

        return TBase::Resize(NewWidth, NewHeight, NewPreTransform);
    }

    void CreateDXGISwapChain(IUnknown* pD3D11DeviceOrD3D12CmdQueue)
    {
#if PLATFORM_WIN32
        auto hWnd = reinterpret_cast<HWND>(m_Window.hWnd);
        if (m_SwapChainDesc.Width == 0 || m_SwapChainDesc.Height == 0)
        {
            RECT rc;
            if (m_FSDesc.Fullscreen)
            {
                const HWND hDesktop = GetDesktopWindow();
                GetWindowRect(hDesktop, &rc);
            }
            else
            {
                GetClientRect(hWnd, &rc);
            }
            m_SwapChainDesc.Width  = rc.right - rc.left;
            m_SwapChainDesc.Height = rc.bottom - rc.top;
        }
#endif

        VERIFY(m_DesiredPreTransform == SURFACE_TRANSFORM_OPTIMAL || m_DesiredPreTransform == SURFACE_TRANSFORM_IDENTITY,
               "Direct3D swap chains only support identity transform.");
        m_DesiredPreTransform        = SURFACE_TRANSFORM_OPTIMAL;
        m_SwapChainDesc.PreTransform = SURFACE_TRANSFORM_IDENTITY;

        auto DXGIColorBuffFmt = TexFormatToDXGI_Format(m_SwapChainDesc.ColorBufferFormat);

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};

        swapChainDesc.Width  = m_SwapChainDesc.Width;
        swapChainDesc.Height = m_SwapChainDesc.Height;
        //  Flip model swapchains (DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL and DXGI_SWAP_EFFECT_FLIP_DISCARD) only support the following Formats:
        //  - DXGI_FORMAT_R16G16B16A16_FLOAT
        //  - DXGI_FORMAT_B8G8R8A8_UNORM
        //  - DXGI_FORMAT_R8G8B8A8_UNORM
        //  - DXGI_FORMAT_R10G10B10A2_UNORM
        // If RGBA8_UNORM_SRGB swap chain is required, we will create RGBA8_UNORM swap chain, but
        // create RGBA8_UNORM_SRGB render target view
        switch (DXGIColorBuffFmt)
        {
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
                swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                break;

            case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
                swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
                break;

            default:
                swapChainDesc.Format = DXGIColorBuffFmt;
        }

        swapChainDesc.Stereo = FALSE;

        // Multi-sampled swap chains are not supported anymore. CreateSwapChainForHwnd() fails when sample count is not 1
        // for any swap effect.
        swapChainDesc.SampleDesc.Count   = 1;
        swapChainDesc.SampleDesc.Quality = 0;

        DEV_CHECK_ERR(m_SwapChainDesc.Usage != 0, "No swap chain usage flags defined");
        swapChainDesc.BufferUsage = 0;
        if (m_SwapChainDesc.Usage & SWAP_CHAIN_USAGE_RENDER_TARGET)
            swapChainDesc.BufferUsage |= DXGI_USAGE_RENDER_TARGET_OUTPUT;
        if (m_SwapChainDesc.Usage & SWAP_CHAIN_USAGE_SHADER_INPUT)
            swapChainDesc.BufferUsage |= DXGI_USAGE_SHADER_INPUT;
        //if (m_SwapChainDesc.Usage & SWAP_CHAIN_USAGE_COPY_SOURCE)
        //    ;

        swapChainDesc.BufferCount = m_SwapChainDesc.BufferCount;
        swapChainDesc.Scaling     = DXGI_SCALING_NONE;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        // DXGI_SCALING_NONE is supported starting with Windows 8
        if (!IsWindows8OrGreater())
            swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
#endif

        // DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL is the flip presentation model, where the contents of the back
        // buffer is preserved after the call to Present. This flag cannot be used with multisampling.
        // The only swap effect that supports multisampling is DXGI_SWAP_EFFECT_DISCARD.
        // Windows Store apps must use DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL or DXGI_SWAP_EFFECT_FLIP_DISCARD.
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED; //  Transparency behavior is not specified

        // DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH enables an application to switch modes by calling
        // IDXGISwapChain::ResizeTarget(). When switching from windowed to fullscreen mode, the display
        // mode (or monitor resolution) will be changed to match the dimensions of the application window.
        swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;


        CComPtr<IDXGIFactory2> pDXGIFactory;

        HRESULT hr = CreateDXGIFactory1(__uuidof(pDXGIFactory), reinterpret_cast<void**>(static_cast<IDXGIFactory2**>(&pDXGIFactory)));
        CHECK_D3D_RESULT_THROW(hr, "Failed to create DXGI factory");

        // DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT enables querying a waitable object that can be
        // used to synchronize presentation with CPU timeline.
        // The flag is not supported in D3D11 fullscreen mode.
        if (!(m_FSDesc.Fullscreen && m_pRenderDevice->GetDeviceCaps().DevType == RENDER_DEVICE_TYPE_D3D11))
        {
            // We do not need pDXGIFactory3 itself, however DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT flag
            // is only supported starting with Windows 8.1, and so is IDXGIFactory3 interface. We query this
            // interface to check Windows 8.1.
            // Note that we can't use IsWindows8Point1OrGreater because unlike IsWindows8OrGreater, it returns
            // false if an application is not manifested for Windows 8.1 or Windows 10, even if the current
            // operating system version is Windows 8.1 or Windows 10.
            CComPtr<IDXGIFactory3> pDXGIFactory3;
            if (SUCCEEDED(pDXGIFactory.QueryInterface(&pDXGIFactory3)))
            {
                swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
            }
        }


        CComPtr<IDXGISwapChain1> pSwapChain1;

#if PLATFORM_WIN32

        DXGI_SWAP_CHAIN_FULLSCREEN_DESC FullScreenDesc = {};

        FullScreenDesc.Windowed                = m_FSDesc.Fullscreen ? FALSE : TRUE;
        FullScreenDesc.RefreshRate.Numerator   = m_FSDesc.RefreshRateNumerator;
        FullScreenDesc.RefreshRate.Denominator = m_FSDesc.RefreshRateDenominator;
        FullScreenDesc.Scaling                 = static_cast<DXGI_MODE_SCALING>(m_FSDesc.Scaling);
        FullScreenDesc.ScanlineOrdering        = static_cast<DXGI_MODE_SCANLINE_ORDER>(m_FSDesc.ScanlineOrder);

        hr = pDXGIFactory->CreateSwapChainForHwnd(pD3D11DeviceOrD3D12CmdQueue, hWnd, &swapChainDesc, &FullScreenDesc, nullptr, &pSwapChain1);
        CHECK_D3D_RESULT_THROW(hr, "Failed to create Swap Chain");

        {
            // This is silly, but IDXGIFactory used for MakeWindowAssociation must be retrieved via
            // calling IDXGISwapchain::GetParent first, otherwise it won't work
            // https://www.gamedev.net/forums/topic/634235-dxgidisabling-altenter/?do=findComment&comment=4999990
            CComPtr<IDXGIFactory1> pFactoryFromSC;
            if (SUCCEEDED(pSwapChain1->GetParent(__uuidof(pFactoryFromSC), (void**)&pFactoryFromSC)))
            {
                // Do not allow the swap chain to handle Alt+Enter
                pFactoryFromSC->MakeWindowAssociation(hWnd, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER);
            }
        }

#elif PLATFORM_UNIVERSAL_WINDOWS

        if (m_FSDesc.Fullscreen)
            LOG_WARNING_MESSAGE("UWP applications do not support fullscreen mode");

        hr = pDXGIFactory->CreateSwapChainForCoreWindow(
            pD3D11DeviceOrD3D12CmdQueue,
            reinterpret_cast<IUnknown*>(m_Window.pCoreWindow),
            &swapChainDesc,
            nullptr,
            &pSwapChain1);
        CHECK_D3D_RESULT_THROW(hr, "Failed to create DXGI swap chain");

#endif

        hr = pSwapChain1.QueryInterface(&m_pSwapChain);
        CHECK_D3D_RESULT_THROW(hr, "Failed to query the required swap chain interface");

        if ((swapChainDesc.Flags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT) != 0)
        {
            CComPtr<IDXGISwapChain2> pSwapChain2;
            if (SUCCEEDED(pSwapChain1.QueryInterface(&pSwapChain2)))
            {
                // IMPORTANT: SetMaximumFrameLatency must be called BEFORE GetFrameLatencyWaitableObject!
                pSwapChain2->SetMaximumFrameLatency(m_MaxFrameLatency);

                m_FrameLatencyWaitableObject = pSwapChain2->GetFrameLatencyWaitableObject();
                VERIFY(m_FrameLatencyWaitableObject != NULL, "Waitable object must not be null");
            }
        }
        else
        {
            m_FrameLatencyWaitableObject = NULL;
            SetDXGIDeviceMaximumFrameLatency();
        }
    }

    void WaitForFrame()
    {
        // https://docs.microsoft.com/en-us/windows/uwp/gaming/reduce-latency-with-dxgi-1-3-swap-chains#step-4-wait-before-rendering-each-frame
        if (m_FrameLatencyWaitableObject != NULL)
        {
            auto Res = WaitForSingleObjectEx(m_FrameLatencyWaitableObject,
                                             500, // 0.5 second timeout (shouldn't ever occur)
                                             true);
            if (Res != WAIT_OBJECT_0)
            {
                const char* ErrorMsg = Res == WAIT_TIMEOUT ?
                    "Timeout elapsed while waiting for the frame waitable object." :
                    "Waiting for the frame waitable object failed.";
                LOG_ERROR_MESSAGE(ErrorMsg, " This is a strong indication of a synchronization error.");
            }
        }
    }

    virtual void DILIGENT_CALL_TYPE SetFullscreenMode(const DisplayModeAttribs& DisplayMode) override final
    {
        if (m_pSwapChain)
        {
            // If we are already in fullscreen mode, we need to switch to windowed mode first,
            // because a swap chain must be in windowed mode when it is released.
            // https://msdn.microsoft.com/en-us/library/windows/desktop/bb205075(v=vs.85).aspx#Destroying
            if (m_FSDesc.Fullscreen)
                m_pSwapChain->SetFullscreenState(FALSE, nullptr);
            m_FSDesc.Fullscreen             = True;
            m_FSDesc.RefreshRateNumerator   = DisplayMode.RefreshRateNumerator;
            m_FSDesc.RefreshRateDenominator = DisplayMode.RefreshRateDenominator;
            m_FSDesc.Scaling                = DisplayMode.Scaling;
            m_FSDesc.ScanlineOrder          = DisplayMode.ScanlineOrder;
            m_SwapChainDesc.Width           = DisplayMode.Width;
            m_SwapChainDesc.Height          = DisplayMode.Height;
            if (DisplayMode.Format != TEX_FORMAT_UNKNOWN)
                m_SwapChainDesc.ColorBufferFormat = DisplayMode.Format;
            UpdateSwapChain(true);
        }
    }

    virtual void DILIGENT_CALL_TYPE SetWindowedMode() override final
    {
        if (m_FSDesc.Fullscreen)
        {
            m_FSDesc.Fullscreen = False;
            m_pSwapChain->SetFullscreenState(FALSE, nullptr);
        }
    }

    virtual void DILIGENT_CALL_TYPE SetMaximumFrameLatency(Uint32 MaxLatency) override final
    {
        if (m_MaxFrameLatency == MaxLatency)
            return;

        m_MaxFrameLatency = MaxLatency;

        if (m_FrameLatencyWaitableObject != NULL)
        {
            // A swap chain must be in windowed mode when it is released.
            // https://msdn.microsoft.com/en-us/library/windows/desktop/bb205075(v=vs.85).aspx#Destroying
            if (m_FSDesc.Fullscreen)
            {
                // SetFullscreenState(FALSE) calls Resize that initializes Width and Height
                // with the window size. We need to save current values and restore them.
                Uint32 Width  = m_SwapChainDesc.Width;
                Uint32 Height = m_SwapChainDesc.Height;
                m_pSwapChain->SetFullscreenState(FALSE, nullptr);
                m_SwapChainDesc.Width  = Width;
                m_SwapChainDesc.Height = Height;
            }

            // Destroying the swap chain and creating a new one is the only reliable way to
            // change the maximum frame latency of a waitable swap chain.
            UpdateSwapChain(true);
        }
        else
        {
            SetDXGIDeviceMaximumFrameLatency();
        }
    }

    virtual void SetDXGIDeviceMaximumFrameLatency() {}

    FullScreenModeDesc         m_FSDesc;
    CComPtr<DXGISwapChainType> m_pSwapChain;
    NativeWindow               m_Window;

    HANDLE m_FrameLatencyWaitableObject = NULL;

    Uint32 m_MaxFrameLatency = 0;
};

} // namespace Diligent
