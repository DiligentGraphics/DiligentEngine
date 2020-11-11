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

#pragma once

#include <memory>
#include "ImGuiImplDiligent.hpp"

namespace Diligent
{

class ImGuiImplWin32 final : public ImGuiImplDiligent
{
public:
    ImGuiImplWin32(HWND           hWnd,
                   IRenderDevice* pDevice,
                   TEXTURE_FORMAT BackBufferFmt,
                   TEXTURE_FORMAT DepthBufferFmt,
                   Uint32         InitialVertexBufferSize = ImGuiImplDiligent::DefaultInitialVBSize,
                   Uint32         InitialIndexBufferSize  = ImGuiImplDiligent::DefaultInitialIBSize);
    ~ImGuiImplWin32();

    LRESULT Win32_ProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // clang-format off
    ImGuiImplWin32             (const ImGuiImplWin32&)  = delete;
    ImGuiImplWin32             (      ImGuiImplWin32&&) = delete;
    ImGuiImplWin32& operator = (const ImGuiImplWin32&)  = delete;
    ImGuiImplWin32& operator = (      ImGuiImplWin32&&) = delete;
    // clang-format on

    virtual void NewFrame(Uint32 RenderSurfaceWidth, Uint32 RenderSurfaceHeight, SURFACE_TRANSFORM SurfacePreTransform) override final;
};

} // namespace Diligent
