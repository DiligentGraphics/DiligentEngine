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

#include "imgui.h"

#include <xcb/xcb.h>
#include <X11/keysym.h>
#include "xcb_keysyms/xcb_keysyms.h"

#ifdef Bool
#    undef Bool
#endif

#ifdef True
#    undef True
#endif

#ifdef False
#    undef False
#endif

#include "ImGuiImplLinuxXCB.hpp"

#include "DebugUtilities.hpp"

namespace Diligent
{

ImGuiImplLinuxXCB::ImGuiImplLinuxXCB(xcb_connection_t* connection,
                                     IRenderDevice*    pDevice,
                                     TEXTURE_FORMAT    BackBufferFmt,
                                     TEXTURE_FORMAT    DepthBufferFmt,
                                     Uint32            DisplayWidth,
                                     Uint32            DisplayHeight,
                                     Uint32            InitialVertexBufferSize,
                                     Uint32            InitialIndexBufferSize) :
    ImGuiImplDiligent{pDevice, BackBufferFmt, DepthBufferFmt, InitialVertexBufferSize, InitialIndexBufferSize}
{
    m_syms = xcb_key_symbols_alloc((xcb_connection_t*)connection);

    auto& io       = ImGui::GetIO();
    io.DisplaySize = ImVec2(DisplayWidth, DisplayHeight);

    io.BackendPlatformName = "Diligent-ImGuiImplLinuxXCB";

    // Keyboard mapping. ImGui will use those indices to peek into the io.KeysDown[] array that we will update during the application lifetime.
    io.KeyMap[ImGuiKey_Tab]        = 0x17;
    io.KeyMap[ImGuiKey_LeftArrow]  = 0x71;
    io.KeyMap[ImGuiKey_RightArrow] = 0x72;
    io.KeyMap[ImGuiKey_UpArrow]    = 0x6F;
    io.KeyMap[ImGuiKey_DownArrow]  = 0x74;
    io.KeyMap[ImGuiKey_PageUp]     = 0x70;
    io.KeyMap[ImGuiKey_PageDown]   = 0x75;
    io.KeyMap[ImGuiKey_Home]       = 0x6E;
    io.KeyMap[ImGuiKey_End]        = 0x73;
    io.KeyMap[ImGuiKey_Insert]     = 0x76;
    io.KeyMap[ImGuiKey_Delete]     = 0x77;
    io.KeyMap[ImGuiKey_Backspace]  = 0x16;
    //io.KeyMap[ImGuiKey_Space] = 0;//VK_SPACE;
    io.KeyMap[ImGuiKey_Enter]       = 0x24;
    io.KeyMap[ImGuiKey_Escape]      = 0x09;
    io.KeyMap[ImGuiKey_KeyPadEnter] = 0x68;
    io.KeyMap[ImGuiKey_A]           = 'A';
    io.KeyMap[ImGuiKey_C]           = 'C';
    io.KeyMap[ImGuiKey_V]           = 'V';
    io.KeyMap[ImGuiKey_X]           = 'X';
    io.KeyMap[ImGuiKey_Y]           = 'Y';
    io.KeyMap[ImGuiKey_Z]           = 'Z';

    m_LastTimestamp = std::chrono::high_resolution_clock::now();
}

ImGuiImplLinuxXCB::~ImGuiImplLinuxXCB()
{
    if (m_syms)
    {
        xcb_key_symbols_free(m_syms);
    }
}

void ImGuiImplLinuxXCB::NewFrame(Uint32            RenderSurfaceWidth,
                                 Uint32            RenderSurfaceHeight,
                                 SURFACE_TRANSFORM SurfacePreTransform)
{
    auto now        = std::chrono::high_resolution_clock::now();
    auto elapsed_ns = now - m_LastTimestamp;
    m_LastTimestamp = now;
    auto& io        = ImGui::GetIO();
    io.DeltaTime    = static_cast<float>(elapsed_ns.count() / 1e+9);

    VERIFY(io.DisplaySize.x == 0 || io.DisplaySize.x == static_cast<float>(RenderSurfaceWidth), "io.DisplaySize.x (",
           io.DisplaySize.x, " does not match RenderSurfaceWidth (", RenderSurfaceWidth, ")");
    VERIFY(io.DisplaySize.y == 0 || io.DisplaySize.y == static_cast<float>(RenderSurfaceHeight), "io.DisplaySize.y (",
           io.DisplaySize.y, " does not match RenderSurfaceHeight (", RenderSurfaceHeight, ")");

    ImGuiImplDiligent::NewFrame(RenderSurfaceWidth, RenderSurfaceHeight, SurfacePreTransform);
}

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
void ImGuiImplLinuxXCB::HandleKeyEvent(xcb_key_release_event_t* event)
{
    bool IsKeyPressed = (event->response_type & 0x7f) == XCB_KEY_PRESS;

    auto& io = ImGui::GetIO();

    io.KeyCtrl  = event->state & XCB_KEY_BUT_MASK_CONTROL;
    io.KeyShift = event->state & XCB_KEY_BUT_MASK_SHIFT;
    io.KeyAlt   = event->state & XCB_KEY_BUT_MASK_MOD_1;

    int k = 0;
    switch (event->detail)
    {
        // clang-format off
        case 0x09:  k = io.KeyMap[ImGuiKey_Escape];     break;
        case 0x6F:  k = io.KeyMap[ImGuiKey_UpArrow];    break;
        case 0x74:  k = io.KeyMap[ImGuiKey_DownArrow];  break;
        case 0x72:  k = io.KeyMap[ImGuiKey_RightArrow]; break;
        case 0x71:  k = io.KeyMap[ImGuiKey_LeftArrow];  break;
        case 0x24:  k = io.KeyMap[ImGuiKey_Enter];      break;
        case 0x76:  k = io.KeyMap[ImGuiKey_Insert];     break;
        case 0x77:  k = io.KeyMap[ImGuiKey_Delete];     break;
        case 0x16:  k = io.KeyMap[ImGuiKey_Backspace];  break;
        case 0x6E:  k = io.KeyMap[ImGuiKey_Home];       break;
        case 0x17:  k = io.KeyMap[ImGuiKey_Tab];        break;
        case 0x73:  k = io.KeyMap[ImGuiKey_End];        break;
        case 0x68:  k = io.KeyMap[ImGuiKey_KeyPadEnter];break;
        case 0x70:  k = io.KeyMap[ImGuiKey_PageUp];     break;
        case 0x75:  k = io.KeyMap[ImGuiKey_PageDown];   break;
            // clang-format on
    }

    if (k == 0 && IsKeyPressed)
    {
        xcb_keysym_t keysym = xcb_key_press_lookup_keysym(m_syms, event, 0);
        switch (keysym)
        {
#if 0
            case XK_Control_L:
            case XK_Control_R: /*s_KMod |= TW_KMOD_CTRL;*/  break;

            case XK_Shift_L:
            case XK_Shift_R:   /*s_KMod |= TW_KMOD_SHIFT;*/ break;

            case XK_Alt_L:
            case XK_Alt_R:     /*s_KMod |= TW_KMOD_ALT;*/   break;

#    ifdef XK_Enter
            case XK_Enter:     k = TW_KEY_RETURN;    break;
#    endif

#    ifdef XK_KP_Home
            case XK_KP_Home:   k = io.KeyMap[ImGuiKey_Home];      break;
            case XK_KP_End:    k = io.KeyMap[ImGuiKey_End];       break;
            case XK_KP_Delete: k = io.KeyMap[ImGuiKey_Delete];    break;
#    endif

#    ifdef XK_KP_Up
            case XK_KP_Up:     k = io.KeyMap[ImGuiKey_UpArrow];    break;
            case XK_KP_Down:   k = io.KeyMap[ImGuiKey_DownArrow];  break;
            case XK_KP_Right:  k = io.KeyMap[ImGuiKey_RightArrow]; break;
            case XK_KP_Left:   k = io.KeyMap[ImGuiKey_LeftArrow];  break;
#    endif

#    ifdef XK_KP_Page_Up
            case XK_KP_Page_Up:   k = io.KeyMap[ImGuiKey_PageUp];    break;
            case XK_KP_Page_Down: k = io.KeyMap[ImGuiKey_PageDown];  break;
#    endif

#    ifdef XK_KP_Tab
            case XK_KP_Tab:    k = io.KeyMap[ImGuiKey_Tab];       break;
#    endif
#endif
            default:
                if (keysym > 12 && keysym < 127)
                {
                    if (io.KeyShift)
                    {
                        if (keysym >= 'a' && keysym <= 'z')
                            keysym += (int)'A' - (int)'a';
                        else
                        {
                            switch (keysym)
                            {
                                case '`': keysym = '~'; break;
                                case '1': keysym = '!'; break;
                                case '2': keysym = '@'; break;
                                case '3': keysym = '#'; break;
                                case '4': keysym = '$'; break;
                                case '5': keysym = '%'; break;
                                case '6': keysym = '^'; break;
                                case '7': keysym = '&'; break;
                                case '8': keysym = '*'; break;
                                case '9': keysym = '('; break;
                                case '0': keysym = ')'; break;
                                case '-': keysym = '_'; break;
                                case '=': keysym = '+'; break;
                                case '[': keysym = '{'; break;
                                case ']': keysym = '}'; break;
                                case '\\': keysym = '|'; break;
                                case ';': keysym = ':'; break;
                                case '\'': keysym = '\"'; break;
                                case ',': keysym = '<'; break;
                                case '.': keysym = '>'; break;
                                case '/': keysym = '?'; break;
                            }
                        }
                    }

                    io.AddInputCharacter(keysym);
                }
        }
    }

    if (k != 0)
    {
        io.KeysDown[k] = IsKeyPressed;
    }
}

bool ImGuiImplLinuxXCB::HandleXCBEvent(xcb_generic_event_t* event)
{
    auto& io = ImGui::GetIO();
    switch (event->response_type & 0x7f)
    {
        case XCB_MOTION_NOTIFY:
        {
            xcb_motion_notify_event_t* motion = (xcb_motion_notify_event_t*)event;
            io.MousePos                       = ImVec2(motion->event_x, motion->event_y);
            return io.WantCaptureMouse;
        }
        break;

        case XCB_BUTTON_PRESS:
        {
            xcb_button_press_event_t* press = (xcb_button_press_event_t*)event;
            switch (press->detail)
            {
                case XCB_BUTTON_INDEX_1: io.MouseDown[0] = true; break; // left
                case XCB_BUTTON_INDEX_2: io.MouseDown[2] = true; break; // middle
                case XCB_BUTTON_INDEX_3: io.MouseDown[1] = true; break; // right
                case XCB_BUTTON_INDEX_4: io.MouseWheel += 1; break;
                case XCB_BUTTON_INDEX_5: io.MouseWheel -= 1; break;
            }

            return io.WantCaptureMouse;
        }
        break;

        case XCB_BUTTON_RELEASE:
        {
            xcb_button_release_event_t* press = (xcb_button_release_event_t*)event;
            switch (press->detail)
            {
                case XCB_BUTTON_INDEX_1: io.MouseDown[0] = false; break; // left
                case XCB_BUTTON_INDEX_2: io.MouseDown[2] = false; break; // middle
                case XCB_BUTTON_INDEX_3: io.MouseDown[1] = false; break; // right
            }

            return io.WantCaptureMouse;
        }
        break;

        case XCB_KEY_RELEASE:
        case XCB_KEY_PRESS:
        {
            xcb_key_press_event_t* keyEvent = (xcb_key_press_event_t*)event;
            HandleKeyEvent(keyEvent);
            return io.WantCaptureKeyboard;
        }
        break;

        case XCB_CONFIGURE_NOTIFY:
        {
            const xcb_configure_notify_event_t* cfgEvent = (const xcb_configure_notify_event_t*)event;

            io.DisplaySize = ImVec2(cfgEvent->width, cfgEvent->height);
            return false;
        }
        break;

        default:
            break;
    }

    return false;
}

} // namespace Diligent
