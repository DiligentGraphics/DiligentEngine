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
#include <dxgi1_4.h>
#include "RenderDeviceD3D12Impl.hpp"
#include "PipelineStateD3D12Impl.hpp"
#include "ShaderD3D12Impl.hpp"
#include "TextureD3D12Impl.hpp"
#include "DXGITypeConversions.hpp"
#include "SamplerD3D12Impl.hpp"
#include "BufferD3D12Impl.hpp"
#include "ShaderResourceBindingD3D12Impl.hpp"
#include "DeviceContextD3D12Impl.hpp"
#include "FenceD3D12Impl.hpp"
#include "QueryD3D12Impl.hpp"
#include "RenderPassD3D12Impl.hpp"
#include "FramebufferD3D12Impl.hpp"
#include "EngineMemory.h"

namespace Diligent
{

static CComPtr<IDXGIAdapter1> DXGIAdapterFromD3D12Device(ID3D12Device* pd3d12Device)
{
    CComPtr<IDXGIFactory4> pDXIFactory;

    HRESULT hr = CreateDXGIFactory1(__uuidof(pDXIFactory), reinterpret_cast<void**>(static_cast<IDXGIFactory4**>(&pDXIFactory)));
    if (SUCCEEDED(hr))
    {
        auto AdapterLUID = pd3d12Device->GetAdapterLuid();

        CComPtr<IDXGIAdapter1> pDXGIAdapter1;
        pDXIFactory->EnumAdapterByLuid(AdapterLUID, __uuidof(pDXGIAdapter1), reinterpret_cast<void**>(static_cast<IDXGIAdapter1**>(&pDXGIAdapter1)));
        return pDXGIAdapter1;
    }
    else
    {
        LOG_ERROR("Unable to create DXIFactory");
    }
    return nullptr;
}

ShaderVersion RenderDeviceD3D12Impl::GetMaxShaderModel() const
{
    return ShaderVersion{static_cast<Uint8>((m_MaxShaderModel >> 4) & 0xF), static_cast<Uint8>(m_MaxShaderModel & 0xF)};
}

D3D_FEATURE_LEVEL RenderDeviceD3D12Impl::GetD3DFeatureLevel() const
{
    D3D_FEATURE_LEVEL FeatureLevels[] =
        {
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0 //
        };
    D3D12_FEATURE_DATA_FEATURE_LEVELS FeatureLevelsData = {};

    FeatureLevelsData.pFeatureLevelsRequested = FeatureLevels;
    FeatureLevelsData.NumFeatureLevels        = _countof(FeatureLevels);
    m_pd3d12Device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &FeatureLevelsData, sizeof(FeatureLevelsData));
    return FeatureLevelsData.MaxSupportedFeatureLevel;
}

#ifdef D3D12_H_HAS_MESH_SHADER
ID3D12Device2* RenderDeviceD3D12Impl::GetD3D12Device2()
{
    if (!m_pd3d12Device2)
    {
        CHECK_D3D_RESULT_THROW(m_pd3d12Device->QueryInterface(IID_PPV_ARGS(&m_pd3d12Device2)), "Failed to get ID3D12Device2");
    }
    return m_pd3d12Device2;
}
#endif

