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

#include "pch.h"
#include <sstream>
#include "RenderPassCache.hpp"
#include "RenderDeviceVkImpl.hpp"
#include "PipelineStateVkImpl.hpp"
#include "RenderPassVkImpl.hpp"

namespace Diligent
{

RenderPassCache::RenderPassCache(RenderDeviceVkImpl& DeviceVk) noexcept :
    m_DeviceVkImpl{DeviceVk}
{}


RenderPassCache::~RenderPassCache()
{
    // Render pass cache is part of the render device, so we can't release
    // render pass objects from here as their destructors will attmept to
    // call SafeReleaseDeviceObject.
    VERIFY(m_Cache.empty(), "Render pass cache is not empty. Did you call Destroy?");
}

void RenderPassCache::Destroy()
{
    auto& FBCache = m_DeviceVkImpl.GetFramebufferCache();
    for (auto it = m_Cache.begin(); it != m_Cache.end(); ++it)
    {
        FBCache.OnDestroyRenderPass(it->second->GetVkRenderPass());
    }
    m_Cache.clear();
}


RenderPassVkImpl* RenderPassCache::GetRenderPass(const RenderPassCacheKey& Key)
{
    std::lock_guard<std::mutex> Lock{m_Mutex};
    auto                        it = m_Cache.find(Key);
    if (it == m_Cache.end())
    {
        // Do not zero-intitialize arrays
        std::array<RenderPassAttachmentDesc, MAX_RENDER_TARGETS + 1> Attachments;
        std::array<AttachmentReference, MAX_RENDER_TARGETS + 1>      AttachmentReferences;

        SubpassDesc Subpass;

        auto RPDesc =
            PipelineStateVkImpl::GetImplicitRenderPassDesc(Key.NumRenderTargets, Key.RTVFormats, Key.DSVFormat,
                                                           Key.SampleCount, Attachments, AttachmentReferences, Subpass);
        std::stringstream PassNameSS;
        PassNameSS << "Implicit render pass: RT count: " << Uint32{Key.NumRenderTargets} << "; sample count: " << Uint32{Key.SampleCount}
                   << "; DSV Format: " << GetTextureFormatAttribs(Key.DSVFormat).Name;
        if (Key.NumRenderTargets > 0)
        {
            PassNameSS << (Key.NumRenderTargets > 1 ? "; RTV Formats: " : "; RTV Format: ");
            for (Uint32 rt = 0; rt < Key.NumRenderTargets; ++rt)
            {
                PassNameSS << (rt > 0 ? ", " : "") << GetTextureFormatAttribs(Key.RTVFormats[rt]).Name;
            }
        }

        RefCntAutoPtr<RenderPassVkImpl> pRenderPass;
        m_DeviceVkImpl.CreateRenderPass(RPDesc, pRenderPass.GetRawDblPtr<IRenderPass>(), /* IsDeviceInternal = */ true);
        VERIFY_EXPR(pRenderPass != nullptr);
        it = m_Cache.emplace(Key, std::move(pRenderPass)).first;
    }

    return it->second;
}

} // namespace Diligent
