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
#include <X11/Xlib.h>
#include <X11/Xutil.h>

// Undef symbols defined by XLib
#ifdef Bool
#    undef Bool
#endif
#ifdef True
#    undef True
#endif
#ifdef False
#    undef False
#endif

#include <xcb/xcb.h>
#include "xcb_keysyms/xcb_keysyms.h"

#include "InputController.hpp"
#include "DebugUtilities.hpp"

namespace Diligent
{

int InputControllerLinux::HandleKeyEvevnt(unsigned int keysym, bool IsKeyPressed)
{
    int  handled        = 0;
    auto UpdateKeyState = [&](InputKeys Key) //
    {
        auto& KeyState = m_Keys[static_cast<size_t>(Key)];
        if (IsKeyPressed)
        {
            KeyState &= ~INPUT_KEY_STATE_FLAG_KEY_WAS_DOWN;
            KeyState |= INPUT_KEY_STATE_FLAG_KEY_IS_DOWN;
        }
        else
        {
            KeyState &= ~INPUT_KEY_STATE_FLAG_KEY_IS_DOWN;
            KeyState |= INPUT_KEY_STATE_FLAG_KEY_WAS_DOWN;
        }
        handled = 1;
    };

    //if (event->xkey.state & ControlMask)
    //    UpdateKeyState(InputKeys::ControlDown);
    //if (event->xkey.state & ShiftMask)
    //    UpdateKeyState(InputKeys::ShiftDown);
    //if (event->xkey.state & Mod1Mask)
    //    UpdateKeyState(InputKeys::AltDown);

    switch (keysym)
    {
        case XK_Control_L:
        case XK_Control_R:
            UpdateKeyState(InputKeys::ControlDown);
            break;

        case XK_Shift_L:
        case XK_Shift_R:
            UpdateKeyState(InputKeys::ShiftDown);
            break;

        case XK_Alt_L:
        case XK_Alt_R:
            UpdateKeyState(InputKeys::AltDown);
            break;

        case XK_Up:
        case 'w':
        case 'W':
            UpdateKeyState(InputKeys::MoveForward);
            break;

        case XK_Down:
        case 's':
        case 'S':
            UpdateKeyState(InputKeys::MoveBackward);
            break;

        case XK_Right:
        case 'd':
        case 'D':
            UpdateKeyState(InputKeys::MoveRight);
            break;

        case XK_Left:
        case 'a':
        case 'A':
            UpdateKeyState(InputKeys::MoveLeft);
            break;

        case XK_Home:
            UpdateKeyState(InputKeys::Reset);
            break;

        case XK_Page_Up:
        case 'e':
        case 'E':
            UpdateKeyState(InputKeys::MoveUp);
            break;

        case XK_Page_Down:
        case 'q':
        case 'Q':
            UpdateKeyState(InputKeys::MoveDown);
            break;

        case XK_plus:
            UpdateKeyState(InputKeys::ZoomIn);
            break;

        case XK_minus:
            UpdateKeyState(InputKeys::ZoomOut);
            break;

#ifdef XK_KP_Home
        case XK_KP_Home:
            UpdateKeyState(InputKeys::Reset);
            break;
#endif

#ifdef XK_KP_Up
        case XK_KP_Up:
            UpdateKeyState(InputKeys::MoveForward);
            break;

        case XK_KP_Down:
            UpdateKeyState(InputKeys::MoveBackward);
            break;

        case XK_KP_Right:
            UpdateKeyState(InputKeys::MoveRight);
            break;

        case XK_KP_Left:
            UpdateKeyState(InputKeys::MoveLeft);
            break;

        case XK_KP_Page_Up:
            UpdateKeyState(InputKeys::MoveUp);
            break;

        case XK_KP_Page_Down:
            UpdateKeyState(InputKeys::MoveDown);
            break;
#endif
    }

    return handled;
}

int InputControllerLinux::HandleXEvent(void* xevent)
{
    auto* event = reinterpret_cast<XEvent*>(xevent);
    switch (event->type)
    {
        case KeyPress:
        case KeyRelease:
        {
            KeySym keysym;
            int    num_char = XLookupString((XKeyEvent*)event, nullptr, 0, &keysym, 0);
            return HandleKeyEvevnt(keysym, event->type == KeyPress);
        }

        case ButtonPress:
        {
            auto* xbe = reinterpret_cast<XButtonEvent*>(event);
            switch (xbe->button)
            {
                case Button1:
                    m_MouseState.ButtonFlags |= MouseState::BUTTON_FLAG_LEFT;
                    break;

                case Button2:
                    m_MouseState.ButtonFlags |= MouseState::BUTTON_FLAG_MIDDLE;
                    break;

                case Button3:
                    m_MouseState.ButtonFlags |= MouseState::BUTTON_FLAG_RIGHT;
                    break;

                case Button4:
                    m_MouseState.WheelDelta += 1;
                    break;

                case Button5:
                    m_MouseState.WheelDelta -= 1;
                    break;
            }
            return 1;
        }

        case ButtonRelease:
        {
            auto* xbe = reinterpret_cast<XButtonEvent*>(event);
            switch (xbe->button)
            {
                case Button1:
                    m_MouseState.ButtonFlags &= ~MouseState::BUTTON_FLAG_LEFT;
                    break;

                case Button2:
                    m_MouseState.ButtonFlags &= ~MouseState::BUTTON_FLAG_MIDDLE;
                    break;

                case Button3:
                    m_MouseState.ButtonFlags &= ~MouseState::BUTTON_FLAG_RIGHT;
                    break;
            }
            return 1;
        }

        case MotionNotify:
        {
            auto* xme = (XMotionEvent*)event;

            m_MouseState.PosX = static_cast<float>(xme->x);
            m_MouseState.PosY = static_cast<float>(xme->y);
            return 1;
        }

        default:
            break;
    }

    return 0;
}


void InputControllerLinux::InitXCBKeysms(void* connection)
{
    VERIFY_EXPR(m_XCBKeySymbols == nullptr);
    m_XCBKeySymbols = xcb_key_symbols_alloc(reinterpret_cast<xcb_connection_t*>(connection));
}

InputControllerLinux::~InputControllerLinux()
{
    if (m_XCBKeySymbols != nullptr)
    {
        xcb_key_symbols_free(reinterpret_cast<xcb_key_symbols_t*>(m_XCBKeySymbols));
    }
}

int InputControllerLinux::HandleXCBEvent(void* xcb_event)
{
    auto* event = reinterpret_cast<xcb_generic_event_t*>(xcb_event);

    const auto event_type = event->response_type & 0x7f;
    switch (event_type)
    {
        case XCB_MOTION_NOTIFY:
        {
            xcb_motion_notify_event_t* motion = (xcb_motion_notify_event_t*)event;

            m_MouseState.PosX = static_cast<float>(motion->event_x);
            m_MouseState.PosY = static_cast<float>(motion->event_y);
            return 1;
        }
        break;

        case XCB_BUTTON_PRESS:
        {
            xcb_button_press_event_t* press = (xcb_button_press_event_t*)event;
            switch (press->detail)
            {
                case XCB_BUTTON_INDEX_1:
                    m_MouseState.ButtonFlags |= MouseState::BUTTON_FLAG_LEFT;
                    break;

                case XCB_BUTTON_INDEX_2:
                    m_MouseState.ButtonFlags |= MouseState::BUTTON_FLAG_MIDDLE;
                    break;

                case XCB_BUTTON_INDEX_3:
                    m_MouseState.ButtonFlags |= MouseState::BUTTON_FLAG_RIGHT;
                    break;

                case XCB_BUTTON_INDEX_4:
                    m_MouseState.WheelDelta += 1;
                    break;

                case XCB_BUTTON_INDEX_5:
                    m_MouseState.WheelDelta -= 1;
                    break;
            }
            return 1;
        }
        break;

        case XCB_BUTTON_RELEASE:
        {
            xcb_button_release_event_t* press = (xcb_button_release_event_t*)event;
            switch (press->detail)
            {
                case XCB_BUTTON_INDEX_1:
                    m_MouseState.ButtonFlags &= ~MouseState::BUTTON_FLAG_LEFT;
                    break;

                case XCB_BUTTON_INDEX_2:
                    m_MouseState.ButtonFlags &= ~MouseState::BUTTON_FLAG_MIDDLE;
                    break;

                case XCB_BUTTON_INDEX_3:
                    m_MouseState.ButtonFlags &= ~MouseState::BUTTON_FLAG_RIGHT;
                    break;
            }
            return 1;
        }
        break;

        case XCB_KEY_PRESS:
        case XCB_KEY_RELEASE:
        {
            xcb_keysym_t keysym = xcb_key_press_lookup_keysym(
                reinterpret_cast<xcb_key_symbols_t*>(m_XCBKeySymbols),
                reinterpret_cast<xcb_key_press_event_t*>(event), 0);
            return HandleKeyEvevnt(keysym, event_type == XCB_KEY_PRESS);
        }
        break;

        default:
            break;
    }

    return 0;
}

} // namespace Diligent
