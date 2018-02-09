#pragma once

#include <vector>

#include "IUnityInterface.h"
#include "IUnityGraphics.h"


class UnityGraphicsEmulator
{
public:
    UnityGraphicsEmulator();
    virtual void Release() = 0;
    virtual void Present() = 0;
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    virtual void ResizeSwapChain(unsigned int Width, unsigned int Height) = 0;
    virtual bool SwapChainInitialized() = 0;
    virtual bool UsesReverseZ()const { return false; }
    virtual IUnityInterface* GetUnityGraphicsAPIInterface() = 0;
    IUnityInterfaces &GeUnityInterfaces();

    static UnityGraphicsEmulator& GetInstance() { return *m_Instance; }
    void RegisterInterface(const UnityInterfaceGUID &guid, IUnityInterface* ptr);
    IUnityInterface* GetInterface(const UnityInterfaceGUID &guid);
    virtual UnityGfxRenderer GetUnityGfxRenderer() = 0;

    void RegisterDeviceEventCallback(IUnityGraphicsDeviceEventCallback callback);
    void UnregisterDeviceEventCallback(IUnityGraphicsDeviceEventCallback callback);
    void InvokeDeviceEventCallback(UnityGfxDeviceEventType eventType);
    virtual void GetBackBufferSize(unsigned int &Width, unsigned int &Height) = 0;

private:
    UnityGraphicsEmulator(const UnityGraphicsEmulator&) = delete;
    UnityGraphicsEmulator(UnityGraphicsEmulator&&) = delete;
    UnityGraphicsEmulator& operator = (const UnityGraphicsEmulator&) = delete;
    UnityGraphicsEmulator& operator = (UnityGraphicsEmulator&&) = delete;

    std::vector< std::pair<UnityInterfaceGUID, IUnityInterface*> > m_Interfaces;
    static UnityGraphicsEmulator *m_Instance;
    std::vector<IUnityGraphicsDeviceEventCallback> m_DeviceEventCallbacks;
};
