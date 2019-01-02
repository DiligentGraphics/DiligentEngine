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
#include "TestBufferCreation.h"

#if D3D11_SUPPORTED
#include "TestCreateObjFromNativeResD3D11.h"
#endif

#if D3D12_SUPPORTED
#include "TestCreateObjFromNativeResD3D12.h"
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
#include "TestCreateObjFromNativeResGL.h"
#endif

#if VULKAN_SUPPORTED
#include "TestCreateObjFromNativeResVK.h"
#endif

using namespace Diligent;

TestBufferCreation::TestBufferCreation(Diligent::IRenderDevice *pDevice, Diligent::IDeviceContext *pContext) :
    UnitTestBase("Buffer creation test")
{
    std::unique_ptr<TestCreateObjFromNativeRes> pTestCreateObjFromNativeRes;
    const auto DevCaps = pDevice->GetDeviceCaps();
    switch (DevCaps.DevType)
    {
#if D3D11_SUPPORTED
        case DeviceType::D3D11:
            pTestCreateObjFromNativeRes.reset(new TestCreateObjFromNativeResD3D11(pDevice));
        break;

#endif

#if D3D12_SUPPORTED
        case DeviceType::D3D12:
            pTestCreateObjFromNativeRes.reset(new TestCreateObjFromNativeResD3D12(pDevice));
        break;
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
        case DeviceType::OpenGL:
        case DeviceType::OpenGLES:
            pTestCreateObjFromNativeRes.reset(new TestCreateObjFromNativeResGL(pDevice));
        break;
#endif

#if VULKAN_SUPPORTED
        case DeviceType::Vulkan:
            pTestCreateObjFromNativeRes.reset(new TestCreateObjFromNativeResVK(pDevice));
            break;
#endif

        default: UNEXPECTED("Unexpected device type");
    }

    int BuffersCreated = 0;
    {
        Diligent::BufferDesc BuffDesc;
        BuffDesc.Name = "Buffer creation test 0";
        BuffDesc.uiSizeInBytes = 256;
        BuffDesc.BindFlags = BIND_VERTEX_BUFFER;
        RefCntAutoPtr<IBuffer> pBuffer;
        BufferData InitData;
        InitData.DataSize = BuffDesc.uiSizeInBytes;
        std::vector<Uint8> DummyData(InitData.DataSize);
        InitData.pData = DummyData.data();
        pDevice->CreateBuffer(BuffDesc, InitData, &pBuffer);
        VERIFY_EXPR(pBuffer);

        pTestCreateObjFromNativeRes->CreateBuffer(pBuffer);
        ++BuffersCreated;
    }

    {
        Diligent::BufferDesc BuffDesc;
        BuffDesc.Name = "Buffer creation test 1";
        BuffDesc.uiSizeInBytes = 256;
        BuffDesc.BindFlags = BIND_VERTEX_BUFFER;
        RefCntAutoPtr<IBuffer> pBuffer;
        pDevice->CreateBuffer(BuffDesc, BufferData(), &pBuffer);
        VERIFY_EXPR(pBuffer);

        pTestCreateObjFromNativeRes->CreateBuffer(pBuffer);
        ++BuffersCreated;
    }

    if(DevCaps.bComputeShadersSupported && DevCaps.bIndirectRenderingSupported)
    {
        Diligent::BufferDesc BuffDesc;
        BuffDesc.Name = "Buffer creation test 2";
        BuffDesc.uiSizeInBytes = 256;
        BuffDesc.BindFlags = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
        BuffDesc.Mode = BUFFER_MODE_FORMATTED;
        BuffDesc.ElementByteStride = 16;
        RefCntAutoPtr<IBuffer> pBuffer;
        pDevice->CreateBuffer(BuffDesc, BufferData(), &pBuffer);
        VERIFY_EXPR(pBuffer);

        BufferViewDesc ViewDesc;
        ViewDesc.ViewType = BUFFER_VIEW_SHADER_RESOURCE;
        ViewDesc.ByteOffset = 32;
        ViewDesc.Format.NumComponents = 4;
        ViewDesc.Format.ValueType = VT_FLOAT32;
        ViewDesc.Format.IsNormalized = false;
        RefCntAutoPtr<IBufferView> pBufferSRV;
        pBuffer->CreateView(ViewDesc, &pBufferSRV);

        ViewDesc.ViewType = BUFFER_VIEW_UNORDERED_ACCESS;
        RefCntAutoPtr<IBufferView> pBufferUAV;
        pBuffer->CreateView(ViewDesc, &pBufferUAV);

        pTestCreateObjFromNativeRes->CreateBuffer(pBuffer);
        ++BuffersCreated;
    }

    if(DevCaps.bComputeShadersSupported)
    {
        Diligent::BufferDesc BuffDesc;
        BuffDesc.Name = "Buffer creation test 3";
        BuffDesc.uiSizeInBytes = 256;
        BuffDesc.BindFlags =  BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
        BuffDesc.Mode = BUFFER_MODE_STRUCTURED;
        BuffDesc.ElementByteStride = 16;
        RefCntAutoPtr<IBuffer> pBuffer;
        pDevice->CreateBuffer(BuffDesc, BufferData(), &pBuffer);
        VERIFY_EXPR(pBuffer);

        pTestCreateObjFromNativeRes->CreateBuffer(pBuffer);
        ++BuffersCreated;
    }

    {
        Diligent::BufferDesc BuffDesc;
        BuffDesc.Name = "Buffer creation test 4";
        BuffDesc.uiSizeInBytes = 256;
        BuffDesc.BindFlags =  BIND_UNIFORM_BUFFER;
        RefCntAutoPtr<IBuffer> pBuffer;
        pDevice->CreateBuffer(BuffDesc, BufferData(), &pBuffer);
        VERIFY_EXPR(pBuffer);

        pTestCreateObjFromNativeRes->CreateBuffer(pBuffer);
        ++BuffersCreated;
    }

    {
        Diligent::BufferDesc BuffDesc;
        BuffDesc.Name = "Buffer creation test 5";
        BuffDesc.uiSizeInBytes = 256;
        BuffDesc.BindFlags =  BIND_VERTEX_BUFFER | BIND_INDEX_BUFFER | BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
        BuffDesc.Mode = BUFFER_MODE_RAW;
        BuffDesc.ElementByteStride = 16;
        RefCntAutoPtr<IBuffer> pBuffer;
        pDevice->CreateBuffer(BuffDesc, BufferData(), &pBuffer);
        VERIFY_EXPR(pBuffer);

        BufferViewDesc ViewDesc;
        ViewDesc.ViewType = BUFFER_VIEW_UNORDERED_ACCESS;
        ViewDesc.ByteOffset = 32;
        ViewDesc.Format.NumComponents = 4;
        ViewDesc.Format.ValueType = VT_FLOAT32;
        RefCntAutoPtr<IBufferView> pBufferUAV;
        pBuffer->CreateView(ViewDesc, &pBufferUAV);

        ViewDesc.ViewType = BUFFER_VIEW_SHADER_RESOURCE;
        RefCntAutoPtr<IBufferView> pBufferSRV;
        pBuffer->CreateView(ViewDesc, &pBufferSRV);

        pTestCreateObjFromNativeRes->CreateBuffer(pBuffer);
        ++BuffersCreated;
    }

    std::stringstream infoss;
    infoss << "Created " << BuffersCreated << " test buffers.";
    SetStatus(TestResult::Succeeded, infoss.str().c_str());
}
