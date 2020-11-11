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

#include "HLSL2GLSLConverterApp.h"
#include "Errors.hpp"
#include "RenderDevice.h"
#include "EngineFactoryOpenGL.h"
#include "RefCntAutoPtr.hpp"

#include <Windows.h>
#include <crtdbg.h>

using namespace Diligent;

// Called every time the application receives a message
LRESULT CALLBACK MessageProc(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // Send event message to AntTweakBar
    switch (message)
    {
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            BeginPaint(wnd, &ps);
            EndPaint(wnd, &ps);
            return 0;
        }
        case WM_SIZE: // Window size has been changed
            return 0;

        case WM_CHAR:
            if (wParam == VK_ESCAPE)
                PostQuitMessage(0);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProc(wnd, message, wParam, lParam);
    }
}

// Main
int main(int argc, char** argv)
{
#if defined(_DEBUG) || defined(DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    HLSL2GLSLConverterApp Converter;

    if (argc == 1)
    {
        Converter.PrintHelp();
        return 0;
    }

    {
        auto ret = Converter.ParseCmdLine(argc, argv);
        if (ret != 0)
            return ret;
    }

    RefCntAutoPtr<IRenderDevice>  pDevice;
    RefCntAutoPtr<IDeviceContext> pContext;
    RefCntAutoPtr<ISwapChain>     pSwapChain;

    if (Converter.NeedsCompileShader())
    {
        // Register window cla  ss
        WNDCLASSEX wcex = {sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, MessageProc,
                           0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"HLSL2GLSLConverter", NULL};
        RegisterClassEx(&wcex);

        // Create dummy window
        RECT rc = {0, 0, 512, 512};
        AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
        HWND wnd = CreateWindow(L"HLSL2GLSLConverter", L"HLSL2GLSL Converter",
                                WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                                rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, wcex.hInstance, NULL);

        if (!wnd)
        {
            LOG_ERROR_MESSAGE("Failed to create window");
            return -1;
        }
        EngineGLCreateInfo EngineCI;
        SwapChainDesc      SCDesc;
        EngineCI.Window.hWnd = wnd;

        auto* pFactory = Converter.GetFactoryGL();
        pFactory->CreateDeviceAndSwapChainGL(
            EngineCI, &pDevice, &pContext, SCDesc, &pSwapChain);
        if (!pDevice)
        {
            LOG_ERROR_MESSAGE("Failed to create render device");
            return -1;
        }
    }

    return Converter.Convert(pDevice);
}