RenderDeviceD3D12Impl::RenderDeviceD3D12Impl(IReferenceCounters*          pRefCounters,
                                             IMemoryAllocator&            RawMemAllocator,
                                             IEngineFactory*              pEngineFactory,
                                             const EngineD3D12CreateInfo& EngineCI,
                                             ID3D12Device*                pd3d12Device,
                                             size_t                       CommandQueueCount,
                                             ICommandQueueD3D12**         ppCmdQueues) :
    // clang-format off
    TRenderDeviceBase
    {
        pRefCounters,
        RawMemAllocator,
        pEngineFactory,
        CommandQueueCount,
        ppCmdQueues,
        EngineCI.NumDeferredContexts,
        DeviceObjectSizes
        {
            sizeof(TextureD3D12Impl),
            sizeof(TextureViewD3D12Impl),
            sizeof(BufferD3D12Impl),
            sizeof(BufferViewD3D12Impl),
            sizeof(ShaderD3D12Impl),
            sizeof(SamplerD3D12Impl),
            sizeof(PipelineStateD3D12Impl),
            sizeof(ShaderResourceBindingD3D12Impl),
            sizeof(FenceD3D12Impl),
            sizeof(QueryD3D12Impl),
            sizeof(RenderPassD3D12Impl),
            sizeof(FramebufferD3D12Impl)
        }
    },
    m_pd3d12Device  {pd3d12Device},
    m_EngineAttribs {EngineCI    },
    m_CmdListManager{*this       },
    m_CPUDescriptorHeaps
    {
        {RawMemAllocator, *this, EngineCI.CPUDescriptorHeapAllocationSize[0], D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE},
        {RawMemAllocator, *this, EngineCI.CPUDescriptorHeapAllocationSize[1], D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,     D3D12_DESCRIPTOR_HEAP_FLAG_NONE},
        {RawMemAllocator, *this, EngineCI.CPUDescriptorHeapAllocationSize[2], D3D12_DESCRIPTOR_HEAP_TYPE_RTV,         D3D12_DESCRIPTOR_HEAP_FLAG_NONE},
        {RawMemAllocator, *this, EngineCI.CPUDescriptorHeapAllocationSize[3], D3D12_DESCRIPTOR_HEAP_TYPE_DSV,         D3D12_DESCRIPTOR_HEAP_FLAG_NONE}
    },
    m_GPUDescriptorHeaps
    {
        {RawMemAllocator, *this, EngineCI.GPUDescriptorHeapSize[0], EngineCI.GPUDescriptorHeapDynamicSize[0], D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE},
        {RawMemAllocator, *this, EngineCI.GPUDescriptorHeapSize[1], EngineCI.GPUDescriptorHeapDynamicSize[1], D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,     D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE}
    },
    m_ContextPool         (STD_ALLOCATOR_RAW_MEM(PooledCommandContext, GetRawAllocator(), "Allocator for vector<PooledCommandContext>")),
    m_DynamicMemoryManager{GetRawAllocator(), *this, EngineCI.NumDynamicHeapPagesToReserve, EngineCI.DynamicHeapPageSize},
    m_MipsGenerator       {pd3d12Device},
    m_QueryMgr            {pd3d12Device, EngineCI.QueryPoolSizes},
    m_pDxCompiler         {CreateDXCompiler(DXCompilerTarget::Direct3D12, EngineCI.pDxCompilerPath)}
