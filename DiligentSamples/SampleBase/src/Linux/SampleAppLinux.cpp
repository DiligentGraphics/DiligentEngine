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

#include "SampleApp.hpp"
#if VULKAN_SUPPORTED
#    include "ImGuiImplLinuxXCB.hpp"
#endif
#include "ImGuiImplLinuxX11.hpp"

namespace Diligent
{

class SampleAppLinux final : public SampleApp
{
public:
    SampleAppLinux()
    {
        m_DeviceType = RENDER_DEVICE_TYPE_GL;
    }

    ~SampleAppLinux()
    {
    }
    virtual bool OnGLContextCreated(Display* display, Window window) override final
    {
        try
        {
            LinuxNativeWindow LinuxWindow;
            LinuxWindow.pDisplay = display;
            LinuxWindow.WindowId = window;
            InitializeDiligentEngine(&LinuxWindow);
            const auto& SCDesc = m_pSwapChain->GetDesc();
            m_pImGui.reset(new ImGuiImplLinuxX11(m_pDevice, SCDesc.ColorBufferFormat, SCDesc.DepthBufferFormat, SCDesc.Width, SCDesc.Height));
            InitializeSample();
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    virtual int HandleXEvent(XEvent* xev) override final
    {
        auto handled = static_cast<ImGuiImplLinuxX11*>(m_pImGui.get())->HandleXEvent(xev);
        // Always handle mouse move, button release and key release events
        if (!handled || xev->type == ButtonRelease || xev->type == MotionNotify || xev->type == KeyRelease)
        {
            handled = m_TheSample->GetInputController().HandleXEvent(xev);
        }
        return handled;
    }

#if VULKAN_SUPPORTED
    virtual bool InitVulkan(xcb_connection_t* connection, uint32_t window) override final
    {
        try
        {
            m_DeviceType = RENDER_DEVICE_TYPE_VULKAN;
            LinuxNativeWindow LinuxWindow;
            LinuxWindow.WindowId       = window;
            LinuxWindow.pXCBConnection = connection;
            InitializeDiligentEngine(&LinuxWindow);
            const auto& SCDesc = m_pSwapChain->GetDesc();
            m_pImGui.reset(new ImGuiImplLinuxXCB(connection, m_pDevice, SCDesc.ColorBufferFormat, SCDesc.DepthBufferFormat, SCDesc.Width, SCDesc.Height));
            m_TheSample->GetInputController().InitXCBKeysms(connection);
            InitializeSample();
            return true;
        }
        catch (...)
        {
            return false;
        }
    }
    virtual void HandleXCBEvent(xcb_generic_event_t* event) override final
    {
        auto handled   = static_cast<ImGuiImplLinuxXCB*>(m_pImGui.get())->HandleXCBEvent(event);
        auto EventType = event->response_type & 0x7f;
        // Always handle mouse move, button release and key release events
        if (!handled || EventType == XCB_MOTION_NOTIFY || EventType == XCB_BUTTON_RELEASE || EventType == XCB_KEY_RELEASE)
        {
            handled = m_TheSample->GetInputController().HandleXCBEvent(event);
        }
    }
#endif
};

NativeAppBase* CreateApplication()
{
    return new SampleAppLinux;
}

} // namespace Diligent
