#pragma once

#include <memory>
#include "UnityGraphicsEmulator.h"

class UnityGraphicsD3D12Impl;
class IResourceStateTransitionHandler;

class UnityGraphicsD3D12Emulator final : public UnityGraphicsEmulator
{
public:
    static UnityGraphicsD3D12Emulator& GetInstance();
    void CreateD3D12DeviceAndCommandQueue();
    void CreateSwapChain(void *pNativeWndHandle, unsigned int Width, unsigned int Height);
    void SetTransitionHandler(IResourceStateTransitionHandler *pTransitionHandler);

    virtual void Present()override final;
    virtual void Release()override final;
    virtual void BeginFrame()override final;
    virtual void EndFrame()override final;
    virtual void ResizeSwapChain(unsigned int Width, unsigned int Height)override final;
    virtual bool UsesReverseZ()const override final { return true; }
    virtual IUnityInterface* GetUnityGraphicsAPIInterface()override final;
    static UnityGraphicsD3D12Impl* GetGraphicsImpl();
    virtual UnityGfxRenderer GetUnityGfxRenderer()override final;
    virtual bool SwapChainInitialized()override final;
    void* GetD3D12Device();
    void* GetDXGISwapChain();
    virtual void GetBackBufferSize(unsigned int &Width, unsigned int &Height)override final;

private:
    UnityGraphicsD3D12Emulator();
    static std::unique_ptr<UnityGraphicsD3D12Impl> m_GraphicsImpl;
};
