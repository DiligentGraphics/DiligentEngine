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

#include <climits>

#include "NuklearDemo.hpp"

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_VARARGS
#include "../../../ThirdParty/nuklear/nuklear.h"

#include "NkDiligent.h"

#include "../../../ThirdParty/nuklear/demo/style.c"
#include "../../../ThirdParty/nuklear/demo/overview.c"


namespace Diligent
{

SampleBase* CreateSample()
{
    return new NuklearDemo();
}

NuklearDemo::~NuklearDemo()
{
    nk_diligent_shutdown(m_pNkDlgCtx);
}

void NuklearDemo::Initialize(const SampleInitInfo& InitInfo)
{
    SampleBase::Initialize(InitInfo);

    constexpr Uint32 NuklearMaxVBSize = 512 * 1024;
    constexpr Uint32 NuklearMaxIBSize = 128 * 1024;

    const auto& SCDesc = m_pSwapChain->GetDesc();

    m_pNkDlgCtx = nk_diligent_init(m_pDevice, SCDesc.Width, SCDesc.Height, SCDesc.ColorBufferFormat, SCDesc.DepthBufferFormat, NuklearMaxVBSize, NuklearMaxIBSize);
    m_pNkCtx    = nk_diligent_get_nk_ctx(m_pNkDlgCtx);

    nk_font_atlas* atlas = nullptr;
    nk_diligent_font_stash_begin(m_pNkDlgCtx, &atlas);
    nk_diligent_font_stash_end(m_pNkDlgCtx, m_pImmediateContext);

    //set_style(m_pNkCtx, THEME_WHITE);
    //set_style(m_pNkCtx, THEME_RED);
    //set_style(m_pNkCtx, THEME_BLUE);
    set_style(m_pNkCtx, THEME_DARK);
}

void NuklearDemo::UpdateUI()
{
    nk_input_end(m_pNkCtx); // Needs to go after msg loop

    overview(m_pNkCtx);

    nk_input_begin(m_pNkCtx); // Needs to go before msg loop
}

// Render a frame
void NuklearDemo::Render()
{
    auto* pRTV = m_pSwapChain->GetCurrentBackBufferRTV();

    const float4 ClearColor = {0.45f, 0.55f, 0.60f, 1.00f};
    m_pImmediateContext->ClearRenderTarget(pRTV, &ClearColor.x, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    nk_diligent_render(m_pNkDlgCtx, m_pImmediateContext, NK_ANTI_ALIASING_ON);
}


void NuklearDemo::Update(double CurrTime, double ElapsedTime)
{
    SampleBase::Update(CurrTime, ElapsedTime);

    UpdateUI();
}

void NuklearDemo::WindowResize(Uint32 Width, Uint32 Height)
{
    nk_diligent_resize(m_pNkDlgCtx, m_pImmediateContext, Width, Height);
}

} // namespace Diligent
