/*     Copyright 2019 Diligent Graphics LLC
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

#include "imgui.h"
#include "ImGuiImplMacOS.hpp"
#include "examples/imgui_impl_osx.h"
#import <Cocoa/Cocoa.h>

namespace Diligent
{

ImGuiImplMacOS::ImGuiImplMacOS(IRenderDevice*  pDevice,
                               TEXTURE_FORMAT  BackBufferFmt,
                               TEXTURE_FORMAT  DepthBufferFmt,
                               Uint32          InitialVertexBufferSize,
                               Uint32          InitialIndexBufferSize) :
    ImGuiImplDiligent(pDevice, BackBufferFmt, DepthBufferFmt, InitialVertexBufferSize, InitialIndexBufferSize)
{
    ImGui_ImplOSX_Init();
    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = 2;
    io.BackendPlatformName = "Diligent-ImGuiImplMacOS";
}

ImGuiImplMacOS::~ImGuiImplMacOS()
{
    ImGui_ImplOSX_Shutdown();
}

void ImGuiImplMacOS::NewFrame(Uint32            RenderSurfaceWidth,
                              Uint32            RenderSurfaceHeight,
                              SURFACE_TRANSFORM SurfacePreTransform)
{
    std::lock_guard<std::mutex> Lock(m_Mtx);
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(RenderSurfaceWidth, RenderSurfaceHeight);
    ImGui_ImplOSX_NewFrame(nil);
    ImGuiImplDiligent::NewFrame(RenderSurfaceWidth, RenderSurfaceHeight, SurfacePreTransform);
}

bool ImGuiImplMacOS::HandleOSXEvent(NSEvent *_Nonnull event, NSView *_Nonnull view)
{
    std::lock_guard<std::mutex> Lock(m_Mtx);
    ImGuiIO& io = ImGui::GetIO();
    if (event.type == NSEventTypeMouseMoved || event.type == NSEventTypeLeftMouseDragged)
    {
        NSRect viewRectPoints = [view bounds];
        NSRect viewRectPixels = [view convertRectToBacking:viewRectPoints];
        NSPoint curPoint = [view convertPoint:[event locationInWindow] fromView:nil];
        curPoint = [view convertPointToBacking:curPoint];
        io.MousePos = ImVec2(curPoint.x, viewRectPixels.size.height-1 - curPoint.y);
        return io.WantCaptureMouse;
    }

    return ImGui_ImplOSX_HandleEvent((NSEvent*)event, (NSView*)view);
}

void ImGuiImplMacOS::Render(IDeviceContext* pCtx)
{
    std::lock_guard<std::mutex> Lock(m_Mtx);
    ImGuiImplDiligent::Render(pCtx);
}

}