// clang-format on
{
    try
    {
        m_DeviceCaps.DevType = RENDER_DEVICE_TYPE_D3D12;
        auto FeatureLevel    = GetD3DFeatureLevel();
        switch (FeatureLevel)
        {
            case D3D_FEATURE_LEVEL_12_0:
            case D3D_FEATURE_LEVEL_12_1:
                m_DeviceCaps.MajorVersion = 12;
                m_DeviceCaps.MinorVersion = FeatureLevel == D3D_FEATURE_LEVEL_12_1 ? 1 : 0;
                break;

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

        if (auto pDXGIAdapter1 = DXGIAdapterFromD3D12Device(pd3d12Device))
        {
            ReadAdapterInfo(pDXGIAdapter1);
        }

        // Direct3D12 supports shader model 5.1 on all feature levels (even on 11.0),
        // so bindless resources are always available.
        // https://docs.microsoft.com/en-us/windows/win32/direct3d12/hardware-feature-levels#feature-level-support
        m_DeviceCaps.Features.BindlessResources = DEVICE_FEATURE_STATE_ENABLED;

        m_DeviceCaps.Features.VertexPipelineUAVWritesAndAtomics = DEVICE_FEATURE_STATE_ENABLED;

        // Detect maximum  shader model.
        {
            // Direct3D12 supports shader model 5.1 on all feature levels.
            // https://docs.microsoft.com/en-us/windows/win32/direct3d12/hardware-feature-levels#feature-level-support
            m_MaxShaderModel = D3D_SHADER_MODEL_5_1;

            // Header may not have constants for D3D_SHADER_MODEL_6_1 and above.
            const D3D_SHADER_MODEL Models[] = //
                {
                    static_cast<D3D_SHADER_MODEL>(0x65), // minimum required for mesh shader
                    static_cast<D3D_SHADER_MODEL>(0x64),
                    static_cast<D3D_SHADER_MODEL>(0x63),
                    static_cast<D3D_SHADER_MODEL>(0x62),
                    static_cast<D3D_SHADER_MODEL>(0x61),
                    D3D_SHADER_MODEL_6_0 //
                };

            for (auto Model : Models)
            {
                D3D12_FEATURE_DATA_SHADER_MODEL ShaderModel = {Model};
                if (SUCCEEDED(m_pd3d12Device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &ShaderModel, sizeof(ShaderModel))))
                {
                    m_MaxShaderModel = ShaderModel.HighestShaderModel;
                    break;
                }
            }
            LOG_INFO_MESSAGE("Max device shader model: ", (m_MaxShaderModel >> 4) & 0xF, '_', m_MaxShaderModel & 0xF);
        }

        // Check if mesh shader is supported.
        bool MeshShadersSupported = false;
#ifdef D3D12_H_HAS_MESH_SHADER
        {
            D3D12_FEATURE_DATA_D3D12_OPTIONS7 FeatureData = {};

            MeshShadersSupported =
                SUCCEEDED(m_pd3d12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &FeatureData, sizeof(FeatureData))) &&
                FeatureData.MeshShaderTier != D3D12_MESH_SHADER_TIER_NOT_SUPPORTED;

            MeshShadersSupported = (m_MaxShaderModel >= D3D_SHADER_MODEL_6_5 && MeshShadersSupported);
        }
#else
        if (EngineCI.Features.MeshShaders == DEVICE_FEATURE_STATE_ENABLED)
        {
            LOG_ERROR_AND_THROW("Mesh shaders are requested to be enabled, but the engine was built with the Windows SDK that does "
                                "not support the feature. Please update the SDK to version 10.0.19041.0 or later and rebuild the engine.");
        }
#endif

        if (EngineCI.Features.MeshShaders == DEVICE_FEATURE_STATE_ENABLED && !MeshShadersSupported)
        {
            LOG_ERROR_AND_THROW("This device/driver does not support mesh shaders. Please make sure that you have compatible GPU and that your "
                                "Winodws is up to date (version 2004 or later is required)");
        }

        m_DeviceCaps.Features.MeshShaders = MeshShadersSupported ? DEVICE_FEATURE_STATE_ENABLED : DEVICE_FEATURE_STATE_DISABLED;


        {
            D3D12_FEATURE_DATA_D3D12_OPTIONS d3d12Features = {};
            if (SUCCEEDED(m_pd3d12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &d3d12Features, sizeof(d3d12Features))))
            {
                if (d3d12Features.MinPrecisionSupport & D3D12_SHADER_MIN_PRECISION_SUPPORT_16_BIT)
                {
                    m_DeviceCaps.Features.ShaderFloat16 = DEVICE_FEATURE_STATE_ENABLED;
                }
            }
        }

        {
            D3D12_FEATURE_DATA_D3D12_OPTIONS4 d3d12Features4 = {};
            if (SUCCEEDED(m_pd3d12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS4, &d3d12Features4, sizeof(d3d12Features4))))
            {
                if (d3d12Features4.Native16BitShaderOpsSupported)
                {
                    m_DeviceCaps.Features.ResourceBuffer16BitAccess = DEVICE_FEATURE_STATE_ENABLED;
                    m_DeviceCaps.Features.UniformBuffer16BitAccess  = DEVICE_FEATURE_STATE_ENABLED;
                    m_DeviceCaps.Features.ShaderInputOutput16       = DEVICE_FEATURE_STATE_ENABLED;
                }
            }
        }

#define CHECK_REQUIRED_FEATURE(Feature, FeatureName)                          \
    do                                                                        \
    {                                                                         \
        if (EngineCI.Features.Feature == DEVICE_FEATURE_STATE_ENABLED &&      \
            m_DeviceCaps.Features.Feature != DEVICE_FEATURE_STATE_ENABLED)    \
            LOG_ERROR_AND_THROW(FeatureName, "not supported by this device"); \
    } while (false)

        // clang-format off
        CHECK_REQUIRED_FEATURE(ShaderFloat16,             "16-bit float shader operations are");
        CHECK_REQUIRED_FEATURE(ResourceBuffer16BitAccess, "16-bit resoure buffer access is");
        CHECK_REQUIRED_FEATURE(UniformBuffer16BitAccess,  "16-bit uniform buffer access is");
        CHECK_REQUIRED_FEATURE(ShaderInputOutput16,       "16-bit shader inputs/outputs are");

        CHECK_REQUIRED_FEATURE(ShaderInt8,               "8-bit shader operations are");
        CHECK_REQUIRED_FEATURE(ResourceBuffer8BitAccess, "8-bit resoure buffer access is");
        CHECK_REQUIRED_FEATURE(UniformBuffer8BitAccess,  "8-bit uniform buffer access is");
        // clang-format on
#undef CHECK_REQUIRED_FEATURE

