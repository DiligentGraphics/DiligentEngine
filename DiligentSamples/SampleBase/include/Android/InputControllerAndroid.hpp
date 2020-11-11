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

#include "BasicMath.hpp"

namespace Diligent
{

class InputControllerAndroid : public InputControllerBase
{
public:
    void BeginDrag(float x, float y)
    {
        EndPinch();
        m_MouseState.ButtonFlags |= MouseState::BUTTON_FLAG_LEFT;
        m_MouseState.PosX = x;
        m_MouseState.PosY = y;
    }

    void EndDrag()
    {
        m_MouseState.ButtonFlags &= ~MouseState::BUTTON_FLAG_LEFT;
    }

    void DragMove(float x, float y)
    {
        m_MouseState.ButtonFlags |= MouseState::BUTTON_FLAG_LEFT;
        m_MouseState.PosX = x;
        m_MouseState.PosY = y;
    }

    void StartPinch(float x0, float y0, float x1, float y1);
    void PinchMove(float x0, float y0, float x1, float y1);
    void EndPinch();

private:
    float2 m_PrvePinchPoint0;
    float2 m_PrvePinchPoint1;
};

} // namespace Diligent
