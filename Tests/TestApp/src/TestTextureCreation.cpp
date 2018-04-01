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
#include "TestTextureCreation.h"
#include "RenderDevice.h"
#include "GraphicsUtilities.h"
#include "Errors.h"
#include "TestCreateObjFromNativeRes.h"

#if D3D11_SUPPORTED
#include "TestCreateObjFromNativeResD3D11.h"
#endif

#if D3D12_SUPPORTED
#include "TestCreateObjFromNativeResD3D12.h"
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
#include "TestCreateObjFromNativeResGL.h"
#endif

using namespace Diligent;

class TextureCreationVerifier
{
public:
    TextureCreationVerifier( IRenderDevice *pDevice, IDeviceContext *pContext ) :
        m_pDevice(pDevice),
        m_pDeviceContext(pContext),
        m_BindFlags(0),
        m_TextureFormat(TEX_FORMAT_UNKNOWN),
        m_PixelSize(0),
        m_bTestDataUpload(False)
    {
        auto DevType = m_pDevice->GetDeviceCaps().DevType;
        switch (DevType)
        {
#if D3D11_SUPPORTED
            case DeviceType::D3D11:
                m_pTestCreateObjFromNativeRes.reset(new TestCreateObjFromNativeResD3D11(pDevice));
            break;
#endif

#if D3D12_SUPPORTED
            case DeviceType::D3D12:
                m_pTestCreateObjFromNativeRes.reset(new TestCreateObjFromNativeResD3D12(pDevice));
            break;
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
            case DeviceType::OpenGL:
            case DeviceType::OpenGLES:
                m_pTestCreateObjFromNativeRes.reset(new TestCreateObjFromNativeResGL(pDevice));
            break;
#endif

            default: UNEXPECTED("Unexpected device type");
        }
    }
    
