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

#include "TestingEnvironment.hpp"

#ifndef NOMINMAX
#    define NOMINMAX
#endif
#include <Windows.h>

namespace Diligent
{

namespace Testing
{

Win32NativeWindow TestingEnvironment::CreateNativeWindow()
{
#ifdef UNICODE
    const auto* const WindowClassName = L"SampleApp";
#else
    const auto* const WindowClassName = "SampleApp";
#endif
    // Register window class
    HINSTANCE instance = NULL;

    WNDCLASSEX wcex = {sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, DefWindowProc,
                       0L, 0L, instance, NULL, NULL, NULL, NULL, WindowClassName, NULL};
    RegisterClassEx(&wcex);

    LONG WindowWidth  = 512;
    LONG WindowHeight = 512;
    RECT rc           = {0, 0, WindowWidth, WindowHeight};
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    HWND wnd = CreateWindowA("SampleApp", "Dummy Window",
                             WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                             rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, instance, NULL);
    if (wnd == NULL)
        LOG_ERROR_AND_THROW("Unable to create a window");

    return Win32NativeWindow{wnd};
}

} // namespace Testing

} // namespace Diligent
