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

#include "TextureUploader.hpp"
#include "TestingEnvironment.hpp"

#include "gtest/gtest.h"

#include <atomic>
#include <thread>

using namespace Diligent;
using namespace Diligent::Testing;

namespace
{

Uint32 WriteOrVerifyRGBAData(MappedTextureSubresource& MappedData,
                             const UploadBufferDesc&   UploadBuffDesc,
                             Uint32                    Mip,
                             Uint32                    Slice,
                             Uint32&                   cnt,
                             bool                      Verify)
{
    Uint8* pRGBAData        = reinterpret_cast<Uint8*>(MappedData.pData);
    Uint32 NumInvalidPixels = 0;

    const Uint32 Width  = UploadBuffDesc.Width >> Mip;
    const Uint32 Height = UploadBuffDesc.Height >> Mip;
    if (Verify)
    {
        for (Uint32 y = 0; y < Height; ++y)
        {
            for (Uint32 x = 0; x < Width; ++x)
            {
                const Uint8* pRGBA = pRGBAData + x * 4 + MappedData.Stride * y;
                NumInvalidPixels += (*(pRGBA++) != ((cnt += 13) & 0xFF)) ? 1 : 0;
                NumInvalidPixels += (*(pRGBA++) != ((cnt += 27) & 0xFF)) ? 1 : 0;
                NumInvalidPixels += (*(pRGBA++) != ((cnt += 7) & 0xFF)) ? 1 : 0;
                NumInvalidPixels += (*(pRGBA++) != ((cnt += 3) & 0xFF)) ? 1 : 0;
            }
        }

        if (NumInvalidPixels != 0)
        {
            ADD_FAILURE() << "Found " << NumInvalidPixels << " invalid pixels in mip level " << Mip << ", slice " << Slice;
        }
    }
    else
    {
        for (Uint32 y = 0; y < Height; ++y)
        {
            for (Uint32 x = 0; x < Width; ++x)
            {
                Uint8* pRGBA = pRGBAData + x * 4 + MappedData.Stride * y;

                *(pRGBA++) = (cnt += 13) & 0xFF;
                *(pRGBA++) = (cnt += 27) & 0xFF;
                *(pRGBA++) = (cnt += 7) & 0xFF;
                *(pRGBA++) = (cnt += 3) & 0xFF;
            }
        }
    }

    return NumInvalidPixels;
}

void TextureUploaderTest(bool IsRenderThread)
{
    auto* pEnv     = TestingEnvironment::GetInstance();
    auto* pDevice  = pEnv->GetDevice();
    auto* pContext = pEnv->GetDeviceContext();

    TextureUploaderDesc             UploaderDesc;
    RefCntAutoPtr<ITextureUploader> pTexUploader;
    CreateTextureUploader(pDevice, UploaderDesc, &pTexUploader);
    ASSERT_TRUE(pTexUploader);

    TextureDesc TexDesc;
    TexDesc.Name      = "Texture uploading dst texture";
    TexDesc.Type      = RESOURCE_DIM_TEX_2D_ARRAY;
    TexDesc.Width     = 256;
    TexDesc.Height    = 128;
    TexDesc.MipLevels = 0;
    TexDesc.ArraySize = 8;
    TexDesc.BindFlags = BIND_SHADER_RESOURCE;
    TexDesc.Format    = TEX_FORMAT_RGBA8_UNORM;
    RefCntAutoPtr<ITexture> pDstTexture;
    pDevice->CreateTexture(TexDesc, nullptr, &pDstTexture);


    TexDesc.Name           = "Texture uploading staging texture";
    TexDesc.Usage          = USAGE_STAGING;
    TexDesc.CPUAccessFlags = CPU_ACCESS_READ;
    TexDesc.BindFlags      = BIND_NONE;
    RefCntAutoPtr<ITexture> pStagingTexture;
    pDevice->CreateTexture(TexDesc, nullptr, &pStagingTexture);

    constexpr Uint32 StartDstSlice = 2;
    constexpr Uint32 StartDstMip   = 1;

    UploadBufferDesc UploadBuffDesc;
    UploadBuffDesc.Width     = TexDesc.Width >> StartDstMip;
    UploadBuffDesc.Height    = TexDesc.Height >> StartDstMip;
    UploadBuffDesc.Format    = TEX_FORMAT_RGBA8_UNORM;
    UploadBuffDesc.MipLevels = 2;
    UploadBuffDesc.ArraySize = 4;

    Uint32 cnt = 0;
    for (Uint32 i = 0; i < 3; ++i)
    {
        auto ref_cnt = cnt;

        std::atomic_bool BufferPopulated;
        BufferPopulated.store(false);

        auto PopulateBuffer = [&](IDeviceContext* pCtx) //
        {
            RefCntAutoPtr<IUploadBuffer> pUploadBuffer;
            pTexUploader->AllocateUploadBuffer(pCtx, UploadBuffDesc, &pUploadBuffer);

            for (Uint32 slice = 0; slice < UploadBuffDesc.ArraySize; ++slice)
            {
                for (Uint32 mip = 0; mip < UploadBuffDesc.MipLevels; ++mip)
                {
                    auto MappedData = pUploadBuffer->GetMappedData(mip, slice);
                    WriteOrVerifyRGBAData(MappedData, UploadBuffDesc, mip, slice, cnt, false);
                }
            }
            pTexUploader->ScheduleGPUCopy(pCtx, pDstTexture, StartDstSlice, StartDstMip, pUploadBuffer);
            if (pCtx == nullptr)
            {
                pUploadBuffer->WaitForCopyScheduled();
            }
            pTexUploader->RecycleBuffer(pUploadBuffer);
            BufferPopulated.store(true);
        };

        if (IsRenderThread)
        {
            PopulateBuffer(pContext);
        }
        else
        {
            std::thread WokerThread{PopulateBuffer, nullptr};
            while (!BufferPopulated)
            {
                pTexUploader->RenderThreadUpdate(pContext);
            }
            WokerThread.join();
        }


        for (Uint32 slice = StartDstSlice; slice < StartDstSlice + UploadBuffDesc.ArraySize; ++slice)
        {
            for (Uint32 mip = StartDstMip; mip < StartDstMip + UploadBuffDesc.MipLevels; ++mip)
            {
                CopyTextureAttribs CopyAttribs{pDstTexture, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, pStagingTexture, RESOURCE_STATE_TRANSITION_MODE_TRANSITION};
                CopyAttribs.SrcMipLevel = mip;
                CopyAttribs.SrcSlice    = slice;
                CopyAttribs.DstMipLevel = mip;
                CopyAttribs.DstSlice    = slice;
                pContext->CopyTexture(CopyAttribs);
            }
        }

        pContext->WaitForIdle();

        for (Uint32 slice = 0; slice < UploadBuffDesc.ArraySize; ++slice)
        {
            for (Uint32 mip = 0; mip < UploadBuffDesc.MipLevels; ++mip)
            {
                MappedTextureSubresource MappedData;
                pContext->MapTextureSubresource(pStagingTexture, StartDstMip + mip, StartDstSlice + slice, MAP_READ, MAP_FLAG_DO_NOT_WAIT, nullptr, MappedData);
                WriteOrVerifyRGBAData(MappedData, UploadBuffDesc, mip, slice, ref_cnt, true);
                pContext->UnmapTextureSubresource(pStagingTexture, StartDstMip + mip, StartDstSlice + slice);
            }
        }
    }
}

TEST(TextureUploaderTest, RenderThread)
{
    TextureUploaderTest(true);
}

TEST(TextureUploaderTest, WorkerThread)
{
    TextureUploaderTest(false);
}

} // namespace
