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

#include <stdlib.h>

#include "RenderDevice.h"

int TestObjectCInterface(struct IObject* pObject);

int TestRenderDeviceCInterface_Misc(struct IRenderDevice* pRenderDevice)
{
    IObject*                  pUnknown = NULL;
    ReferenceCounterValueType RefCnt1 = 0, RefCnt2 = 0;
    DeviceCaps                deviceCaps;
    TextureFormatInfo         TexFmtInfo;
    TextureFormatInfoExt      TexFmtInfoExt;
    IEngineFactory*           pFactory = NULL;

    int num_errors = TestObjectCInterface((struct IObject*)pRenderDevice);

    IObject_QueryInterface(pRenderDevice, &IID_Unknown, &pUnknown);
    if (pUnknown != NULL)
        IObject_Release(pUnknown);
    else
        ++num_errors;

    RefCnt1 = IObject_AddRef(pRenderDevice);
    if (RefCnt1 <= 1)
        ++num_errors;
    RefCnt2 = IObject_Release(pRenderDevice);
    if (RefCnt2 <= 0)
        ++num_errors;
    if (RefCnt2 != RefCnt1 - 1)
        ++num_errors;

    deviceCaps = *IRenderDevice_GetDeviceCaps(pRenderDevice);
    if (deviceCaps.DevType == RENDER_DEVICE_TYPE_UNDEFINED)
        ++num_errors;

    TexFmtInfo = *IRenderDevice_GetTextureFormatInfo(pRenderDevice, TEX_FORMAT_RGBA8_UNORM);
    if (TexFmtInfo._TextureFormatAttribs.Format != TEX_FORMAT_RGBA8_UNORM)
        ++num_errors;

    TexFmtInfoExt = *IRenderDevice_GetTextureFormatInfoExt(pRenderDevice, TEX_FORMAT_RGBA8_UNORM);
    if (TexFmtInfoExt._TextureFormatInfo._TextureFormatAttribs.Format != TEX_FORMAT_RGBA8_UNORM)
        ++num_errors;

    IRenderDevice_IdleGPU(pRenderDevice);
    IRenderDevice_ReleaseStaleResources(pRenderDevice, false);

    pFactory = IRenderDevice_GetEngineFactory(pRenderDevice);
    if (pFactory == NULL)
        ++num_errors;

    return num_errors;
}

int TestRenderDeviceCInterface_CreateBuffer(struct IRenderDevice* pRenderDevice)
{
    struct BufferDesc BuffDesc;
    struct BufferData InitData;
    struct IBuffer*   pBuffer = NULL;

    int num_errors = 0;

    memset(&BuffDesc, 0, sizeof(BuffDesc));
    memset(&InitData, 0, sizeof(InitData));

    BuffDesc._DeviceObjectAttribs.Name = "Vertex buffer";

    BuffDesc.uiSizeInBytes    = 256;
    BuffDesc.BindFlags        = BIND_VERTEX_BUFFER;
    BuffDesc.Usage            = USAGE_DEFAULT;
    BuffDesc.CommandQueueMask = 1;

    InitData.DataSize = BuffDesc.uiSizeInBytes;
    void* pData       = malloc(InitData.DataSize);
    memset(pData, 0, InitData.DataSize);
    InitData.pData = pData;
    IRenderDevice_CreateBuffer(pRenderDevice, &BuffDesc, &InitData, &pBuffer);
    if (pBuffer != NULL)
        IObject_Release(pBuffer);
    else
        ++num_errors;

    free(pData);

    return num_errors;
}

int TestRenderDeviceCInterface_CreateShader(struct IRenderDevice* pRenderDevice)
{
    static const char*      ShaderSource = "float4 main() : SV_Target {return float4(0.0, 0.0, 0.0, 0.0);}";
    struct ShaderCreateInfo ShaderCI;
    struct IShader*         pShader = NULL;

    int num_errors = 0;

    memset(&ShaderCI, 0, sizeof(ShaderCI));
    ShaderCI.Desc._DeviceObjectAttribs.Name = "Test shader";
    ShaderCI.Desc.ShaderType                = SHADER_TYPE_PIXEL;

    ShaderCI.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.UseCombinedTextureSamplers = true;
    ShaderCI.EntryPoint                 = "main";
    ShaderCI.Source                     = ShaderSource;

    IRenderDevice_CreateShader(pRenderDevice, &ShaderCI, &pShader);
    if (pShader != NULL)
        IObject_Release(pShader);
    else
        ++num_errors;

    return num_errors;
}

