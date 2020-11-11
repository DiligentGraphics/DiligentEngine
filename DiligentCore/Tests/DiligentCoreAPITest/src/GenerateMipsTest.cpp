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

TEST(GenerateMipsTest, GenerateMips)
{
    TestingEnvironment::ScopedReleaseResources AutoResetEnvironment;

    auto* pEnv     = TestingEnvironment::GetInstance();
    auto* pDevice  = pEnv->GetDevice();
    auto* pContext = pEnv->GetDeviceContext();

    TEXTURE_FORMAT TestFormats[] = //
        {
            TEX_FORMAT_RGBA8_UNORM,
            TEX_FORMAT_RGBA8_UNORM_SRGB,
            TEX_FORMAT_RGBA32_FLOAT //
        };

    for (auto f = 0; f < _countof(TestFormats); ++f)
    {
        TextureDesc TexDesc;
        TexDesc.Name      = "Mips generation test texture";
        TexDesc.Type      = RESOURCE_DIM_TEX_2D;
        TexDesc.Format    = TestFormats[f];
        TexDesc.Width     = 128 + 6;
        TexDesc.Height    = 128 + 5;
        TexDesc.BindFlags = BIND_SHADER_RESOURCE;
        TexDesc.MipLevels = 6;
        TexDesc.Usage     = USAGE_DEFAULT;
        TexDesc.MiscFlags = MISC_TEXTURE_FLAG_GENERATE_MIPS;

        std::vector<Uint8> ZeroData(TexDesc.Width * TexDesc.Height * 16);

        {
            RefCntAutoPtr<ITexture>        pTex;
            std::vector<TextureSubResData> SubresData(TexDesc.MipLevels);
            for (auto& MipLevelData : SubresData)
            {
                MipLevelData.pData  = ZeroData.data();
                MipLevelData.Stride = TexDesc.Width * 16;
            }
            TextureData InitData{SubresData.data(), TexDesc.MipLevels};

            pDevice->CreateTexture(TexDesc, &InitData, &pTex);
            ASSERT_NE(pTex, nullptr) << "Failed to create texture: " << TexDesc;

            pContext->GenerateMips(pTex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));

            TextureViewDesc ViewDesc(TEXTURE_VIEW_SHADER_RESOURCE, RESOURCE_DIM_TEX_2D_ARRAY);
            ViewDesc.MostDetailedMip = 1;
            ViewDesc.NumMipLevels    = 3;
            ViewDesc.Flags           = TEXTURE_VIEW_FLAG_ALLOW_MIP_MAP_GENERATION;
            RefCntAutoPtr<ITextureView> pTexView;
            pTex->CreateView(ViewDesc, &pTexView);
            ASSERT_NE(pTexView, nullptr) << "Failed to create SRV for texture: " << TexDesc;
            pContext->GenerateMips(pTexView);
        }

        TexDesc.Name      = "Mips generation test texture array";
        TexDesc.Type      = RESOURCE_DIM_TEX_2D_ARRAY;
        TexDesc.ArraySize = 16;
        {
            std::vector<TextureSubResData> SubresData(TexDesc.MipLevels * TexDesc.ArraySize);
            for (auto& MipLevelData : SubresData)
            {
                MipLevelData.pData  = ZeroData.data();
                MipLevelData.Stride = TexDesc.Width * 16;
            }
            TextureData InitData{SubresData.data(), TexDesc.MipLevels * TexDesc.ArraySize};

            RefCntAutoPtr<ITexture> pTex;
            pDevice->CreateTexture(TexDesc, &InitData, &pTex);
            ASSERT_NE(pTex, nullptr) << "Failed to create texture array: " << TexDesc;

            TextureViewDesc ViewDesc(TEXTURE_VIEW_SHADER_RESOURCE, RESOURCE_DIM_TEX_2D_ARRAY);
            ViewDesc.FirstArraySlice = 2;
            ViewDesc.NumArraySlices  = 5;
            ViewDesc.MostDetailedMip = 1;
            ViewDesc.NumMipLevels    = 5;
            if (pDevice->GetDeviceCaps().DevType == RENDER_DEVICE_TYPE_D3D11)
            {
                // The appears to be a bug in D3D11 Warp: the warp crashes if
                // the view addresses only a subset of mip levels
                ViewDesc.MostDetailedMip = 0;
                ViewDesc.NumMipLevels    = TexDesc.MipLevels;
            }

            ViewDesc.Flags = TEXTURE_VIEW_FLAG_ALLOW_MIP_MAP_GENERATION;
            RefCntAutoPtr<ITextureView> pTexView;
            pTex->CreateView(ViewDesc, &pTexView);
            ASSERT_NE(pTexView, nullptr) << "Failed to create SRV for texture array: " << TexDesc;
            pContext->GenerateMips(pTexView);

            pContext->GenerateMips(pTex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
        }
    }
}

} // namespace
