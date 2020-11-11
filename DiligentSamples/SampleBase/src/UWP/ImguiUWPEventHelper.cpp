/*     Copyright 2015-2016 Egor Yusov
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
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

//  ---------------------------------------------------------------------------
//
//  @file       ImguiUWPEventHelper.cpp
//  @brief      Helper: 
//              translate and re-send mouse and keyboard events 
//              from Windows Runtime message proc to imgui
//  
//  @author     Egor Yusov
//  @date       2015/06/30
//  @note       This file is not part of the AntTweakBar library because
//              it is not recommended to pack Windows Runtime extensions 
//              into a static library
//  ---------------------------------------------------------------------------

#define NOMINIMAX
#include <wrl.h>
#include <wrl/client.h>

#include "ImguiUWPEventHelper.h"

#include "imgui.h"

// Mouse wheel support
#if !defined WHEEL_DELTA
#define      WHEEL_DELTA 120
#endif    // WHEEL_DELTA



using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Windows::UI;
using namespace Windows::Foundation;
using namespace Microsoft::WRL;
using namespace Windows::Devices::Input;
using namespace Windows::System;

ImguiUWPEventHelper^ ImguiUWPEventHelper::Create(_In_ CoreWindow^ window)
{
    auto p = ref new ImguiUWPEventHelper(window);
    return static_cast<ImguiUWPEventHelper^>(p);
}

ImguiUWPEventHelper::ImguiUWPEventHelper(_In_ CoreWindow^ window)
{
    window->PointerPressed +=
        ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &ImguiUWPEventHelper::OnPointerPressed);

    window->PointerMoved +=
        ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &ImguiUWPEventHelper::OnPointerMoved);

    window->PointerReleased +=
        ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &ImguiUWPEventHelper::OnPointerReleased);

    window->PointerExited +=
        ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &ImguiUWPEventHelper::OnPointerExited);

    window->KeyDown +=
        ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &ImguiUWPEventHelper::OnKeyDown);

    window->KeyUp +=
        ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &ImguiUWPEventHelper::OnKeyUp);

    window->PointerWheelChanged +=
        ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &ImguiUWPEventHelper::OnPointerWheelChanged);

    // There is a separate handler for mouse only relative mouse movement events.
#if (WINAPI_FAMILY != WINAPI_FAMILY_PHONE_APP)
    MouseDevice::GetForCurrentView()->MouseMoved +=
        ref new TypedEventHandler<MouseDevice^, MouseEventArgs^>(this, &ImguiUWPEventHelper::OnMouseMoved);
#endif
}

void ImguiUWPEventHelper::UpdateImguiMouseProperties(_In_ PointerEventArgs^ args)
{
    PointerPoint^ point = args->CurrentPoint;
    Point pointerPosition = point->Position;
    PointerPointProperties^ pointProperties = point->Properties;

    if (ImGui::GetCurrentContext() != nullptr)
    {
        ImGuiIO& io = ImGui::GetIO();
        io.MouseDown[0] = pointProperties->IsLeftButtonPressed;
        io.MouseDown[1] = pointProperties->IsRightButtonPressed;
        io.MouseDown[2] = pointProperties->IsMiddleButtonPressed;
        io.MousePos     = ImVec2(pointerPosition.X, pointerPosition.Y);
        args->Handled = io.WantCaptureMouse;
    }
}

void ImguiUWPEventHelper::OnPointerPressed(
    _In_ CoreWindow^ /* sender */,
    _In_ PointerEventArgs^ args
    )
{
    UpdateImguiMouseProperties(args);
}

//----------------------------------------------------------------------

void ImguiUWPEventHelper::OnPointerMoved(
    _In_ CoreWindow^ /* sender */,
    _In_ PointerEventArgs^ args
    )
{
    UpdateImguiMouseProperties(args);
}

void ImguiUWPEventHelper::OnPointerWheelChanged(
    _In_ CoreWindow^ /* sender */,
    _In_ PointerEventArgs^ args
    )
{
    PointerPoint^ point = args->CurrentPoint;
    uint32 pointerID = point->PointerId;
    Point pointerPosition = point->Position;
    PointerPointProperties^ pointProperties = point->Properties;
    auto pointerDevice = point->PointerDevice;

    if (ImGui::GetCurrentContext() != nullptr)
    {
        ImGuiIO& io = ImGui::GetIO();
        io.MouseWheel += pointProperties->MouseWheelDelta/WHEEL_DELTA;
        args->Handled = io.WantCaptureMouse;
    }
}
//----------------------------------------------------------------------