    void Test(TEXTURE_FORMAT TextureFormat,
              Uint32 PixelSize,      
              Uint32 BindFlags,
              Bool TestDataUpload)
    {
        m_TextureFormat = TextureFormat;
        m_BindFlags = BindFlags;
        m_PixelSize = PixelSize;
        m_bTestDataUpload = TestDataUpload;

        auto PixelFormatAttribs = m_pDevice->GetTextureFormatInfoExt(TextureFormat);

        const auto &TexCaps = m_pDevice->GetDeviceCaps().TexCaps;
        // Test texture 1D / texture 1D array
        if( TexCaps.bTexture1DSupported )
        {
            if( PixelFormatAttribs.Tex1DFmt )
                CreateTestTexture( RESOURCE_DIM_TEX_1D, 1 );
        }
        else
        {
            static bool FirstTime = true;
            if( FirstTime )
            {
                LOG_WARNING_MESSAGE( "Texture 1D is not supported\n" );
                FirstTime = false;
            }
        }
            
        if( TexCaps.bTexture1DArraySupported )
        {
            if( PixelFormatAttribs.Tex1DFmt )
                CreateTestTexture( RESOURCE_DIM_TEX_1D_ARRAY, 1 );
        }
        else
        {
            static bool FirstTime = true;
            if( FirstTime )
            {
                LOG_WARNING_MESSAGE( "Texture 1D array is not supported\n" );
                FirstTime = false;
            }
        }

        // Test texture 2D / texture 2D array
        CreateTestTexture(RESOURCE_DIM_TEX_2D, 1);
        CreateTestTexture(RESOURCE_DIM_TEX_2D, 1);
        CreateTestTexture(RESOURCE_DIM_TEX_2D_ARRAY, 1);
        CreateTestTexture(RESOURCE_DIM_TEX_2D_ARRAY, 1);
        CreateTestCubemap();

        if( m_TextureFormat != TEX_FORMAT_RGB9E5_SHAREDEXP && 
            PixelFormatAttribs.ComponentType != COMPONENT_TYPE_COMPRESSED )
        {
            if( TexCaps.bTexture2DMSSupported && (BindFlags & (BIND_RENDER_TARGET|BIND_DEPTH_STENCIL)) != 0 )
            {
                if( PixelFormatAttribs.SupportsMS )
                {
                    CreateTestTexture( RESOURCE_DIM_TEX_2D, 4 );
                    CreateTestTexture( RESOURCE_DIM_TEX_2D, 4 );
                }
            }
            else
            {
                static bool FirstTime = true;
                if( FirstTime )
                {
                    LOG_WARNING_MESSAGE( "Texture 2D MS is not supported\n" );
                    FirstTime = false;
                }
            }

            if( TexCaps.bTexture2DMSArraySupported && (BindFlags & (BIND_RENDER_TARGET|BIND_DEPTH_STENCIL)) != 0 )
            {
                if( PixelFormatAttribs.SupportsMS )
                {
                    CreateTestTexture( RESOURCE_DIM_TEX_2D_ARRAY, 4 );
                    CreateTestTexture( RESOURCE_DIM_TEX_2D_ARRAY, 4 );
                }
            }
            else
            {
                static bool FirstTime = true;
                if( FirstTime )
                {
                    LOG_WARNING_MESSAGE( "Texture 2D MS Array is not supported\n" );
                    FirstTime = false;
                }
            }
        }

        // Test texture 3D
        if( PixelFormatAttribs.Tex3DFmt )
            CreateTestTexture(RESOURCE_DIM_TEX_3D, 1);
    }

private:
    void PrepareSubresourceData(TextureDesc &TexDesc,
                                TextureFormatInfo &PixelFormatAttribs,
                                std::vector< std::vector<Uint8> > &Data,
                                std::vector<TextureSubResData> &SubResources,
                                TextureData &InitData)
    {

        auto PixelSize = PixelFormatAttribs.ComponentSize * PixelFormatAttribs.NumComponents;
        assert( PixelSize == m_PixelSize );
            
        Uint32 ArrSize = (TexDesc.Type == RESOURCE_DIM_TEX_3D) ? 1 : TexDesc.ArraySize;
        InitData.NumSubresources = ArrSize * TexDesc.MipLevels;
        Data.resize(InitData.NumSubresources);
        SubResources.resize(InitData.NumSubresources);
        Uint32 SubRes = 0;
        for(Uint32 Slice = 0; Slice < ArrSize; ++Slice)
        {
            for(Uint32 Mip = 0; Mip < TexDesc.MipLevels; ++Mip)
            {
                Uint32 MipWidth  = std::max(TexDesc.Width  >> Mip, 1U);
                Uint32 MipHeight = std::max(TexDesc.Height >> Mip, 1U);
                auto &CurrSubResData = Data[SubRes];
                auto &SubResInfo = SubResources[SubRes];
                SubResInfo.Stride = (MipWidth + 128) * PixelSize;
                SubResInfo.Stride = (SubResInfo.Stride + 3) & (-4);
                auto SubresDataSize = 0;
                if(TexDesc.Type == RESOURCE_DIM_TEX_3D)
                {
                    SubResInfo.DepthStride = SubResInfo.Stride * (MipHeight + 32);
                    Uint32 MipDeth = std::max(TexDesc.Depth >> Mip, 1U);
                    SubresDataSize = SubResInfo.DepthStride * MipDeth;
                }
                else
                    SubresDataSize = SubResInfo.Stride * MipHeight;
                    
                CurrSubResData.resize(SubresDataSize);
                SubResInfo.pData = CurrSubResData.data();
                ++SubRes;
            }
        }
        InitData.pSubResources = SubResources.data();
    }
    void CreateTestTexture(RESOURCE_DIMENSION Type, Uint32 SampleCount)
    {
        TextureDesc TexDesc;
        TexDesc.Name = "TestTextureCreation";
        TexDesc.Type = Type;
        TexDesc.Width = 211;
        if( TexDesc.Type == RESOURCE_DIM_TEX_1D || TexDesc.Type == RESOURCE_DIM_TEX_1D_ARRAY )
            TexDesc.Height = 1;
        else
            TexDesc.Height = 243;

        if( TexDesc.Type == RESOURCE_DIM_TEX_3D )
            TexDesc.Depth = 16;

        if( TexDesc.Type == RESOURCE_DIM_TEX_1D_ARRAY || TexDesc.Type == RESOURCE_DIM_TEX_2D_ARRAY )
            TexDesc.ArraySize = 16;

        auto PixelFormatAttribs = m_pDevice->GetTextureFormatInfoExt(m_TextureFormat);
        if( PixelFormatAttribs.ComponentType == COMPONENT_TYPE_COMPRESSED )
        {
            TexDesc.Width  = (TexDesc.Width+3)  & (-4);
            TexDesc.Height = (TexDesc.Height+3) & (-4);
        }

        TexDesc.MipLevels = SampleCount == 1 ? 0 : 1;
        TexDesc.Format = m_TextureFormat;
        TexDesc.Usage = USAGE_DEFAULT;
        TexDesc.BindFlags = m_BindFlags;
        if( SampleCount > 1 )
            TexDesc.BindFlags &= ~BIND_UNORDERED_ACCESS;

        TexDesc.SampleCount = SampleCount;

        RefCntAutoPtr<Diligent::ITexture> pTestTex;
        m_pDevice->CreateTexture( TexDesc, TextureData(), &pTestTex );
        m_pTestCreateObjFromNativeRes->CreateTexture(pTestTex);
        TexDesc.MipLevels = pTestTex->GetDesc().MipLevels;
        if( SampleCount == 1 && m_bTestDataUpload )
        {
            std::vector< std::vector<Uint8> > Data;
            std::vector<TextureSubResData> SubResources;
            TextureData InitData;

            PrepareSubresourceData(TexDesc, PixelFormatAttribs, Data, SubResources, InitData);

            TexDesc.Name = "TestTexture2";
            pTestTex.Release();
            m_pDevice->CreateTexture( TexDesc, InitData, &pTestTex );
            m_pTestCreateObjFromNativeRes->CreateTexture(pTestTex);
        }

        const auto &DeviceCaps = m_pDevice->GetDeviceCaps();
        const auto &TextureCaps = DeviceCaps.TexCaps;
        if( TextureCaps.bTextureViewSupported )
        {
            TextureViewDesc ViewDesc;
            ViewDesc.TextureDim = TexDesc.Type;
            if( TexDesc.Type == RESOURCE_DIM_TEX_1D_ARRAY || TexDesc.Type == RESOURCE_DIM_TEX_2D_ARRAY )
            {
                ViewDesc.FirstArraySlice = 3;
                ViewDesc.NumArraySlices = (DeviceCaps.DevType == DeviceType::D3D11 || DeviceCaps.DevType == DeviceType::D3D12 || ViewDesc.ViewType == TEXTURE_VIEW_SHADER_RESOURCE) ? 4 : 1;
            }
            else if(TexDesc.Type == RESOURCE_DIM_TEX_3D)
            {
                if( DeviceCaps.DevType == DeviceType::D3D11 || DeviceCaps.DevType == DeviceType::D3D12 )
                {
                    ViewDesc.FirstDepthSlice = 3;
                    ViewDesc.NumDepthSlices = 4;
                }
                else
                {
                    // OpenGL cannot create views for separate depth slices
                    ViewDesc.FirstDepthSlice = 0;
                    ViewDesc.NumDepthSlices = 1;
                }
            }
            else
            {
                ViewDesc.FirstArraySlice = 0;
                ViewDesc.NumArraySlices = 1;
            }

            if( SampleCount > 1 )
            {
                ViewDesc.MostDetailedMip = 0;
                ViewDesc.NumMipLevels = 1;
            }
            else
            {
                ViewDesc.MostDetailedMip = 1;
                ViewDesc.NumMipLevels = 2;
            }

            if( TexDesc.BindFlags & BIND_SHADER_RESOURCE )
            {
                ViewDesc.ViewType = TEXTURE_VIEW_SHADER_RESOURCE;
                RefCntAutoPtr<ITextureView> pSRV;
                pTestTex->CreateView( ViewDesc, &pSRV );
            }

            // RTV, DSV & UAV can reference only one mip level
            ViewDesc.NumMipLevels = 1;

            if( TexDesc.BindFlags & BIND_RENDER_TARGET )
            {
                ViewDesc.ViewType = TEXTURE_VIEW_RENDER_TARGET;
                RefCntAutoPtr<ITextureView> pRTV;
                pTestTex->CreateView( ViewDesc, &pRTV );
            }

            if( TexDesc.BindFlags & BIND_DEPTH_STENCIL )
            {
                ViewDesc.ViewType = TEXTURE_VIEW_DEPTH_STENCIL;
                RefCntAutoPtr<ITextureView> pDSV;
                pTestTex->CreateView( ViewDesc, &pDSV );
            }

            if( TexDesc.BindFlags & BIND_UNORDERED_ACCESS )
            {
                ViewDesc.ViewType = TEXTURE_VIEW_UNORDERED_ACCESS;
                ViewDesc.AccessFlags = UAV_ACCESS_FLAG_READ | UAV_ACCESS_FLAG_WRITE;
                RefCntAutoPtr<ITextureView> pUAV;
                pTestTex->CreateView( ViewDesc, &pUAV );
            }
        }
        else
        {
            static bool FirstTime = true;
            if( FirstTime )
            {
                LOG_WARNING_MESSAGE("Texture views are not supported!\n");
                FirstTime = false;
            }
        }

        // It is necessary to call Flush() to force the driver to release the resources.
        // Without flushing the command buffer, the memory is not released until sometimes 
        // later causing out-of-memory error
        m_pDeviceContext->Flush();
    }
    