#if defined(_MSC_VER) && defined(_WIN64)
        static_assert(sizeof(DeviceFeatures) == 31, "Did you add a new feature to DeviceFeatures? Please handle its satus here.");
#endif

        auto& TexCaps = m_DeviceCaps.TexCaps;

        TexCaps.MaxTexture1DDimension     = D3D12_REQ_TEXTURE1D_U_DIMENSION;
        TexCaps.MaxTexture1DArraySlices   = D3D12_REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION;
        TexCaps.MaxTexture2DDimension     = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;
        TexCaps.MaxTexture2DArraySlices   = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
        TexCaps.MaxTexture3DDimension     = D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
        TexCaps.MaxTextureCubeDimension   = D3D12_REQ_TEXTURECUBE_DIMENSION;
        TexCaps.Texture2DMSSupported      = True;
        TexCaps.Texture2DMSArraySupported = True;
        TexCaps.TextureViewSupported      = True;
        TexCaps.CubemapArraysSupported    = True;

        auto& SamCaps = m_DeviceCaps.SamCaps;

        SamCaps.BorderSamplingModeSupported   = True;
        SamCaps.AnisotropicFilteringSupported = True;
        SamCaps.LODBiasSupported              = True;
    }
    catch (...)
    {
        m_DynamicMemoryManager.Destroy();
        throw;
    }
}

RenderDeviceD3D12Impl::~RenderDeviceD3D12Impl()
{
    // Wait for the GPU to complete all its operations
    IdleGPU();
    ReleaseStaleResources(true);

#ifdef DILIGENT_DEVELOPMENT
    for (auto i = 0; i < _countof(m_CPUDescriptorHeaps); ++i)
    {
        DEV_CHECK_ERR(m_CPUDescriptorHeaps[i].DvpGetTotalAllocationCount() == 0, "All CPU descriptor heap allocations must be released");
    }
    for (auto i = 0; i < _countof(m_GPUDescriptorHeaps); ++i)
    {
        DEV_CHECK_ERR(m_GPUDescriptorHeaps[i].DvpGetTotalAllocationCount() == 0, "All GPU descriptor heap allocations must be released");
    }
#endif

    DEV_CHECK_ERR(m_DynamicMemoryManager.GetAllocatedPageCounter() == 0, "All allocated dynamic pages must have been returned to the manager at this point.");
    m_DynamicMemoryManager.Destroy();
    DEV_CHECK_ERR(m_CmdListManager.GetAllocatorCounter() == 0, "All allocators must have been returned to the manager at this point.");
    DEV_CHECK_ERR(m_AllocatedCtxCounter == 0, "All contexts must have been released.");

    m_ContextPool.clear();
    DestroyCommandQueues();
}

void RenderDeviceD3D12Impl::DisposeCommandContext(PooledCommandContext&& Ctx)
{
    CComPtr<ID3D12CommandAllocator> pAllocator;
    Ctx->Close(pAllocator);
    // Since allocator has not been used, we cmd list manager can put it directly into the free allocator list
    m_CmdListManager.FreeAllocator(std::move(pAllocator));
    FreeCommandContext(std::move(Ctx));
}

void RenderDeviceD3D12Impl::FreeCommandContext(PooledCommandContext&& Ctx)
{
    std::lock_guard<std::mutex> LockGuard(m_ContextPoolMutex);
    m_ContextPool.emplace_back(std::move(Ctx));
#ifdef DILIGENT_DEVELOPMENT
    Atomics::AtomicDecrement(m_AllocatedCtxCounter);
#endif
}

void RenderDeviceD3D12Impl::CloseAndExecuteTransientCommandContext(Uint32 CommandQueueIndex, PooledCommandContext&& Ctx)
{
    CComPtr<ID3D12CommandAllocator> pAllocator;
    ID3D12GraphicsCommandList*      pCmdList   = Ctx->Close(pAllocator);
    Uint64                          FenceValue = 0;
    // Execute command list directly through the queue to avoid interference with command list numbers in the queue
    LockCmdQueueAndRun(CommandQueueIndex,
                       [&](ICommandQueueD3D12* pCmdQueue) //
                       {
                           FenceValue = pCmdQueue->Submit(pCmdList);
                       });
    m_CmdListManager.ReleaseAllocator(std::move(pAllocator), CommandQueueIndex, FenceValue);
    FreeCommandContext(std::move(Ctx));
}

