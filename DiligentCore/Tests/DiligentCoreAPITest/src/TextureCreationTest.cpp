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

#include <algorithm>

#include "RenderDevice.h"
#include "GraphicsAccessories.hpp"

#if D3D11_SUPPORTED
#    include "D3D11/CreateObjFromNativeResD3D11.hpp"
#endif

#if D3D12_SUPPORTED
#    include "D3D12/CreateObjFromNativeResD3D12.hpp"
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
#    include "GL/CreateObjFromNativeResGL.hpp"
#endif

#if VULKAN_SUPPORTED
#    include "Vulkan/CreateObjFromNativeResVK.hpp"
#endif

#include "TestingEnvironment.hpp"

#include "gtest/gtest.h"

using namespace Diligent;
using namespace Diligent::Testing;

extern "C"
{
    int TestTextureCInterface(void* pTexture);
    int TestTextureViewCInterface(void* pView, void* pSampler);
}

namespace
{

struct TextureTestAttribs
{
    TEXTURE_FORMAT Fmt;
    Uint32         PixelSize;
    BIND_FLAGS     BindFlags;
    Bool           TestDataUpload;
};

inline std::ostream& operator<<(std::ostream& os, const TextureTestAttribs& TestAttribs)
{
    return os << "Pixel Size: " << TestAttribs.PixelSize
              << ", Bind Flags: " << GetBindFlagsString(TestAttribs.BindFlags)
              << ", test data upload: " << (TestAttribs.TestDataUpload ? "yes" : "no");
}

class TextureCreationTest : public testing::TestWithParam<TextureTestAttribs>
{
protected:
    static void SetUpTestSuite()
    {
        auto* pEnv    = TestingEnvironment::GetInstance();
        auto* pDevice = pEnv->GetDevice();

        const auto DevCaps = pDevice->GetDeviceCaps();
        switch (DevCaps.DevType)
        {
#if D3D11_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D11:
                pCreateObjFromNativeRes.reset(new TestCreateObjFromNativeResD3D11(pDevice));
                break;

#endif

#if D3D12_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D12:
                pCreateObjFromNativeRes.reset(new TestCreateObjFromNativeResD3D12(pDevice));
                break;
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
            case RENDER_DEVICE_TYPE_GL:
            case RENDER_DEVICE_TYPE_GLES:
                pCreateObjFromNativeRes.reset(new TestCreateObjFromNativeResGL(pDevice));
                break;
#endif

#if VULKAN_SUPPORTED
            case RENDER_DEVICE_TYPE_VULKAN:
                pCreateObjFromNativeRes.reset(new TestCreateObjFromNativeResVK(pDevice));
                break;
#endif

            default: UNEXPECTED("Unexpected device type");
        }

        pDevice->CreateSampler(SamplerDesc{}, &pSampler);
    }

    static void TearDownTestSuite()
    {
        pCreateObjFromNativeRes.reset();
        pSampler.Release();
        TestingEnvironment::GetInstance()->Reset();
    }

    static void PrepareSubresourceData(const TextureDesc&               TexDesc,
                                       const TextureFormatInfo&         FmtInfo,
                                       std::vector<std::vector<Uint8>>& Data,
                                       std::vector<TextureSubResData>&  SubResources,
                                       TextureData&                     InitData)
    {
        Uint32 ArrSize = (TexDesc.Type == RESOURCE_DIM_TEX_3D) ? 1 : TexDesc.ArraySize;

        VERIFY(FmtInfo.ComponentType != COMPONENT_TYPE_COMPRESSED, "Uploading data to compressed formats is not yet being tested");

        InitData.NumSubresources = ArrSize * TexDesc.MipLevels;
        Data.resize(InitData.NumSubresources);
        SubResources.resize(InitData.NumSubresources);
        Uint32 SubRes = 0;
        for (Uint32 Slice = 0; Slice < ArrSize; ++Slice)
        {
            for (Uint32 Mip = 0; Mip < TexDesc.MipLevels; ++Mip)
            {
                auto MipLevelInfo = GetMipLevelProperties(TexDesc, Mip);

                auto& CurrSubResData = Data[SubRes];
                auto& SubResInfo     = SubResources[SubRes];

                SubResInfo.Stride   = ((MipLevelInfo.RowSize + 3) & (-4)) + 192;
                auto SubresDataSize = 0;
                if (TexDesc.Type == RESOURCE_DIM_TEX_3D)
                {
                    SubResInfo.DepthStride = (MipLevelInfo.StorageHeight + 32) * SubResInfo.Stride;
                    SubresDataSize         = SubResInfo.DepthStride * MipLevelInfo.Depth;
                }
                else
                    SubresDataSize = SubResInfo.Stride * MipLevelInfo.StorageHeight;

                CurrSubResData.resize(SubresDataSize);
                SubResInfo.pData = CurrSubResData.data();
                ++SubRes;
            }
        }
        InitData.pSubResources = SubResources.data();
    }


