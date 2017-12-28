#include "UnityGraphicsEmulator.h"
#include "DebugUtilities.h"
#include "IUnityGraphics.h"

UnityGraphicsEmulator *UnityGraphicsEmulator::m_Instance = nullptr;

UnityGfxRenderer UNITY_INTERFACE_API GetUnityRenderer()
{
    return UnityGraphicsEmulator::GetInstance().GetUnityGfxRenderer();
}

void UNITY_INTERFACE_API RegisterUnityDeviceEventCallback(IUnityGraphicsDeviceEventCallback callback)
{
    UnityGraphicsEmulator::GetInstance().RegisterDeviceEventCallback(callback);
}

void UNITY_INTERFACE_API UnregisterUnityDeviceEventCallback(IUnityGraphicsDeviceEventCallback callback)
{
    UnityGraphicsEmulator::GetInstance().UnregisterDeviceEventCallback(callback);
}

void UnityGraphicsEmulator::RegisterDeviceEventCallback(IUnityGraphicsDeviceEventCallback callback)
{
    m_DeviceEventCallbacks.emplace_back(callback);
}

void UnityGraphicsEmulator::UnregisterDeviceEventCallback(IUnityGraphicsDeviceEventCallback callback)
{
    size_t i = 0;
    while (i < m_DeviceEventCallbacks.size())
    {
        if (m_DeviceEventCallbacks[i] == callback)
            m_DeviceEventCallbacks.erase(m_DeviceEventCallbacks.begin() + i);
        else
            ++i;
    }
}

void UnityGraphicsEmulator::InvokeDeviceEventCallback(UnityGfxDeviceEventType eventType)
{
    for (auto callback : m_DeviceEventCallbacks)
        (*callback)(eventType);
}

//static void Get
UnityGraphicsEmulator::UnityGraphicsEmulator()
{ 
    VERIFY(m_Instance == nullptr, "Only single instance of UnityGraphicsEmulator must be initialized");
    m_Instance = this; 
    static IUnityGraphics UnityGraphics;
    UnityGraphics.GetRenderer = GetUnityRenderer;
    UnityGraphics.RegisterDeviceEventCallback = RegisterUnityDeviceEventCallback;
    UnityGraphics.UnregisterDeviceEventCallback = UnregisterUnityDeviceEventCallback;
    RegisterInterface(IUnityGraphics_GUID, &UnityGraphics);
}

static IUnityInterface* UNITY_INTERFACE_API GetUnityInterface(UnityInterfaceGUID guid)
{
    return UnityGraphicsEmulator::GetInstance().GetInterface(guid);
}

static void UNITY_INTERFACE_API RegisterUnityInterface(UnityInterfaceGUID guid, IUnityInterface* ptr)
{
    return UnityGraphicsEmulator::GetInstance().RegisterInterface(guid, ptr);
}

void UnityGraphicsEmulator::RegisterInterface(const UnityInterfaceGUID &guid, IUnityInterface* ptr)
{
    m_Interfaces.emplace_back(guid, ptr);
}

IUnityInterface* UnityGraphicsEmulator::GetInterface(const UnityInterfaceGUID &guid)
{
    for (auto &ifaces : m_Interfaces)
    {
        if (ifaces.first == guid)
            return ifaces.second;
    }
    return nullptr;
}


IUnityInterfaces &UnityGraphicsEmulator::GeUnityInterfaces()
{
    static IUnityInterfaces UnityInterfaces;
    UnityInterfaces.GetInterface = GetUnityInterface;
    UnityInterfaces.RegisterInterface = RegisterUnityInterface;

    return UnityInterfaces;
}