Uint64 RenderDeviceD3D12Impl::CloseAndExecuteCommandContext(Uint32 QueueIndex, PooledCommandContext&& Ctx, bool DiscardStaleObjects, std::vector<std::pair<Uint64, RefCntAutoPtr<IFence>>>* pSignalFences)
{
    CComPtr<ID3D12CommandAllocator> pAllocator;
    ID3D12GraphicsCommandList*      pCmdList = Ctx->Close(pAllocator);

    Uint64 FenceValue = 0;
    {
        // Stale objects should only be discarded when submitting cmd list from
        // the immediate context, otherwise the basic requirement may be violated
        // as in the following scenario
        //
        //  Signaled        |                                        |
        //  Fence Value     |        Immediate Context               |            InitContext            |
        //                  |                                        |                                   |
        //    N             |  Draw(ResourceX)                       |                                   |
        //                  |  Release(ResourceX)                    |                                   |
        //                  |   - (ResourceX, N) -> Release Queue    |                                   |
        //                  |                                        | CopyResource()                    |
        //   N+1            |                                        | CloseAndExecuteCommandContext()   |
        //                  |                                        |                                   |
        //   N+2            |  CloseAndExecuteCommandContext()       |                                   |
        //                  |   - Cmd list is submitted with number  |                                   |
        //                  |     N+1, but resource it references    |                                   |
        //                  |     was added to the delete queue      |                                   |
        //                  |     with number N                      |                                   |
        auto SubmittedCmdBuffInfo = TRenderDeviceBase::SubmitCommandBuffer(QueueIndex, pCmdList, true);
        FenceValue                = SubmittedCmdBuffInfo.FenceValue;
        if (pSignalFences != nullptr)
            SignalFences(QueueIndex, *pSignalFences);
    }

    m_CmdListManager.ReleaseAllocator(std::move(pAllocator), QueueIndex, FenceValue);
    FreeCommandContext(std::move(Ctx));

    PurgeReleaseQueue(QueueIndex);

    return FenceValue;
}

void RenderDeviceD3D12Impl::SignalFences(Uint32 QueueIndex, std::vector<std::pair<Uint64, RefCntAutoPtr<IFence>>>& SignalFences)
{
    for (auto& val_fence : SignalFences)
    {
        auto* pFenceD3D12Impl = val_fence.second.RawPtr<FenceD3D12Impl>();
        auto* pd3d12Fence     = pFenceD3D12Impl->GetD3D12Fence();
        m_CommandQueues[QueueIndex].CmdQueue->SignalFence(pd3d12Fence, val_fence.first);
    }
}

void RenderDeviceD3D12Impl::IdleGPU()
{
    IdleAllCommandQueues(true);
    ReleaseStaleResources();
}

void RenderDeviceD3D12Impl::FlushStaleResources(Uint32 CmdQueueIndex)
{
    // Submit empty command list to the queue. This will effectively signal the fence and
    // discard all resources
    ID3D12GraphicsCommandList* pNullCmdList = nullptr;
    TRenderDeviceBase::SubmitCommandBuffer(CmdQueueIndex, pNullCmdList, true);
}

void RenderDeviceD3D12Impl::ReleaseStaleResources(bool ForceRelease)
{
    PurgeReleaseQueues(ForceRelease);
}


RenderDeviceD3D12Impl::PooledCommandContext RenderDeviceD3D12Impl::AllocateCommandContext(const Char* ID)
{
    {
        std::lock_guard<std::mutex> LockGuard(m_ContextPoolMutex);
        if (!m_ContextPool.empty())
        {
            PooledCommandContext Ctx = std::move(m_ContextPool.back());
            m_ContextPool.pop_back();
            Ctx->Reset(m_CmdListManager);
            Ctx->SetID(ID);
#ifdef DILIGENT_DEVELOPMENT
            Atomics::AtomicIncrement(m_AllocatedCtxCounter);
#endif
            return Ctx;
        }
    }

    auto& CmdCtxAllocator = GetRawAllocator();
    auto* pRawMem         = ALLOCATE(CmdCtxAllocator, "CommandContext instance", CommandContext, 1);
    auto  pCtx            = new (pRawMem) CommandContext(m_CmdListManager);
    pCtx->SetID(ID);
#ifdef DILIGENT_DEVELOPMENT
    Atomics::AtomicIncrement(m_AllocatedCtxCounter);
#endif
    return PooledCommandContext(pCtx, CmdCtxAllocator);
}


