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

#include <memory>
#include <iomanip>
#include <iostream>

#include <Windows.h>
#include <crtdbg.h>
#include "NativeAppBase.hpp"
#include "StringTools.hpp"
#include "Timer.hpp"

using namespace Diligent;

std::unique_ptr<NativeAppBase> g_pTheApp;

LRESULT CALLBACK MessageProc(HWND, UINT, WPARAM, LPARAM);
// Main
int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int cmdShow)
{
#if defined(_DEBUG) || defined(DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    g_pTheApp.reset(CreateApplication());

    const auto* cmdLine = GetCommandLineA();
    g_pTheApp->ProcessCommandLine(cmdLine);

    const auto* AppTitle = g_pTheApp->GetAppTitle();

#ifdef UNICODE
    const auto* const WindowClassName = L"SampleApp";
#else
    const auto* const WindowClassName = "SampleApp";
#endif

    // Register our window class
    WNDCLASSEX wcex = {sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, MessageProc,
                       0L, 0L, instance, NULL, NULL, NULL, NULL, WindowClassName, NULL};
    RegisterClassEx(&wcex);

    int DesiredWidth  = 0;
    int DesiredHeight = 0;
    g_pTheApp->GetDesiredInitialWindowSize(DesiredWidth, DesiredHeight);
    // Create a window
    LONG WindowWidth  = DesiredWidth > 0 ? DesiredWidth : 1280;
    LONG WindowHeight = DesiredHeight > 0 ? DesiredHeight : 1024;
    RECT rc           = {0, 0, WindowWidth, WindowHeight};
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    HWND wnd = CreateWindowA("SampleApp", AppTitle,
                             WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                             rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, instance, NULL);
    if (!wnd)
    {
        std::cerr << "Failed to create a window";
        return 1;
    }

    g_pTheApp->OnWindowCreated(wnd, WindowWidth, WindowHeight);

    auto GoldenImgMode = g_pTheApp->GetGoldenImageMode();
    if (GoldenImgMode != NativeAppBase::GoldenImageMode::None)
    {
        g_pTheApp->Update(0, 0);
        g_pTheApp->Render();
        // Dear imgui windows that don't have initial size are not rendered in the first frame,
        // see https://github.com/ocornut/imgui/issues/2949
        g_pTheApp->Update(0, 0);
        g_pTheApp->Render();
        g_pTheApp->Present();
        auto ExitCode = g_pTheApp->GetExitCode();
        g_pTheApp.reset();
        return ExitCode;
    }

    ShowWindow(wnd, cmdShow);
    UpdateWindow(wnd);

    AppTitle = g_pTheApp->GetAppTitle();

    Diligent::Timer Timer;

    auto   PrevTime          = Timer.GetElapsedTime();
    double filteredFrameTime = 0.0;

    // Main message loop
    MSG msg = {0};
    while (WM_QUIT != msg.message)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            auto CurrTime    = Timer.GetElapsedTime();
            auto ElapsedTime = CurrTime - PrevTime;
            PrevTime         = CurrTime;

            if (g_pTheApp->IsReady())
            {
                g_pTheApp->Update(CurrTime, ElapsedTime);

                g_pTheApp->Render();

                g_pTheApp->Present();

                double filterScale = 0.2;
                filteredFrameTime  = filteredFrameTime * (1.0 - filterScale) + filterScale * ElapsedTime;
                std::stringstream fpsCounterSS;
                fpsCounterSS << AppTitle << " - " << std::fixed << std::setprecision(1) << filteredFrameTime * 1000;
                fpsCounterSS << " ms (" << 1.0 / filteredFrameTime << " fps)";
                SetWindowTextA(wnd, fpsCounterSS.str().c_str());
            }
        }
    }

    g_pTheApp.reset();

    return (int)msg.wParam;
}

// Called every time the NativeNativeAppBase receives a message
LRESULT CALLBACK MessageProc(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (g_pTheApp)
    {
        auto res = g_pTheApp->HandleWin32Message(wnd, message, wParam, lParam);
        if (res != 0)
            return res;
    }

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
            if (g_pTheApp)
            {
                g_pTheApp->WindowResize(LOWORD(lParam), HIWORD(lParam));
            }
            return 0;

        case WM_CHAR:
            if (wParam == VK_ESCAPE)
                PostQuitMessage(0);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_GETMINMAXINFO:
        {
            LPMINMAXINFO lpMMI      = (LPMINMAXINFO)lParam;
            lpMMI->ptMinTrackSize.x = 320;
            lpMMI->ptMinTrackSize.y = 240;
            return 0;
        }

        default:
            return DefWindowProc(wnd, message, wParam, lParam);
    }
}