    void CreateTestCubemap()
    {
        TextureDesc TexDesc;
        TexDesc.Name = "Test Cube MapTextureCreation";
        TexDesc.Type = RESOURCE_DIM_TEX_CUBE;
        TexDesc.Width = 256;
        TexDesc.Height = 256;
        TexDesc.ArraySize = 6;
        TexDesc.MipLevels = 0;
        TexDesc.Format = m_TextureFormat;
        TexDesc.Usage = USAGE_DEFAULT;
        TexDesc.BindFlags = m_BindFlags;
        TexDesc.SampleCount = 1;

        RefCntAutoPtr<Diligent::ITexture> pTestCubemap, pTestCubemapArr;
        m_pDevice->CreateTexture( TexDesc, TextureData(), &pTestCubemap );
        m_pTestCreateObjFromNativeRes->CreateTexture(pTestCubemap);
        TexDesc.MipLevels = pTestCubemap->GetDesc().MipLevels;

        auto PixelFormatAttribs = m_pDevice->GetTextureFormatInfoExt(m_TextureFormat);
        if( m_bTestDataUpload )
        {
            std::vector< std::vector<Uint8> > Data;
            std::vector<TextureSubResData> SubResources;
            TextureData InitData;

            PrepareSubresourceData(TexDesc, PixelFormatAttribs, Data, SubResources, InitData);

            InitData.pSubResources = SubResources.data();
            TexDesc.Name = "TestCubemap2";
            pTestCubemap.Release();
            m_pDevice->CreateTexture( TexDesc, InitData, &pTestCubemap );
            m_pTestCreateObjFromNativeRes->CreateTexture(pTestCubemap);
        }

        const auto &DeviceCaps = m_pDevice->GetDeviceCaps();
        const auto &TextureCaps = DeviceCaps.TexCaps;
        if( TextureCaps.bTextureViewSupported )
        {
            TextureViewDesc ViewDesc;
            if( TexDesc.BindFlags & BIND_SHADER_RESOURCE )
            {
                ViewDesc.TextureDim = RESOURCE_DIM_TEX_CUBE;
                ViewDesc.ViewType = TEXTURE_VIEW_SHADER_RESOURCE;
                ViewDesc.MostDetailedMip = 1;
                ViewDesc.NumMipLevels = 6;
                {
                    RefCntAutoPtr<ITextureView> pSRV;
                    pTestCubemap->CreateView( ViewDesc, &pSRV );
                }

                ViewDesc.TextureDim = RESOURCE_DIM_TEX_2D;
                ViewDesc.FirstArraySlice = 0;
                ViewDesc.NumArraySlices = 1;
                ViewDesc.MostDetailedMip = 2;
                ViewDesc.NumMipLevels = 4;
                {
                    RefCntAutoPtr<ITextureView> pSRV;
                    pTestCubemap->CreateView( ViewDesc, &pSRV );
                }

                ViewDesc.TextureDim = RESOURCE_DIM_TEX_2D_ARRAY;
                ViewDesc.FirstArraySlice = 2;
                ViewDesc.NumArraySlices = 3;
                {
                    RefCntAutoPtr<ITextureView> pSRV;
                    pTestCubemap->CreateView( ViewDesc, &pSRV );
                }
            }

            // RTV, DSV & UAV can reference only one mip level
            ViewDesc.NumMipLevels = 1;

            if( TexDesc.BindFlags & BIND_RENDER_TARGET )
            {
                ViewDesc.ViewType = TEXTURE_VIEW_RENDER_TARGET;
                ViewDesc.TextureDim = RESOURCE_DIM_TEX_2D;
                ViewDesc.FirstArraySlice = 0;
                ViewDesc.NumArraySlices = 1;
                {
                    RefCntAutoPtr<ITextureView> pRTV;
                    pTestCubemap->CreateView( ViewDesc, &pRTV );
                }

                ViewDesc.TextureDim = RESOURCE_DIM_TEX_2D_ARRAY;
                ViewDesc.FirstArraySlice = 2;
                ViewDesc.NumArraySlices = 3;
                {
                    RefCntAutoPtr<ITextureView> pRTV;
                    pTestCubemap->CreateView( ViewDesc, &pRTV );
                }
            }

            if( TexDesc.BindFlags & BIND_DEPTH_STENCIL )
            {
                ViewDesc.ViewType = TEXTURE_VIEW_DEPTH_STENCIL;
                ViewDesc.TextureDim = RESOURCE_DIM_TEX_2D;
                ViewDesc.NumArraySlices = 1;
                ViewDesc.FirstArraySlice = 0;
                ViewDesc.MostDetailedMip = 2;
                {
                    RefCntAutoPtr<ITextureView> pDSV;
                    pTestCubemap->CreateView( ViewDesc, &pDSV );
                }

                ViewDesc.TextureDim = RESOURCE_DIM_TEX_2D_ARRAY;
                ViewDesc.NumArraySlices = 3;
                ViewDesc.FirstArraySlice = 2;
                {
                    RefCntAutoPtr<ITextureView> pDSV;
                    pTestCubemap->CreateView( ViewDesc, &pDSV );
                }
            }

            if( TexDesc.BindFlags & BIND_UNORDERED_ACCESS )
            {
                ViewDesc.ViewType = TEXTURE_VIEW_UNORDERED_ACCESS;
                ViewDesc.AccessFlags = UAV_ACCESS_FLAG_READ | UAV_ACCESS_FLAG_WRITE;
                ViewDesc.TextureDim = RESOURCE_DIM_TEX_2D;
                ViewDesc.NumArraySlices = 1;
                ViewDesc.FirstArraySlice = 0;
                {
                    RefCntAutoPtr<ITextureView> pUAV;
                    pTestCubemap->CreateView( ViewDesc, &pUAV );
                }

                ViewDesc.TextureDim = RESOURCE_DIM_TEX_2D_ARRAY;
                if( DeviceCaps.DevType == DeviceType::OpenGL || DeviceCaps.DevType == DeviceType::OpenGLES )
                {
                    ViewDesc.NumArraySlices = 1;
                    ViewDesc.FirstArraySlice = 2;
                }
                else
                {
                    ViewDesc.NumArraySlices = 3;
                    ViewDesc.FirstArraySlice = 2;
                }

                {
                    RefCntAutoPtr<ITextureView> pUAV;
                    pTestCubemap->CreateView( ViewDesc, &pUAV );
                }
            }
        }

        if(DeviceCaps.TexCaps.bCubemapArraysSupported)
        {
            TexDesc.ArraySize = 24;
            TexDesc.Type = RESOURCE_DIM_TEX_CUBE_ARRAY;

            std::vector< std::vector<Uint8> > Data;
            std::vector<TextureSubResData> SubResources;
            TextureData InitData;

            if( m_bTestDataUpload )
            {
                PrepareSubresourceData(TexDesc, PixelFormatAttribs, Data, SubResources, InitData);
            }

            TexDesc.Name = "TestCubemapArray";

            m_pDevice->CreateTexture( TexDesc, InitData, &pTestCubemapArr );
            m_pTestCreateObjFromNativeRes->CreateTexture(pTestCubemapArr);
        }

        // It is necessary to call Flush() to force the driver to release the resources.
        // Without flushing the command buffer, the memory is not released until sometimes 
        // later causing out-of-memory error
        m_pDeviceContext->Flush();
    }
    IRenderDevice *m_pDevice;
    IDeviceContext *m_pDeviceContext;
    Uint32 m_BindFlags;
    TEXTURE_FORMAT m_TextureFormat;
    Uint32 m_PixelSize;
    Bool m_bTestDataUpload;
    std::unique_ptr<TestCreateObjFromNativeRes> m_pTestCreateObjFromNativeRes;
};

