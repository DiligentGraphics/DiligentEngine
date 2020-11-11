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

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>

#ifdef Bool
#    undef Bool
#endif

#ifdef True
#    undef True
#endif

#ifdef False
#    undef False
#endif


#include "imgui.h"

#include "ImGuiImplLinuxX11.hpp"

#include "DebugUtilities.hpp"

namespace Diligent
{

ImGuiImplLinuxX11::ImGuiImplLinuxX11(IRenderDevice* pDevice,
                                     TEXTURE_FORMAT BackBufferFmt,
                                     TEXTURE_FORMAT DepthBufferFmt,
                                     Uint32         DisplayWidth,
                                     Uint32         DisplayHeight,
                                     Uint32         InitialVertexBufferSize,
                                     Uint32         InitialIndexBufferSize) :
    ImGuiImplDiligent(pDevice, BackBufferFmt, DepthBufferFmt, InitialVertexBufferSize, InitialIndexBufferSize)
{

    auto& io       = ImGui::GetIO();
    io.DisplaySize = ImVec2(DisplayWidth, DisplayHeight);

    io.BackendPlatformName = "Diligent-ImGuiImplLinuxX11";

    // Keyboard mapping. ImGui will use those indices to peek into the io.KeysDown[] array that we will update during the application lifetime.
    io.KeyMap[ImGuiKey_Tab]        = 256;
    io.KeyMap[ImGuiKey_LeftArrow]  = 257;
    io.KeyMap[ImGuiKey_RightArrow] = 258;
    io.KeyMap[ImGuiKey_UpArrow]    = 259;
    io.KeyMap[ImGuiKey_DownArrow]  = 260;
    io.KeyMap[ImGuiKey_PageUp]     = 261;
    io.KeyMap[ImGuiKey_PageDown]   = 262;
    io.KeyMap[ImGuiKey_Home]       = 263;
    io.KeyMap[ImGuiKey_End]        = 264;
    io.KeyMap[ImGuiKey_Insert]     = 265;
    io.KeyMap[ImGuiKey_Delete]     = 266;
    io.KeyMap[ImGuiKey_Backspace]  = 267;
    io.KeyMap[ImGuiKey_Space]      = 268;
    io.KeyMap[ImGuiKey_Enter]      = 269;
    //io.KeyMap[ImGuiKey_Escape]      = 270;
    io.KeyMap[ImGuiKey_KeyPadEnter] = 271;
    io.KeyMap[ImGuiKey_A]           = 'A';
    io.KeyMap[ImGuiKey_C]           = 'C';
    io.KeyMap[ImGuiKey_V]           = 'V';
    io.KeyMap[ImGuiKey_X]           = 'X';
    io.KeyMap[ImGuiKey_Y]           = 'Y';
    io.KeyMap[ImGuiKey_Z]           = 'Z';

    m_LastTimestamp = std::chrono::high_resolution_clock::now();
}

ImGuiImplLinuxX11::~ImGuiImplLinuxX11()
{
}

void ImGuiImplLinuxX11::NewFrame(Uint32            RenderSurfaceWidth,
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


bool ImGuiImplLinuxX11::HandleXEvent(XEvent* event)
{
    auto& io = ImGui::GetIO();
    switch (event->type)
    {
        case ButtonPress:
        case ButtonRelease:
        {
            bool  IsPressed = event->type == ButtonPress;
            auto* xbe       = reinterpret_cast<XButtonEvent*>(event);
            switch (xbe->button)
            {
                case Button1: io.MouseDown[0] = IsPressed; break; // Left
                case Button2: io.MouseDown[2] = IsPressed; break; // Middle
                case Button3: io.MouseDown[1] = IsPressed; break; // Right
                case Button4: io.MouseWheel += 1; break;
                case Button5: io.MouseWheel -= 1; break;
            }
            return io.WantCaptureMouse;
        }

        case MotionNotify:
        {
            XMotionEvent* xme = (XMotionEvent*)event;
            io.MousePos       = ImVec2(xme->x, xme->y);
            return io.WantCaptureMouse;
        }

        case ConfigureNotify:
        {
            XConfigureEvent* xce = (XConfigureEvent*)event;
            io.DisplaySize       = ImVec2(xce->width, xce->height);
            return false;
        }

        case KeyPress:
        case KeyRelease:
        {
            bool IsPressed = event->type == KeyPress;
            io.KeyCtrl     = (event->xkey.state & ControlMask) != 0;
            io.KeyShift    = (event->xkey.state & ShiftMask) != 0;
            io.KeyAlt      = (event->xkey.state & Mod1Mask) != 0;

            KeySym        keysym  = 0;
            constexpr int buff_sz = 80;
            char          buffer[buff_sz];
            int           num_char = XLookupString((XKeyEvent*)event, buffer, buff_sz, &keysym, 0);
            int           k        = 0;
            switch (keysym)
            {
                // clang-format off
                case XK_Tab:       k = io.KeyMap[ImGuiKey_Tab];        break;
                case XK_Left:      k = io.KeyMap[ImGuiKey_LeftArrow];  break;
                case XK_Right:     k = io.KeyMap[ImGuiKey_RightArrow]; break;
                case XK_Up:        k = io.KeyMap[ImGuiKey_UpArrow];    break;
                case XK_Down:      k = io.KeyMap[ImGuiKey_DownArrow];  break;
                case XK_Page_Up:   k = io.KeyMap[ImGuiKey_PageUp];     break;
                case XK_Page_Down: k = io.KeyMap[ImGuiKey_PageDown];   break;
                case XK_Home:      k = io.KeyMap[ImGuiKey_Home];       break;
                case XK_End:       k = io.KeyMap[ImGuiKey_End];        break;
                case XK_Insert:    k = io.KeyMap[ImGuiKey_Insert];     break;
                case XK_Delete:    k = io.KeyMap[ImGuiKey_Delete];     break;
                case XK_BackSpace: k = io.KeyMap[ImGuiKey_Backspace];  break;
                //case XK_space:     k = io.KeyMap[ImGuiKey_Space];      break;
                case XK_Return:    k = io.KeyMap[ImGuiKey_Enter];      break;
                case XK_Escape:    k = io.KeyMap[ImGuiKey_Escape];     break;
                case XK_KP_Enter:  k = io.KeyMap[ImGuiKey_KeyPadEnter];break;
                    // clang-format on
            }

            if (k != 0)
                io.KeysDown[k] = IsPressed;

            if (k == 0 && IsPressed)
            {
                for (int i = 0; i < num_char; ++i)
                    io.AddInputCharacter(buffer[i]);
            }

            return io.WantCaptureKeyboard;
        }

        default:
            break;
    }
    return false;
}

} // namespace Diligent