void ImguiUWPEventHelper::OnMouseMoved(
    _In_ MouseDevice^ /* mouseDevice */,
    _In_ MouseEventArgs^ args
    )
{
    // Handle Mouse Input via dedicated relative movement handler.
    //XMFLOAT2 mouseDelta;
    //mouseDelta.x = static_cast<float>(args->MouseDelta.X);
    //mouseDelta.y = static_cast<float>(args->MouseDelta.Y);
}

//----------------------------------------------------------------------

void ImguiUWPEventHelper::OnPointerReleased(
    _In_ CoreWindow^ /* sender */,
    _In_ PointerEventArgs^ args
    )
{
    UpdateImguiMouseProperties(args);
}

//----------------------------------------------------------------------

void ImguiUWPEventHelper::OnPointerExited(
    _In_ CoreWindow^ /* sender */,
    _In_ PointerEventArgs^ args
    )
{
    UpdateImguiMouseProperties(args);
}

//----------------------------------------------------------------------
static char VirtualKeyToChar(VirtualKey Key, bool bAltPressed, bool bShiftPressed, bool bCtrlPressed)
{
    int Char = 0;
    if( bShiftPressed )
    {
        if(Key>=VirtualKey::A && Key<=VirtualKey::Z)
            Char = 'A' + ((int)Key - (int)VirtualKey::A);
        else
        {
            switch(Key)
            {
                case VirtualKey::Number0: Char = ')'; break;
                case VirtualKey::Number1: Char = '!'; break;
                case VirtualKey::Number2: Char = '@'; break;
                case VirtualKey::Number3: Char = '#'; break;
                case VirtualKey::Number4: Char = '$'; break;
                case VirtualKey::Number5: Char = '%'; break;
                case VirtualKey::Number6: Char = '^'; break;
                case VirtualKey::Number7: Char = '&'; break;
                case VirtualKey::Number8: Char = '*'; break;
                case VirtualKey::Number9: Char = '('; break;

                case static_cast<VirtualKey>(189): Char = '_'; break;
                case static_cast<VirtualKey>(187): Char = '+'; break;
                case static_cast<VirtualKey>(219): Char = '{'; break;
                case static_cast<VirtualKey>(221): Char = '}'; break;
                case static_cast<VirtualKey>(220): Char = '|'; break;
                case static_cast<VirtualKey>(186): Char = ':'; break;
                case static_cast<VirtualKey>(222): Char = '\"'; break;
                case static_cast<VirtualKey>(188): Char = '<'; break;
                case static_cast<VirtualKey>(190): Char = '>'; break;
                case static_cast<VirtualKey>(191): Char = '?'; break;
            }
        }
    }
    else
    {
        if( Key>=VirtualKey::Number0 && Key<=VirtualKey::Number9 )
            Char = '0' + ((int)Key - (int)VirtualKey::Number0);
        else if( Key>=VirtualKey::NumberPad0 && Key<=VirtualKey::NumberPad9)
            Char = '0' + ((int)Key - (int)VirtualKey::NumberPad0);
        else if(Key>=VirtualKey::A && Key<=VirtualKey::Z)
            Char = 'a' + ((int)Key - (int)VirtualKey::A);
        else
        {
            switch(Key)
            {
                case static_cast<VirtualKey>(189): Char = '-'; break;
                case static_cast<VirtualKey>(187): Char = '='; break;
                case static_cast<VirtualKey>(219): Char = '['; break;
                case static_cast<VirtualKey>(221): Char = ']'; break;
                case static_cast<VirtualKey>(220): Char = '\\'; break;
                case static_cast<VirtualKey>(186): Char = ';'; break;
                case static_cast<VirtualKey>(222): Char = '\''; break;
                case static_cast<VirtualKey>(188): Char = ','; break;
                case static_cast<VirtualKey>(190): Char = '.'; break;
                case static_cast<VirtualKey>(191): Char = '/'; break;
            }
        }
    }
    return static_cast<char>(Char);
}


