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

#include <vector>

#include "ObjectBase.hpp"
#include "RefCntAutoPtr.hpp"
#include "RenderDevice.h"
#include "DeviceContext.h"
#include "SwapChain.h"
#include "Texture.h"
#include "TextureView.h"
#include "TestingEnvironment.hpp"

namespace Diligent
{

namespace Testing
{

void CompareTestImages(const Uint8*   pReferencePixels,
                       Uint32         RefPixelsStride,
                       const Uint8*   pPixels,
                       Uint32         PixelsStride,
                       Uint32         Width,
                       Uint32         Height,
                       TEXTURE_FORMAT Format);

// {41BF4655-9B33-4E6C-9300-0CB45FBFE104}
static constexpr INTERFACE_ID IID_TestingSwapChain =
    {0x41bf4655, 0x9b33, 0x4e6c, {0x93, 0x0, 0xc, 0xb4, 0x5f, 0xbf, 0xe1, 0x4}};

class ITestingSwapChain : public IObject
{
public:
    virtual void TakeSnapshot() = 0;

    virtual ITextureView* GetCurrentBackBufferUAV() = 0;
};

template <typename SwapChainInterface>
class SwapChainCombinedBaseInterface : public ITestingSwapChain, public SwapChainInterface
{};

template <class TSwapChainInterface>
class TestingSwapChainBase : public RefCountedObject<SwapChainCombinedBaseInterface<TSwapChainInterface>>
{
public:
    using TObjectBase = RefCountedObject<SwapChainCombinedBaseInterface<TSwapChainInterface>>;

    TestingSwapChainBase(IReferenceCounters*  pRefCounters,
                         IRenderDevice*       pDevice,
                         IDeviceContext*      pContext,
                         const SwapChainDesc& SCDesc) :
        TObjectBase{pRefCounters},
        m_SwapChainDesc{SCDesc},
        m_pDevice{pDevice},
        m_pContext{pContext}
    {
        VERIFY_EXPR(m_SwapChainDesc.ColorBufferFormat != TEX_FORMAT_UNKNOWN);
        VERIFY_EXPR(m_SwapChainDesc.Width != 0);
        VERIFY_EXPR(m_SwapChainDesc.Height != 0);

        {
            TextureDesc RenderTargetDesc;
            RenderTargetDesc.Name        = "Testing color buffer";
            RenderTargetDesc.Type        = RESOURCE_DIM_TEX_2D;
            RenderTargetDesc.Width       = m_SwapChainDesc.Width;
            RenderTargetDesc.Height      = m_SwapChainDesc.Height;
            RenderTargetDesc.Format      = m_SwapChainDesc.ColorBufferFormat;
            RenderTargetDesc.SampleCount = 1;
            RenderTargetDesc.Usage       = USAGE_DEFAULT;
            RenderTargetDesc.BindFlags   = BIND_RENDER_TARGET;
            if (pDevice->GetDeviceCaps().Features.ComputeShaders)
                RenderTargetDesc.BindFlags |= BIND_UNORDERED_ACCESS;
            m_pDevice->CreateTexture(RenderTargetDesc, nullptr, static_cast<ITexture**>(&m_pRenderTarget));
            VERIFY_EXPR(m_pRenderTarget != nullptr);
            m_pRTV = m_pRenderTarget->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);
            VERIFY_EXPR(m_pRTV != nullptr);

            if (pDevice->GetDeviceCaps().Features.ComputeShaders)
            {
                m_pUAV = m_pRenderTarget->GetDefaultView(TEXTURE_VIEW_UNORDERED_ACCESS);
                VERIFY_EXPR(m_pUAV != nullptr);
            }

            RenderTargetDesc.Name           = "Staging color buffer copy";
            RenderTargetDesc.Usage          = USAGE_STAGING;
            RenderTargetDesc.CPUAccessFlags = CPU_ACCESS_READ;
            RenderTargetDesc.BindFlags      = BIND_NONE;
            m_pDevice->CreateTexture(RenderTargetDesc, nullptr, static_cast<ITexture**>(&m_pStagingTexture));
        }

