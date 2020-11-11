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
#include "GenerateMipsVkHelper.hpp"
#include "RenderDeviceVkImpl.hpp"
#include "DeviceContextVkImpl.hpp"
#include "TextureViewVkImpl.hpp"
#include "TextureVkImpl.hpp"
#include "PlatformMisc.hpp"
#include "VulkanTypeConversions.hpp"
#include "../../GraphicsTools/interface/ShaderMacroHelper.hpp"
#include "../../GraphicsTools/interface/CommonlyUsedStates.h"
#include "../../GraphicsTools/interface/MapHelper.hpp"


// clang-format off
static const char* g_GenerateMipsCSSource =
{
    #include "../shaders/GenerateMipsCS_inc.h"
};
// clang-format on

namespace Diligent
{

void GenerateMipsVkHelper::GetGlImageFormat(const TextureFormatAttribs& FmtAttribs, std::array<char, 16>& GlFmt)
{
    size_t pos   = 0;
    GlFmt[pos++] = 'r';
    if (FmtAttribs.NumComponents >= 2)
        GlFmt[pos++] = 'g';
    if (FmtAttribs.NumComponents >= 3)
        GlFmt[pos++] = 'b';
    if (FmtAttribs.NumComponents >= 4)
        GlFmt[pos++] = 'a';
    VERIFY_EXPR(FmtAttribs.NumComponents <= 4);
    auto ComponentSize = Uint32{FmtAttribs.ComponentSize} * 8;

    int pow10 = 1;
    while (ComponentSize / (10 * pow10) != 0)
        pow10 *= 10;
    VERIFY_EXPR(ComponentSize != 0);
    while (ComponentSize != 0)
    {
        char digit   = static_cast<char>(ComponentSize / pow10);
        GlFmt[pos++] = '0' + digit;
        ComponentSize -= digit * pow10;
        pow10 /= 10;
    }

    switch (FmtAttribs.ComponentType)
    {
        case COMPONENT_TYPE_FLOAT:
            GlFmt[pos++] = 'f';
            break;

        case COMPONENT_TYPE_UNORM:
        case COMPONENT_TYPE_UNORM_SRGB:
            // No suffix
            break;

        case COMPONENT_TYPE_SNORM:
            GlFmt[pos++] = '_';
            GlFmt[pos++] = 's';
            GlFmt[pos++] = 'n';
            GlFmt[pos++] = 'o';
            GlFmt[pos++] = 'r';
            GlFmt[pos++] = 'm';
            break;

        case COMPONENT_TYPE_SINT:
            GlFmt[pos++] = 'i';
            break;

        case COMPONENT_TYPE_UINT:
            GlFmt[pos++] = 'u';
            GlFmt[pos++] = 'i';
            break;

        default:
            UNSUPPORTED("Unsupported component type");
    }

    GlFmt[pos] = 0;
}

std::array<RefCntAutoPtr<IPipelineState>, 4> GenerateMipsVkHelper::CreatePSOs(TEXTURE_FORMAT Fmt)
{
    std::array<RefCntAutoPtr<IPipelineState>, 4> PSOs;

#if !DILIGENT_NO_GLSLANG
    ShaderCreateInfo CSCreateInfo;

    CSCreateInfo.Source          = g_GenerateMipsCSSource;
    CSCreateInfo.EntryPoint      = "main";
    CSCreateInfo.SourceLanguage  = SHADER_SOURCE_LANGUAGE_GLSL;
    CSCreateInfo.Desc.ShaderType = SHADER_TYPE_COMPUTE;

    const auto& FmtAttribs = GetTextureFormatAttribs(Fmt);
    bool        IsGamma    = FmtAttribs.ComponentType == COMPONENT_TYPE_UNORM_SRGB;

    std::array<char, 16> GlFmt;
    GetGlImageFormat(FmtAttribs, GlFmt);

    for (Int32 NonPowOfTwo = 0; NonPowOfTwo < 4; ++NonPowOfTwo)
    {
        ShaderMacroHelper Macros;
        Macros.AddShaderMacro("NON_POWER_OF_TWO", NonPowOfTwo);
        Macros.AddShaderMacro("CONVERT_TO_SRGB", IsGamma);
        Macros.AddShaderMacro("IMG_FORMAT", GlFmt.data());

        Macros.Finalize();
        CSCreateInfo.Macros = Macros;

        std::stringstream name_ss;
        name_ss << "Generate mips " << GlFmt.data();
        switch (NonPowOfTwo)
        {
            case 0: name_ss << " even"; break;
            case 1: name_ss << " odd X"; break;
            case 2: name_ss << " odd Y"; break;
            case 3: name_ss << " odd XY"; break;
            default: UNEXPECTED("Unexpected value");
        }
        auto name              = name_ss.str();
        CSCreateInfo.Desc.Name = name.c_str();
        RefCntAutoPtr<IShader> pCS;

        m_DeviceVkImpl.CreateShader(CSCreateInfo, &pCS);

        ComputePipelineStateCreateInfo PSOCreateInfo;
        PipelineStateDesc&             PSODesc = PSOCreateInfo.PSODesc;

        PSODesc.PipelineType = PIPELINE_TYPE_COMPUTE;
        PSODesc.Name         = name.c_str();
        PSOCreateInfo.pCS    = pCS;

        PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC;
        ShaderResourceVariableDesc VarDesc{SHADER_TYPE_COMPUTE, "CB", SHADER_RESOURCE_VARIABLE_TYPE_STATIC};
        PSODesc.ResourceLayout.Variables    = &VarDesc;
        PSODesc.ResourceLayout.NumVariables = 1;

        const ImmutableSamplerDesc ImtblSampler{SHADER_TYPE_COMPUTE, "SrcMip", Sam_LinearClamp};
        PSODesc.ResourceLayout.ImmutableSamplers    = &ImtblSampler;
        PSODesc.ResourceLayout.NumImmutableSamplers = 1;

        m_DeviceVkImpl.CreateComputePipelineState(PSOCreateInfo, &PSOs[NonPowOfTwo]);
        PSOs[NonPowOfTwo]->GetStaticVariableByName(SHADER_TYPE_COMPUTE, "CB")->Set(m_ConstantsCB);
    }
#endif

    return PSOs;
}

GenerateMipsVkHelper::GenerateMipsVkHelper(RenderDeviceVkImpl& DeviceVkImpl) :
    m_DeviceVkImpl(DeviceVkImpl)
{
#if !DILIGENT_NO_GLSLANG
    BufferDesc ConstantsCBDesc;
    ConstantsCBDesc.Name           = "Constants CB buffer";
    ConstantsCBDesc.BindFlags      = BIND_UNIFORM_BUFFER;
    ConstantsCBDesc.Usage          = USAGE_DYNAMIC;
    ConstantsCBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
    ConstantsCBDesc.uiSizeInBytes  = 32;
    DeviceVkImpl.CreateBuffer(ConstantsCBDesc, nullptr, &m_ConstantsCB);

    FindPSOs(TEX_FORMAT_RGBA8_UNORM);
    FindPSOs(TEX_FORMAT_BGRA8_UNORM);
#endif
}

void GenerateMipsVkHelper::CreateSRB(IShaderResourceBinding** ppSRB)
{
#if !DILIGENT_NO_GLSLANG
    // All PSOs are compatible
    auto& PSO = FindPSOs(TEX_FORMAT_RGBA8_UNORM);
    PSO[0]->CreateShaderResourceBinding(ppSRB, true);
#endif
}

std::array<RefCntAutoPtr<IPipelineState>, 4>& GenerateMipsVkHelper::FindPSOs(TEXTURE_FORMAT Fmt)
{
    std::lock_guard<std::mutex> Lock{m_PSOMutex};

    auto it = m_PSOHash.find(Fmt);
    if (it == m_PSOHash.end())
        it = m_PSOHash.emplace(Fmt, CreatePSOs(Fmt)).first;
    return it->second;
}

void GenerateMipsVkHelper::WarmUpCache(TEXTURE_FORMAT Fmt)
{
    FindPSOs(Fmt);
}

void GenerateMipsVkHelper::GenerateMips(TextureViewVkImpl& TexView, DeviceContextVkImpl& Ctx, IShaderResourceBinding* pSRB)
{
    auto* pTexVk = TexView.GetTexture<TextureVkImpl>();
    if (!pTexVk->IsInKnownState())
    {
        LOG_ERROR_MESSAGE("Unable to generate mips for texture '", pTexVk->GetDesc().Name, "' because the texture state is unknown");
        return;
    }

    const auto  OriginalState  = pTexVk->GetState();
    const auto  OriginalLayout = pTexVk->GetLayout();
    const auto& TexDesc        = pTexVk->GetDesc();
    const auto& ViewDesc       = TexView.GetDesc();

    DEV_CHECK_ERR(ViewDesc.NumMipLevels > 1, "Number of mip levels in the view must be greater than 1");
    DEV_CHECK_ERR(OriginalState != RESOURCE_STATE_UNDEFINED,
                  "Attempting to generate mipmaps for texture '", TexDesc.Name,
                  "' which is in RESOURCE_STATE_UNDEFINED state ."
                  "This is not expected in Vulkan backend as textures are transition to a defined state when created.");
    (void)OriginalState;

    const auto& FmtAttribs = GetTextureFormatAttribs(ViewDesc.Format);

    VkImageSubresourceRange SubresRange = {};
    if (FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH)
        SubresRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    else if (FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH_STENCIL)
    {
        // If image has a depth / stencil format with both depth and stencil components, then the
        // aspectMask member of subresourceRange must include both VK_IMAGE_ASPECT_DEPTH_BIT and
        // VK_IMAGE_ASPECT_STENCIL_BIT (6.7.3)
        SubresRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    else
        SubresRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    SubresRange.baseArrayLayer = ViewDesc.FirstArraySlice;
    SubresRange.layerCount     = ViewDesc.NumArraySlices;
    SubresRange.baseMipLevel   = ViewDesc.MostDetailedMip;
    SubresRange.levelCount     = 1;

    VkImageLayout AffectedMipLevelLayout;
#if !DILIGENT_NO_GLSLANG
    if (TexView.HasMipLevelViews())
    {
        VERIFY_EXPR(pSRB != nullptr);
        AffectedMipLevelLayout = GenerateMipsCS(TexView, Ctx, *pSRB, SubresRange);
    }
    else
#endif
    {
        AffectedMipLevelLayout = GenerateMipsBlit(TexView, Ctx, SubresRange);
    }

    // All affected mip levels are now in VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL state
    if (AffectedMipLevelLayout != OriginalLayout)
    {
        bool IsAllSlices = (TexDesc.Type != RESOURCE_DIM_TEX_1D_ARRAY &&
                            TexDesc.Type != RESOURCE_DIM_TEX_2D_ARRAY &&
                            TexDesc.Type != RESOURCE_DIM_TEX_CUBE_ARRAY) ||
            TexDesc.ArraySize == ViewDesc.NumArraySlices;
        bool IsAllMips = ViewDesc.NumMipLevels == TexDesc.MipLevels;
        if (IsAllSlices && IsAllMips)
        {
            pTexVk->SetLayout(AffectedMipLevelLayout);
        }
        else
        {
            VERIFY(OriginalLayout != VK_IMAGE_LAYOUT_UNDEFINED, "Original layout must not be undefined");
            SubresRange.baseMipLevel = ViewDesc.MostDetailedMip;
            SubresRange.levelCount   = ViewDesc.NumMipLevels;
            // Transition all affected subresources back to original layout
            Ctx.TransitionImageLayout(*pTexVk, AffectedMipLevelLayout, OriginalLayout, SubresRange);
            VERIFY_EXPR(pTexVk->GetLayout() == OriginalLayout);
        }
    }
}

VkImageLayout GenerateMipsVkHelper::GenerateMipsCS(TextureViewVkImpl& TexView, DeviceContextVkImpl& Ctx, IShaderResourceBinding& SRB, VkImageSubresourceRange& SubresRange)
{
    auto*       pTexVk  = TexView.GetTexture<TextureVkImpl>();
    const auto& TexDesc = pTexVk->GetDesc();

    VERIFY(TexDesc.Type == RESOURCE_DIM_TEX_2D || TexDesc.Type == RESOURCE_DIM_TEX_2D_ARRAY,
           "CS-based mipmap generation is only supported for 2D textures and texture arrays");

    const auto& ViewDesc   = TexView.GetDesc();
    auto*       pSrcMipVar = SRB.GetVariableByName(SHADER_TYPE_COMPUTE, "SrcMip");

    IShaderResourceVariable* pOutMipVar[4] =
        {
            SRB.GetVariableByName(SHADER_TYPE_COMPUTE, "OutMip0"),
            SRB.GetVariableByName(SHADER_TYPE_COMPUTE, "OutMip1"),
            SRB.GetVariableByName(SHADER_TYPE_COMPUTE, "OutMip2"),
            SRB.GetVariableByName(SHADER_TYPE_COMPUTE, "OutMip3") //
        };

    auto& PSOs = FindPSOs(ViewDesc.Format);

    const auto OriginalState  = pTexVk->GetState();
    const auto OriginalLayout = pTexVk->GetLayout();

    // Transition the lowest mip level to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    SubresRange.baseMipLevel = ViewDesc.MostDetailedMip;
    SubresRange.levelCount   = 1;
    if (OriginalState != RESOURCE_STATE_SHADER_RESOURCE)
        Ctx.TransitionTextureState(*pTexVk, OriginalState, RESOURCE_STATE_SHADER_RESOURCE, false /*UpdateTextureState*/, &SubresRange);
    VERIFY_EXPR(ResourceStateToVkImageLayout(RESOURCE_STATE_SHADER_RESOURCE) == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // Note that mip levels are relative to the view's most detailed mip
    auto BottomMip = ViewDesc.NumMipLevels - 1;
    for (uint32_t TopMip = 0; TopMip < BottomMip;)
    {
        // In Vulkan, all subresources of a view must be transitioned to the same layout, so
        // we can't bind the entire texture and have to bind single mip level at a time
        auto SrcMipLevelSRV = TexView.GetMipLevelSRV(TopMip);
        VERIFY_EXPR(SrcMipLevelSRV != nullptr);
        pSrcMipVar->Set(SrcMipLevelSRV);

        uint32_t SrcWidth  = std::max(TexDesc.Width >> (TopMip + ViewDesc.MostDetailedMip), 1u);
        uint32_t SrcHeight = std::max(TexDesc.Height >> (TopMip + ViewDesc.MostDetailedMip), 1u);
        uint32_t DstWidth  = std::max(SrcWidth >> 1, 1u);
        uint32_t DstHeight = std::max(SrcHeight >> 1, 1u);

        // Determine if the first downsample is more than 2:1.  This happens whenever
        // the source width or height is odd.
        uint32_t NonPowerOfTwo = (SrcWidth & 1) | (SrcHeight & 1) << 1;
        Ctx.SetPipelineState(PSOs[NonPowerOfTwo]);

        // We can downsample up to four times, but if the ratio between levels is not
        // exactly 2:1, we have to shift our blend weights, which gets complicated or
        // expensive.  Maybe we can update the code later to compute sample weights for
        // each successive downsample.  We use _BitScanForward to count number of zeros
        // in the low bits.  Zeros indicate we can divide by two without truncating.
        uint32_t AdditionalMips = PlatformMisc::GetLSB(DstWidth | DstHeight);
        uint32_t NumMips        = 1 + (AdditionalMips > 3 ? 3 : AdditionalMips);
        if (TopMip + NumMips > BottomMip)
            NumMips = BottomMip - TopMip;

        // These are clamped to 1 after computing additional mips because clamped
        // dimensions should not limit us from downsampling multiple times.  (E.g.
        // 16x1 -> 8x1 -> 4x1 -> 2x1 -> 1x1.)
        if (DstWidth == 0)
            DstWidth = 1;
        if (DstHeight == 0)
            DstHeight = 1;

        {
            struct CBData
            {
                Int32 SrcMipLevel;  // Texture level of source mip
                Int32 NumMipLevels; // Number of OutMips to write: [1, 4]
                Int32 ArraySlice;
                Int32 Dummy;
                float TexelSize[2]; // 1.0 / OutMip1.Dimensions
            };
            MapHelper<CBData> MappedData(&Ctx, m_ConstantsCB, MAP_WRITE, MAP_FLAG_DISCARD);

            *MappedData =
                {
                    static_cast<Int32>(TopMip), // Mip levels are relateive to the view's most detailed mip
                    static_cast<Int32>(NumMips),
                    0, // Array slices are relative to the view's first array slice
                    0, // Unused
                    {
                        1.0f / static_cast<float>(DstWidth), 1.0f / static_cast<float>(DstHeight) //
                    }                                                                             //
                };
        }

        constexpr const Uint32 MaxMipsHandledByCS = 4; // Max number of mip levels processed by one CS shader invocation
        for (Uint32 u = 0; u < MaxMipsHandledByCS; ++u)
        {
            auto* MipLevelUAV = TexView.GetMipLevelUAV(TopMip + std::min(u + 1, NumMips));
            pOutMipVar[u]->Set(MipLevelUAV);
        }

        SubresRange.baseMipLevel = ViewDesc.MostDetailedMip + TopMip + 1;
        SubresRange.levelCount   = NumMips;
        if (OriginalLayout != VK_IMAGE_LAYOUT_GENERAL)
            Ctx.TransitionImageLayout(*pTexVk, OriginalLayout, VK_IMAGE_LAYOUT_GENERAL, SubresRange);

        Ctx.CommitShaderResources(&SRB, RESOURCE_STATE_TRANSITION_MODE_NONE);
        DispatchComputeAttribs DispatchAttrs((DstWidth + 7) / 8, (DstHeight + 7) / 8, ViewDesc.NumArraySlices);
        Ctx.DispatchCompute(DispatchAttrs);

        Ctx.TransitionImageLayout(*pTexVk, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, SubresRange);

        TopMip += NumMips;
    }

    return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

VkImageLayout GenerateMipsVkHelper::GenerateMipsBlit(TextureViewVkImpl& TexView, DeviceContextVkImpl& Ctx, VkImageSubresourceRange& SubresRange) const
{
    auto*       pTexVk   = TexView.GetTexture<TextureVkImpl>();
    const auto& TexDesc  = pTexVk->GetDesc();
    const auto& ViewDesc = TexView.GetDesc();
    auto        vkImage  = pTexVk->GetVkImage();

    const auto OriginalState  = pTexVk->GetState();
    const auto OriginalLayout = ResourceStateToVkImageLayout(OriginalState);

    VkImageBlit BlitRegion = {};

    BlitRegion.srcSubresource.baseArrayLayer = ViewDesc.FirstArraySlice;
    BlitRegion.srcSubresource.layerCount     = ViewDesc.NumArraySlices;
    BlitRegion.srcSubresource.aspectMask     = SubresRange.aspectMask;
    BlitRegion.dstSubresource.baseArrayLayer = BlitRegion.srcSubresource.baseArrayLayer;
    BlitRegion.dstSubresource.layerCount     = BlitRegion.srcSubresource.layerCount;
    BlitRegion.dstSubresource.aspectMask     = BlitRegion.srcSubresource.aspectMask;
    BlitRegion.srcOffsets[0]                 = VkOffset3D{0, 0, 0};
    BlitRegion.dstOffsets[0]                 = VkOffset3D{0, 0, 0};

    SubresRange.baseMipLevel = ViewDesc.MostDetailedMip;
    SubresRange.levelCount   = 1;
    if (OriginalState != RESOURCE_STATE_COPY_SOURCE)
        Ctx.TransitionTextureState(*pTexVk, OriginalState, RESOURCE_STATE_COPY_SOURCE, false /*UpdateTextureState*/, &SubresRange);

    auto& CmdBuffer = Ctx.GetCommandBuffer();
    for (uint32_t mip = ViewDesc.MostDetailedMip + 1; mip < ViewDesc.MostDetailedMip + ViewDesc.NumMipLevels; ++mip)
    {
        BlitRegion.srcSubresource.mipLevel = mip - 1;
        BlitRegion.dstSubresource.mipLevel = mip;

        BlitRegion.srcOffsets[1] =
            VkOffset3D //
            {
                static_cast<int32_t>(std::max(TexDesc.Width >> (mip - 1), 1u)),
                static_cast<int32_t>(std::max(TexDesc.Height >> (mip - 1), 1u)),
                1 //
            };
        BlitRegion.dstOffsets[1] =
            VkOffset3D //
            {
                static_cast<int32_t>(std::max(TexDesc.Width >> mip, 1u)),
                static_cast<int32_t>(std::max(TexDesc.Height >> mip, 1u)),
                1 //
            };
        if (TexDesc.Type == RESOURCE_DIM_TEX_3D)
        {
            BlitRegion.srcOffsets[1].z = std::max(TexDesc.Depth >> (mip - 1), 1u);
            BlitRegion.dstOffsets[1].z = std::max(TexDesc.Depth >> mip, 1u);
        }

        SubresRange.baseMipLevel = mip;
        if (OriginalLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
            Ctx.TransitionImageLayout(*pTexVk, OriginalLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, SubresRange);

        CmdBuffer.BlitImage(vkImage,
                            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, //  must be VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL or VK_IMAGE_LAYOUT_GENERAL
                            vkImage,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, //  must be VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL or VK_IMAGE_LAYOUT_GENERAL
                            1,
                            &BlitRegion,
                            VK_FILTER_LINEAR);
        Ctx.TransitionImageLayout(*pTexVk, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, SubresRange);
    }

    return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
}

} // namespace Diligent
