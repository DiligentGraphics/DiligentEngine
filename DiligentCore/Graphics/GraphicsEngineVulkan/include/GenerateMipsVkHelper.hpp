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

/// \file
/// Implementation of mipmap generation routines

#include <array>
#include <unordered_map>
#include "VulkanUtilities/VulkanLogicalDevice.hpp"
#include "VulkanUtilities/VulkanCommandBuffer.hpp"

namespace Diligent
{

class RenderDeviceVkImpl;
class TextureViewVkImpl;
class DeviceContextVkImpl;

class GenerateMipsVkHelper
{
public:
    GenerateMipsVkHelper(RenderDeviceVkImpl& DeviceVkImpl);

    // clang-format off
    GenerateMipsVkHelper             (const GenerateMipsVkHelper&)  = delete;
    GenerateMipsVkHelper             (      GenerateMipsVkHelper&&) = delete;
    GenerateMipsVkHelper& operator = (const GenerateMipsVkHelper&)  = delete;
    GenerateMipsVkHelper& operator = (      GenerateMipsVkHelper&&) = delete;
    // clang-format on

    void GenerateMips(TextureViewVkImpl& TexView, DeviceContextVkImpl& Ctx, IShaderResourceBinding* pSRB);
    void CreateSRB(IShaderResourceBinding** ppSRB);
    void WarmUpCache(TEXTURE_FORMAT Fmt);

private:
    std::array<RefCntAutoPtr<IPipelineState>, 4>  CreatePSOs(TEXTURE_FORMAT Fmt);
    std::array<RefCntAutoPtr<IPipelineState>, 4>& FindPSOs(TEXTURE_FORMAT Fmt);

    VkImageLayout GenerateMipsCS(TextureViewVkImpl& TexView, DeviceContextVkImpl& Ctx, IShaderResourceBinding& SRB, VkImageSubresourceRange& SubresRange);
    VkImageLayout GenerateMipsBlit(TextureViewVkImpl& TexView, DeviceContextVkImpl& Ctx, VkImageSubresourceRange& SubresRange) const;

    RenderDeviceVkImpl& m_DeviceVkImpl;

    std::mutex                                                                       m_PSOMutex;
    std::unordered_map<TEXTURE_FORMAT, std::array<RefCntAutoPtr<IPipelineState>, 4>> m_PSOHash;

    static void GetGlImageFormat(const TextureFormatAttribs& FmtAttribs, std::array<char, 16>& GlFmt);

    RefCntAutoPtr<IBuffer> m_ConstantsCB;
};

} // namespace Diligent