    static void CreateTestTexture(RESOURCE_DIMENSION Type,
                                  TEXTURE_FORMAT     TextureFormat,
                                  BIND_FLAGS         BindFlags,
                                  Uint32             SampleCount,
                                  bool               UploadData)
    {
        auto* pEnv = TestingEnvironment::GetInstance();

        auto*       pDevice        = pEnv->GetDevice();
        auto*       pDeviceContext = pEnv->GetDeviceContext();
        const auto& deviceCaps     = pDevice->GetDeviceCaps();
        const auto& TexCaps        = deviceCaps.TexCaps;

        TextureDesc TexDesc;
        TexDesc.Name  = "Test Texture";
        TexDesc.Type  = Type;
        TexDesc.Width = 211;
        if (TexDesc.Type == RESOURCE_DIM_TEX_1D || TexDesc.Type == RESOURCE_DIM_TEX_1D_ARRAY)
            TexDesc.Height = 1;
        else
            TexDesc.Height = 243;

        if (TexDesc.Type == RESOURCE_DIM_TEX_3D)
            TexDesc.Depth = 16;

        if (TexDesc.Type == RESOURCE_DIM_TEX_1D_ARRAY || TexDesc.Type == RESOURCE_DIM_TEX_2D_ARRAY)
            TexDesc.ArraySize = 16;

        const auto& FmtInfo = pDevice->GetTextureFormatInfoExt(TextureFormat);
        if (FmtInfo.ComponentType == COMPONENT_TYPE_COMPRESSED)
        {
            TexDesc.Width  = (TexDesc.Width + 3) & (-4);
            TexDesc.Height = (TexDesc.Height + 3) & (-4);
        }

        TexDesc.MipLevels = SampleCount == 1 ? 0 : 1;
        TexDesc.Format    = TextureFormat;
        TexDesc.Usage     = USAGE_DEFAULT;
        TexDesc.BindFlags = BindFlags;
        if (SampleCount > 1)
            TexDesc.BindFlags &= ~BIND_UNORDERED_ACCESS;

        TexDesc.SampleCount = SampleCount;

        RefCntAutoPtr<ITexture> pTestTex;
        TextureData             NullData;
        pDevice->CreateTexture(TexDesc, &NullData, &pTestTex);
        ASSERT_NE(pTestTex, nullptr) << GetObjectDescString(TexDesc);

        pCreateObjFromNativeRes->CreateTexture(pTestTex);
        TexDesc.MipLevels = pTestTex->GetDesc().MipLevels;
        if (SampleCount == 1 && UploadData)
        {
            std::vector<std::vector<Uint8>> Data;
            std::vector<TextureSubResData>  SubResources;
            TextureData                     InitData;

            PrepareSubresourceData(TexDesc, FmtInfo, Data, SubResources, InitData);

            TexDesc.Name = "Test Texture 2";
            RefCntAutoPtr<ITexture> pTestTex2;
            pDevice->CreateTexture(TexDesc, &InitData, &pTestTex2);
            ASSERT_NE(pTestTex2, nullptr) << GetObjectDescString(TexDesc);

            pCreateObjFromNativeRes->CreateTexture(pTestTex2);

            if (deviceCaps.DevType == RENDER_DEVICE_TYPE_D3D11 &&
                ((TexDesc.BindFlags & BIND_DEPTH_STENCIL) != 0 || TexDesc.SampleCount > 1))
            {
                // In D3D11 if CopySubresourceRegion is used with Multisampled or D3D11_BIND_DEPTH_STENCIL Resources,
                // then the whole Subresource must be copied.
                CopyTextureAttribs CopyAttribs(pTestTex2, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, pTestTex, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
                pDeviceContext->CopyTexture(CopyAttribs);
            }
            else
            {
                Box SrcBox;
                SrcBox.MinX = TexDesc.Width / 4;
                SrcBox.MinY = TexDesc.Height / 4;
                SrcBox.MinZ = (TexDesc.Type == RESOURCE_DIM_TEX_3D) ? TexDesc.Depth / 4 : 0;
                SrcBox.MaxX = std::max(TexDesc.Width / 3, SrcBox.MinX + 1);
                SrcBox.MaxY = std::max(TexDesc.Height / 3, SrcBox.MinY + 1);
                SrcBox.MaxZ = (TexDesc.Type == RESOURCE_DIM_TEX_3D) ? std::max(TexDesc.Depth / 3, SrcBox.MinZ + 1) : 1;
                //pTestTex2->UpdateData(m_pDeviceContext, 0, 0, DstBox, SubResources[0]);
                CopyTextureAttribs CopyAttribs(pTestTex2, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, pTestTex, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
                CopyAttribs.pSrcBox = &SrcBox;
                pDeviceContext->CopyTexture(CopyAttribs);
            }
        }

        if (!TexCaps.TextureViewSupported)
        {
            LOG_WARNING_MESSAGE_ONCE("Texture views are not supported!\n");
        }

        if (TexCaps.TextureViewSupported && !FmtInfo.IsTypeless)
        {
            TextureViewDesc ViewDesc;
            ViewDesc.TextureDim = TexDesc.Type;

            if (SampleCount > 1)
            {
                ViewDesc.MostDetailedMip = 0;
                ViewDesc.NumMipLevels    = 1;
            }
            else
            {
                ViewDesc.MostDetailedMip = 1;
                ViewDesc.NumMipLevels    = 2;
            }

            if (TexDesc.Type == RESOURCE_DIM_TEX_1D_ARRAY || TexDesc.Type == RESOURCE_DIM_TEX_2D_ARRAY)
            {
                ViewDesc.FirstArraySlice = 3;
                ViewDesc.NumArraySlices  = (deviceCaps.DevType == RENDER_DEVICE_TYPE_D3D11 || deviceCaps.DevType == RENDER_DEVICE_TYPE_D3D12 || ViewDesc.ViewType == TEXTURE_VIEW_SHADER_RESOURCE) ? 4 : 1;
            }
            else if (TexDesc.Type == RESOURCE_DIM_TEX_3D)
            {
                if (deviceCaps.DevType == RENDER_DEVICE_TYPE_D3D11 || deviceCaps.DevType == RENDER_DEVICE_TYPE_D3D12)
                {
                    ViewDesc.FirstDepthSlice = 3;
                    ViewDesc.NumDepthSlices  = 4;
                }
                else if ((deviceCaps.DevType == RENDER_DEVICE_TYPE_VULKAN && ViewDesc.ViewType != TEXTURE_VIEW_RENDER_TARGET && ViewDesc.ViewType != TEXTURE_VIEW_DEPTH_STENCIL) ||
                         deviceCaps.DevType != RENDER_DEVICE_TYPE_VULKAN)
                {
                    // OpenGL cannot create views for separate depth slices
                    ViewDesc.FirstDepthSlice = 0;
                    ViewDesc.NumDepthSlices  = TexDesc.Depth >> ViewDesc.MostDetailedMip;
                }
            }
            else
            {
                ViewDesc.FirstArraySlice = 0;
                ViewDesc.NumArraySlices  = 1;
            }

            if (TexDesc.BindFlags & BIND_SHADER_RESOURCE)
            {
                ViewDesc.ViewType = TEXTURE_VIEW_SHADER_RESOURCE;
                RefCntAutoPtr<ITextureView> pSRV;
                pTestTex->CreateView(ViewDesc, &pSRV);
                EXPECT_NE(pSRV, nullptr) << GetObjectDescString(TexDesc);

                EXPECT_EQ(TestTextureCInterface(pTestTex), 0);
                EXPECT_EQ(TestTextureViewCInterface(pSRV.RawPtr(), pSampler.RawPtr()), 0);
            }

            // RTV, DSV & UAV can reference only one mip level
            ViewDesc.NumMipLevels = 1;

            if (TexDesc.BindFlags & BIND_RENDER_TARGET)
            {
                ViewDesc.ViewType = TEXTURE_VIEW_RENDER_TARGET;
                RefCntAutoPtr<ITextureView> pRTV;
                pTestTex->CreateView(ViewDesc, &pRTV);
                EXPECT_NE(pRTV, nullptr) << GetObjectDescString(TexDesc);
            }

            if (TexDesc.BindFlags & BIND_DEPTH_STENCIL)
            {
                ViewDesc.ViewType = TEXTURE_VIEW_DEPTH_STENCIL;
                RefCntAutoPtr<ITextureView> pDSV;
                pTestTex->CreateView(ViewDesc, &pDSV);
                EXPECT_NE(pDSV, nullptr) << GetObjectDescString(TexDesc);
            }

            if (TexDesc.BindFlags & BIND_UNORDERED_ACCESS)
            {
                ViewDesc.ViewType    = TEXTURE_VIEW_UNORDERED_ACCESS;
                ViewDesc.AccessFlags = UAV_ACCESS_FLAG_READ | UAV_ACCESS_FLAG_WRITE;
                RefCntAutoPtr<ITextureView> pUAV;
                pTestTex->CreateView(ViewDesc, &pUAV);
                EXPECT_NE(pUAV, nullptr) << GetObjectDescString(TexDesc);
            }
        }

        // Flush, FinishFrame, and ReleaseStaleResources.
        pEnv->ReleaseResources();
    }

    void CreateTestCubemap(TEXTURE_FORMAT TextureFormat,
                           BIND_FLAGS     BindFlags,
                           bool           UploadData)
    {
        auto* pEnv = TestingEnvironment::GetInstance();

        auto*       pDevice    = pEnv->GetDevice();
        const auto& deviceCaps = pDevice->GetDeviceCaps();
        const auto& TexCaps    = deviceCaps.TexCaps;

        TextureDesc TexDesc;
        TexDesc.Name        = "Test Cube MapTextureCreation";
        TexDesc.Type        = RESOURCE_DIM_TEX_CUBE;
        TexDesc.Width       = 256;
        TexDesc.Height      = 256;
        TexDesc.ArraySize   = 6;
        TexDesc.MipLevels   = 0;
        TexDesc.Format      = TextureFormat;
        TexDesc.Usage       = USAGE_DEFAULT;
        TexDesc.BindFlags   = BindFlags;
        TexDesc.SampleCount = 1;

        RefCntAutoPtr<ITexture> pTestCubemap, pTestCubemapArr;
        pDevice->CreateTexture(TexDesc, nullptr, &pTestCubemap);
        ASSERT_NE(pTestCubemap, nullptr) << GetObjectDescString(TexDesc);
        pCreateObjFromNativeRes->CreateTexture(pTestCubemap);
        TexDesc.MipLevels = pTestCubemap->GetDesc().MipLevels;

        const auto& FmtInfo = pDevice->GetTextureFormatInfoExt(TextureFormat);
        if (UploadData)
        {
            std::vector<std::vector<Uint8>> Data;
            std::vector<TextureSubResData>  SubResources;
            TextureData                     InitData;

            PrepareSubresourceData(TexDesc, FmtInfo, Data, SubResources, InitData);

            InitData.pSubResources = SubResources.data();
            TexDesc.Name           = "TestCubemap2";
            pTestCubemap.Release();
            pDevice->CreateTexture(TexDesc, &InitData, &pTestCubemap);
            ASSERT_NE(pTestCubemap, nullptr) << GetObjectDescString(TexDesc);
            pCreateObjFromNativeRes->CreateTexture(pTestCubemap);
        }

        if (TexCaps.TextureViewSupported && !FmtInfo.IsTypeless)
        {
            TextureViewDesc ViewDesc;
            if (TexDesc.BindFlags & BIND_SHADER_RESOURCE)
            {
                ViewDesc.TextureDim      = RESOURCE_DIM_TEX_CUBE;
                ViewDesc.ViewType        = TEXTURE_VIEW_SHADER_RESOURCE;
                ViewDesc.MostDetailedMip = 1;
                ViewDesc.NumMipLevels    = 6;
                {
                    RefCntAutoPtr<ITextureView> pSRV;
                    pTestCubemap->CreateView(ViewDesc, &pSRV);
                    EXPECT_NE(pSRV, nullptr) << GetObjectDescString(TexDesc);
                }

                ViewDesc.TextureDim      = RESOURCE_DIM_TEX_2D;
                ViewDesc.FirstArraySlice = 0;
                ViewDesc.NumArraySlices  = 1;
                ViewDesc.MostDetailedMip = 2;
                ViewDesc.NumMipLevels    = 4;
                {
                    RefCntAutoPtr<ITextureView> pSRV;
                    pTestCubemap->CreateView(ViewDesc, &pSRV);
                    EXPECT_NE(pSRV, nullptr) << GetObjectDescString(TexDesc);
                }

                ViewDesc.TextureDim      = RESOURCE_DIM_TEX_2D_ARRAY;
                ViewDesc.FirstArraySlice = 2;
                ViewDesc.NumArraySlices  = 3;
                {
                    RefCntAutoPtr<ITextureView> pSRV;
                    pTestCubemap->CreateView(ViewDesc, &pSRV);
                    EXPECT_NE(pSRV, nullptr) << GetObjectDescString(TexDesc);
                }
            }

            // RTV, DSV & UAV can reference only one mip level
            ViewDesc.NumMipLevels = 1;

            if (TexDesc.BindFlags & BIND_RENDER_TARGET)
            {
                ViewDesc.ViewType        = TEXTURE_VIEW_RENDER_TARGET;
                ViewDesc.TextureDim      = RESOURCE_DIM_TEX_2D;
                ViewDesc.FirstArraySlice = 0;
                ViewDesc.NumArraySlices  = 1;
                {
                    RefCntAutoPtr<ITextureView> pRTV;
                    pTestCubemap->CreateView(ViewDesc, &pRTV);
                    EXPECT_NE(pRTV, nullptr) << GetObjectDescString(TexDesc);
                }

                ViewDesc.TextureDim      = RESOURCE_DIM_TEX_2D_ARRAY;
                ViewDesc.FirstArraySlice = 2;
                ViewDesc.NumArraySlices  = 3;
                {
                    RefCntAutoPtr<ITextureView> pRTV;
                    pTestCubemap->CreateView(ViewDesc, &pRTV);
                    EXPECT_NE(pRTV, nullptr) << GetObjectDescString(TexDesc);
                }
            }

            if (TexDesc.BindFlags & BIND_DEPTH_STENCIL)
            {
                ViewDesc.ViewType        = TEXTURE_VIEW_DEPTH_STENCIL;
                ViewDesc.TextureDim      = RESOURCE_DIM_TEX_2D;
                ViewDesc.NumArraySlices  = 1;
                ViewDesc.FirstArraySlice = 0;
                ViewDesc.MostDetailedMip = 2;
                {
                    RefCntAutoPtr<ITextureView> pDSV;
                    pTestCubemap->CreateView(ViewDesc, &pDSV);
                    EXPECT_NE(pDSV, nullptr) << GetObjectDescString(TexDesc);
                }

                ViewDesc.TextureDim      = RESOURCE_DIM_TEX_2D_ARRAY;
                ViewDesc.NumArraySlices  = 3;
                ViewDesc.FirstArraySlice = 2;
                {
                    RefCntAutoPtr<ITextureView> pDSV;
                    pTestCubemap->CreateView(ViewDesc, &pDSV);
                    EXPECT_NE(pDSV, nullptr) << GetObjectDescString(TexDesc);
                }
            }

            if (TexDesc.BindFlags & BIND_UNORDERED_ACCESS)
            {
                ViewDesc.ViewType        = TEXTURE_VIEW_UNORDERED_ACCESS;
                ViewDesc.AccessFlags     = UAV_ACCESS_FLAG_READ | UAV_ACCESS_FLAG_WRITE;
                ViewDesc.TextureDim      = RESOURCE_DIM_TEX_2D;
                ViewDesc.NumArraySlices  = 1;
                ViewDesc.FirstArraySlice = 0;
                {
                    RefCntAutoPtr<ITextureView> pUAV;
                    pTestCubemap->CreateView(ViewDesc, &pUAV);
                    EXPECT_NE(pUAV, nullptr) << GetObjectDescString(TexDesc);
                }

                ViewDesc.TextureDim = RESOURCE_DIM_TEX_2D_ARRAY;
                if (deviceCaps.DevType == RENDER_DEVICE_TYPE_GL || deviceCaps.DevType == RENDER_DEVICE_TYPE_GLES)
                {
                    ViewDesc.NumArraySlices  = 1;
                    ViewDesc.FirstArraySlice = 2;
                }
                else
                {
                    ViewDesc.NumArraySlices  = 3;
                    ViewDesc.FirstArraySlice = 2;
                }

                {
                    RefCntAutoPtr<ITextureView> pUAV;
                    pTestCubemap->CreateView(ViewDesc, &pUAV);
                    EXPECT_NE(pUAV, nullptr) << GetObjectDescString(TexDesc);
                }
            }
        }

        if (deviceCaps.TexCaps.CubemapArraysSupported)
        {
            TexDesc.ArraySize = 24;
            TexDesc.Type      = RESOURCE_DIM_TEX_CUBE_ARRAY;

            std::vector<std::vector<Uint8>> Data;
            std::vector<TextureSubResData>  SubResources;
            TextureData                     InitData;

            if (UploadData)
            {
                PrepareSubresourceData(TexDesc, FmtInfo, Data, SubResources, InitData);
            }

            TexDesc.Name = "TestCubemapArray";

            pDevice->CreateTexture(TexDesc, &InitData, &pTestCubemapArr);
            EXPECT_NE(pTestCubemapArr, nullptr) << GetObjectDescString(TexDesc);
            pCreateObjFromNativeRes->CreateTexture(pTestCubemapArr);
        }

        // Flush, FinishFrame, and ReleaseStaleResources.
        pEnv->ReleaseResources();
    }

    static std::unique_ptr<CreateObjFromNativeResTestBase> pCreateObjFromNativeRes;
    static RefCntAutoPtr<ISampler>                         pSampler;
};

std::unique_ptr<CreateObjFromNativeResTestBase> TextureCreationTest::pCreateObjFromNativeRes;
RefCntAutoPtr<ISampler>                         TextureCreationTest::pSampler;

TEST_P(TextureCreationTest, CreateTexture)
{
    auto* pEnv    = TestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();

    const auto& TestInfo = GetParam();

    const auto& FmtInfo = pDevice->GetTextureFormatInfoExt(TestInfo.Fmt);

    if (!FmtInfo.Supported)
    {
        GTEST_SKIP() << "Texture format " << FmtInfo.Name << " is not supported";
    }

    EXPECT_TRUE(TestInfo.PixelSize == Uint32{FmtInfo.ComponentSize} * Uint32{FmtInfo.NumComponents} ||
                FmtInfo.ComponentType == COMPONENT_TYPE_COMPRESSED);

    const auto& TexCaps = pDevice->GetDeviceCaps().TexCaps;
    // Test texture 1D / texture 1D array
    if (TexCaps.MaxTexture1DDimension != 0)
    {
        if (FmtInfo.Dimensions & RESOURCE_DIMENSION_SUPPORT_TEX_1D)
            CreateTestTexture(RESOURCE_DIM_TEX_1D, TestInfo.Fmt, TestInfo.BindFlags, 1, TestInfo.TestDataUpload);
    }
    else
    {
        LOG_WARNING_MESSAGE_ONCE("Texture 1D is not supported\n");
    }

    if (TexCaps.MaxTexture1DArraySlices != 0)
    {
        if (FmtInfo.Dimensions & RESOURCE_DIMENSION_SUPPORT_TEX_1D_ARRAY)
            CreateTestTexture(RESOURCE_DIM_TEX_1D_ARRAY, TestInfo.Fmt, TestInfo.BindFlags, 1, TestInfo.TestDataUpload);
    }
    else
    {
        LOG_WARNING_MESSAGE_ONCE("Texture 1D array is not supported\n");
    }

    // Test texture 2D / texture 2D array
    if (FmtInfo.Dimensions & RESOURCE_DIMENSION_SUPPORT_TEX_2D)
    {
        CreateTestTexture(RESOURCE_DIM_TEX_2D, TestInfo.Fmt, TestInfo.BindFlags, 1, TestInfo.TestDataUpload);
        CreateTestTexture(RESOURCE_DIM_TEX_2D, TestInfo.Fmt, TestInfo.BindFlags, 1, TestInfo.TestDataUpload);
    }
    if (FmtInfo.Dimensions & RESOURCE_DIMENSION_SUPPORT_TEX_2D_ARRAY)
    {
        CreateTestTexture(RESOURCE_DIM_TEX_2D_ARRAY, TestInfo.Fmt, TestInfo.BindFlags, 1, TestInfo.TestDataUpload);
        CreateTestTexture(RESOURCE_DIM_TEX_2D_ARRAY, TestInfo.Fmt, TestInfo.BindFlags, 1, TestInfo.TestDataUpload);
    }
    if (FmtInfo.Dimensions & RESOURCE_DIMENSION_SUPPORT_TEX_CUBE)
    {
        CreateTestCubemap(TestInfo.Fmt, TestInfo.BindFlags, TestInfo.TestDataUpload);
    }

    if (TestInfo.Fmt != TEX_FORMAT_RGB9E5_SHAREDEXP &&
        FmtInfo.ComponentType != COMPONENT_TYPE_COMPRESSED)
    {
        if (TexCaps.Texture2DMSSupported)
        {
            if ((FmtInfo.SampleCounts & 0x04) != 0 && (TestInfo.BindFlags & (BIND_RENDER_TARGET | BIND_DEPTH_STENCIL)) != 0)
            {
                CreateTestTexture(RESOURCE_DIM_TEX_2D, TestInfo.Fmt, TestInfo.BindFlags, 4, TestInfo.TestDataUpload);
                CreateTestTexture(RESOURCE_DIM_TEX_2D, TestInfo.Fmt, TestInfo.BindFlags, 4, TestInfo.TestDataUpload);
            }
        }
        else
        {
            LOG_WARNING_MESSAGE_ONCE("Texture 2D MS is not supported\n");
        }

        if (TexCaps.Texture2DMSArraySupported)
        {
            if ((FmtInfo.SampleCounts & 0x04) != 0 && (TestInfo.BindFlags & (BIND_RENDER_TARGET | BIND_DEPTH_STENCIL)) != 0)
            {
                CreateTestTexture(RESOURCE_DIM_TEX_2D_ARRAY, TestInfo.Fmt, TestInfo.BindFlags, 4, TestInfo.TestDataUpload);
                CreateTestTexture(RESOURCE_DIM_TEX_2D_ARRAY, TestInfo.Fmt, TestInfo.BindFlags, 4, TestInfo.TestDataUpload);
            }
        }
        else
        {
            LOG_WARNING_MESSAGE_ONCE("Texture 2D MS Array is not supported\n");
        }
    }

    // Test texture 3D
    if (FmtInfo.Dimensions & RESOURCE_DIMENSION_SUPPORT_TEX_3D)
        CreateTestTexture(RESOURCE_DIM_TEX_3D, TestInfo.Fmt, TestInfo.BindFlags, 1, TestInfo.TestDataUpload);
}



constexpr BIND_FLAGS BindSRU = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET | BIND_UNORDERED_ACCESS;
constexpr BIND_FLAGS BindSR  = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET;
//constexpr BIND_FLAGS BindSD = BIND_SHADER_RESOURCE | BIND_DEPTH_STENCIL;
constexpr BIND_FLAGS BindSU = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
constexpr BIND_FLAGS BindSD = BIND_SHADER_RESOURCE | BIND_DEPTH_STENCIL;
constexpr BIND_FLAGS BindD  = BIND_DEPTH_STENCIL;
constexpr BIND_FLAGS BindS  = BIND_SHADER_RESOURCE;

// clang-format off
const TextureTestAttribs TestList[] =
{
    {TEX_FORMAT_RGBA32_TYPELESS, 16, BindSRU, true},
    {TEX_FORMAT_RGBA32_FLOAT,    16, BindSRU, true},
    {TEX_FORMAT_RGBA32_UINT,     16, BindSRU, true},
    {TEX_FORMAT_RGBA32_SINT,     16, BindSRU, true},
  //{TEX_FORMAT_RGB32_TYPELESS   12,   BindS, true},

    // These formats are ill-supported
  //{TEX_FORMAT_RGB32_FLOAT,      12,   BindS, true},
  //{TEX_FORMAT_RGB32_UINT,       12,   BindS, true},
  //{TEX_FORMAT_RGB32_SINT,       12,   BindS, true},

    {TEX_FORMAT_RGBA16_TYPELESS,   8, BindSRU, true},
    {TEX_FORMAT_RGBA16_FLOAT,      8, BindSRU, true},
    {TEX_FORMAT_RGBA16_UNORM,      8, BindSRU, true},
    {TEX_FORMAT_RGBA16_UINT,       8, BindSRU, true},
    {TEX_FORMAT_RGBA16_SNORM,      8, BindSU,  true},
    {TEX_FORMAT_RGBA16_SINT,       8, BindSRU, true},
                                       
    {TEX_FORMAT_RG32_TYPELESS,     8, BindSRU, true},
    {TEX_FORMAT_RG32_FLOAT,        8, BindSRU, true},
    {TEX_FORMAT_RG32_UINT,         8, BindSRU, true},
    {TEX_FORMAT_RG32_SINT,         8, BindSRU, true},

    {TEX_FORMAT_R32G8X24_TYPELESS,        8, BindD, false},
    {TEX_FORMAT_D32_FLOAT_S8X24_UINT,     8, BindD, false},
  //{TEX_FORMAT_R32_FLOAT_X8X24_TYPELESS, 8, BindD, false},
  //{TEX_FORMAT_X32_TYPELESS_G8X24_UINT,  8, BindD, false},

    {TEX_FORMAT_RGB10A2_TYPELESS,        4, BindSR,  true},
    {TEX_FORMAT_RGB10A2_UNORM,           4, BindSR,  true},
    {TEX_FORMAT_RGB10A2_UINT,            4, BindS,   true},
    {TEX_FORMAT_R11G11B10_FLOAT,         4, BindSRU, false},

    {TEX_FORMAT_RGBA8_TYPELESS,          4, BindSRU, true},
    {TEX_FORMAT_RGBA8_UNORM,             4, BindSRU, true},
    {TEX_FORMAT_RGBA8_UNORM_SRGB,        4, BindSR,  true},
    {TEX_FORMAT_RGBA8_UINT,              4, BindSRU, true},
    {TEX_FORMAT_RGBA8_SNORM,             4, BindSU,  true},
    {TEX_FORMAT_RGBA8_SINT,              4, BindSRU, true},

    {TEX_FORMAT_RG16_TYPELESS,           4, BindSRU, true},
    {TEX_FORMAT_RG16_FLOAT,              4, BindSRU, true},
    {TEX_FORMAT_RG16_UNORM,              4, BindSRU, true},
    {TEX_FORMAT_RG16_UINT,               4, BindSRU, true},
    {TEX_FORMAT_RG16_SNORM,              4, BindSU,  true},
    {TEX_FORMAT_RG16_SINT,               4, BindSRU, true},

    {TEX_FORMAT_R32_TYPELESS,            4, BindSRU, true},
    {TEX_FORMAT_D32_FLOAT,               4, BindD,   false},
    {TEX_FORMAT_R32_FLOAT,               4, BindSRU, true},
    {TEX_FORMAT_R32_UINT,                4, BindSRU, true},
    {TEX_FORMAT_R32_SINT,                4, BindSRU, true},

    {TEX_FORMAT_R24G8_TYPELESS,          4, BindSD, false},
    {TEX_FORMAT_D24_UNORM_S8_UINT,       4, BindD,  false},
  //{TEX_FORMAT_R24_UNORM_X8_TYPELESS,   4, BindD,   true},
  //{TEX_FORMAT_X24_TYPELESS_G8_UINT,    4, BindD,   true},
                                                       
    {TEX_FORMAT_RG8_TYPELESS,            2, BindSRU, true},
    {TEX_FORMAT_RG8_UNORM,               2, BindSRU, true},
    {TEX_FORMAT_RG8_UINT,                2, BindSRU, true},
    {TEX_FORMAT_RG8_SNORM,               2, BindSU,  true},
    {TEX_FORMAT_RG8_SINT,                2, BindSRU, true},

    {TEX_FORMAT_R16_TYPELESS,            2, BindSRU, true},
    {TEX_FORMAT_R16_FLOAT,               2, BindSRU, true},
    {TEX_FORMAT_D16_UNORM,               2, BindD,   false},
    {TEX_FORMAT_R16_UNORM,               2, BindSRU, true},
    {TEX_FORMAT_R16_UINT,                2, BindSRU, true},
    {TEX_FORMAT_R16_SNORM,               2, BindSU,  true},
    {TEX_FORMAT_R16_SINT,                2, BindSRU, true},

    {TEX_FORMAT_R8_TYPELESS,             1, BindSRU, true},
    {TEX_FORMAT_R8_UNORM,                1, BindSRU, true},
    {TEX_FORMAT_R8_UINT,                 1, BindSRU, true},
    {TEX_FORMAT_R8_SNORM,                1, BindSU,  true},
    {TEX_FORMAT_R8_SINT,                 1, BindSRU, true},
  //{TEX_FORMAT_A8_UNORM,                1, BindSRU, true},
  //{TEX_FORMAT_R1_UNORM,                1, BindSRU, true},

    {TEX_FORMAT_RGB9E5_SHAREDEXP,        4, BindS,   false},
  //{TEX_FORMAT_RG8_B8G8_UNORM,          4, BindSRU, false},
  //{TEX_FORMAT_G8R8_G8B8_UNORM,         4, BindSRU, false},

  //{TEX_FORMAT_BC1_TYPELESS,            16, BindS, false},
    {TEX_FORMAT_BC1_UNORM,               16, BindS, false},
    {TEX_FORMAT_BC1_UNORM_SRGB,          16, BindS, false},
  //{TEX_FORMAT_BC2_TYPELESS,            16, BindS, false},
    {TEX_FORMAT_BC2_UNORM,               16, BindS, false},
    {TEX_FORMAT_BC2_UNORM_SRGB,          16, BindS, false},
  //{TEX_FORMAT_BC3_TYPELESS,            16, BindS, false},
    {TEX_FORMAT_BC3_UNORM,               16, BindS, false},
    {TEX_FORMAT_BC3_UNORM_SRGB,          16, BindS, false},
  //{TEX_FORMAT_BC4_TYPELESS,            16, BindS, false},
    {TEX_FORMAT_BC4_UNORM,               16, BindS, false},
    {TEX_FORMAT_BC4_SNORM,               16, BindS, false},
  //{TEX_FORMAT_BC5_TYPELESS,            16, BindS, false},
    {TEX_FORMAT_BC5_UNORM,               16, BindS, false},
    {TEX_FORMAT_BC5_SNORM,               16, BindS, false},

  //{TEX_FORMAT_B5G6R5_UNORM,            2, BindSRU, true},
  //{TEX_FORMAT_B5G5R5A1_UNORM,          2, BindSRU, true},
  //{TEX_FORMAT_BGRA8_UNORM,             4, BindSRU, true},
  //{TEX_FORMAT_BGRX8_UNORM,             4, BindSRU, true},
  //{TEX_FORMAT_R10G10B10_XR_BIAS_A2_UNORM, 4, BindS, false},

    {TEX_FORMAT_BGRA8_TYPELESS,          4, BindSRU, true},
    {TEX_FORMAT_BGRA8_UNORM_SRGB,        4, BindSR,  true},
  //{TEX_FORMAT_BGRX8_TYPELESS,          4, BindSRU, true},
  //{TEX_FORMAT_BGRX8_UNORM_SRGB,        4, BindSR,  true},

  //{TEX_FORMAT_BC6H_TYPELESS,           16, BindS, false},
    {TEX_FORMAT_BC6H_UF16,               16, BindS, false},
    {TEX_FORMAT_BC6H_SF16,               16, BindS, false},
  //{TEX_FORMAT_BC7_TYPELESS,            16, BindS, false},
    {TEX_FORMAT_BC7_UNORM,               16, BindS, false},
    {TEX_FORMAT_BC7_UNORM_SRGB,          16, BindS, false},
};
// clang-format on


INSTANTIATE_TEST_SUITE_P(TextureCreation,
                         TextureCreationTest,
                         testing::ValuesIn(std::begin(TestList), std::end(TestList)),
                         [](const testing::TestParamInfo<TextureTestAttribs>& info) //
                         {
                             const auto& FmtAttribs = GetTextureFormatAttribs(info.param.Fmt);
                             return FmtAttribs.Name;
                         } //
);

} // namespace
