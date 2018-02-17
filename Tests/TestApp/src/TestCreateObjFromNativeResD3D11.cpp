/*     Copyright 2015-2017 Egor Yusov
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
#include "TestCreateObjFromNativeResD3D11.h"

#include <d3d11.h>
#include "RenderDeviceD3D11.h"
#include "TextureD3D11.h"
#include "BufferD3D11.h"

using namespace Diligent;

void TestCreateObjFromNativeResD3D11::CreateTexture(Diligent::ITexture *pTexture)
{
    RefCntAutoPtr<IRenderDeviceD3D11> pDeviceD3D11(m_pDevice, IID_RenderDeviceD3D11);
    const auto &SrcTexDesc = pTexture->GetDesc();
    RefCntAutoPtr<ITextureD3D11> pTextureD3D11(pTexture, IID_TextureD3D11);
    auto *pd3d11Texture = pTextureD3D11->GetD3D11Texture();
    RefCntAutoPtr<ITexture> pTextureFromNativeD3D11Handle;
    if (SrcTexDesc.Type == RESOURCE_DIM_TEX_1D || SrcTexDesc.Type == RESOURCE_DIM_TEX_1D_ARRAY)
    {
        pDeviceD3D11->CreateTextureFromD3DResource(static_cast<ID3D11Texture1D*>(pd3d11Texture), &pTextureFromNativeD3D11Handle);
        ++m_NumTexturesCreated;
    }
    else if (SrcTexDesc.Type == RESOURCE_DIM_TEX_2D || SrcTexDesc.Type == RESOURCE_DIM_TEX_2D_ARRAY || 
             SrcTexDesc.Type == RESOURCE_DIM_TEX_CUBE || SrcTexDesc.Type == RESOURCE_DIM_TEX_CUBE_ARRAY)
    {
        pDeviceD3D11->CreateTextureFromD3DResource(static_cast<ID3D11Texture2D*>(pd3d11Texture), &pTextureFromNativeD3D11Handle);
        ++m_NumTexturesCreated;
    }
    else if (RESOURCE_DIM_TEX_3D)
    {
        pDeviceD3D11->CreateTextureFromD3DResource(static_cast<ID3D11Texture3D*>(pd3d11Texture), &pTextureFromNativeD3D11Handle);
        ++m_NumTexturesCreated;
    }
    else
    {
        UNEXPECTED("Unexpected texture dimensions");
    }

    auto TestTexDesc = pTextureFromNativeD3D11Handle->GetDesc();
    if (SrcTexDesc.Type == RESOURCE_DIM_TEX_CUBE || SrcTexDesc.Type == RESOURCE_DIM_TEX_CUBE_ARRAY)
    {
        VERIFY_EXPR(TestTexDesc.Type == RESOURCE_DIM_TEX_2D_ARRAY);
        TestTexDesc.Type = SrcTexDesc.Type;
    }
    VERIFY_EXPR(TestTexDesc == SrcTexDesc);
    RefCntAutoPtr<ITextureD3D11> pTestTextureD3D11(pTextureFromNativeD3D11Handle, IID_TextureD3D11);
    VERIFY_EXPR(pTestTextureD3D11->GetD3D11Texture() == pd3d11Texture);
    VERIFY_EXPR(pTestTextureD3D11->GetNativeHandle() == pd3d11Texture);
}

void TestCreateObjFromNativeResD3D11::CreateBuffer(Diligent::IBuffer *pBuffer)
{
    RefCntAutoPtr<IRenderDeviceD3D11> pDeviceD3D11(m_pDevice, IID_RenderDeviceD3D11);
    const auto &SrcBuffDesc = pBuffer->GetDesc();
    RefCntAutoPtr<IBufferD3D11> pBufferD3D11(pBuffer, IID_BufferD3D11);
    auto *pd3d11Buffer = pBufferD3D11->GetD3D11Buffer();
    
    {
        RefCntAutoPtr<IBuffer> pBufferFromNativeD3D11Handle;
        pDeviceD3D11->CreateBufferFromD3DResource(pd3d11Buffer, SrcBuffDesc, &pBufferFromNativeD3D11Handle);
        ++m_NumBuffersCreated;
        
        const auto &TestBufferDesc = pBufferFromNativeD3D11Handle->GetDesc();
        VERIFY_EXPR(TestBufferDesc == SrcBuffDesc);

        RefCntAutoPtr<IBufferD3D11> pTestBufferD3D11(pBufferFromNativeD3D11Handle, IID_BufferD3D11);
        VERIFY_EXPR(pTestBufferD3D11->GetD3D11Buffer() == pd3d11Buffer);
        VERIFY_EXPR(pTestBufferD3D11->GetNativeHandle() == pd3d11Buffer);
    }

    {
        BufferDesc BuffDesc;
        BuffDesc.Name = "Test buffer from D3D11 buffer";
        BuffDesc.Format = SrcBuffDesc.Format;
        RefCntAutoPtr<IBuffer> pBufferFromNativeD3D11Handle;
        pDeviceD3D11->CreateBufferFromD3DResource(pd3d11Buffer, BuffDesc, &pBufferFromNativeD3D11Handle);
        ++m_NumBuffersCreated;
        
        const auto &TestBufferDesc = pBufferFromNativeD3D11Handle->GetDesc();
        VERIFY_EXPR(TestBufferDesc == SrcBuffDesc);

        RefCntAutoPtr<IBufferD3D11> pTestBufferD3D11(pBufferFromNativeD3D11Handle, IID_BufferD3D11);
        VERIFY_EXPR(pTestBufferD3D11->GetD3D11Buffer() == pd3d11Buffer);
        VERIFY_EXPR(pTestBufferD3D11->GetNativeHandle() == pd3d11Buffer);
    }
}
