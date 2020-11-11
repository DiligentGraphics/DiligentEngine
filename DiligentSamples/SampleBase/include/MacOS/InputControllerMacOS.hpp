/*     Copyright 2015-2019 Egor Yusov
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

#pragma once

namespace Diligent
{

class InputControllerMacOS : public InputControllerBase
{
public:
    enum class MouseButtonEvent
    {
        LMB_Pressed,
        LMB_Released,
        RMB_Pressed,
        RMB_Released
    };
    void OnMouseButtonEvent(MouseButtonEvent Event);

    void OnMouseMove(int MouseX, int MouseY)
    {
        m_MouseState.PosX = static_cast<float>(MouseX);
        m_MouseState.PosY = static_cast<float>(MouseY);
    }

    void OnMouseWheel(float WheelDelta)
    {
        m_MouseState.WheelDelta = WheelDelta;
    }

    void OnKeyPressed(int key);
    void OnKeyReleased(int key);
    void OnFlagsChanged(bool ShiftPressed, bool CtrlPressed, bool AltPressed);

private:
    void ProcessKeyEvent(int key, bool IsKeyPressed);
};

} // namespace Diligent
