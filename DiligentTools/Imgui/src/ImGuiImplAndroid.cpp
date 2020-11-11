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

#include "ImGuiImplAndroid.hpp"
#include "GraphicsTypes.h"
#include "DebugUtilities.hpp"

namespace Diligent
{

ImGuiImplAndroid::ImGuiImplAndroid(IRenderDevice* pDevice,
                                   TEXTURE_FORMAT BackBufferFmt,
                                   TEXTURE_FORMAT DepthBufferFmt,
                                   Uint32         InitialVertexBufferSize,
                                   Uint32         InitialIndexBufferSize) :
    ImGuiImplDiligent{pDevice, BackBufferFmt, DepthBufferFmt, InitialVertexBufferSize, InitialIndexBufferSize}
{
    auto& io = ImGui::GetIO();

    io.FontGlobalScale     = 2;
    io.BackendPlatformName = "Diligent-ImGuiImplAndroid";

    m_LastTimestamp = std::chrono::high_resolution_clock::now();
}

ImGuiImplAndroid::~ImGuiImplAndroid()
{
}

void ImGuiImplAndroid::NewFrame(Uint32            RenderSurfaceWidth,
                                Uint32            RenderSurfaceHeight,
                                SURFACE_TRANSFORM SurfacePreTransform)
{
    auto now        = std::chrono::high_resolution_clock::now();
    auto elapsed_ns = now - m_LastTimestamp;
    m_LastTimestamp = now;
    auto& io        = ImGui::GetIO();
    io.DeltaTime    = static_cast<float>(elapsed_ns.count() / 1e+9);

    // DisplaySize must always refer to the logical size that accounts for the pre-transform
    io.DisplaySize.x = static_cast<float>(RenderSurfaceWidth);
    io.DisplaySize.y = static_cast<float>(RenderSurfaceHeight);
    switch (SurfacePreTransform)
    {
        case SURFACE_TRANSFORM_IDENTITY:
        case SURFACE_TRANSFORM_ROTATE_180:
        case SURFACE_TRANSFORM_HORIZONTAL_MIRROR:
        case SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180:
            // Do nothing
            break;

        case SURFACE_TRANSFORM_ROTATE_90:
        case SURFACE_TRANSFORM_ROTATE_270:
        case SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90:
        case SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270:
            std::swap(io.DisplaySize.x, io.DisplaySize.y);
            break;

        case SURFACE_TRANSFORM_OPTIMAL:
            UNEXPECTED("SURFACE_TRANSFORM_OPTIMAL is only valid as parameter during swap chain initialization.");
            break;

        default:
            UNEXPECTED("Unknown transform");
    }

    ImGuiImplDiligent::NewFrame(RenderSurfaceWidth, RenderSurfaceHeight, SurfacePreTransform);
}

bool ImGuiImplAndroid::BeginDrag(float x, float y)
{
    auto& io        = ImGui::GetIO();
    io.MousePos     = ImVec2(x, y);
    io.MouseDown[0] = true;
    return io.WantCaptureMouse;
}

bool ImGuiImplAndroid::DragMove(float x, float y)
{
    auto& io    = ImGui::GetIO();
    io.MousePos = ImVec2(x, y);
    return io.WantCaptureMouse;
}

bool ImGuiImplAndroid::EndDrag()
{
    auto& io        = ImGui::GetIO();
    io.MouseDown[0] = false;
    return io.WantCaptureMouse;
}

} // namespace Diligent