int TestRenderDeviceCInterface_CreateTexture(struct IRenderDevice* pRenderDevice)
{
    TextureDesc       TexDesc;
    TextureData       InitData;
    TextureSubResData Mip0Data;

    ITexture* pTexture = NULL;

    int num_errors = 0;

    memset(&TexDesc, 0, sizeof(TexDesc));
    memset(&InitData, 0, sizeof(InitData));
    memset(&Mip0Data, 0, sizeof(Mip0Data));

    TexDesc._DeviceObjectAttribs.Name = "Test Texture";
    TexDesc.Type                      = RESOURCE_DIM_TEX_2D;
    TexDesc.Width                     = 512;
    TexDesc.Height                    = 512;
    TexDesc.MipLevels                 = 1;
    TexDesc.SampleCount               = 1;
    TexDesc.ArraySize                 = 1;
    TexDesc.Format                    = TEX_FORMAT_RGBA8_UNORM;
    TexDesc.Usage                     = USAGE_DEFAULT;
    TexDesc.BindFlags                 = BIND_SHADER_RESOURCE;
    TexDesc.CommandQueueMask          = 1;

    int   DataSize = TexDesc.Width * TexDesc.Height * 4;
    void* pData    = malloc(DataSize);
    memset(pData, 0, DataSize);
    Mip0Data.pData  = pData;
    Mip0Data.Stride = TexDesc.Width * 4;

    InitData.NumSubresources = 1;
    InitData.pSubResources   = &Mip0Data;

    IRenderDevice_CreateTexture(pRenderDevice, &TexDesc, &InitData, &pTexture);
    if (pTexture != NULL)
        IObject_Release(pTexture);
    else
        ++num_errors;

    free(pData);

    return num_errors;
}

int TestRenderDeviceCInterface_CreateSampler(struct IRenderDevice* pRenderDevice)
{
    struct SamplerDesc SamDesc;
    struct ISampler*   pSampler = NULL;

    int num_errors = 0;

    memset(&SamDesc, 0, sizeof(SamDesc));
    SamDesc.AddressU       = TEXTURE_ADDRESS_WRAP;
    SamDesc.AddressV       = TEXTURE_ADDRESS_WRAP;
    SamDesc.AddressW       = TEXTURE_ADDRESS_WRAP;
    SamDesc.MinFilter      = FILTER_TYPE_LINEAR;
    SamDesc.MagFilter      = FILTER_TYPE_LINEAR;
    SamDesc.MipFilter      = FILTER_TYPE_LINEAR;
    SamDesc.ComparisonFunc = COMPARISON_FUNC_NEVER;

    IRenderDevice_CreateSampler(pRenderDevice, &SamDesc, &pSampler);
    if (pSampler != NULL)
        IObject_Release(pSampler);
    else
        ++num_errors;

    return num_errors;
}

int TestRenderDeviceCInterface_CreateResourceMapping(struct IRenderDevice* pRenderDevice)
{
    struct ResourceMappingDesc ResMappingDesc;
    struct IResourceMapping*   pResMapping = NULL;

    int num_errors = 0;

    memset(&ResMappingDesc, 0, sizeof(ResMappingDesc));

    IRenderDevice_CreateResourceMapping(pRenderDevice, &ResMappingDesc, &pResMapping);
    if (pResMapping != NULL)
        IObject_Release(pResMapping);
    else
        ++num_errors;

    return num_errors;
}

int TestRenderDeviceCInterface_CreateGraphicsPipelineState(struct IRenderDevice* pRenderDevice, struct GraphicsPipelineStateCreateInfo* pPSOCreateInfo)
{
    struct IPipelineState* pPSO = NULL;

    int num_errors = 0;

    IRenderDevice_CreateGraphicsPipelineState(pRenderDevice, pPSOCreateInfo, &pPSO);
    if (pPSO != NULL)
        IObject_Release(pPSO);
    else
        ++num_errors;

    return num_errors;
}


int TestRenderDeviceCInterface_CreateFence(struct IRenderDevice* pRenderDevice)
{
    struct FenceDesc fenceDesc;
    struct IFence*   pFence = NULL;

    int num_errors = 0;

    memset(&fenceDesc, 0, sizeof(fenceDesc));
    fenceDesc._DeviceObjectAttribs.Name = "Test Fence";

    IRenderDevice_CreateFence(pRenderDevice, &fenceDesc, &pFence);
    if (pFence != NULL)
        IObject_Release(pFence);
    else
        ++num_errors;

    return num_errors;
}


int TestRenderDeviceCInterface_CreateQuery(struct IRenderDevice* pRenderDevice)
{
    struct QueryDesc  queryDesc;
    struct IQuery*    pQuery = NULL;
    struct DeviceCaps deviceCaps;

    int num_errors = 0;

    deviceCaps = *IRenderDevice_GetDeviceCaps(pRenderDevice);
    if (deviceCaps.Features.TimestampQueries)
    {
        memset(&queryDesc, 0, sizeof(queryDesc));
        queryDesc._DeviceObjectAttribs.Name = "Test Query";
        queryDesc.Type                      = QUERY_TYPE_TIMESTAMP;

        IRenderDevice_CreateQuery(pRenderDevice, &queryDesc, &pQuery);
        if (pQuery != NULL)
            IObject_Release(pQuery);
        else
            ++num_errors;
    }

    return num_errors;
}