void ImguiUWPEventHelper::UpdateKeyStates(_In_ Windows::UI::Core::KeyEventArgs^ args, bool IsDown)
{
    if (ImGui::GetCurrentContext() == nullptr)
        return;
    
    ImGuiIO& io = ImGui::GetIO();
    int c = -1;

    switch(args->VirtualKey)
    {
        case VirtualKey::Tab:
            io.KeysDown[VK_TAB] = IsDown;
            break;
        case VirtualKey::Up:
            io.KeysDown[VK_UP] = IsDown;
            break;
        case VirtualKey::Down:
            io.KeysDown[VK_DOWN] = IsDown;
            break;
        case VirtualKey::Left:
            io.KeysDown[VK_LEFT] = IsDown;
            break;
        case VirtualKey::Right:
            io.KeysDown[VK_RIGHT] = IsDown;
            break;
        case VirtualKey::Insert:
            io.KeysDown[VK_INSERT] = IsDown;
            break;
        case VirtualKey::Delete:
            io.KeysDown[VK_DELETE] = IsDown;
            break;
        case VirtualKey::Back:
            io.KeysDown[VK_BACK] = IsDown;
            break;
        case VirtualKey::PageUp:
            io.KeysDown[VK_PRIOR] = IsDown;
            break;
        case VirtualKey::PageDown:
            io.KeysDown[VK_NEXT] = IsDown;
            break;
        case VirtualKey::Home:
            io.KeysDown[VK_HOME] = IsDown;
            break;
        case VirtualKey::End:
            io.KeysDown[VK_END] = IsDown;
            break;
        case VirtualKey::Enter:
            io.KeysDown[VK_RETURN] = IsDown;
            break;
        case VirtualKey::Escape:
            io.KeysDown[VK_ESCAPE] = IsDown;
            break;
        case VirtualKey::Space:
            io.KeysDown[VK_SPACE] = IsDown;
            c = ' ';
            break;

        case VirtualKey::Divide:    c = '/'; break;
        case VirtualKey::Multiply:  c = '*'; break;
        case VirtualKey::Subtract:  c = '-'; break;
        case VirtualKey::Add:       c = '+'; break;
        case VirtualKey::Decimal:   c = '.'; break;
    
        case VirtualKey::Shift:     io.KeyShift = IsDown; break;
        case VirtualKey::Control:   io.KeyCtrl  = IsDown; break;
        case VirtualKey::Menu:      io.KeyAlt   = IsDown; break;

        default:
            c = VirtualKeyToChar(args->VirtualKey, io.KeyAlt, io.KeyShift, io.KeyCtrl);
            
    }
    if (IsDown && c >= 0)
        io.AddInputCharacter(c);

    io.KeySuper = false;

    args->Handled = io.WantCaptureKeyboard;
}

void ImguiUWPEventHelper::OnKeyDown(
    _In_ CoreWindow^ /* sender */,
    _In_ KeyEventArgs^ args
    )
{
    UpdateKeyStates(args, true);
}

//----------------------------------------------------------------------

void ImguiUWPEventHelper::OnKeyUp(
    _In_ CoreWindow^ /* sender */,
    _In_ KeyEventArgs^ args
    )
{
    UpdateKeyStates(args, false);
}


//----------------------------------------------------------------------

void ImguiUWPEventHelper::ShowCursor()
{
    // Turn on mouse cursor.
    // This also disables relative mouse movement tracking.
    auto window = CoreWindow::GetForCurrentThread();
    if (window)
    {
        // Protect case where there isn't a window associated with the current thread.
        // This happens on initialization or when being called from a background thread.
        window->PointerCursor = ref new CoreCursor(CoreCursorType::Arrow, 0);
    }
}

//----------------------------------------------------------------------

void ImguiUWPEventHelper::HideCursor()
{
    // Turn mouse cursor off (hidden).
    // This enables relative mouse movement tracking.
    auto window = CoreWindow::GetForCurrentThread();
    if (window)
    {
        // Protect case where there isn't a window associated with the current thread.
        // This happens on initialization or when being called from a background thread.
        window->PointerCursor = nullptr;
    }
}
