/*     Copyright 2015-2019 Egor Yusov
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

#include "pch.h"
#include "TestMipMapsGeneration.h"
#include "RenderDevice.h"
#include "GraphicsUtilities.h"
#include "Errors.h"

using namespace Diligent;

TestMipMapsGeneration::TestMipMapsGeneration( IRenderDevice *pDevice, IDeviceContext *pContext ) :
    UnitTestBase("Texture mipmap generation")
{
    TEXTURE_FORMAT TestFormats[] = 
    {
        TEX_FORMAT_RGBA8_UNORM,
        TEX_FORMAT_RGBA8_UNORM_SRGB,
        TEX_FORMAT_RGBA32_FLOAT
    };

    int NumFormatsTested = 0;
    for( auto f = 0; f < _countof( TestFormats ); ++f )
    {
        TextureDesc TexDesc;
        TexDesc.Type = RESOURCE_DIM_TEX_2D;
        TexDesc.Format = TestFormats[f];
        TexDesc.Width = 128 + 6;
        TexDesc.Height = 128 + 5;
        TexDesc.BindFlags = BIND_SHADER_RESOURCE;
        TexDesc.MipLevels = 6;
        TexDesc.Usage = USAGE_DEFAULT;
        TexDesc.MiscFlags = MISC_TEXTURE_FLAG_GENERATE_MIPS;

        std::vector<Uint8> NullData(TexDesc.Width * TexDesc.Height * 16);
        
        {
            RefCntAutoPtr<ITexture> pTex;
            std::vector<TextureSubResData> SubresData(TexDesc.MipLevels);
            for(auto& MipLevelData : SubresData)
            {
                MipLevelData.pData  = NullData.data();
                MipLevelData.Stride = TexDesc.Width * 16;
            }
            TextureData InitData{SubresData.data(), TexDesc.MipLevels};

            pDevice->CreateTexture(TexDesc, &InitData, &pTex);
            pContext->GenerateMips(pTex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));

            TextureViewDesc ViewDesc(TEXTURE_VIEW_SHADER_RESOURCE, RESOURCE_DIM_TEX_2D_ARRAY);
            ViewDesc.MostDetailedMip = 1;
            ViewDesc.NumMipLevels = 3;
            ViewDesc.Flags = TEXTURE_VIEW_FLAG_ALLOW_MIP_MAP_GENERATION;
            RefCntAutoPtr<ITextureView> pTexView;
            pTex->CreateView(ViewDesc, &pTexView);
            pContext->GenerateMips(pTexView);
        }

        TexDesc.Type = RESOURCE_DIM_TEX_2D_ARRAY;
        TexDesc.ArraySize = 16;
        {
            RefCntAutoPtr<ITexture> pTex;
            pDevice->CreateTexture(TexDesc, nullptr, &pTex);

            TextureViewDesc ViewDesc(TEXTURE_VIEW_SHADER_RESOURCE, RESOURCE_DIM_TEX_2D_ARRAY);
            ViewDesc.FirstArraySlice = 2;
            ViewDesc.NumArraySlices = 5;
            ViewDesc.MostDetailedMip = 1;
            ViewDesc.NumMipLevels = 5;
            ViewDesc.Flags = TEXTURE_VIEW_FLAG_ALLOW_MIP_MAP_GENERATION;
            RefCntAutoPtr<ITextureView> pTexView;
            pTex->CreateView(ViewDesc, &pTexView);
            pContext->GenerateMips(pTexView);

            pContext->GenerateMips(pTex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
        }
    }
    std::stringstream infoss;
    infoss << "Formats tested: " << NumFormatsTested;
    SetStatus(TestResult::Succeeded, infoss.str().c_str());
}
