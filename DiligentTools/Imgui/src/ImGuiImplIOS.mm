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
#include "ImGuiImplIOS.hpp"
#import <CoreFoundation/CoreFoundation.h>

namespace Diligent
{

ImGuiImplIOS::ImGuiImplIOS(IRenderDevice*  pDevice,
                           TEXTURE_FORMAT  BackBufferFmt,
                           TEXTURE_FORMAT  DepthBufferFmt,
                           Uint32          InitialVertexBufferSize,
                           Uint32          InitialIndexBufferSize) :
    ImGuiImplDiligent(pDevice, BackBufferFmt, DepthBufferFmt, InitialVertexBufferSize, InitialIndexBufferSize)
{
    ImGuiIO& io = ImGui::GetIO();
    //io.FontGlobalScale = 2;
    io.BackendPlatformName = "Diligent-ImGuiImplIOS";
}

ImGuiImplIOS::~ImGuiImplIOS()
{
}

void ImGuiImplIOS::NewFrame(Uint32 RenderSurfaceWidth, Uint32 RenderSurfaceHeight, SURFACE_TRANSFORM SurfacePreTransform)
{
    std::lock_guard<std::mutex> Lock(m_Mtx);
    if (m_Time == 0.0)
        m_Time = CFAbsoluteTimeGetCurrent();
    CFAbsoluteTime current_time = CFAbsoluteTimeGetCurrent();
    ImGuiIO& io = ImGui::GetIO();
    io.DeltaTime = current_time - m_Time;
    m_Time = current_time;

    io.DisplaySize = ImVec2(RenderSurfaceWidth, RenderSurfaceHeight);

    ImGuiImplDiligent::NewFrame(RenderSurfaceWidth, RenderSurfaceHeight, SurfacePreTransform);
}

bool ImGuiImplIOS::OnTouchEvent(float x, float y, bool IsActive)
{
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2(x, y);
    io.MouseDown[0] = IsActive;
    return io.WantCaptureMouse;
}

void ImGuiImplIOS::Render(IDeviceContext* pCtx)
{
    std::lock_guard<std::mutex> Lock(m_Mtx);
    ImGuiImplDiligent::Render(pCtx);
}

}
