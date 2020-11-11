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

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_VARARGS
#include "../../../ThirdParty/nuklear/nuklear.h"

#ifndef NOMINMAX
#    define NOMINMAX
#endif
#include <Windows.h>

#include "NkInputWin32.h"

NK_API int
nk_diligent_handle_win32_event(nk_context* ctx, HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        {
            int down = !((lparam >> 31) & 1);
            int ctrl = GetKeyState(VK_CONTROL) & (1 << 15);

            switch (wparam)
            {
                case VK_SHIFT:
                case VK_LSHIFT:
                case VK_RSHIFT:
                    nk_input_key(ctx, NK_KEY_SHIFT, down);
                    return 1;

                case VK_DELETE:
                    nk_input_key(ctx, NK_KEY_DEL, down);
                    return 1;

                case VK_RETURN:
                    nk_input_key(ctx, NK_KEY_ENTER, down);
                    return 1;

                case VK_TAB:
                    nk_input_key(ctx, NK_KEY_TAB, down);
                    return 1;

                case VK_LEFT:
                    if (ctrl)
                        nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT, down);
                    else
                        nk_input_key(ctx, NK_KEY_LEFT, down);
                    return 1;

                case VK_RIGHT:
                    if (ctrl)
                        nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, down);
                    else
                        nk_input_key(ctx, NK_KEY_RIGHT, down);
                    return 1;

                case VK_BACK:
                    nk_input_key(ctx, NK_KEY_BACKSPACE, down);
                    return 1;

                case VK_HOME:
                    nk_input_key(ctx, NK_KEY_TEXT_START, down);
                    nk_input_key(ctx, NK_KEY_SCROLL_START, down);
                    return 1;

                case VK_END:
                    nk_input_key(ctx, NK_KEY_TEXT_END, down);
                    nk_input_key(ctx, NK_KEY_SCROLL_END, down);
                    return 1;

                case VK_NEXT:
                    nk_input_key(ctx, NK_KEY_SCROLL_DOWN, down);
                    return 1;

                case VK_PRIOR:
                    nk_input_key(ctx, NK_KEY_SCROLL_UP, down);
                    return 1;

                case 'C':
                    if (ctrl)
                    {
                        nk_input_key(ctx, NK_KEY_COPY, down);
                        return 1;
                    }
                    break;

                case 'V':
                    if (ctrl)
                    {
                        nk_input_key(ctx, NK_KEY_PASTE, down);
                        return 1;
                    }
                    break;

                case 'X':
                    if (ctrl)
                    {
                        nk_input_key(ctx, NK_KEY_CUT, down);
                        return 1;
                    }
                    break;

                case 'Z':
                    if (ctrl)
                    {
                        nk_input_key(ctx, NK_KEY_TEXT_UNDO, down);
                        return 1;
                    }
                    break;

                case 'R':
                    if (ctrl)
                    {
                        nk_input_key(ctx, NK_KEY_TEXT_REDO, down);
                        return 1;
                    }
                    break;
            }
            return 0;
        }

        case WM_CHAR:
            if (wparam >= 32)
            {
                nk_input_unicode(ctx, (nk_rune)wparam);
                return 1;
            }
            break;

        case WM_LBUTTONDOWN:
            nk_input_button(ctx, NK_BUTTON_LEFT, (short)LOWORD(lparam), (short)HIWORD(lparam), 1);
            SetCapture(wnd);
            return 1;

        case WM_LBUTTONUP:
            nk_input_button(ctx, NK_BUTTON_DOUBLE, (short)LOWORD(lparam), (short)HIWORD(lparam), 0);
            nk_input_button(ctx, NK_BUTTON_LEFT, (short)LOWORD(lparam), (short)HIWORD(lparam), 0);
            ReleaseCapture();
            return 1;

        case WM_RBUTTONDOWN:
            nk_input_button(ctx, NK_BUTTON_RIGHT, (short)LOWORD(lparam), (short)HIWORD(lparam), 1);
            SetCapture(wnd);
            return 1;

        case WM_RBUTTONUP:
            nk_input_button(ctx, NK_BUTTON_RIGHT, (short)LOWORD(lparam), (short)HIWORD(lparam), 0);
            ReleaseCapture();
            return 1;

        case WM_MBUTTONDOWN:
            nk_input_button(ctx, NK_BUTTON_MIDDLE, (short)LOWORD(lparam), (short)HIWORD(lparam), 1);
            SetCapture(wnd);
            return 1;

        case WM_MBUTTONUP:
            nk_input_button(ctx, NK_BUTTON_MIDDLE, (short)LOWORD(lparam), (short)HIWORD(lparam), 0);
            ReleaseCapture();
            return 1;

        case WM_MOUSEWHEEL:
            nk_input_scroll(ctx, nk_vec2(0, (float)(short)HIWORD(wparam) / WHEEL_DELTA));
            return 1;

        case WM_MOUSEMOVE:
            nk_input_motion(ctx, (short)LOWORD(lparam), (short)HIWORD(lparam));
            return 1;

        case WM_LBUTTONDBLCLK:
            nk_input_button(ctx, NK_BUTTON_DOUBLE, (short)LOWORD(lparam), (short)HIWORD(lparam), 1);
            return 1;
    }

    return 0;
}
