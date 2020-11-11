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

#include <cstddef>
#include "imgui.h"
#include "ImGuiImplDiligent.hpp"
#include "ImGuiDiligentRenderer.hpp"
#include "RenderDevice.h"
#include "DeviceContext.h"
#include "RefCntAutoPtr.hpp"
#include "BasicMath.hpp"
#include "MapHelper.hpp"

namespace Diligent
{

ImGuiImplDiligent::ImGuiImplDiligent(IRenderDevice* pDevice,
                                     TEXTURE_FORMAT BackBufferFmt,
                                     TEXTURE_FORMAT DepthBufferFmt,
                                     Uint32         InitialVertexBufferSize,
                                     Uint32         InitialIndexBufferSize)
{
    ImGui::CreateContext();
    ImGuiIO& io    = ImGui::GetIO();
    io.IniFilename = nullptr;
    m_pRenderer.reset(new ImGuiDiligentRenderer(pDevice, BackBufferFmt, DepthBufferFmt, InitialVertexBufferSize, InitialIndexBufferSize));
}

ImGuiImplDiligent::~ImGuiImplDiligent()
{
    ImGui::DestroyContext();
}

void ImGuiImplDiligent::NewFrame(Uint32 RenderSurfaceWidth, Uint32 RenderSurfaceHeight, SURFACE_TRANSFORM SurfacePreTransform)
{
    m_pRenderer->NewFrame(RenderSurfaceWidth, RenderSurfaceHeight, SurfacePreTransform);
    ImGui::NewFrame();
}

void ImGuiImplDiligent::EndFrame()
{
    ImGui::EndFrame();
}

void ImGuiImplDiligent::Render(IDeviceContext* pCtx)
{
    // No need to call ImGui::EndFrame as ImGui::Render calls it automatically
    ImGui::Render();
    m_pRenderer->RenderDrawData(pCtx, ImGui::GetDrawData());
}

// Use if you want to reset your rendering device without losing ImGui state.
void ImGuiImplDiligent::InvalidateDeviceObjects()
{
    m_pRenderer->InvalidateDeviceObjects();
}

void ImGuiImplDiligent::CreateDeviceObjects()
{
    m_pRenderer->CreateDeviceObjects();
}

void ImGuiImplDiligent::UpdateFontsTexture()
{
    m_pRenderer->CreateFontsTexture();
}

} // namespace Diligent
