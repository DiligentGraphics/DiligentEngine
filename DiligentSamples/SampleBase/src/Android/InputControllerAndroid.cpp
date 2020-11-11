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

#include "InputController.hpp"

namespace Diligent
{

void InputControllerAndroid::StartPinch(float x0, float y0, float x1, float y1)
{
    EndDrag();
    m_PrvePinchPoint0 = float2{x0, y0};
    m_PrvePinchPoint1 = float2{x1, y1};
}

void InputControllerAndroid::PinchMove(float x0, float y0, float x1, float y1)
{
    float PrevDist    = length(m_PrvePinchPoint0 - m_PrvePinchPoint1);
    m_PrvePinchPoint0 = float2{x0, y0};
    m_PrvePinchPoint1 = float2{x1, y1};

    float NewDist           = length(m_PrvePinchPoint0 - m_PrvePinchPoint1);
    m_MouseState.WheelDelta = (NewDist - PrevDist) / 100.f;
}

void InputControllerAndroid::EndPinch()
{
    m_MouseState.WheelDelta = 0;
}

} // namespace Diligent
