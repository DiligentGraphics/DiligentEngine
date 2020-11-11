/*
 *  Copyright 2019-2020 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *  
 *      http://www.apache.org/licenses/LICENSE-2.0
 *  
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
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
#include <algorithm>

#ifndef NOMINMAX
#    define NOMINMAX
#endif
#include "Windows.h"

namespace Diligent
{

InputKeys MapCameraKeyWnd(UINT nKey)
{
    switch (nKey)
    {
        case VK_CONTROL:
            return InputKeys::ControlDown;

        case VK_SHIFT:
            return InputKeys::ShiftDown;

        case VK_MENU:
            return InputKeys::AltDown;

        case VK_LEFT:
        case 'A':
            return InputKeys::MoveLeft;

        case VK_RIGHT:
        case 'D':
            return InputKeys::MoveRight;

        case VK_UP:
        case 'W':
            return InputKeys::MoveForward;

        case VK_DOWN:
        case 'S':
            return InputKeys::MoveBackward;

        case VK_PRIOR:
        case 'E':
            return InputKeys::MoveUp; // pgup

        case VK_NEXT:
        case 'Q':
            return InputKeys::MoveDown; // pgdn

        case VK_HOME:
            return InputKeys::Reset;

        case VK_ADD:
            return InputKeys::ZoomIn;

        case VK_SUBTRACT:
            return InputKeys::ZoomOut;

        default:
            return InputKeys::Unknown;
    }
}

InputControllerWin32::InputControllerWin32()
{
    UpdateMousePos();
}

const MouseState& InputControllerWin32::GetMouseState()
{
    UpdateMousePos();
    return InputControllerBase::GetMouseState();
}

bool InputControllerWin32::HandleNativeMessage(const void* MsgData)
{
    m_MouseState.WheelDelta = 0;

    struct WindowMessageData
    {
        HWND   hWnd;
        UINT   message;
        WPARAM wParam;
        LPARAM lParam;
    };
    const WindowMessageData& WndMsg = *reinterpret_cast<const WindowMessageData*>(MsgData);

    auto hWnd   = WndMsg.hWnd;
    auto uMsg   = WndMsg.message;
    auto wParam = WndMsg.wParam;
    auto lParam = WndMsg.lParam;


    bool MsgHandled = false;
    switch (uMsg)
    {
        case WM_KEYDOWN:
        {
            // Map this key to a InputKeys enum and update the
            // state of m_aKeys[] by adding the INPUT_KEY_STATE_FLAG_KEY_WAS_DOWN|INPUT_KEY_STATE_FLAG_KEY_IS_DOWN mask
            // only if the key is not down
            auto mappedKey = MapCameraKeyWnd((UINT)wParam);
            if (mappedKey != InputKeys::Unknown && mappedKey < InputKeys::TotalKeys)
            {
                auto& Key = m_Keys[static_cast<Int32>(mappedKey)];
                Key &= ~INPUT_KEY_STATE_FLAG_KEY_WAS_DOWN;
                Key |= INPUT_KEY_STATE_FLAG_KEY_IS_DOWN;
            }
            MsgHandled = true;
            break;
        }

        case WM_KEYUP:
        {
            // Map this key to a InputKeys enum and update the
            // state of m_aKeys[] by removing the INPUT_KEY_STATE_FLAG_KEY_IS_DOWN mask.
            auto mappedKey = MapCameraKeyWnd((UINT)wParam);
            if (mappedKey != InputKeys::Unknown && mappedKey < InputKeys::TotalKeys)
            {
                auto& Key = m_Keys[static_cast<Int32>(mappedKey)];
                Key &= ~INPUT_KEY_STATE_FLAG_KEY_IS_DOWN;
                Key |= INPUT_KEY_STATE_FLAG_KEY_WAS_DOWN;
            }
            MsgHandled = true;
            break;
        }

        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDBLCLK:
        case WM_LBUTTONDBLCLK:
        {
            // Update member var state
            if ((uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONDBLCLK))
            {
                m_MouseState.ButtonFlags |= MouseState::BUTTON_FLAG_LEFT;
            }
            if ((uMsg == WM_MBUTTONDOWN || uMsg == WM_MBUTTONDBLCLK))
            {
                m_MouseState.ButtonFlags |= MouseState::BUTTON_FLAG_MIDDLE;
            }
            if ((uMsg == WM_RBUTTONDOWN || uMsg == WM_RBUTTONDBLCLK))
            {
                m_MouseState.ButtonFlags |= MouseState::BUTTON_FLAG_RIGHT;
            }

            // Capture the mouse, so if the mouse button is
            // released outside the window, we'll get the WM_LBUTTONUP message
            SetCapture(hWnd);
            UpdateMousePos();

            MsgHandled = true;
            break;
        }

        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        case WM_LBUTTONUP:
        {
            // Update member var state
            if (uMsg == WM_LBUTTONUP)
            {
                m_MouseState.ButtonFlags &= ~MouseState::BUTTON_FLAG_LEFT;
            }
            if (uMsg == WM_MBUTTONUP)
            {
                m_MouseState.ButtonFlags &= ~MouseState::BUTTON_FLAG_MIDDLE;
            }
            if (uMsg == WM_RBUTTONUP)
            {
                m_MouseState.ButtonFlags &= ~MouseState::BUTTON_FLAG_RIGHT;
            }

            // Release the capture if no mouse buttons down
            if ((m_MouseState.ButtonFlags & (MouseState::BUTTON_FLAG_LEFT | MouseState::BUTTON_FLAG_MIDDLE | MouseState::BUTTON_FLAG_RIGHT)) == 0)
            {
                ReleaseCapture();
            }

            MsgHandled = true;
            break;
        }

        case WM_CAPTURECHANGED:
        {
            if ((HWND)lParam != hWnd)
            {
                if ((m_MouseState.ButtonFlags & MouseState::BUTTON_FLAG_LEFT) ||
                    (m_MouseState.ButtonFlags & MouseState::BUTTON_FLAG_MIDDLE) ||
                    (m_MouseState.ButtonFlags & MouseState::BUTTON_FLAG_RIGHT))
                {
                    m_MouseState.ButtonFlags &= ~MouseState::BUTTON_FLAG_LEFT;
                    m_MouseState.ButtonFlags &= ~MouseState::BUTTON_FLAG_MIDDLE;
                    m_MouseState.ButtonFlags &= ~MouseState::BUTTON_FLAG_RIGHT;
                    ReleaseCapture();
                }
            }

            MsgHandled = true;
            break;
        }

        case WM_MOUSEWHEEL:
            // Update member var state
            m_MouseState.WheelDelta = (float)((short)HIWORD(wParam)) / (float)WHEEL_DELTA;
            MsgHandled              = true;
            break;
    }

    return MsgHandled;
}

void InputControllerWin32::UpdateMousePos()
{
    POINT MousePosition;
    GetCursorPos(&MousePosition);
    ScreenToClient(GetActiveWindow(), &MousePosition);
    m_MouseState.PosX = static_cast<float>(MousePosition.x);
    m_MouseState.PosY = static_cast<float>(MousePosition.y);

    /*if( m_bResetCursorAfterMove )
    {
        // Set position of camera to center of desktop, 
        // so it always has room to move.  This is very useful
        // if the cursor is hidden.  If this isn't done and cursor is hidden, 
        // then invisible cursor will hit the edge of the screen 
        // and the user can't tell what happened
        POINT ptCenter;

        // Get the center of the current monitor
        MONITORINFO mi;
        mi.cbSize = sizeof( MONITORINFO );
        DXUTGetMonitorInfo( DXUTMonitorFromWindow( DXUTGetHWND(), MONITOR_DEFAULTTONEAREST ), &mi );
        ptCenter.x = ( mi.rcMonitor.left + mi.rcMonitor.right ) / 2;
        ptCenter.y = ( mi.rcMonitor.top + mi.rcMonitor.bottom ) / 2;
        SetCursorPos( ptCenter.x, ptCenter.y );
        m_ptLastMousePosition = ptCenter;
    }*/
}

} // namespace Diligent