TestTextureCreation::TestTextureCreation( IRenderDevice *pDevice, IDeviceContext *pContext ) :
    UnitTestBase("Texture creation test"),
    m_pDevice(pDevice)
{
    TestTextureFormatAttribs();

    TextureCreationVerifier Verifier(pDevice, pContext);
    const Uint32 BindSRU = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET | BIND_UNORDERED_ACCESS;
    const Uint32 BindSR = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET;
    //const Uint32 BindSD = BIND_SHADER_RESOURCE | BIND_DEPTH_STENCIL;
    const Uint32 BindSU = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
    const Uint32 BindD = BIND_DEPTH_STENCIL;
    const Uint32 BindS = BIND_SHADER_RESOURCE;

    struct TextureTestAttribs
    {
        TEXTURE_FORMAT Fmt;
        Uint32 PixelSize;
        Uint32 BindFlags;
        Bool TestDataUpload;
        const char* Name;
    };

    const TextureTestAttribs TestAttribs[] =
    {
        {TEX_FORMAT_RGBA32_FLOAT,   16, BindSRU, true, "RGBA32_FLOAT"},
        {TEX_FORMAT_RGBA32_UINT,    16, BindSRU, true, "RGBA32_UINT"},
        {TEX_FORMAT_RGBA32_SINT,    16, BindSRU, true, "RGBA32_SINT"},
        //{TEX_FORMAT_RGB32_TYPELESS,          , 5, "RGB32_TYPELESS"},

        // These formats are ill-supported
        //{TEX_FORMAT_RGB32_FLOAT,    12, BindS, true, "RGB32_FLOAT"},
        //{TEX_FORMAT_RGB32_UINT,     12, BindS, true, "RGB32_UINT"},
        //{TEX_FORMAT_RGB32_SINT,     12, BindS, true, "RGB32_SINT"},

      //{TEX_FORMAT_RGBA16_TYPELESS,   8, BindSRU, true, "RGBA16_TYPELESS"},
        {TEX_FORMAT_RGBA16_FLOAT,      8, BindSRU, true, "RGBA16_FLOAT"},
        {TEX_FORMAT_RGBA16_UNORM,      8, BindSRU, true, "RGBA16_UNORM"},
        {TEX_FORMAT_RGBA16_UINT,       8, BindSRU, true, "RGBA16_UINT"},
        {TEX_FORMAT_RGBA16_SNORM,      8, BindSU, true, "RGBA16_SNORM"},
        {TEX_FORMAT_RGBA16_SINT,       8, BindSRU, true, "RGBA16_SINT"},
                                       
      //{TEX_FORMAT_RG32_TYPELESS,     8, BindSRU, true, "RG32_TYPELESS"},
        {TEX_FORMAT_RG32_FLOAT,        8, BindSRU, true, "RG32_FLOAT"},
        {TEX_FORMAT_RG32_UINT,         8, BindSRU, true, "RG32_UINT"},
        {TEX_FORMAT_RG32_SINT,         8, BindSRU, true, "RG32_SINT"},

      //{TEX_FORMAT_R32G8X24_TYPELESS,        8, BindD, false, "R32G8X24_TYPELESS"},
        {TEX_FORMAT_D32_FLOAT_S8X24_UINT,     8, BindD, false, "D32_FLOAT_S8X24_UINT"},
      //{TEX_FORMAT_R32_FLOAT_X8X24_TYPELESS, 8, BindD, false, "R32_FLOAT_X8X24_TYPELESS"},
      //{TEX_FORMAT_X32_TYPELESS_G8X24_UINT,  8, BindD, false, "X32_TYPELESS_G8X24_UINT"},

      //{TEX_FORMAT_RGB10A2_TYPELESS,        4, BindSRU, true, "RGB10A2_TYPELESS"},
        {TEX_FORMAT_RGB10A2_UNORM,           4, BindSRU, true, "RGB10A2_UNORM"},
        {TEX_FORMAT_RGB10A2_UINT,            4, BindSRU, true, "RGB10A2_UINT"},
        {TEX_FORMAT_R11G11B10_FLOAT,         4, BindSRU, false,"R11G11B10_FLOAT"},

      //{TEX_FORMAT_RGBA8_TYPELESS,          4, BindSRU, true, "RGBA8_TYPELESS"},
        {TEX_FORMAT_RGBA8_UNORM,             4, BindSRU, true, "RGBA8_UNORM"},
        {TEX_FORMAT_RGBA8_UNORM_SRGB,        4, BindSR,  true, "RGBA8_UNORM_SRGB"},
        {TEX_FORMAT_RGBA8_UINT,              4, BindSRU, true, "RGBA8_UINT"},
        {TEX_FORMAT_RGBA8_SNORM,             4, BindSU, true, "RGBA8_SNORM"},
        {TEX_FORMAT_RGBA8_SINT,              4, BindSRU, true, "RGBA8_SINT"},

      //{TEX_FORMAT_RG16_TYPELESS,           4, BindSRU, true, "RG16_TYPELESS"},
        {TEX_FORMAT_RG16_FLOAT,              4, BindSRU, true, "RG16_FLOAT"},
        {TEX_FORMAT_RG16_UNORM,              4, BindSRU, true, "RG16_UNORM"},
        {TEX_FORMAT_RG16_UINT,               4, BindSRU, true, "RG16_UINT"},
        {TEX_FORMAT_RG16_SNORM,              4, BindSU, true, "RG16_SNORM"},
        {TEX_FORMAT_RG16_SINT,               4, BindSRU, true, "RG16_SINT"},

      //{TEX_FORMAT_R32_TYPELESS,            4, BindSRU, true, "R32_TYPELESS"},
        {TEX_FORMAT_D32_FLOAT,               4, BindD,   true, "D32_FLOAT"},
        {TEX_FORMAT_R32_FLOAT,               4, BindSRU, true, "R32_FLOAT"},
        {TEX_FORMAT_R32_UINT,                4, BindSRU, true, "R32_UINT"},
        {TEX_FORMAT_R32_SINT,                4, BindSRU, true, "R32_SINT"},

      //{TEX_FORMAT_R24G8_TYPELESS,          4, BindD, true, "R24G8_TYPELESS"},
        {TEX_FORMAT_D24_UNORM_S8_UINT,       4, BindD, true, "D24_UNORM_S8_UINT"},
      //{TEX_FORMAT_R24_UNORM_X8_TYPELESS,   4, BindD, true, "R24_UNORM_X8_TYPELESS"},
      //{TEX_FORMAT_X24_TYPELESS_G8_UINT,    4, BindD, true, "X24_TYPELESS_G8_UINT"},
                                                       
      //{TEX_FORMAT_RG8_TYPELESS,            2, BindSRU, true, "RG8_TYPELESS"},
        {TEX_FORMAT_RG8_UNORM,               2, BindSRU, true, "RG8_UNORM"},
        {TEX_FORMAT_RG8_UINT,                2, BindSRU, true, "RG8_UINT"},
        {TEX_FORMAT_RG8_SNORM,               2, BindSU, true, "RG8_SNORM"},
        {TEX_FORMAT_RG8_SINT,                2, BindSRU, true, "RG8_SINT"},

      //{TEX_FORMAT_R16_TYPELESS,            2, BindSRU, true, "R16_TYPELESS"},
        {TEX_FORMAT_R16_FLOAT,               2, BindSRU, true, "R16_FLOAT"},
        {TEX_FORMAT_D16_UNORM,               2, BindD,   true, "D16_UNORM"},
        {TEX_FORMAT_R16_UNORM,               2, BindSRU, true, "R16_UNORM"},
        {TEX_FORMAT_R16_UINT,                2, BindSRU, true, "R16_UINT"},
        {TEX_FORMAT_R16_SNORM,               2, BindSU, true, "R16_SNORM"},
        {TEX_FORMAT_R16_SINT,                2, BindSRU, true, "R16_SINT"},

      //{TEX_FORMAT_R8_TYPELESS,             1, BindSRU, true, "R8_TYPELESS"},
        {TEX_FORMAT_R8_UNORM,                1, BindSRU, true, "R8_UNORM"},
        {TEX_FORMAT_R8_UINT,                 1, BindSRU, true, "R8_UINT"},
        {TEX_FORMAT_R8_SNORM,                1, BindSU, true, "R8_SNORM"},
        {TEX_FORMAT_R8_SINT,                 1, BindSRU, true, "R8_SINT"},
      //{TEX_FORMAT_A8_UNORM,                1, BindSRU, true, "A8_UNORM"},
      //{TEX_FORMAT_R1_UNORM,                1, BindSRU, true, "R1_UNORM"},

        {TEX_FORMAT_RGB9E5_SHAREDEXP,        4, BindS,   false, "RGB9E5_SHAREDEXP"},
      //{TEX_FORMAT_RG8_B8G8_UNORM,          4, BindSRU, false, "RG8_B8G8_UNORM"},
      //{TEX_FORMAT_G8R8_G8B8_UNORM,         4, BindSRU, false, "G8R8_G8B8_UNORM"},

      //{TEX_FORMAT_BC1_TYPELESS,            16, BindS, false, "BC1_TYPELESS"},
        {TEX_FORMAT_BC1_UNORM,               16, BindS, false, "BC1_UNORM"},
        {TEX_FORMAT_BC1_UNORM_SRGB,          16, BindS, false, "BC1_UNORM_SRGB"},
      //{TEX_FORMAT_BC2_TYPELESS,            16, BindS, false, "BC2_TYPELESS"},
        {TEX_FORMAT_BC2_UNORM,               16, BindS, false, "BC2_UNORM"},
        {TEX_FORMAT_BC2_UNORM_SRGB,          16, BindS, false, "BC2_UNORM_SRGB"},
      //{TEX_FORMAT_BC3_TYPELESS,            16, BindS, false, "BC3_TYPELESS"},
        {TEX_FORMAT_BC3_UNORM,               16, BindS, false, "BC3_UNORM"},
        {TEX_FORMAT_BC3_UNORM_SRGB,          16, BindS, false, "BC3_UNORM_SRGB"},
      //{TEX_FORMAT_BC4_TYPELESS,            16, BindS, false, "BC4_TYPELESS"},
        {TEX_FORMAT_BC4_UNORM,               16, BindS, false, "BC4_UNORM"},
        {TEX_FORMAT_BC4_SNORM,               16, BindS, false, "BC4_SNORM"},
      //{TEX_FORMAT_BC5_TYPELESS,            16, BindS, false, "BC5_TYPELESS"},
        {TEX_FORMAT_BC5_UNORM,               16, BindS, false, "BC5_UNORM"},
        {TEX_FORMAT_BC5_SNORM,               16, BindS, false, "BC5_SNORM"},

      //{TEX_FORMAT_B5G6R5_UNORM,            2, BindSRU, true, "B5G6R5_UNORM"},
      //{TEX_FORMAT_B5G5R5A1_UNORM,          2, BindSRU, true, "B5G5R5A1_UNORM"},
      //{TEX_FORMAT_BGRA8_UNORM,             4, BindSRU, true, "BGRA8_UNORM"},
      //{TEX_FORMAT_BGRX8_UNORM,             4, BindSRU, true, "BGRX8_UNORM"},
      //{TEX_FORMAT_R10G10B10_XR_BIAS_A2_UNORM, 4, BindS, false, "R10G10B10_XR_BIAS_A2_UNORM"},

      //{TEX_FORMAT_BGRA8_TYPELESS,          4, BindSRU, true, "BGRA8_TYPELESS"},
      //{TEX_FORMAT_BGRA8_UNORM_SRGB,        4, BindSR,  true, "BGRA8_UNORM_SRGB",},
      //{TEX_FORMAT_BGRX8_TYPELESS,          4, BindSRU, true, "BGRX8_TYPELESS"},
      //{TEX_FORMAT_BGRX8_UNORM_SRGB,        4, BindSR,  true, "BGRX8_UNORM_SRGB",},

      //{TEX_FORMAT_BC6H_TYPELESS,           16, BindS, false, "BC6H_TYPELESS"},
        {TEX_FORMAT_BC6H_UF16,               16, BindS, false, "BC6H_UF16"},
        {TEX_FORMAT_BC6H_SF16,               16, BindS, false, "BC6H_SF16"},
      //{TEX_FORMAT_BC7_TYPELESS,            16, BindS, false, "BC7_TYPELESS"},
        {TEX_FORMAT_BC7_UNORM,               16, BindS, false, "BC7_UNORM"},
        {TEX_FORMAT_BC7_UNORM_SRGB,          16, BindS, false, "BC7_UNORM_SRGB"},
    };
    for(int Test = 0; Test < _countof(TestAttribs); ++Test)
    {
        const auto &CurrAttrs = TestAttribs[Test];
        const auto PixelFormatAttribs = pDevice->GetTextureFormatInfoExt(CurrAttrs.Fmt);
        if( !PixelFormatAttribs.Supported )
        {
            LOG_WARNING_MESSAGE( "Texture format ", CurrAttrs.Name, " is not supported!\n" );
            continue;
        }
        ++m_NumFormatsTested;
        assert(CurrAttrs.PixelSize == PixelFormatAttribs.ComponentSize * PixelFormatAttribs.NumComponents || 
               PixelFormatAttribs.ComponentType == COMPONENT_TYPE_COMPRESSED);
        Verifier.Test(CurrAttrs.Fmt, CurrAttrs.PixelSize, CurrAttrs.BindFlags, CurrAttrs.TestDataUpload);
    }
    std::stringstream infoss;
    infoss << "Formats tested: " << m_NumFormatsTested;
    SetStatus(TestResult::Succeeded, infoss.str().c_str());
}

