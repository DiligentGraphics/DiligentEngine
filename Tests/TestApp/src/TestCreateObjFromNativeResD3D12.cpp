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
#include "TestCreateObjFromNativeResD3D12.h"
#include <d3d12.h>
#include "RenderDeviceD3D12.h"
#include "TextureD3D12.h"
#include "BufferD3D12.h"

using namespace Diligent;

void TestCreateObjFromNativeResD3D12::CreateTexture(Diligent::ITexture *pTexture)
{
    RefCntAutoPtr<IRenderDeviceD3D12> pDeviceD3D12(m_pDevice, IID_RenderDeviceD3D12);
    const auto &SrcTexDesc = pTexture->GetDesc();
    RefCntAutoPtr<ITextureD3D12> pTextureD3D12(pTexture, IID_TextureD3D12);
    auto *pD3D12Texture = pTextureD3D12->GetD3D12Texture();
    RefCntAutoPtr<ITexture> pTextureFromNativeD3D12Handle;
    pDeviceD3D12->CreateTextureFromD3DResource(pD3D12Texture, &pTextureFromNativeD3D12Handle);
    ++m_NumTexturesCreated;
    
    auto TestTexDesc = pTextureFromNativeD3D12Handle->GetDesc();
    if (SrcTexDesc.Type == RESOURCE_DIM_TEX_CUBE || SrcTexDesc.Type == RESOURCE_DIM_TEX_CUBE_ARRAY)
    {
        VERIFY_EXPR(TestTexDesc.Type == RESOURCE_DIM_TEX_2D_ARRAY);
        TestTexDesc.Type = SrcTexDesc.Type;
    }
    VERIFY_EXPR(TestTexDesc == SrcTexDesc);

    RefCntAutoPtr<ITextureD3D12> pTestTextureD3D12(pTextureFromNativeD3D12Handle, IID_TextureD3D12);
    VERIFY_EXPR(pTestTextureD3D12->GetD3D12Texture() == pD3D12Texture);
    VERIFY_EXPR(pTestTextureD3D12->GetNativeHandle() == pD3D12Texture);
}

void TestCreateObjFromNativeResD3D12::CreateBuffer(Diligent::IBuffer *pBuffer)
{
    RefCntAutoPtr<IRenderDeviceD3D12> pDeviceD3D12(m_pDevice, IID_RenderDeviceD3D12);
    const auto &SrcBuffDesc = pBuffer->GetDesc();
    RefCntAutoPtr<IBufferD3D12> pBufferD3D12(pBuffer, IID_BufferD3D12);
    size_t DataStartByteOffset;
    auto *pD3D12Buffer = pBufferD3D12->GetD3D12Buffer(DataStartByteOffset, 0);
    VERIFY_EXPR(DataStartByteOffset == 0);
    
    {
        RefCntAutoPtr<IBuffer> pBufferFromNativeD3D12Handle;
        pDeviceD3D12->CreateBufferFromD3DResource(pD3D12Buffer, SrcBuffDesc, &pBufferFromNativeD3D12Handle);
        ++m_NumBuffersCreated;
        
        const auto &TestBufferDesc = pBufferFromNativeD3D12Handle->GetDesc();
        VERIFY_EXPR(TestBufferDesc == SrcBuffDesc);

        RefCntAutoPtr<IBufferD3D12> pTestBufferD3D12(pBufferFromNativeD3D12Handle, IID_BufferD3D12);
        size_t TestBuffDataStartByteOffset;
        VERIFY_EXPR(pTestBufferD3D12->GetD3D12Buffer(TestBuffDataStartByteOffset, 0) == pD3D12Buffer);
        VERIFY_EXPR(TestBuffDataStartByteOffset == 0);
        VERIFY_EXPR(pTestBufferD3D12->GetNativeHandle() == pD3D12Buffer);
    }
}
