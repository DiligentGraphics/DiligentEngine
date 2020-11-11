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

#include "pch.h"
#include "RenderDeviceD3D11Impl.hpp"
#include "DeviceContextD3D11Impl.hpp"
#include "BufferD3D11Impl.hpp"
#include "ShaderD3D11Impl.hpp"
#include "Texture1D_D3D11.hpp"
#include "Texture2D_D3D11.hpp"
#include "Texture3D_D3D11.hpp"
#include "SamplerD3D11Impl.hpp"
#include "D3D11TypeConversions.hpp"
#include "TextureViewD3D11Impl.hpp"
#include "PipelineStateD3D11Impl.hpp"
#include "ShaderResourceBindingD3D11Impl.hpp"
#include "FenceD3D11Impl.hpp"
#include "QueryD3D11Impl.hpp"
#include "RenderPassD3D11Impl.hpp"
#include "FramebufferD3D11Impl.hpp"
#include "EngineMemory.h"

namespace Diligent
{

static CComPtr<IDXGIAdapter1> DXGIAdapterFromD3D11Device(ID3D11Device* pd3d11Device)
{
    CComPtr<IDXGIDevice> pDXGIDevice;

    auto hr = pd3d11Device->QueryInterface(__uuidof(pDXGIDevice), reinterpret_cast<void**>(static_cast<IDXGIDevice**>(&pDXGIDevice)));
    if (SUCCEEDED(hr))
    {
        CComPtr<IDXGIAdapter> pDXGIAdapter;
        hr = pDXGIDevice->GetAdapter(&pDXGIAdapter);
        if (SUCCEEDED(hr))
        {
            CComPtr<IDXGIAdapter1> pDXGIAdapter1;
            pDXGIAdapter.QueryInterface(&pDXGIAdapter1);
            return pDXGIAdapter1;
        }
        else
        {
            LOG_ERROR("Failed to get DXGI Adapter from DXGI Device.");
        }
    }
    else
    {
        LOG_ERROR("Failed to query IDXGIDevice from D3D device.");
    }

    return nullptr;
}

RenderDeviceD3D11Impl::RenderDeviceD3D11Impl(IReferenceCounters*          pRefCounters,
                                             IMemoryAllocator&            RawMemAllocator,
                                             IEngineFactory*              pEngineFactory,
                                             const EngineD3D11CreateInfo& EngineAttribs,
                                             ID3D11Device*                pd3d11Device,
                                             Uint32                       NumDeferredContexts) :
    // clang-format off
    TRenderDeviceBase
    {
        pRefCounters,
        RawMemAllocator,
        pEngineFactory,
        NumDeferredContexts,
        DeviceObjectSizes
        {
            sizeof(TextureBaseD3D11),
            sizeof(TextureViewD3D11Impl),
            sizeof(BufferD3D11Impl),
            sizeof(BufferViewD3D11Impl),
            sizeof(ShaderD3D11Impl),
            sizeof(SamplerD3D11Impl),
            sizeof(PipelineStateD3D11Impl),
            sizeof(ShaderResourceBindingD3D11Impl),
            sizeof(FenceD3D11Impl),
            sizeof(QueryD3D11Impl),
            sizeof(RenderPassD3D11Impl),
            sizeof(FramebufferD3D11Impl)
        }
    },
    m_EngineAttribs{EngineAttribs},
    m_pd3d11Device {pd3d11Device }
// clang-format on
{
    m_DeviceCaps.DevType = RENDER_DEVICE_TYPE_D3D11;
    auto FeatureLevel    = m_pd3d11Device->GetFeatureLevel();
    switch (FeatureLevel)
    {
        case D3D_FEATURE_LEVEL_11_0:
        case D3D_FEATURE_LEVEL_11_1:
            m_DeviceCaps.MajorVersion = 11;
            m_DeviceCaps.MinorVersion = FeatureLevel == D3D_FEATURE_LEVEL_11_1 ? 1 : 0;
            break;

        case D3D_FEATURE_LEVEL_10_0:
        case D3D_FEATURE_LEVEL_10_1:
            m_DeviceCaps.MajorVersion = 10;
            m_DeviceCaps.MinorVersion = FeatureLevel == D3D_FEATURE_LEVEL_10_1 ? 1 : 0;
            break;

        default:
            UNEXPECTED("Unexpected D3D feature level");
    }

    if (auto pDXGIAdapter1 = DXGIAdapterFromD3D11Device(pd3d11Device))
    {
        ReadAdapterInfo(pDXGIAdapter1);
    }

#define UNSUPPORTED_FEATURE(Feature, Name)                                   \
    do                                                                       \
    {                                                                        \
        if (EngineAttribs.Features.Feature == DEVICE_FEATURE_STATE_ENABLED)  \
            LOG_ERROR_AND_THROW(Name " not supported by Direct3D11 device"); \
        m_DeviceCaps.Features.Feature = DEVICE_FEATURE_STATE_DISABLED;       \
    } while (false)

    // Direct3D11 only supports shader model 5.0 even if the device feature level is
    // above 11.0 (for example, 11.1 or 12.0), so bindless resources are never available.
    // https://docs.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-devices-downlevel-intro#overview-for-each-feature-level
    UNSUPPORTED_FEATURE(BindlessResources, "Bindless resources are");
    UNSUPPORTED_FEATURE(VertexPipelineUAVWritesAndAtomics, "Vertex pipeline UAV writes and atomics are");
    UNSUPPORTED_FEATURE(MeshShaders, "Mesh shaders are");

    {
        bool ShaderFloat16Supported = false;

        D3D11_FEATURE_DATA_SHADER_MIN_PRECISION_SUPPORT d3d11MinPrecisionSupport = {};
        if (SUCCEEDED(m_pd3d11Device->CheckFeatureSupport(D3D11_FEATURE_SHADER_MIN_PRECISION_SUPPORT, &d3d11MinPrecisionSupport, sizeof(d3d11MinPrecisionSupport))))
        {
            ShaderFloat16Supported =
                (d3d11MinPrecisionSupport.PixelShaderMinPrecision & D3D11_SHADER_MIN_PRECISION_16_BIT) != 0 &&
                (d3d11MinPrecisionSupport.AllOtherShaderStagesMinPrecision & D3D11_SHADER_MIN_PRECISION_16_BIT) != 0;
        }
        if (EngineAttribs.Features.ShaderFloat16 == DEVICE_FEATURE_STATE_ENABLED && !ShaderFloat16Supported)
            LOG_ERROR_AND_THROW("16-bit float shader operations are");
        m_DeviceCaps.Features.ShaderFloat16 = ShaderFloat16Supported ? DEVICE_FEATURE_STATE_ENABLED : DEVICE_FEATURE_STATE_DISABLED;
    }

    // Explicit fp16 is only supported in DXC through Shader Model 6.2, so there's no support for FXC or D3D11.
    // clang-format off
    UNSUPPORTED_FEATURE(ResourceBuffer16BitAccess, "16-bit native access to resource buffers is");
    UNSUPPORTED_FEATURE(UniformBuffer16BitAccess,  "16-bit native access to uniform buffers is");
    UNSUPPORTED_FEATURE(ShaderInputOutput16,       "16-bit shader input/output is");

    UNSUPPORTED_FEATURE(ShaderInt8,               "Native 8-bit shader operations are");
    UNSUPPORTED_FEATURE(ResourceBuffer8BitAccess, "8-bit native access to resource buffers is");
    UNSUPPORTED_FEATURE(UniformBuffer8BitAccess,  "8-bit native access to uniform buffers is");
    // clang-format on
#undef UNSUPPORTED_FEATURE

#if defined(_MSC_VER) && defined(_WIN64)
    static_assert(sizeof(DeviceFeatures) == 31, "Did you add a new feature to DeviceFeatures? Please handle its satus here.");
#endif

    auto& TexCaps = m_DeviceCaps.TexCaps;

    TexCaps.MaxTexture1DDimension     = D3D11_REQ_TEXTURE1D_U_DIMENSION;
    TexCaps.MaxTexture1DArraySlices   = D3D11_REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION;
    TexCaps.MaxTexture2DDimension     = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
    TexCaps.MaxTexture2DArraySlices   = D3D11_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
    TexCaps.MaxTexture3DDimension     = D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
    TexCaps.MaxTextureCubeDimension   = D3D11_REQ_TEXTURECUBE_DIMENSION;
    TexCaps.Texture2DMSSupported      = True;
    TexCaps.Texture2DMSArraySupported = True;
    TexCaps.TextureViewSupported      = True;
    TexCaps.CubemapArraysSupported    = True;

    auto& SamCaps = m_DeviceCaps.SamCaps;

    SamCaps.BorderSamplingModeSupported   = True;
    SamCaps.AnisotropicFilteringSupported = True;
    SamCaps.LODBiasSupported              = True;
}

void RenderDeviceD3D11Impl::TestTextureFormat(TEXTURE_FORMAT TexFormat)
{
    auto& TexFormatInfo = m_TextureFormatsInfo[TexFormat];
    VERIFY(TexFormatInfo.Supported, "Texture format is not supported");

    auto DXGIFormat = TexFormatToDXGI_Format(TexFormat);

    UINT FormatSupport = 0;
    auto hr            = m_pd3d11Device->CheckFormatSupport(DXGIFormat, &FormatSupport);
    if (FAILED(hr))
    {
        LOG_ERROR_MESSAGE("CheckFormatSupport() failed for format ", DXGIFormat);
        return;
    }

    TexFormatInfo.Filterable =
        ((FormatSupport & D3D11_FORMAT_SUPPORT_SHADER_SAMPLE) != 0) ||
        ((FormatSupport & D3D11_FORMAT_SUPPORT_SHADER_SAMPLE_COMPARISON) != 0);

    TexFormatInfo.BindFlags = BIND_SHADER_RESOURCE;
    if ((FormatSupport & D3D11_FORMAT_SUPPORT_RENDER_TARGET) != 0)
        TexFormatInfo.BindFlags |= BIND_RENDER_TARGET;
    if ((FormatSupport & D3D11_FORMAT_SUPPORT_DEPTH_STENCIL) != 0)
        TexFormatInfo.BindFlags |= BIND_DEPTH_STENCIL;
    if ((FormatSupport & D3D11_FORMAT_SUPPORT_TYPED_UNORDERED_ACCESS_VIEW) != 0)
        TexFormatInfo.BindFlags |= BIND_UNORDERED_ACCESS;

    TexFormatInfo.Dimensions = RESOURCE_DIMENSION_SUPPORT_NONE;
    if ((FormatSupport & D3D11_FORMAT_SUPPORT_TEXTURE1D) != 0)
        TexFormatInfo.Dimensions |= RESOURCE_DIMENSION_SUPPORT_TEX_1D | RESOURCE_DIMENSION_SUPPORT_TEX_1D_ARRAY;
    if ((FormatSupport & D3D11_FORMAT_SUPPORT_TEXTURE2D) != 0)
        TexFormatInfo.Dimensions |= RESOURCE_DIMENSION_SUPPORT_TEX_2D | RESOURCE_DIMENSION_SUPPORT_TEX_2D_ARRAY;
    if ((FormatSupport & D3D11_FORMAT_SUPPORT_TEXTURE3D) != 0)
        TexFormatInfo.Dimensions |= RESOURCE_DIMENSION_SUPPORT_TEX_3D;
    if ((FormatSupport & D3D11_FORMAT_SUPPORT_TEXTURECUBE) != 0)
        TexFormatInfo.Dimensions |= RESOURCE_DIMENSION_SUPPORT_TEX_CUBE | RESOURCE_DIMENSION_SUPPORT_TEX_CUBE_ARRAY;

    TexFormatInfo.SampleCounts = 0x0;
    for (Uint32 SampleCount = 1; SampleCount <= D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT; SampleCount *= 2)
    {
        UINT QualityLevels = 0;

        hr = m_pd3d11Device->CheckMultisampleQualityLevels(DXGIFormat, SampleCount, &QualityLevels);
        if (SUCCEEDED(hr) && QualityLevels > 0)
            TexFormatInfo.SampleCounts |= SampleCount;
    }
}

IMPLEMENT_QUERY_INTERFACE(RenderDeviceD3D11Impl, IID_RenderDeviceD3D11, TRenderDeviceBase)

void RenderDeviceD3D11Impl::CreateBufferFromD3DResource(ID3D11Buffer* pd3d11Buffer, const BufferDesc& BuffDesc, RESOURCE_STATE InitialState, IBuffer** ppBuffer)
{
    CreateDeviceObject("buffer", BuffDesc, ppBuffer,
                       [&]() //
                       {
                           BufferD3D11Impl* pBufferD3D11{NEW_RC_OBJ(m_BufObjAllocator, "BufferD3D11Impl instance", BufferD3D11Impl)(m_BuffViewObjAllocator, this, BuffDesc, InitialState, pd3d11Buffer)};
                           pBufferD3D11->QueryInterface(IID_Buffer, reinterpret_cast<IObject**>(ppBuffer));
                           pBufferD3D11->CreateDefaultViews();
                           OnCreateDeviceObject(pBufferD3D11);
                       });
}

void RenderDeviceD3D11Impl::CreateBuffer(const BufferDesc& BuffDesc, const BufferData* pBuffData, IBuffer** ppBuffer)
{
    CreateDeviceObject("buffer", BuffDesc, ppBuffer,
                       [&]() //
                       {
                           BufferD3D11Impl* pBufferD3D11{NEW_RC_OBJ(m_BufObjAllocator, "BufferD3D11Impl instance", BufferD3D11Impl)(m_BuffViewObjAllocator, this, BuffDesc, pBuffData)};
                           pBufferD3D11->QueryInterface(IID_Buffer, reinterpret_cast<IObject**>(ppBuffer));
                           pBufferD3D11->CreateDefaultViews();
                           OnCreateDeviceObject(pBufferD3D11);
                       });
}

void RenderDeviceD3D11Impl::CreateShader(const ShaderCreateInfo& ShaderCI, IShader** ppShader)
{
    CreateDeviceObject("shader", ShaderCI.Desc, ppShader,
                       [&]() //
                       {
                           ShaderD3D11Impl* pShaderD3D11{NEW_RC_OBJ(m_ShaderObjAllocator, "ShaderD3D11Impl instance", ShaderD3D11Impl)(this, ShaderCI)};
                           pShaderD3D11->QueryInterface(IID_Shader, reinterpret_cast<IObject**>(ppShader));

                           OnCreateDeviceObject(pShaderD3D11);
                       });
}

void RenderDeviceD3D11Impl::CreateTexture1DFromD3DResource(ID3D11Texture1D* pd3d11Texture, RESOURCE_STATE InitialState, ITexture** ppTexture)
{
    if (pd3d11Texture == nullptr)
        return;

    TextureDesc TexDesc;
    TexDesc.Name = "Texture1D from native d3d11 texture";
    CreateDeviceObject("texture", TexDesc, ppTexture,
                       [&]() //
                       {
                           TextureBaseD3D11* pTextureD3D11{NEW_RC_OBJ(m_TexObjAllocator, "Texture1D_D3D11 instance", Texture1D_D3D11)(m_TexViewObjAllocator, this, InitialState, pd3d11Texture)};
                           pTextureD3D11->QueryInterface(IID_Texture, reinterpret_cast<IObject**>(ppTexture));
                           pTextureD3D11->CreateDefaultViews();
                           OnCreateDeviceObject(pTextureD3D11);
                       });
}

void RenderDeviceD3D11Impl::CreateTexture2DFromD3DResource(ID3D11Texture2D* pd3d11Texture, RESOURCE_STATE InitialState, ITexture** ppTexture)
{
    if (pd3d11Texture == nullptr)
        return;

    TextureDesc TexDesc;
    TexDesc.Name = "Texture2D from native d3d11 texture";
    CreateDeviceObject("texture", TexDesc, ppTexture,
                       [&]() //
                       {
                           TextureBaseD3D11* pTextureD3D11{NEW_RC_OBJ(m_TexObjAllocator, "Texture2D_D3D11 instance", Texture2D_D3D11)(m_TexViewObjAllocator, this, InitialState, pd3d11Texture)};
                           pTextureD3D11->QueryInterface(IID_Texture, reinterpret_cast<IObject**>(ppTexture));
                           pTextureD3D11->CreateDefaultViews();
                           OnCreateDeviceObject(pTextureD3D11);
                       });
}

void RenderDeviceD3D11Impl::CreateTexture3DFromD3DResource(ID3D11Texture3D* pd3d11Texture, RESOURCE_STATE InitialState, ITexture** ppTexture)
{
    if (pd3d11Texture == nullptr)
        return;

    TextureDesc TexDesc;
    TexDesc.Name = "Texture3D from native d3d11 texture";
    CreateDeviceObject("texture", TexDesc, ppTexture,
                       [&]() //
                       {
                           TextureBaseD3D11* pTextureD3D11{NEW_RC_OBJ(m_TexObjAllocator, "Texture3D_D3D11 instance", Texture3D_D3D11)(m_TexViewObjAllocator, this, InitialState, pd3d11Texture)};
                           pTextureD3D11->QueryInterface(IID_Texture, reinterpret_cast<IObject**>(ppTexture));
                           pTextureD3D11->CreateDefaultViews();
                           OnCreateDeviceObject(pTextureD3D11);
                       });
}


void RenderDeviceD3D11Impl::CreateTexture(const TextureDesc& TexDesc, const TextureData* pData, ITexture** ppTexture)
{
    CreateDeviceObject("texture", TexDesc, ppTexture,
                       [&]() //
                       {
                           TextureBaseD3D11* pTextureD3D11 = nullptr;
                           switch (TexDesc.Type)
                           {
                               case RESOURCE_DIM_TEX_1D:
                               case RESOURCE_DIM_TEX_1D_ARRAY:
                                   pTextureD3D11 = NEW_RC_OBJ(m_TexObjAllocator, "Texture1D_D3D11 instance", Texture1D_D3D11)(m_TexViewObjAllocator, this, TexDesc, pData);
                                   break;

                               case RESOURCE_DIM_TEX_2D:
                               case RESOURCE_DIM_TEX_2D_ARRAY:
                               case RESOURCE_DIM_TEX_CUBE:
                               case RESOURCE_DIM_TEX_CUBE_ARRAY:
                                   pTextureD3D11 = NEW_RC_OBJ(m_TexObjAllocator, "Texture2D_D3D11 instance", Texture2D_D3D11)(m_TexViewObjAllocator, this, TexDesc, pData);
                                   break;

                               case RESOURCE_DIM_TEX_3D:
                                   pTextureD3D11 = NEW_RC_OBJ(m_TexObjAllocator, "Texture3D_D3D11 instance", Texture3D_D3D11)(m_TexViewObjAllocator, this, TexDesc, pData);
                                   break;

                               default: LOG_ERROR_AND_THROW("Unknown texture type. (Did you forget to initialize the Type member of TextureDesc structure?)");
                           }
                           pTextureD3D11->QueryInterface(IID_Texture, reinterpret_cast<IObject**>(ppTexture));
                           pTextureD3D11->CreateDefaultViews();
                           OnCreateDeviceObject(pTextureD3D11);
                       });
}

void RenderDeviceD3D11Impl::CreateSampler(const SamplerDesc& SamplerDesc, ISampler** ppSampler)
{
    CreateDeviceObject("sampler", SamplerDesc, ppSampler,
                       [&]() //
                       {
                           m_SamplersRegistry.Find(SamplerDesc, reinterpret_cast<IDeviceObject**>(ppSampler));
                           if (*ppSampler == nullptr)
                           {
                               SamplerD3D11Impl* pSamplerD3D11{NEW_RC_OBJ(m_SamplerObjAllocator, "SamplerD3D11Impl instance", SamplerD3D11Impl)(this, SamplerDesc)};
                               pSamplerD3D11->QueryInterface(IID_Sampler, reinterpret_cast<IObject**>(ppSampler));
                               OnCreateDeviceObject(pSamplerD3D11);
                               m_SamplersRegistry.Add(SamplerDesc, *ppSampler);
                           }
                       });
}

template <typename PSOCreateInfoType>
void RenderDeviceD3D11Impl::CreatePipelineState(const PSOCreateInfoType& PSOCreateInfo, IPipelineState** ppPipelineState)
{
    CreateDeviceObject("Pipeline state", PSOCreateInfo.PSODesc, ppPipelineState,
                       [&]() //
                       {
                           PipelineStateD3D11Impl* pPipelineStateD3D11{NEW_RC_OBJ(m_PSOAllocator, "PipelineStateD3D11Impl instance", PipelineStateD3D11Impl)(this, PSOCreateInfo)};
                           pPipelineStateD3D11->QueryInterface(IID_PipelineState, reinterpret_cast<IObject**>(ppPipelineState));
                           OnCreateDeviceObject(pPipelineStateD3D11);
                       });
}


void RenderDeviceD3D11Impl::CreateGraphicsPipelineState(const GraphicsPipelineStateCreateInfo& PSOCreateInfo, IPipelineState** ppPipelineState)
{
    CreatePipelineState(PSOCreateInfo, ppPipelineState);
}

void RenderDeviceD3D11Impl::CreateComputePipelineState(const ComputePipelineStateCreateInfo& PSOCreateInfo, IPipelineState** ppPipelineState)
{
    CreatePipelineState(PSOCreateInfo, ppPipelineState);
}

void RenderDeviceD3D11Impl::CreateFence(const FenceDesc& Desc, IFence** ppFence)
{
    CreateDeviceObject("Fence", Desc, ppFence,
                       [&]() //
                       {
                           FenceD3D11Impl* pFenceD3D11{NEW_RC_OBJ(m_FenceAllocator, "FenceD3D11Impl instance", FenceD3D11Impl)(this, Desc)};
                           pFenceD3D11->QueryInterface(IID_Fence, reinterpret_cast<IObject**>(ppFence));
                           OnCreateDeviceObject(pFenceD3D11);
                       });
}

void RenderDeviceD3D11Impl::CreateQuery(const QueryDesc& Desc, IQuery** ppQuery)
{
    CreateDeviceObject("Query", Desc, ppQuery,
                       [&]() //
                       {
                           QueryD3D11Impl* pQueryD3D11{NEW_RC_OBJ(m_QueryAllocator, "QueryD3D11Impl instance", QueryD3D11Impl)(this, Desc)};
                           pQueryD3D11->QueryInterface(IID_Query, reinterpret_cast<IObject**>(ppQuery));
                           OnCreateDeviceObject(pQueryD3D11);
                       });
}

void RenderDeviceD3D11Impl::CreateRenderPass(const RenderPassDesc& Desc, IRenderPass** ppRenderPass)
{
    CreateDeviceObject("RenderPass", Desc, ppRenderPass,
                       [&]() //
                       {
                           RenderPassD3D11Impl* pRenderPassD3D11{NEW_RC_OBJ(m_RenderPassAllocator, "RenderPassD3D11Impl instance", RenderPassD3D11Impl)(this, Desc)};
                           pRenderPassD3D11->QueryInterface(IID_RenderPass, reinterpret_cast<IObject**>(ppRenderPass));
                           OnCreateDeviceObject(pRenderPassD3D11);
                       });
}

void RenderDeviceD3D11Impl::CreateFramebuffer(const FramebufferDesc& Desc, IFramebuffer** ppFramebuffer)
{
    CreateDeviceObject("Framebuffer", Desc, ppFramebuffer,
                       [&]() //
                       {
                           FramebufferD3D11Impl* pFramebufferD3D11{NEW_RC_OBJ(m_FramebufferAllocator, "FramebufferD3D11Impl instance", FramebufferD3D11Impl)(this, Desc)};
                           pFramebufferD3D11->QueryInterface(IID_Framebuffer, reinterpret_cast<IObject**>(ppFramebuffer));
                           OnCreateDeviceObject(pFramebufferD3D11);
                       });
}

void RenderDeviceD3D11Impl::IdleGPU()
{
    if (auto pImmediateCtx = m_wpImmediateContext.Lock())
    {
        pImmediateCtx->WaitForIdle();
    }
}

} // namespace Diligent