void RenderDeviceD3D12Impl::TestTextureFormat(TEXTURE_FORMAT TexFormat)
{
    auto& TexFormatInfo = m_TextureFormatsInfo[TexFormat];
    VERIFY(TexFormatInfo.Supported, "Texture format is not supported");

    auto DXGIFormat = TexFormatToDXGI_Format(TexFormat);

    D3D12_FEATURE_DATA_FORMAT_SUPPORT FormatSupport = {DXGIFormat};

    auto hr = m_pd3d12Device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &FormatSupport, sizeof(FormatSupport));
    if (FAILED(hr))
    {
        LOG_ERROR_MESSAGE("CheckFormatSupport() failed for format ", DXGIFormat);
        return;
    }

    TexFormatInfo.Filterable =
        ((FormatSupport.Support1 & D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE) != 0) ||
        ((FormatSupport.Support1 & D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE_COMPARISON) != 0);

    TexFormatInfo.BindFlags = BIND_SHADER_RESOURCE;
    if ((FormatSupport.Support1 & D3D12_FORMAT_SUPPORT1_RENDER_TARGET) != 0)
        TexFormatInfo.BindFlags |= BIND_RENDER_TARGET;
    if ((FormatSupport.Support1 & D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL) != 0)
        TexFormatInfo.BindFlags |= BIND_DEPTH_STENCIL;
    if ((FormatSupport.Support1 & D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW) != 0)
        TexFormatInfo.BindFlags |= BIND_UNORDERED_ACCESS;

    TexFormatInfo.Dimensions = RESOURCE_DIMENSION_SUPPORT_NONE;
    if ((FormatSupport.Support1 & D3D12_FORMAT_SUPPORT1_TEXTURE1D) != 0)
        TexFormatInfo.Dimensions |= RESOURCE_DIMENSION_SUPPORT_TEX_1D | RESOURCE_DIMENSION_SUPPORT_TEX_1D_ARRAY;
    if ((FormatSupport.Support1 & D3D12_FORMAT_SUPPORT1_TEXTURE2D) != 0)
        TexFormatInfo.Dimensions |= RESOURCE_DIMENSION_SUPPORT_TEX_2D | RESOURCE_DIMENSION_SUPPORT_TEX_2D_ARRAY;
    if ((FormatSupport.Support1 & D3D12_FORMAT_SUPPORT1_TEXTURE3D) != 0)
        TexFormatInfo.Dimensions |= RESOURCE_DIMENSION_SUPPORT_TEX_3D;
    if ((FormatSupport.Support1 & D3D12_FORMAT_SUPPORT1_TEXTURECUBE) != 0)
        TexFormatInfo.Dimensions |= RESOURCE_DIMENSION_SUPPORT_TEX_CUBE | RESOURCE_DIMENSION_SUPPORT_TEX_CUBE_ARRAY;

    TexFormatInfo.SampleCounts = 0x0;
    for (Uint32 SampleCount = 1; SampleCount <= D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT; SampleCount *= 2)
    {
        D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS QualityLevels = {DXGIFormat, SampleCount};

        hr = m_pd3d12Device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &QualityLevels, sizeof(QualityLevels));
        if (SUCCEEDED(hr) && QualityLevels.NumQualityLevels > 0)
            TexFormatInfo.SampleCounts |= SampleCount;
    }
}


IMPLEMENT_QUERY_INTERFACE(RenderDeviceD3D12Impl, IID_RenderDeviceD3D12, TRenderDeviceBase)

template <typename PSOCreateInfoType>
void RenderDeviceD3D12Impl::CreatePipelineState(const PSOCreateInfoType& PSOCreateInfo, IPipelineState** ppPipelineState)
{
    CreateDeviceObject("Pipeline State", PSOCreateInfo.PSODesc, ppPipelineState,
                       [&]() //
                       {
                           PipelineStateD3D12Impl* pPipelineStateD3D12{NEW_RC_OBJ(m_PSOAllocator, "PipelineStateD3D12Impl instance", PipelineStateD3D12Impl)(this, PSOCreateInfo)};
                           pPipelineStateD3D12->QueryInterface(IID_PipelineState, reinterpret_cast<IObject**>(ppPipelineState));
                           OnCreateDeviceObject(pPipelineStateD3D12);
                       });
}


void RenderDeviceD3D12Impl::CreateGraphicsPipelineState(const GraphicsPipelineStateCreateInfo& PSOCreateInfo, IPipelineState** ppPipelineState)
{
    CreatePipelineState(PSOCreateInfo, ppPipelineState);
}

void RenderDeviceD3D12Impl::CreateComputePipelineState(const ComputePipelineStateCreateInfo& PSOCreateInfo, IPipelineState** ppPipelineState)
{
    CreatePipelineState(PSOCreateInfo, ppPipelineState);
}

