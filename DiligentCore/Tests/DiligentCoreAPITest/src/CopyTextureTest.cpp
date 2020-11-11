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

#include "TestingEnvironment.hpp"

#include "gtest/gtest.h"

using namespace Diligent;
using namespace Diligent::Testing;

namespace
{

// clang-format off
TEXTURE_FORMAT TestFormats[] = 
{
    TEX_FORMAT_RGBA32_FLOAT,
    TEX_FORMAT_RGBA32_UINT,
    TEX_FORMAT_RGBA32_SINT,
    TEX_FORMAT_RGBA16_FLOAT,
    TEX_FORMAT_RGBA16_UINT,
    TEX_FORMAT_RGBA16_SINT,
    TEX_FORMAT_RGBA8_UNORM,
    TEX_FORMAT_RGBA8_SNORM,
    TEX_FORMAT_RGBA8_UINT,
    TEX_FORMAT_RGBA8_SINT,

    TEX_FORMAT_RG32_FLOAT,
    TEX_FORMAT_RG32_UINT,
    TEX_FORMAT_RG32_SINT,
    TEX_FORMAT_RG16_FLOAT,
    TEX_FORMAT_RG16_UINT,
    TEX_FORMAT_RG16_SINT,
    TEX_FORMAT_RG8_UNORM,
    TEX_FORMAT_RG8_SNORM,
    TEX_FORMAT_RG8_UINT,
    TEX_FORMAT_RG8_SINT,

    TEX_FORMAT_R32_FLOAT,
    TEX_FORMAT_R32_UINT,
    TEX_FORMAT_R32_SINT,
    TEX_FORMAT_R16_FLOAT,
    TEX_FORMAT_R16_UINT,
    TEX_FORMAT_R16_SINT,
    TEX_FORMAT_R8_UNORM,
    TEX_FORMAT_R8_SNORM,
    TEX_FORMAT_R8_UINT,
    TEX_FORMAT_R8_SINT
};
// clang-format on

TEST(CopyTexture, Texture2D)
{
    auto* pEnv     = TestingEnvironment::GetInstance();
    auto* pDevice  = pEnv->GetDevice();
    auto* pContext = pEnv->GetDeviceContext();

    TestingEnvironment::ScopedReleaseResources EnvironmentAutoReset;

    for (auto f = 0; f < _countof(TestFormats); ++f)
    {
        auto Format = TestFormats[f];

        TextureDesc TexDesc;
        TexDesc.Type      = RESOURCE_DIM_TEX_2D;
        TexDesc.Format    = Format;
        TexDesc.Width     = 128;
        TexDesc.Height    = 128;
        TexDesc.BindFlags = BIND_SHADER_RESOURCE;
        TexDesc.MipLevels = 5;
        TexDesc.Usage     = USAGE_DEFAULT;

        Diligent::RefCntAutoPtr<ITexture> pSrcTex, pDstTex;

        auto                 FmtAttribs = pDevice->GetTextureFormatInfo(Format);
        auto                 TexelSize  = Uint32{FmtAttribs.ComponentSize} * Uint32{FmtAttribs.NumComponents};
        std::vector<uint8_t> DummyData(size_t{TexDesc.Width} * size_t{TexDesc.Height} * size_t{TexelSize});
        TextureData          InitData;
        InitData.NumSubresources = TexDesc.MipLevels;
        std::vector<TextureSubResData> SubResData(InitData.NumSubresources);
        for (Uint32 mip = 0; mip < TexDesc.MipLevels; ++mip)
        {
            SubResData[mip] = TextureSubResData{DummyData.data(), (TexDesc.Width >> mip) * TexelSize, 0};
        }
        InitData.pSubResources = SubResData.data();
        pDevice->CreateTexture(TexDesc, &InitData, &pSrcTex);
        pDevice->CreateTexture(TexDesc, nullptr, &pDstTex);

        CopyTextureAttribs CopyAttribs;
        CopyAttribs.pSrcTexture              = pSrcTex;
        CopyAttribs.SrcMipLevel              = 2;
        CopyAttribs.pDstTexture              = pDstTex;
        CopyAttribs.DstMipLevel              = 1;
        CopyAttribs.DstX                     = 32;
        CopyAttribs.DstY                     = 16;
        CopyAttribs.DstZ                     = 0;
        CopyAttribs.SrcTextureTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
        CopyAttribs.DstTextureTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
        pContext->CopyTexture(CopyAttribs);

        Box SrcBox;
        SrcBox.MinX = 3;
        SrcBox.MaxX = 19;
        SrcBox.MinY = 1;
        SrcBox.MaxY = 32;

        CopyAttribs.pSrcBox                  = &SrcBox;
        CopyAttribs.SrcTextureTransitionMode = RESOURCE_STATE_TRANSITION_MODE_VERIFY;
        CopyAttribs.DstTextureTransitionMode = RESOURCE_STATE_TRANSITION_MODE_VERIFY;
        pContext->CopyTexture(CopyAttribs);

        pEnv->ReleaseResources();
    }
}

TEST(CopyTexture, Texture2DArray)
{
    auto* pEnv     = TestingEnvironment::GetInstance();
    auto* pDevice  = pEnv->GetDevice();
    auto* pContext = pEnv->GetDeviceContext();

    TestingEnvironment::ScopedReleaseResources EnvironmentAutoReset;

    for (auto f = 0; f < _countof(TestFormats); ++f)
    {
        auto Format = TestFormats[f];

        TextureDesc TexDesc;
        TexDesc.Type      = RESOURCE_DIM_TEX_2D_ARRAY;
        TexDesc.Format    = Format;
        TexDesc.Width     = 128;
        TexDesc.Height    = 128;
        TexDesc.BindFlags = BIND_SHADER_RESOURCE;
        TexDesc.MipLevels = 5;
        TexDesc.ArraySize = 8;
        TexDesc.Usage     = USAGE_DEFAULT;

        Diligent::RefCntAutoPtr<ITexture> pSrcTex, pDstTex;
        auto                              FmtAttribs = pDevice->GetTextureFormatInfo(Format);
        auto                              TexelSize  = Uint32{FmtAttribs.ComponentSize} * Uint32{FmtAttribs.NumComponents};
        std::vector<uint8_t>              DummyData(size_t{TexDesc.Width} * size_t{TexDesc.Height} * size_t{TexelSize});
        TextureData                       InitData;
        InitData.NumSubresources = TexDesc.MipLevels * TexDesc.ArraySize;
        std::vector<TextureSubResData> SubResData(InitData.NumSubresources);
        Uint32                         subres = 0;
        for (Uint32 slice = 0; slice < TexDesc.ArraySize; ++slice)
        {
            for (Uint32 mip = 0; mip < TexDesc.MipLevels; ++mip)
            {
                SubResData[subres++] = TextureSubResData{DummyData.data(), (TexDesc.Width >> mip) * TexelSize, 0};
            }
        }
        VERIFY_EXPR(subres == InitData.NumSubresources);
        InitData.pSubResources = SubResData.data();

        pDevice->CreateTexture(TexDesc, &InitData, &pSrcTex);
        pDevice->CreateTexture(TexDesc, nullptr, &pDstTex);

        CopyTextureAttribs CopyAttribs;
        CopyAttribs.pSrcTexture              = pSrcTex;
        CopyAttribs.SrcMipLevel              = 2;
        CopyAttribs.SrcSlice                 = 3;
        CopyAttribs.pDstTexture              = pDstTex;
        CopyAttribs.DstMipLevel              = 1;
        CopyAttribs.DstSlice                 = 6;
        CopyAttribs.DstX                     = 32;
        CopyAttribs.DstY                     = 16;
        CopyAttribs.DstZ                     = 0;
        CopyAttribs.SrcTextureTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
        CopyAttribs.DstTextureTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
        pContext->CopyTexture(CopyAttribs);

        Box SrcBox;
        SrcBox.MinX = 3;
        SrcBox.MaxX = 19;
        SrcBox.MinY = 1;
        SrcBox.MaxY = 32;

        CopyAttribs.DstSlice                 = 5;
        CopyAttribs.pSrcBox                  = &SrcBox;
        CopyAttribs.SrcTextureTransitionMode = RESOURCE_STATE_TRANSITION_MODE_VERIFY;
        CopyAttribs.DstTextureTransitionMode = RESOURCE_STATE_TRANSITION_MODE_VERIFY;
        pContext->CopyTexture(CopyAttribs);

        pEnv->ReleaseResources();
    }
}

TEST(CopyTexture, Texture3D)
{
    auto* pEnv     = TestingEnvironment::GetInstance();
    auto* pDevice  = pEnv->GetDevice();
    auto* pContext = pEnv->GetDeviceContext();

    TestingEnvironment::ScopedReleaseResources EnvironmentAutoReset;

    for (auto f = 0; f < _countof(TestFormats); ++f)
    {
        auto Format = TestFormats[f];

        TextureDesc TexDesc;
        TexDesc.Type      = RESOURCE_DIM_TEX_3D;
        TexDesc.Format    = Format;
        TexDesc.Width     = 64;
        TexDesc.Height    = 64;
        TexDesc.Depth     = 16;
        TexDesc.BindFlags = BIND_SHADER_RESOURCE;
        TexDesc.MipLevels = 4;
        TexDesc.Usage     = USAGE_DEFAULT;

        Diligent::RefCntAutoPtr<ITexture> pSrcTex, pDstTex;

        auto                 FmtAttribs = pDevice->GetTextureFormatInfo(Format);
        auto                 TexelSize  = Uint32{FmtAttribs.ComponentSize} * Uint32{FmtAttribs.NumComponents};
        std::vector<uint8_t> DummyData(size_t{TexDesc.Width} * size_t{TexDesc.Height} * size_t{TexDesc.Depth} * size_t{TexelSize});
        TextureData          InitData;
        InitData.NumSubresources = TexDesc.MipLevels;
        std::vector<TextureSubResData> SubResData(InitData.NumSubresources);
        for (Uint32 mip = 0; mip < TexDesc.MipLevels; ++mip)
        {
            SubResData[mip] = TextureSubResData{DummyData.data(), (TexDesc.Width >> mip) * TexelSize, (TexDesc.Width >> mip) * (TexDesc.Height >> mip) * TexelSize};
        }
        InitData.pSubResources = SubResData.data();
        pDevice->CreateTexture(TexDesc, &InitData, &pSrcTex);
        pDevice->CreateTexture(TexDesc, nullptr, &pDstTex);

        CopyTextureAttribs CopyAttribs;
        CopyAttribs.pSrcTexture              = pSrcTex;
        CopyAttribs.SrcMipLevel              = 2;
        CopyAttribs.pDstTexture              = pDstTex;
        CopyAttribs.DstMipLevel              = 1;
        CopyAttribs.DstX                     = 16;
        CopyAttribs.DstY                     = 8;
        CopyAttribs.DstZ                     = 0;
        CopyAttribs.SrcTextureTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
        CopyAttribs.DstTextureTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

        pContext->CopyTexture(CopyAttribs);

        Box SrcBox;
        SrcBox.MinX = 3;
        SrcBox.MaxX = 19;
        SrcBox.MinY = 1;
        SrcBox.MaxY = 32;

        CopyAttribs.pSrcBox     = &SrcBox;
        CopyAttribs.SrcMipLevel = 1;
        CopyAttribs.DstMipLevel = 0;
        CopyAttribs.DstX        = 32;
        CopyAttribs.DstY        = 16;
        CopyAttribs.DstZ        = 0;
        pContext->CopyTexture(CopyAttribs);

        pEnv->ReleaseResources();
    }
}

} // namespace
