#pragma once

#include <vector>

#include "Common/interface/RefCntAutoPtr.hpp"
#include "RenderDevice.h"
#include "DeviceContext.h"
#include "SwapChain.h"
#include "Common/interface/BasicMath.hpp"

namespace Diligent
{

class ImGuiImplDiligent;

struct SampleInitInfo
{
    IEngineFactory*    pEngineFactory = nullptr;
    IRenderDevice*     pDevice        = nullptr;
    IDeviceContext**   ppContexts     = nullptr;
    Uint32             NumDeferredCtx = 0;
    ISwapChain*        pSwapChain     = nullptr;
    ImGuiImplDiligent* pImGui         = nullptr;
};

class Sample
{
public:
    virtual ~Sample() {}

    virtual void GetEngineInitializationAttribs(RENDER_DEVICE_TYPE DeviceType, EngineCreateInfo& EngineCI, SwapChainDesc& SCDesc);

    virtual void Initialize(const SampleInitInfo& InitInfo)  = 0;

    virtual void Render()                                    = 0;
    virtual void Update(double CurrTime, double ElapsedTime) = 0;
    virtual void PreWindowResize() {}
    virtual void WindowResize(Uint32 Width, Uint32 Height) {}
    virtual bool HandleNativeMessage(const void* pNativeMsgData) { return false; }

    virtual const Char* GetSampleName() const { return "Diligent Engine Sample"; }
    virtual void        ProcessCommandLine(const char* CmdLine) {}

    void ResetSwapChain(ISwapChain* pNewSwapChain)
    {
        m_pSwapChain = pNewSwapChain;
    }

protected:
    // Returns projection matrix adjusted to the current screen orientation
    float4x4 GetAdjustedProjectionMatrix(float FOV, float NearPlane, float FarPlane) const;

    // Returns pretransform matrix that matches the current screen rotation
    float4x4 GetSurfacePretransformMatrix(const float3& f3CameraViewAxis) const;

    RefCntAutoPtr<IEngineFactory>              m_pEngineFactory;
    RefCntAutoPtr<IRenderDevice>               m_pDevice;
    RefCntAutoPtr<IDeviceContext>              m_pImmediateContext;
    std::vector<RefCntAutoPtr<IDeviceContext>> m_pDeferredContexts;
    RefCntAutoPtr<ISwapChain>                  m_pSwapChain;
    ImGuiImplDiligent*                         m_pImGui = nullptr;

    float  m_fSmoothFPS         = 0;
    double m_LastFPSTime        = 0;
    Uint32 m_NumFramesRendered  = 0;
    Uint32 m_CurrentFrameNumber = 0;
};

inline void Sample::Update(double CurrTime, double ElapsedTime)
{
    ++m_NumFramesRendered;
    ++m_CurrentFrameNumber;
    static const double dFPSInterval = 0.5;
    if (CurrTime - m_LastFPSTime > dFPSInterval)
    {
        m_fSmoothFPS        = static_cast<float>(m_NumFramesRendered / (CurrTime - m_LastFPSTime));
        m_NumFramesRendered = 0;
        m_LastFPSTime       = CurrTime;
    }
}

inline void Sample::Initialize(const SampleInitInfo& InitInfo)
{
    m_pEngineFactory    = InitInfo.pEngineFactory;
    m_pDevice           = InitInfo.pDevice;
    m_pSwapChain        = InitInfo.pSwapChain;
    m_pImmediateContext = InitInfo.ppContexts[0];
    m_pDeferredContexts.resize(InitInfo.NumDeferredCtx);
    for (Uint32 ctx = 0; ctx < InitInfo.NumDeferredCtx; ++ctx)
        m_pDeferredContexts[ctx] = InitInfo.ppContexts[1 + ctx];
    m_pImGui = InitInfo.pImGui;
}

extern Sample* CreateSample();

} // namespace Diligent