void RenderDeviceD3D12Impl::CreateBufferFromD3DResource(ID3D12Resource* pd3d12Buffer, const BufferDesc& BuffDesc, RESOURCE_STATE InitialState, IBuffer** ppBuffer)
{
    CreateDeviceObject("buffer", BuffDesc, ppBuffer,
                       [&]() //
                       {
                           BufferD3D12Impl* pBufferD3D12{NEW_RC_OBJ(m_BufObjAllocator, "BufferD3D12Impl instance", BufferD3D12Impl)(m_BuffViewObjAllocator, this, BuffDesc, InitialState, pd3d12Buffer)};
                           pBufferD3D12->QueryInterface(IID_Buffer, reinterpret_cast<IObject**>(ppBuffer));
                           pBufferD3D12->CreateDefaultViews();
                           OnCreateDeviceObject(pBufferD3D12);
                       });
}

void RenderDeviceD3D12Impl::CreateBuffer(const BufferDesc& BuffDesc, const BufferData* pBuffData, IBuffer** ppBuffer)
{
    CreateDeviceObject("buffer", BuffDesc, ppBuffer,
                       [&]() //
                       {
                           BufferD3D12Impl* pBufferD3D12{NEW_RC_OBJ(m_BufObjAllocator, "BufferD3D12Impl instance", BufferD3D12Impl)(m_BuffViewObjAllocator, this, BuffDesc, pBuffData)};
                           pBufferD3D12->QueryInterface(IID_Buffer, reinterpret_cast<IObject**>(ppBuffer));
                           pBufferD3D12->CreateDefaultViews();
                           OnCreateDeviceObject(pBufferD3D12);
                       });
}


void RenderDeviceD3D12Impl::CreateShader(const ShaderCreateInfo& ShaderCI, IShader** ppShader)
{
    CreateDeviceObject("shader", ShaderCI.Desc, ppShader,
                       [&]() //
                       {
                           ShaderD3D12Impl* pShaderD3D12{NEW_RC_OBJ(m_ShaderObjAllocator, "ShaderD3D12Impl instance", ShaderD3D12Impl)(this, ShaderCI)};
                           pShaderD3D12->QueryInterface(IID_Shader, reinterpret_cast<IObject**>(ppShader));

                           OnCreateDeviceObject(pShaderD3D12);
                       });
}

void RenderDeviceD3D12Impl::CreateTextureFromD3DResource(ID3D12Resource* pd3d12Texture, RESOURCE_STATE InitialState, ITexture** ppTexture)
{
    TextureDesc TexDesc;
    TexDesc.Name = "Texture from d3d12 resource";
    CreateDeviceObject("texture", TexDesc, ppTexture,
                       [&]() //
                       {
                           TextureD3D12Impl* pTextureD3D12{NEW_RC_OBJ(m_TexObjAllocator, "TextureD3D12Impl instance", TextureD3D12Impl)(m_TexViewObjAllocator, this, TexDesc, InitialState, pd3d12Texture)};

                           pTextureD3D12->QueryInterface(IID_Texture, reinterpret_cast<IObject**>(ppTexture));
                           pTextureD3D12->CreateDefaultViews();
                           OnCreateDeviceObject(pTextureD3D12);
                       });
}

void RenderDeviceD3D12Impl::CreateTexture(const TextureDesc& TexDesc, ID3D12Resource* pd3d12Texture, RESOURCE_STATE InitialState, TextureD3D12Impl** ppTexture)
{
    CreateDeviceObject("texture", TexDesc, ppTexture,
                       [&]() //
                       {
                           TextureD3D12Impl* pTextureD3D12{NEW_RC_OBJ(m_TexObjAllocator, "TextureD3D12Impl instance", TextureD3D12Impl)(m_TexViewObjAllocator, this, TexDesc, InitialState, pd3d12Texture)};
                           pTextureD3D12->QueryInterface(IID_TextureD3D12, reinterpret_cast<IObject**>(ppTexture));
                       });
}

void RenderDeviceD3D12Impl::CreateTexture(const TextureDesc& TexDesc, const TextureData* pData, ITexture** ppTexture)
{
    CreateDeviceObject("texture", TexDesc, ppTexture,
                       [&]() //
                       {
                           TextureD3D12Impl* pTextureD3D12{NEW_RC_OBJ(m_TexObjAllocator, "TextureD3D12Impl instance", TextureD3D12Impl)(m_TexViewObjAllocator, this, TexDesc, pData)};

                           pTextureD3D12->QueryInterface(IID_Texture, reinterpret_cast<IObject**>(ppTexture));
                           pTextureD3D12->CreateDefaultViews();
                           OnCreateDeviceObject(pTextureD3D12);
                       });
}

