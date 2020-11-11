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

#include "D3D11/CreateObjFromNativeResD3D11.hpp"

#include <d3d11.h>
#include "RenderDeviceD3D11.h"
#include "TextureD3D11.h"
#include "BufferD3D11.h"
#include "GraphicsAccessories.hpp"

#include "gtest/gtest.h"

namespace Diligent
{

namespace Testing
{

void TestCreateObjFromNativeResD3D11::CreateTexture(ITexture* pTexture)
{
    RefCntAutoPtr<IRenderDeviceD3D11> pDeviceD3D11(m_pDevice, IID_RenderDeviceD3D11);
    RefCntAutoPtr<ITextureD3D11>      pTextureD3D11(pTexture, IID_TextureD3D11);
    ASSERT_NE(pDeviceD3D11, nullptr);
    ASSERT_NE(pTextureD3D11, nullptr);

    auto* pd3d11Texture = pTextureD3D11->GetD3D11Texture();
    ASSERT_NE(pd3d11Texture, nullptr);

    const auto& SrcTexDesc = pTexture->GetDesc();

    RefCntAutoPtr<ITexture> pTextureFromNativeD3D11Handle;
    if (SrcTexDesc.Type == RESOURCE_DIM_TEX_1D ||
        SrcTexDesc.Type == RESOURCE_DIM_TEX_1D_ARRAY)
    {
        pDeviceD3D11->CreateTexture1DFromD3DResource(static_cast<ID3D11Texture1D*>(pd3d11Texture), RESOURCE_STATE_UNKNOWN, &pTextureFromNativeD3D11Handle);
    }
    else if (SrcTexDesc.Type == RESOURCE_DIM_TEX_2D ||
             SrcTexDesc.Type == RESOURCE_DIM_TEX_2D_ARRAY ||
             SrcTexDesc.Type == RESOURCE_DIM_TEX_CUBE ||
             SrcTexDesc.Type == RESOURCE_DIM_TEX_CUBE_ARRAY)
    {
        pDeviceD3D11->CreateTexture2DFromD3DResource(static_cast<ID3D11Texture2D*>(pd3d11Texture), RESOURCE_STATE_UNKNOWN, &pTextureFromNativeD3D11Handle);
    }
    else if (SrcTexDesc.Type == RESOURCE_DIM_TEX_3D)
    {
        pDeviceD3D11->CreateTexture3DFromD3DResource(static_cast<ID3D11Texture3D*>(pd3d11Texture), RESOURCE_STATE_UNKNOWN, &pTextureFromNativeD3D11Handle);
    }
    else
    {
        ADD_FAILURE();
    }

    ASSERT_NE(pTextureFromNativeD3D11Handle, nullptr);

    auto TestTexDesc = pTextureFromNativeD3D11Handle->GetDesc();
    if (SrcTexDesc.Type == RESOURCE_DIM_TEX_CUBE || SrcTexDesc.Type == RESOURCE_DIM_TEX_CUBE_ARRAY)
    {
        EXPECT_EQ(TestTexDesc.Type, RESOURCE_DIM_TEX_2D_ARRAY);
        TestTexDesc.Type = SrcTexDesc.Type;
    }
    EXPECT_EQ(TestTexDesc, SrcTexDesc) << "Src tex desc:  " << GetObjectDescString(SrcTexDesc)
                                       << "\nTest tex desc: " << GetObjectDescString(TestTexDesc);

    RefCntAutoPtr<ITextureD3D11> pTestTextureD3D11(pTextureFromNativeD3D11Handle, IID_TextureD3D11);
    ASSERT_NE(pTestTextureD3D11, nullptr);
    EXPECT_EQ(pTestTextureD3D11->GetD3D11Texture(), pd3d11Texture);
    EXPECT_EQ(pTestTextureD3D11->GetNativeHandle(), pd3d11Texture);
}

void TestCreateObjFromNativeResD3D11::CreateBuffer(IBuffer* pBuffer)
{
    RefCntAutoPtr<IRenderDeviceD3D11> pDeviceD3D11(m_pDevice, IID_RenderDeviceD3D11);
    RefCntAutoPtr<IBufferD3D11>       pBufferD3D11(pBuffer, IID_BufferD3D11);
    ASSERT_NE(pDeviceD3D11, nullptr);
    ASSERT_NE(pBufferD3D11, nullptr);

    auto* pd3d11Buffer = pBufferD3D11->GetD3D11Buffer();
    ASSERT_NE(pd3d11Buffer, nullptr);

    const auto& SrcBuffDesc = pBuffer->GetDesc();
    {
        RefCntAutoPtr<IBuffer> pBufferFromNativeD3D11Handle;
        pDeviceD3D11->CreateBufferFromD3DResource(pd3d11Buffer, SrcBuffDesc, RESOURCE_STATE_UNKNOWN, &pBufferFromNativeD3D11Handle);
        ASSERT_NE(pBufferFromNativeD3D11Handle, nullptr);

        const auto& TestBufferDesc = pBufferFromNativeD3D11Handle->GetDesc();
        EXPECT_EQ(TestBufferDesc, SrcBuffDesc);

        RefCntAutoPtr<IBufferD3D11> pTestBufferD3D11(pBufferFromNativeD3D11Handle, IID_BufferD3D11);
        EXPECT_EQ(pTestBufferD3D11->GetD3D11Buffer(), pd3d11Buffer);
        EXPECT_EQ(pTestBufferD3D11->GetNativeHandle(), pd3d11Buffer);
    }

    {
        BufferDesc BuffDesc;
        BuffDesc.Name              = "Test buffer from D3D11 buffer";
        BuffDesc.ElementByteStride = SrcBuffDesc.ElementByteStride;
        RefCntAutoPtr<IBuffer> pBufferFromNativeD3D11Handle;
        pDeviceD3D11->CreateBufferFromD3DResource(pd3d11Buffer, BuffDesc, RESOURCE_STATE_UNKNOWN, &pBufferFromNativeD3D11Handle);
        ASSERT_NE(pBufferFromNativeD3D11Handle, nullptr);

        const auto& TestBufferDesc = pBufferFromNativeD3D11Handle->GetDesc();
        EXPECT_EQ(TestBufferDesc, SrcBuffDesc) << "Src buff desc:  " << GetObjectDescString(SrcBuffDesc)
                                               << "\nTest buff desc: " << GetObjectDescString(TestBufferDesc);

        RefCntAutoPtr<IBufferD3D11> pTestBufferD3D11(pBufferFromNativeD3D11Handle, IID_BufferD3D11);
        ASSERT_NE(pTestBufferD3D11, nullptr);
        EXPECT_EQ(pTestBufferD3D11->GetD3D11Buffer(), pd3d11Buffer);
        EXPECT_EQ(pTestBufferD3D11->GetNativeHandle(), pd3d11Buffer);
    }
}

} // namespace Testing

} // namespace Diligent
