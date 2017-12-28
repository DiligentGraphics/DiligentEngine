#include "RenderAPI.h"
#include "PlatformBase.h"
#include "Unity/IUnityGraphics.h"
#include "GraphicsAccessories.h"

using namespace Diligent;

void RenderAPI::CreateTextureViews(Diligent::ITexture *pRenderTarget, Diligent::ITexture *pDepthBuffer)
{
    m_RTV.Release();
    m_DSV.Release();
    {
        const auto &RTDesc = pRenderTarget->GetDesc();
        TextureViewDesc RTVDesc;
        RTVDesc.ViewType = TEXTURE_VIEW_RENDER_TARGET;
        m_RenderTargetFormat = GetDefaultTextureViewFormat(RTDesc, TEXTURE_VIEW_RENDER_TARGET);
        RTVDesc.Format = m_RenderTargetFormat;
        pRenderTarget->CreateView(RTVDesc, &m_RTV);
    }

    {
        const auto &TexDesc = pDepthBuffer->GetDesc();
        TextureViewDesc DSVDesc;
        DSVDesc.ViewType = TEXTURE_VIEW_DEPTH_STENCIL;
        m_DepthBufferFormat = GetDefaultTextureViewFormat(TexDesc, TEXTURE_VIEW_DEPTH_STENCIL);
        DSVDesc.Format = m_DepthBufferFormat;
        pDepthBuffer->CreateView(DSVDesc, &m_DSV);
    }
}