void RenderDeviceD3D12Impl::CreateSampler(const SamplerDesc& SamplerDesc, ISampler** ppSampler)
{
    CreateDeviceObject("sampler", SamplerDesc, ppSampler,
                       [&]() //
                       {
                           m_SamplersRegistry.Find(SamplerDesc, reinterpret_cast<IDeviceObject**>(ppSampler));
                           if (*ppSampler == nullptr)
                           {
                               SamplerD3D12Impl* pSamplerD3D12{NEW_RC_OBJ(m_SamplerObjAllocator, "SamplerD3D12Impl instance", SamplerD3D12Impl)(this, SamplerDesc)};
                               pSamplerD3D12->QueryInterface(IID_Sampler, reinterpret_cast<IObject**>(ppSampler));
                               OnCreateDeviceObject(pSamplerD3D12);
                               m_SamplersRegistry.Add(SamplerDesc, *ppSampler);
                           }
                       });
}

void RenderDeviceD3D12Impl::CreateFence(const FenceDesc& Desc, IFence** ppFence)
{
    CreateDeviceObject("Fence", Desc, ppFence,
                       [&]() //
                       {
                           FenceD3D12Impl* pFenceD3D12{NEW_RC_OBJ(m_FenceAllocator, "FenceD3D12Impl instance", FenceD3D12Impl)(this, Desc)};
                           pFenceD3D12->QueryInterface(IID_Fence, reinterpret_cast<IObject**>(ppFence));
                           OnCreateDeviceObject(pFenceD3D12);
                       });
}

void RenderDeviceD3D12Impl::CreateQuery(const QueryDesc& Desc, IQuery** ppQuery)
{
    CreateDeviceObject("Query", Desc, ppQuery,
                       [&]() //
                       {
                           QueryD3D12Impl* pQueryD3D12{NEW_RC_OBJ(m_QueryAllocator, "QueryD3D12Impl instance", QueryD3D12Impl)(this, Desc)};
                           pQueryD3D12->QueryInterface(IID_Query, reinterpret_cast<IObject**>(ppQuery));
                           OnCreateDeviceObject(pQueryD3D12);
                       });
}

void RenderDeviceD3D12Impl::CreateRenderPass(const RenderPassDesc& Desc, IRenderPass** ppRenderPass)
{
    CreateDeviceObject("RenderPass", Desc, ppRenderPass,
                       [&]() //
                       {
                           RenderPassD3D12Impl* pRenderPassD3D12{NEW_RC_OBJ(m_RenderPassAllocator, "RenderPassD3D12Impl instance", RenderPassD3D12Impl)(this, Desc)};
                           pRenderPassD3D12->QueryInterface(IID_RenderPass, reinterpret_cast<IObject**>(ppRenderPass));
                           OnCreateDeviceObject(pRenderPassD3D12);
                       });
}

void RenderDeviceD3D12Impl::CreateFramebuffer(const FramebufferDesc& Desc, IFramebuffer** ppFramebuffer)
{
    CreateDeviceObject("Framebuffer", Desc, ppFramebuffer,
                       [&]() //
                       {
                           FramebufferD3D12Impl* pFramebufferD3D12{NEW_RC_OBJ(m_FramebufferAllocator, "FramebufferD3D12Impl instance", FramebufferD3D12Impl)(this, Desc)};
                           pFramebufferD3D12->QueryInterface(IID_Framebuffer, reinterpret_cast<IObject**>(ppFramebuffer));
                           OnCreateDeviceObject(pFramebufferD3D12);
                       });
}

DescriptorHeapAllocation RenderDeviceD3D12Impl::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE Type, UINT Count /*= 1*/)
{
    VERIFY(Type >= D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV && Type < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES, "Invalid heap type");
    return m_CPUDescriptorHeaps[Type].Allocate(Count);
}

DescriptorHeapAllocation RenderDeviceD3D12Impl::AllocateGPUDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE Type, UINT Count /*= 1*/)
{
    VERIFY(Type >= D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV && Type <= D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, "Invalid heap type");
    return m_GPUDescriptorHeaps[Type].Allocate(Count);
}

} // namespace Diligent