void TestTextureCreation::CheckFormatSize(TEXTURE_FORMAT *begin, TEXTURE_FORMAT *end, Uint32 RefSize)
{
    for(auto fmt = begin; fmt != end; ++fmt)
    {
        auto FmtAttrs = m_pDevice->GetTextureFormatInfo(*fmt);
        assert(FmtAttrs.ComponentSize * FmtAttrs.NumComponents == RefSize);
    }
}

void TestTextureCreation::CheckNumComponents(TEXTURE_FORMAT *begin, TEXTURE_FORMAT *end, Uint32 RefComponents)
{
    for(auto fmt = begin; fmt != end; ++fmt)
    {
        auto FmtAttrs = m_pDevice->GetTextureFormatInfo(*fmt);
        assert(FmtAttrs.NumComponents == RefComponents);
    }
}

void TestTextureCreation::CheckComponentType(TEXTURE_FORMAT *begin, TEXTURE_FORMAT *end, COMPONENT_TYPE RefType)
{
    for(auto fmt = begin; fmt != end; ++fmt)
    {
        auto FmtAttrs = m_pDevice->GetTextureFormatInfo(*fmt);
        assert(FmtAttrs.ComponentType == RefType);
    }
}

void TestTextureCreation::TestTextureFormatAttribs()
{
    TEXTURE_FORMAT _16ByteFormats[] = 
    { 
        TEX_FORMAT_RGBA32_TYPELESS, TEX_FORMAT_RGBA32_FLOAT, TEX_FORMAT_RGBA32_UINT, TEX_FORMAT_RGBA32_SINT 
    };
    CheckFormatSize(std::begin(_16ByteFormats), std::end(_16ByteFormats), 16);

    TEXTURE_FORMAT _12ByteFormats[] = 
    { 
        TEX_FORMAT_RGB32_TYPELESS, TEX_FORMAT_RGB32_FLOAT, TEX_FORMAT_RGB32_UINT, TEX_FORMAT_RGB32_SINT 
    };
    CheckFormatSize(std::begin(_12ByteFormats), std::end(_12ByteFormats), 12);

    TEXTURE_FORMAT _8ByteFormats[] =
    { 
        TEX_FORMAT_RGBA16_TYPELESS, TEX_FORMAT_RGBA16_FLOAT, TEX_FORMAT_RGBA16_UNORM, TEX_FORMAT_RGBA16_UINT, TEX_FORMAT_RGBA16_SNORM, TEX_FORMAT_RGBA16_SINT, 
        TEX_FORMAT_RG32_TYPELESS, TEX_FORMAT_RG32_FLOAT, TEX_FORMAT_RG32_UINT, TEX_FORMAT_RG32_SINT, 
        TEX_FORMAT_R32G8X24_TYPELESS, TEX_FORMAT_D32_FLOAT_S8X24_UINT, TEX_FORMAT_R32_FLOAT_X8X24_TYPELESS, TEX_FORMAT_X32_TYPELESS_G8X24_UINT
    };
    CheckFormatSize(std::begin(_8ByteFormats), std::end(_8ByteFormats), 8);

    TEXTURE_FORMAT _4ByteFormats[] = 
    { 
        TEX_FORMAT_RGB10A2_TYPELESS, TEX_FORMAT_RGB10A2_UNORM, TEX_FORMAT_RGB10A2_UINT, TEX_FORMAT_R11G11B10_FLOAT, 
        TEX_FORMAT_RGBA8_TYPELESS, TEX_FORMAT_RGBA8_UNORM, TEX_FORMAT_RGBA8_UNORM_SRGB, TEX_FORMAT_RGBA8_UINT, TEX_FORMAT_RGBA8_SNORM, TEX_FORMAT_RGBA8_SINT,
        TEX_FORMAT_RG16_TYPELESS, TEX_FORMAT_RG16_FLOAT, TEX_FORMAT_RG16_UNORM, TEX_FORMAT_RG16_UINT, TEX_FORMAT_RG16_SNORM, TEX_FORMAT_RG16_SINT,
        TEX_FORMAT_R32_TYPELESS, TEX_FORMAT_D32_FLOAT, TEX_FORMAT_R32_FLOAT, TEX_FORMAT_R32_UINT, TEX_FORMAT_R32_SINT, 
        TEX_FORMAT_R24G8_TYPELESS, TEX_FORMAT_D24_UNORM_S8_UINT, TEX_FORMAT_R24_UNORM_X8_TYPELESS, TEX_FORMAT_X24_TYPELESS_G8_UINT,
        TEX_FORMAT_RGB9E5_SHAREDEXP, TEX_FORMAT_RG8_B8G8_UNORM, TEX_FORMAT_G8R8_G8B8_UNORM, 
        TEX_FORMAT_BGRA8_UNORM, TEX_FORMAT_BGRX8_UNORM, TEX_FORMAT_R10G10B10_XR_BIAS_A2_UNORM, 
        TEX_FORMAT_BGRA8_TYPELESS, TEX_FORMAT_BGRA8_UNORM_SRGB, TEX_FORMAT_BGRX8_TYPELESS, TEX_FORMAT_BGRX8_UNORM_SRGB
    };
    CheckFormatSize(std::begin(_4ByteFormats), std::end(_4ByteFormats), 4);

    TEXTURE_FORMAT _2ByteFormats[] = 
    { 
        TEX_FORMAT_RG8_TYPELESS, TEX_FORMAT_RG8_UNORM, TEX_FORMAT_RG8_UINT, TEX_FORMAT_RG8_SNORM, TEX_FORMAT_RG8_SINT, 
        TEX_FORMAT_R16_TYPELESS, TEX_FORMAT_R16_FLOAT, TEX_FORMAT_D16_UNORM, TEX_FORMAT_R16_UNORM, TEX_FORMAT_R16_UINT, TEX_FORMAT_R16_SNORM, TEX_FORMAT_R16_SINT,
        TEX_FORMAT_B5G6R5_UNORM, TEX_FORMAT_B5G5R5A1_UNORM
    };
    CheckFormatSize(std::begin(_2ByteFormats), std::end(_2ByteFormats), 2);

    TEXTURE_FORMAT _1ByteFormats[] = 
    { 
        TEX_FORMAT_R8_TYPELESS, TEX_FORMAT_R8_UNORM, TEX_FORMAT_R8_UINT, TEX_FORMAT_R8_SNORM, TEX_FORMAT_R8_SINT, TEX_FORMAT_A8_UNORM, //TEX_FORMAT_R1_UNORM
    };
    CheckFormatSize(std::begin(_1ByteFormats), std::end(_1ByteFormats), 1);

    TEXTURE_FORMAT _4ComponentFormats[] = 
    {
        TEX_FORMAT_RGBA32_TYPELESS, TEX_FORMAT_RGBA32_FLOAT, TEX_FORMAT_RGBA32_UINT, TEX_FORMAT_RGBA32_SINT,
        TEX_FORMAT_RGBA16_TYPELESS, TEX_FORMAT_RGBA16_FLOAT, TEX_FORMAT_RGBA16_UNORM, TEX_FORMAT_RGBA16_UINT, TEX_FORMAT_RGBA16_SNORM, TEX_FORMAT_RGBA16_SINT,
        TEX_FORMAT_RGBA8_TYPELESS, TEX_FORMAT_RGBA8_UNORM, TEX_FORMAT_RGBA8_UNORM_SRGB, TEX_FORMAT_RGBA8_UINT, TEX_FORMAT_RGBA8_SNORM, TEX_FORMAT_RGBA8_SINT,
        TEX_FORMAT_RG8_B8G8_UNORM, TEX_FORMAT_G8R8_G8B8_UNORM,
        TEX_FORMAT_BGRA8_UNORM, TEX_FORMAT_BGRX8_UNORM, TEX_FORMAT_BGRA8_TYPELESS, TEX_FORMAT_BGRA8_UNORM_SRGB, TEX_FORMAT_BGRX8_TYPELESS, TEX_FORMAT_BGRX8_UNORM_SRGB
    };
    CheckNumComponents(std::begin(_4ComponentFormats), std::end(_4ComponentFormats), 4);

    TEXTURE_FORMAT _3ComponentFormats[] = 
    {
        TEX_FORMAT_RGB32_TYPELESS, TEX_FORMAT_RGB32_FLOAT, TEX_FORMAT_RGB32_UINT, TEX_FORMAT_RGB32_SINT,

    };
    CheckNumComponents(std::begin(_3ComponentFormats), std::end(_3ComponentFormats), 3);

    TEXTURE_FORMAT _2ComponentFormats[] = 
    {
        TEX_FORMAT_RG32_TYPELESS, TEX_FORMAT_RG32_FLOAT, TEX_FORMAT_RG32_UINT, TEX_FORMAT_RG32_SINT, 
        TEX_FORMAT_R32G8X24_TYPELESS, TEX_FORMAT_D32_FLOAT_S8X24_UINT, TEX_FORMAT_R32_FLOAT_X8X24_TYPELESS, TEX_FORMAT_X32_TYPELESS_G8X24_UINT,
        TEX_FORMAT_RG16_TYPELESS, TEX_FORMAT_RG16_FLOAT, TEX_FORMAT_RG16_UNORM, TEX_FORMAT_RG16_UINT, TEX_FORMAT_RG16_SNORM, TEX_FORMAT_RG16_SINT,
        TEX_FORMAT_RG8_TYPELESS, TEX_FORMAT_RG8_UNORM, TEX_FORMAT_RG8_UINT, TEX_FORMAT_RG8_SNORM, TEX_FORMAT_RG8_SINT
    };
    CheckNumComponents(std::begin(_2ComponentFormats), std::end(_2ComponentFormats), 2);

    TEXTURE_FORMAT _1ComponentFormats[] = 
    {
        TEX_FORMAT_RGB10A2_TYPELESS, TEX_FORMAT_RGB10A2_UNORM, TEX_FORMAT_RGB10A2_UINT, TEX_FORMAT_R11G11B10_FLOAT,
        TEX_FORMAT_R32_TYPELESS, TEX_FORMAT_D32_FLOAT, TEX_FORMAT_R32_FLOAT, TEX_FORMAT_R32_UINT, TEX_FORMAT_R32_SINT, 
        TEX_FORMAT_R24G8_TYPELESS, TEX_FORMAT_D24_UNORM_S8_UINT, TEX_FORMAT_R24_UNORM_X8_TYPELESS, TEX_FORMAT_X24_TYPELESS_G8_UINT,
        TEX_FORMAT_R16_TYPELESS, TEX_FORMAT_R16_FLOAT, TEX_FORMAT_D16_UNORM, TEX_FORMAT_R16_UNORM, TEX_FORMAT_R16_UINT, TEX_FORMAT_R16_SNORM, TEX_FORMAT_R16_SINT,
        TEX_FORMAT_R8_TYPELESS, TEX_FORMAT_R8_UNORM, TEX_FORMAT_R8_UINT, TEX_FORMAT_R8_SNORM, TEX_FORMAT_R8_SINT, TEX_FORMAT_A8_UNORM, //TEX_FORMAT_R1_UNORM
        TEX_FORMAT_RGB9E5_SHAREDEXP,
        TEX_FORMAT_R10G10B10_XR_BIAS_A2_UNORM, TEX_FORMAT_B5G6R5_UNORM, TEX_FORMAT_B5G5R5A1_UNORM
    };
    CheckNumComponents(std::begin(_1ComponentFormats), std::end(_1ComponentFormats), 1);

    
    TEXTURE_FORMAT FloatFormats[] = 
    {
        TEX_FORMAT_RGBA32_FLOAT,
        TEX_FORMAT_RGB32_FLOAT,
        TEX_FORMAT_RGBA16_FLOAT,
        TEX_FORMAT_RG32_FLOAT,
        TEX_FORMAT_RG16_FLOAT,
        TEX_FORMAT_R32_FLOAT,
        TEX_FORMAT_R16_FLOAT
    };
    CheckComponentType(std::begin(FloatFormats), std::end(FloatFormats), COMPONENT_TYPE_FLOAT);

    TEXTURE_FORMAT SintFormats[] = 
    {
        TEX_FORMAT_RGBA32_SINT,
        TEX_FORMAT_RGB32_SINT,
        TEX_FORMAT_RGBA16_SINT,
        TEX_FORMAT_RG32_SINT,
        TEX_FORMAT_RGBA8_SINT,
        TEX_FORMAT_RG16_SINT,
        TEX_FORMAT_R32_SINT,
        TEX_FORMAT_RG8_SINT,
        TEX_FORMAT_R16_SINT,
        TEX_FORMAT_R8_SINT
    };
    CheckComponentType(std::begin(SintFormats), std::end(SintFormats), COMPONENT_TYPE_SINT);

    TEXTURE_FORMAT UintFormats[] = 
    {
        TEX_FORMAT_RGBA32_UINT,
        TEX_FORMAT_RGB32_UINT,
        TEX_FORMAT_RGBA16_UINT,
        TEX_FORMAT_RG32_UINT,
        TEX_FORMAT_RGBA8_UINT,
        TEX_FORMAT_RG16_UINT,
        TEX_FORMAT_R32_UINT,
        TEX_FORMAT_RG8_UINT,
        TEX_FORMAT_R16_UINT,
        TEX_FORMAT_R8_UINT
    };
    CheckComponentType(std::begin(UintFormats), std::end(UintFormats), COMPONENT_TYPE_UINT);

    TEXTURE_FORMAT UnormFormats[] = 
    {
        TEX_FORMAT_RGBA16_UNORM,
        TEX_FORMAT_RGBA8_UNORM,
        TEX_FORMAT_RG16_UNORM,
        TEX_FORMAT_RG8_UNORM,
        TEX_FORMAT_R16_UNORM,
        TEX_FORMAT_R8_UNORM,
        TEX_FORMAT_A8_UNORM,
        TEX_FORMAT_R1_UNORM,
        TEX_FORMAT_RG8_B8G8_UNORM,
        TEX_FORMAT_G8R8_G8B8_UNORM,
        TEX_FORMAT_BGRA8_UNORM,
        TEX_FORMAT_BGRX8_UNORM
    };
    CheckComponentType(std::begin(UnormFormats), std::end(UnormFormats), COMPONENT_TYPE_UNORM);

    TEXTURE_FORMAT UnormSRGBFormats[] = 
    {
        TEX_FORMAT_RGBA8_UNORM_SRGB,
        TEX_FORMAT_BGRA8_UNORM_SRGB,
        TEX_FORMAT_BGRX8_UNORM_SRGB
    };
    CheckComponentType(std::begin(UnormSRGBFormats), std::end(UnormSRGBFormats), COMPONENT_TYPE_UNORM_SRGB);

    TEXTURE_FORMAT SnormFormats[] = 
    {
        TEX_FORMAT_RGBA16_SNORM,
        TEX_FORMAT_RGBA8_SNORM,
        TEX_FORMAT_RG16_SNORM,
        TEX_FORMAT_RG8_SNORM,
        TEX_FORMAT_R16_SNORM,
        TEX_FORMAT_R8_SNORM
    };
    CheckComponentType(std::begin(SnormFormats), std::end(SnormFormats), COMPONENT_TYPE_SNORM);

    TEXTURE_FORMAT UndefinedFormats[] = 
    {
        TEX_FORMAT_RGBA32_TYPELESS,
        TEX_FORMAT_RGB32_TYPELESS,
        TEX_FORMAT_RGBA16_TYPELESS,
        TEX_FORMAT_RG32_TYPELESS,
        TEX_FORMAT_RGBA8_TYPELESS,
        TEX_FORMAT_RG16_TYPELESS,
        TEX_FORMAT_R32_TYPELESS,
        TEX_FORMAT_RG8_TYPELESS,
        TEX_FORMAT_R16_TYPELESS,
        TEX_FORMAT_R8_TYPELESS,
        TEX_FORMAT_BGRA8_TYPELESS,
        TEX_FORMAT_BGRX8_TYPELESS
    };
    CheckComponentType(std::begin(UndefinedFormats), std::end(UndefinedFormats), COMPONENT_TYPE_UNDEFINED);

    TEXTURE_FORMAT DepthFormats[] = 
    {
        TEX_FORMAT_D32_FLOAT,
        TEX_FORMAT_D16_UNORM
    }; 
    CheckComponentType(std::begin(DepthFormats), std::end(DepthFormats), COMPONENT_TYPE_DEPTH);

    TEXTURE_FORMAT DepthStencilFormats[] = 
    {
        TEX_FORMAT_R32G8X24_TYPELESS,
        TEX_FORMAT_D32_FLOAT_S8X24_UINT,
        TEX_FORMAT_R32_FLOAT_X8X24_TYPELESS,
        TEX_FORMAT_X32_TYPELESS_G8X24_UINT,
        TEX_FORMAT_R24G8_TYPELESS,
        TEX_FORMAT_D24_UNORM_S8_UINT,
        TEX_FORMAT_R24_UNORM_X8_TYPELESS,
        TEX_FORMAT_X24_TYPELESS_G8_UINT
    };
    CheckComponentType(std::begin(DepthStencilFormats), std::end(DepthStencilFormats), COMPONENT_TYPE_DEPTH_STENCIL);

    TEXTURE_FORMAT CompoundFormats[] = 
    {
        TEX_FORMAT_RGB10A2_TYPELESS,
        TEX_FORMAT_RGB10A2_UNORM,
        TEX_FORMAT_RGB10A2_UINT,
        TEX_FORMAT_RGB9E5_SHAREDEXP,
        TEX_FORMAT_R11G11B10_FLOAT,
        TEX_FORMAT_R10G10B10_XR_BIAS_A2_UNORM,
        TEX_FORMAT_B5G6R5_UNORM,
        TEX_FORMAT_B5G5R5A1_UNORM
    };
    CheckComponentType(std::begin(CompoundFormats), std::end(CompoundFormats), COMPONENT_TYPE_COMPOUND);


    TEXTURE_FORMAT CompressedFormats[] = 
    {
        TEX_FORMAT_BC1_TYPELESS, TEX_FORMAT_BC1_UNORM, TEX_FORMAT_BC1_UNORM_SRGB,
        TEX_FORMAT_BC2_TYPELESS, TEX_FORMAT_BC2_UNORM, TEX_FORMAT_BC2_UNORM_SRGB,
        TEX_FORMAT_BC3_TYPELESS, TEX_FORMAT_BC3_UNORM, TEX_FORMAT_BC3_UNORM_SRGB,
        TEX_FORMAT_BC4_TYPELESS, TEX_FORMAT_BC4_UNORM, TEX_FORMAT_BC4_SNORM,
        TEX_FORMAT_BC5_TYPELESS, TEX_FORMAT_BC5_UNORM, TEX_FORMAT_BC5_SNORM,
        TEX_FORMAT_BC6H_TYPELESS, TEX_FORMAT_BC6H_UF16, TEX_FORMAT_BC6H_SF16,
        TEX_FORMAT_BC7_TYPELESS,  TEX_FORMAT_BC7_UNORM, TEX_FORMAT_BC7_UNORM_SRGB
    };
    CheckComponentType(std::begin(CompressedFormats), std::end(CompressedFormats), COMPONENT_TYPE_COMPRESSED);
}