        if (m_SwapChainDesc.DepthBufferFormat != TEX_FORMAT_UNKNOWN)
        {
            TextureDesc DepthBufferDesc;
            DepthBufferDesc.Name        = "Testing depth buffer";
            DepthBufferDesc.Type        = RESOURCE_DIM_TEX_2D;
            DepthBufferDesc.Width       = m_SwapChainDesc.Width;
            DepthBufferDesc.Height      = m_SwapChainDesc.Height;
            DepthBufferDesc.Format      = m_SwapChainDesc.DepthBufferFormat;
            DepthBufferDesc.SampleCount = 1;
            DepthBufferDesc.Usage       = USAGE_DEFAULT;
            DepthBufferDesc.BindFlags   = BIND_DEPTH_STENCIL;

            DepthBufferDesc.ClearValue.Format               = DepthBufferDesc.Format;
            DepthBufferDesc.ClearValue.DepthStencil.Depth   = m_SwapChainDesc.DefaultDepthValue;
            DepthBufferDesc.ClearValue.DepthStencil.Stencil = m_SwapChainDesc.DefaultStencilValue;

            m_pDevice->CreateTexture(DepthBufferDesc, nullptr, &m_pDepthBuffer);
            VERIFY_EXPR(m_pDepthBuffer != nullptr);
            m_pDSV = m_pDepthBuffer->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL);
            VERIFY_EXPR(m_pDSV != nullptr);
        }
    }

    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override
    {
        if (ppInterface == nullptr)
            return;

        if (IID == IID_SwapChain || IID == IID_Unknown)
        {
            *ppInterface = static_cast<TSwapChainInterface*>(this);
            (*ppInterface)->AddRef();
        }
        if (IID == IID_TestingSwapChain)
        {
            *ppInterface = static_cast<ITestingSwapChain*>(this);
            (*ppInterface)->AddRef();
        }
    }

    virtual void DILIGENT_CALL_TYPE Present(Uint32 SyncInterval = 1) override
    {
        m_pContext->SetRenderTargets(0, nullptr, nullptr, RESOURCE_STATE_TRANSITION_MODE_NONE);

        CopyTextureAttribs CopyInfo //
            {
                m_pRenderTarget,
                RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
                m_pStagingTexture,
                RESOURCE_STATE_TRANSITION_MODE_TRANSITION //
            };
        m_pContext->CopyTexture(CopyInfo);
        m_pContext->WaitForIdle();
        MappedTextureSubresource MapData;

        auto MapFlag = MAP_FLAG_DO_NOT_WAIT;
        if (m_pDevice->GetDeviceCaps().DevType == RENDER_DEVICE_TYPE_D3D11)
        {
            // As a matter of fact, we should be able to always use MAP_FLAG_DO_NOT_WAIT flag
            // as we flush the context and idle the GPU before mapping the staging texture.
            // Intel driver, however, still returns null unless we don't use D3D11_MAP_FLAG_DO_NOT_WAIT flag.
            MapFlag = MAP_FLAG_NONE;
        }

        m_pContext->MapTextureSubresource(m_pStagingTexture, 0, 0, MAP_READ, MapFlag, nullptr, MapData);
        CompareTestImages(m_ReferenceData.data(), m_ReferenceDataPitch, reinterpret_cast<const Uint8*>(MapData.pData), MapData.Stride,
                          m_SwapChainDesc.Width, m_SwapChainDesc.Height, m_SwapChainDesc.ColorBufferFormat);

        m_pContext->UnmapTextureSubresource(m_pStagingTexture, 0, 0);
    }

    virtual void DILIGENT_CALL_TYPE Resize(Uint32 NewWidth, Uint32 NewHeight, SURFACE_TRANSFORM NewPreTransform) override final
    {
        UNEXPECTED("Resizing testing swap chains is not supported");
    }

    virtual void DILIGENT_CALL_TYPE SetFullscreenMode(const DisplayModeAttribs& DisplayMode) override final
    {
        UNEXPECTED("Testing swap chain can't go into full screen mode");
    }

    virtual void DILIGENT_CALL_TYPE SetWindowedMode() override final
    {
        UNEXPECTED("Testing swap chain can't switch between windowed and full screen modes");
    }

    virtual void DILIGENT_CALL_TYPE SetMaximumFrameLatency(Uint32 MaxLatency) override final
    {
        UNEXPECTED("Testing swap chain can't set the maximum frame latency");
    }

    virtual ITextureView* DILIGENT_CALL_TYPE GetCurrentBackBufferRTV() override final
    {
        return m_pRTV;
    }

    virtual ITextureView* GetCurrentBackBufferUAV() override final
    {
        return m_pUAV;
    }

    virtual ITextureView* DILIGENT_CALL_TYPE GetDepthBufferDSV() override final
    {
        return m_pDSV;
    }

    virtual const SwapChainDesc& DILIGENT_CALL_TYPE GetDesc() const override final
    {
        return m_SwapChainDesc;
    }

protected:
    const SwapChainDesc           m_SwapChainDesc;
    RefCntAutoPtr<IRenderDevice>  m_pDevice;
    RefCntAutoPtr<IDeviceContext> m_pContext;
    RefCntAutoPtr<ITexture>       m_pRenderTarget;
    RefCntAutoPtr<ITexture>       m_pDepthBuffer;
    RefCntAutoPtr<ITextureView>   m_pRTV;
    RefCntAutoPtr<ITextureView>   m_pUAV;
    RefCntAutoPtr<ITextureView>   m_pDSV;
    RefCntAutoPtr<ITexture>       m_pStagingTexture;

    std::vector<Uint8> m_ReferenceData;
    Uint32             m_ReferenceDataPitch = 0;
};

} // namespace Testing

} // namespace Diligent
