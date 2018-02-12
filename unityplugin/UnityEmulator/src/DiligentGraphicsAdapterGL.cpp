#include "DiligentGraphicsAdapterGL.h"

#if OPENGL_SUPPORTED

#include "UnityGraphicsGLCoreES_Emulator.h"
#include "RenderDeviceFactoryOpenGL.h"
#include "SwapChainBase.h"
#include "DefaultRawMemoryAllocator.h"

#include "UnityGraphicsGL_Impl.h"
#include "SwapChainGL.h"

using namespace Diligent;

namespace
{

class ProxySwapChainGL : public SwapChainBase<ISwapChainGL>
{
public:
    using TBase = SwapChainBase<ISwapChainGL>;

    ProxySwapChainGL( IReferenceCounters *pRefCounters,
                      const UnityGraphicsGLCoreES_Emulator& UnityGraphicsGL,
                      IRenderDevice *pDevice,
                      IDeviceContext *pDeviceContext,
                      const SwapChainDesc& SCDesc ) : 
        TBase(pRefCounters, pDevice, pDeviceContext,SCDesc),
        m_UnityGraphicsGL(UnityGraphicsGL)
    {}
        
    virtual void Present()override final
    {
        UNEXPECTED("Present is not expected to be called directly");
    }

    virtual void Resize(Uint32 NewWidth, Uint32 NewHeight)override final
    {
        TBase::Resize(NewWidth, NewHeight, 0);
    }

    virtual GLuint GetDefaultFBO()const override final
    {
        return m_UnityGraphicsGL.GetGraphicsImpl()->GetDefaultFBO();
    }
    
private:
    const UnityGraphicsGLCoreES_Emulator& m_UnityGraphicsGL;
};

}

DiligentGraphicsAdapterGL::DiligentGraphicsAdapterGL(const UnityGraphicsGLCoreES_Emulator& UnityGraphicsGL)noexcept :
    m_UnityGraphicsGL(UnityGraphicsGL)
{
    auto *UnityGraphicsGLImpl = UnityGraphicsGL.GetGraphicsImpl();

    auto *pFactoryGL = GetEngineFactoryOpenGL();
    EngineGLAttribs Attribs;
    pFactoryGL->AttachToActiveGLContext(Attribs, &m_pDevice, &m_pDeviceCtx);

    auto BackBufferGLFormat = UnityGraphicsGLImpl->GetBackBufferFormat();
    auto DepthBufferGLFormat = UnityGraphicsGLImpl->GetDepthBufferFormat();

    SwapChainDesc SCDesc;
    if(BackBufferGLFormat == GL_RGBA8)
        SCDesc.ColorBufferFormat = TEX_FORMAT_RGBA8_UNORM_SRGB;
    else
    {
        UNEXPECTED("Unexpected back buffer format");
    }

    if (DepthBufferGLFormat == GL_DEPTH_COMPONENT32F)
        SCDesc.DepthBufferFormat = TEX_FORMAT_D32_FLOAT;
    else if (DepthBufferGLFormat == GL_DEPTH_COMPONENT24)
        SCDesc.DepthBufferFormat = TEX_FORMAT_D24_UNORM_S8_UINT;
    else if (DepthBufferGLFormat == GL_DEPTH_COMPONENT16)
        SCDesc.DepthBufferFormat = TEX_FORMAT_D16_UNORM;
    else
    {
        UNEXPECTED("Unexpected depth buffer format");
    }

    SCDesc.Width = UnityGraphicsGLImpl->GetBackBufferWidth();
    SCDesc.Height = UnityGraphicsGLImpl->GetBackBufferHeight();
    // These fields are irrelevant
    SCDesc.SamplesCount = 0;
    SCDesc.BufferCount = 0;

    auto &DefaultAllocator = DefaultRawMemoryAllocator::GetAllocator();
    auto pProxySwapChainGL = NEW_RC_OBJ(DefaultAllocator, "ProxySwapChainGL instance", ProxySwapChainGL)(m_UnityGraphicsGL, m_pDevice, m_pDeviceCtx, SCDesc);
    pProxySwapChainGL->QueryInterface(IID_SwapChain, reinterpret_cast<IObject**>(static_cast<ISwapChain**>(&m_pProxySwapChain)));

    m_pDeviceCtx->SetSwapChain(m_pProxySwapChain);
}

void DiligentGraphicsAdapterGL::BeginFrame()
{
    auto *UnityGraphicsGLImpl = m_UnityGraphicsGL.GetGraphicsImpl();
    Uint32 Width = UnityGraphicsGLImpl->GetBackBufferWidth();
    Uint32 Height = UnityGraphicsGLImpl->GetBackBufferHeight();
    ValidatedCast<ProxySwapChainGL>(m_pProxySwapChain.RawPtr())->Resize(Width, Height);
}
    
void DiligentGraphicsAdapterGL::EndFrame()
{
    m_pDeviceCtx->InvalidateState();
}

bool DiligentGraphicsAdapterGL::UsesReverseZ()
{ 
    return m_UnityGraphicsGL.UsesReverseZ(); 
}

#endif // OPENGL_SUPPORTED
