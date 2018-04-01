/*     Copyright 2015-2018 Egor Yusov
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
#include "TestCopyTexData.h"
#include "RenderDevice.h"
#include "GraphicsUtilities.h"
#include "Errors.h"

using namespace Diligent;

TestCopyTexData::TestCopyTexData( IRenderDevice *pDevice, IDeviceContext *pContext ) :
    UnitTestBase("Texture data copy test"),
    m_pDevice(pDevice),
    m_pContext(pContext)
{
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
    int NumFormatsTested = 0;
    for( auto f = 0; f < _countof( TestFormats ); ++f )
    {
        Test2DTexture(TestFormats[f]);
        Test2DTexArray(TestFormats[f]);
        Test3DTexture(TestFormats[f]);
        ++NumFormatsTested;
    }
    std::stringstream infoss;
    infoss << "Formats tested: " << NumFormatsTested;
    SetStatus(TestResult::Succeeded, infoss.str().c_str());
}

void TestCopyTexData::Test2DTexture( TEXTURE_FORMAT Format )
{
    TextureDesc TexDesc;
    TexDesc.Type = RESOURCE_DIM_TEX_2D;
    TexDesc.Format = Format;
    TexDesc.Width = 128;
    TexDesc.Height = 128;
    TexDesc.BindFlags = BIND_SHADER_RESOURCE;
    TexDesc.MipLevels = 5;
    TexDesc.Usage = USAGE_DEFAULT;

    Diligent::RefCntAutoPtr<ITexture> pSrcTex, pDstTex;
    m_pDevice->CreateTexture( TexDesc, TextureData(), &pSrcTex );
    m_pDevice->CreateTexture( TexDesc, TextureData(), &pDstTex );

    pDstTex->CopyData(m_pContext, pSrcTex,
                      2, // Src mip
                      0, // Src slice
                      nullptr, // Box
                      1, // dst mip
                      0, // dst slice
                      32, 16, 0 // XYZ offset 
                      );

    Box SrcBox;
    SrcBox.MinX = 3;
    SrcBox.MaxX = 19;
    SrcBox.MinY = 1;
    SrcBox.MaxY = 32;
    pDstTex->CopyData(m_pContext, pSrcTex,
                      2, // Src mip
                      0, // Src slice
                      &SrcBox, // Box
                      1, // dst mip
                      0, // dst slice
                      32, 16, 0 // XYZ offset 
                      );

}

void TestCopyTexData::Test2DTexArray( TEXTURE_FORMAT Format )
{
    TextureDesc TexDesc;
    TexDesc.Type = RESOURCE_DIM_TEX_2D_ARRAY;
    TexDesc.Format = Format;
    TexDesc.Width = 128;
    TexDesc.Height = 128;
    TexDesc.BindFlags = BIND_SHADER_RESOURCE;
    TexDesc.MipLevels = 5;
    TexDesc.ArraySize = 8;
    TexDesc.Usage = USAGE_DEFAULT;

    Diligent::RefCntAutoPtr<ITexture> pSrcTex, pDstTex;
    m_pDevice->CreateTexture( TexDesc, TextureData(), &pSrcTex );
    m_pDevice->CreateTexture( TexDesc, TextureData(), &pDstTex );

    pDstTex->CopyData(m_pContext, pSrcTex,
                      2, // Src mip
                      3, // Src slice
                      nullptr, // Box
                      1, // dst mip
                      6, // dst slice
                      32, 16, 0 // XYZ offset 
                      );

    Box SrcBox;
    SrcBox.MinX = 3;
    SrcBox.MaxX = 19;
    SrcBox.MinY = 1;
    SrcBox.MaxY = 32;
    pDstTex->CopyData(m_pContext, pSrcTex,
                      2, // Src mip
                      3, // Src slice
                      &SrcBox, // Box
                      1, // dst mip
                      5, // dst slice
                      32, 16, 0 // XYZ offset 
                      );
}

void TestCopyTexData::Test3DTexture( TEXTURE_FORMAT Format )
{
    TextureDesc TexDesc;
    TexDesc.Type = RESOURCE_DIM_TEX_3D;
    TexDesc.Format = Format;
    TexDesc.Width = 64;
    TexDesc.Height = 64;
    TexDesc.Depth = 16;
    TexDesc.BindFlags = BIND_SHADER_RESOURCE;
    TexDesc.MipLevels = 4;
    TexDesc.Usage = USAGE_DEFAULT;

    Diligent::RefCntAutoPtr<ITexture> pSrcTex, pDstTex;
    m_pDevice->CreateTexture( TexDesc, TextureData(), &pSrcTex );
    m_pDevice->CreateTexture( TexDesc, TextureData(), &pDstTex );

    pDstTex->CopyData(m_pContext, pSrcTex,
                      2, // Src mip
                      0, // Src slice
                      nullptr, // Box
                      1, // dst mip
                      0, // dst slice
                      16, 8, 0 // XYZ offset 
                      );

    Box SrcBox;
    SrcBox.MinX = 3;
    SrcBox.MaxX = 19;
    SrcBox.MinY = 1;
    SrcBox.MaxY = 32;
    pDstTex->CopyData(m_pContext, pSrcTex,
                      1, // Src mip
                      0, // Src slice
                      &SrcBox, // Box
                      0, // dst mip
                      0, // dst slice
                      32, 16, 0 // XYZ offset 
                      );
}
