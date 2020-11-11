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

#include <queue>
#include "SampleApp.hpp"
#include "ImGuiImplIOS.hpp"

namespace Diligent
{

class SampleAppIOS final : public SampleApp
{
public:
    virtual void Initialize(int deviceType, void* layer) override final
    {
        m_DeviceType = static_cast<RENDER_DEVICE_TYPE>(deviceType);
        IOSNativeWindow IOSWindow{layer};
        InitializeDiligentEngine(&IOSWindow);
        const auto& SCDesc = m_pSwapChain->GetDesc();
        m_pImGui.reset(new ImGuiImplIOS(m_pDevice, SCDesc.ColorBufferFormat, SCDesc.DepthBufferFormat));
        InitializeSample();
    }

    virtual void Render() override
    {
        SampleApp::Render();
    }

    virtual void OnTouchBegan(float x, float y) override final
    {
        if (!static_cast<ImGuiImplIOS*>(m_pImGui.get())->OnTouchEvent(x, y, true))
        {
            m_TheSample->GetInputController().OnMouseButtonEvent(InputController::MouseButtonEvent::LMB_Pressed);
        }
        m_TheSample->GetInputController().OnMouseMove(x, y);
    }

    virtual void OnTouchMoved(float x, float y) override final
    {
        static_cast<ImGuiImplIOS*>(m_pImGui.get())->OnTouchEvent(x, y, true);
        m_TheSample->GetInputController().OnMouseMove(x, y);
    }

    virtual void OnTouchEnded(float x, float y) override final
    {
        static_cast<ImGuiImplIOS*>(m_pImGui.get())->OnTouchEvent(x, y, false);
        m_TheSample->GetInputController().OnMouseMove(x, y);
        m_TheSample->GetInputController().OnMouseButtonEvent(InputController::MouseButtonEvent::LMB_Released);
    }

private:
};

NativeAppBase* CreateApplication()
{
    return new SampleAppIOS;
}

} // namespace Diligent
