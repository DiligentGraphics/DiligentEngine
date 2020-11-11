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

#include "D3D12/CreateObjFromNativeResD3D12.hpp"
#include <d3d12.h>
#include "RenderDeviceD3D12.h"
#include "TextureD3D12.h"
#include "BufferD3D12.h"
#include "GraphicsAccessories.hpp"

#include "gtest/gtest.h"

namespace Diligent
{

namespace Testing
{

void TestCreateObjFromNativeResD3D12::CreateTexture(Diligent::ITexture* pTexture)
{
    RefCntAutoPtr<IRenderDeviceD3D12> pDeviceD3D12(m_pDevice, IID_RenderDeviceD3D12);
    RefCntAutoPtr<ITextureD3D12>      pTextureD3D12(pTexture, IID_TextureD3D12);
    ASSERT_NE(pDeviceD3D12, nullptr);
    ASSERT_NE(pTextureD3D12, nullptr);

    const auto& SrcTexDesc    = pTexture->GetDesc();
    auto*       pD3D12Texture = pTextureD3D12->GetD3D12Texture();
    ASSERT_NE(pD3D12Texture, nullptr);

    RefCntAutoPtr<ITexture> pTextureFromNativeD3D12Handle;
    pDeviceD3D12->CreateTextureFromD3DResource(pD3D12Texture, RESOURCE_STATE_UNKNOWN, &pTextureFromNativeD3D12Handle);
    ASSERT_NE(pTextureFromNativeD3D12Handle, nullptr);

    auto TestTexDesc = pTextureFromNativeD3D12Handle->GetDesc();
    if (SrcTexDesc.Type == RESOURCE_DIM_TEX_CUBE || SrcTexDesc.Type == RESOURCE_DIM_TEX_CUBE_ARRAY)
    {
        EXPECT_EQ(TestTexDesc.Type, RESOURCE_DIM_TEX_2D_ARRAY);
        TestTexDesc.Type = SrcTexDesc.Type;
    }
    EXPECT_EQ(TestTexDesc, SrcTexDesc) << "Src tex desc:  " << GetObjectDescString(SrcTexDesc)
                                       << "\nTest tex desc: " << GetObjectDescString(TestTexDesc);

    RefCntAutoPtr<ITextureD3D12> pTestTextureD3D12(pTextureFromNativeD3D12Handle, IID_TextureD3D12);
    ASSERT_NE(pTestTextureD3D12, nullptr);
    EXPECT_EQ(pTestTextureD3D12->GetD3D12Texture(), pD3D12Texture);
    EXPECT_EQ(pTestTextureD3D12->GetNativeHandle(), pD3D12Texture);
}

void TestCreateObjFromNativeResD3D12::CreateBuffer(Diligent::IBuffer* pBuffer)
{
    RefCntAutoPtr<IRenderDeviceD3D12> pDeviceD3D12(m_pDevice, IID_RenderDeviceD3D12);
    RefCntAutoPtr<IBufferD3D12>       pBufferD3D12(pBuffer, IID_BufferD3D12);
    ASSERT_NE(pDeviceD3D12, nullptr);
    ASSERT_NE(pBufferD3D12, nullptr);


    Uint64 DataStartByteOffset;
    auto*  pD3D12Buffer = pBufferD3D12->GetD3D12Buffer(DataStartByteOffset, nullptr);
    ASSERT_NE(pD3D12Buffer, nullptr);
    EXPECT_EQ(DataStartByteOffset, 0);

    const auto& SrcBuffDesc = pBuffer->GetDesc();
    {
        RefCntAutoPtr<IBuffer> pBufferFromNativeD3D12Handle;
        pDeviceD3D12->CreateBufferFromD3DResource(pD3D12Buffer, SrcBuffDesc, RESOURCE_STATE_UNKNOWN, &pBufferFromNativeD3D12Handle);
        ASSERT_NE(pBufferFromNativeD3D12Handle, nullptr);

        const auto& TestBufferDesc = pBufferFromNativeD3D12Handle->GetDesc();
        EXPECT_EQ(TestBufferDesc, SrcBuffDesc) << "Src buff desc:  " << GetObjectDescString(SrcBuffDesc)
                                               << "\nTest buff desc: " << GetObjectDescString(TestBufferDesc);

        RefCntAutoPtr<IBufferD3D12> pTestBufferD3D12(pBufferFromNativeD3D12Handle, IID_BufferD3D12);
        ASSERT_NE(pTestBufferD3D12, nullptr);

        Uint64 TestBuffDataStartByteOffset;
        EXPECT_EQ(pTestBufferD3D12->GetD3D12Buffer(TestBuffDataStartByteOffset, nullptr), pD3D12Buffer);
        EXPECT_EQ(TestBuffDataStartByteOffset, 0);
        EXPECT_EQ(pTestBufferD3D12->GetNativeHandle(), pD3D12Buffer);
    }
}

} // namespace Testing

} // namespace Diligent
