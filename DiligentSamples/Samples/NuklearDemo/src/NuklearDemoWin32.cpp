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

#include "NuklearDemo.hpp"

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_VARARGS
#include "../../../ThirdParty/nuklear/nuklear.h"

#ifndef NOMINMAX
#    define NOMINMAX
#endif
#include <Windows.h>

#include "NkInputWin32.h"

namespace Diligent
{

bool NuklearDemo::HandleNativeMessage(const void* pNativeMsgData)
{
    struct WindowsMessageData
    {
        HWND   hWnd;
        UINT   message;
        WPARAM wParam;
        LPARAM lParam;
    } const* pWin32Msg = reinterpret_cast<const WindowsMessageData*>(pNativeMsgData);

    return nk_diligent_handle_win32_event(m_pNkCtx, pWin32Msg->hWnd, pWin32Msg->message, pWin32Msg->wParam, pWin32Msg->lParam) != 0;
}

} // namespace Diligent
