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

#include "TestingEnvironment.hpp"
#include "PlatformDebug.hpp"
#include "TestingSwapChainBase.hpp"

#if D3D11_SUPPORTED
#    include "EngineFactoryD3D11.h"
#endif

#if D3D12_SUPPORTED
#    include "EngineFactoryD3D12.h"
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
#    include "EngineFactoryOpenGL.h"
#endif

#if VULKAN_SUPPORTED
#    include "EngineFactoryVk.h"
#endif

#if METAL_SUPPORTED
#    include "EngineFactoryMtl.h"
#endif

namespace Diligent
{

namespace Testing
{

TestingEnvironment* TestingEnvironment::m_pTheEnvironment = nullptr;
std::atomic_int     TestingEnvironment::m_NumAllowedErrors;

void TestingEnvironment::MessageCallback(DEBUG_MESSAGE_SEVERITY Severity,
                                         const Char*            Message,
                                         const char*            Function,
                                         const char*            File,
                                         int                    Line)
{
    if (Severity == DEBUG_MESSAGE_SEVERITY_ERROR || Severity == DEBUG_MESSAGE_SEVERITY_FATAL_ERROR)
    {
        if (m_NumAllowedErrors == 0)
        {
            ADD_FAILURE() << "Unexpected error";
        }
        else
        {
            m_NumAllowedErrors--;
        }
    }

    PlatformDebug::OutputDebugMessage(Severity, Message, Function, File, Line);
}

void TestingEnvironment::SetErrorAllowance(int NumErrorsToAllow, const char* InfoMessage)
{
    m_NumAllowedErrors = NumErrorsToAllow;
    if (InfoMessage != nullptr)
    {
        std::cout << InfoMessage;
    }
}

Uint32 TestingEnvironment::FindAdapater(const std::vector<GraphicsAdapterInfo>& Adapters,
                                        ADAPTER_TYPE                            AdapterType,
                                        Uint32                                  AdapterId)
{
    if (AdapterId != DEFAULT_ADAPTER_ID && AdapterId >= Adapters.size())
    {
        LOG_ERROR_MESSAGE("Adapter ID (", AdapterId, ") is invalid. Only ", Adapters.size(), " adapter(s) found on the system");
        AdapterId = DEFAULT_ADAPTER_ID;
    }

    if (AdapterId == DEFAULT_ADAPTER_ID && AdapterType != ADAPTER_TYPE_UNKNOWN)
    {
        for (Uint32 i = 0; i < Adapters.size(); ++i)
        {
            if (Adapters[i].Type == AdapterType)
            {
                AdapterId = i;
                break;
            }
        }
        if (AdapterId == DEFAULT_ADAPTER_ID)
            LOG_WARNING_MESSAGE("Unable to find the requested adapter type. Using default adapter.");
    }

    if (AdapterId != DEFAULT_ADAPTER_ID)
        LOG_INFO_MESSAGE("Using adapter ", AdapterId, ": '", Adapters[AdapterId].Description, "'");

    return AdapterId;
}

TestingEnvironment::TestingEnvironment(const CreateInfo& CI, const SwapChainDesc& SCDesc) :
    m_DeviceType{CI.deviceType}
{
    VERIFY(m_pTheEnvironment == nullptr, "Testing environment object has already been initialized!");
    m_pTheEnvironment = this;

    Uint32 NumDeferredCtx = 0;

    std::vector<IDeviceContext*>     ppContexts;
    std::vector<GraphicsAdapterInfo> Adapters;

#if D3D11_SUPPORTED || D3D12_SUPPORTED
    auto PrintAdapterInfo = [](Uint32 AdapterId, const GraphicsAdapterInfo& AdapterInfo, const std::vector<DisplayModeAttribs>& DisplayModes) //
    {
        const char* AdapterTypeStr = nullptr;
        switch (AdapterInfo.Type)
        {
            case ADAPTER_TYPE_HARDWARE: AdapterTypeStr = "HW"; break;
            case ADAPTER_TYPE_SOFTWARE: AdapterTypeStr = "SW"; break;
            default: AdapterTypeStr = "Type unknown";
        }
        LOG_INFO_MESSAGE("Adapter ", AdapterId, ": '", AdapterInfo.Description, "' (",
                         AdapterTypeStr, ", ", AdapterInfo.DeviceLocalMemory / (1 << 20), " MB); ",
                         DisplayModes.size(), (DisplayModes.size() == 1 ? " display mode" : " display modes"));
    };
#endif

    switch (m_DeviceType)
    {
#if D3D11_SUPPORTED
        case RENDER_DEVICE_TYPE_D3D11:
        {
#    if ENGINE_DLL
            // Load the dll and import GetEngineFactoryD3D11() function
            auto GetEngineFactoryD3D11 = LoadGraphicsEngineD3D11();
            if (GetEngineFactoryD3D11 == nullptr)
            {
                LOG_ERROR_AND_THROW("Failed to load the engine");
            }
#    endif

            EngineD3D11CreateInfo CreateInfo;
            CreateInfo.DebugMessageCallback = MessageCallback;
            CreateInfo.Features             = DeviceFeatures{DEVICE_FEATURE_STATE_OPTIONAL};

#    ifdef DILIGENT_DEBUG
            CreateInfo.DebugFlags =
                D3D11_DEBUG_FLAG_CREATE_DEBUG_DEVICE |
                D3D11_DEBUG_FLAG_VERIFY_COMMITTED_RESOURCE_RELEVANCE |
                D3D11_DEBUG_FLAG_VERIFY_COMMITTED_SHADER_RESOURCES;
#    endif
            auto*  pFactoryD3D11 = GetEngineFactoryD3D11();
            Uint32 NumAdapters   = 0;
            pFactoryD3D11->EnumerateAdapters(DIRECT3D_FEATURE_LEVEL_11_0, NumAdapters, 0);
            Adapters.resize(NumAdapters);
            pFactoryD3D11->EnumerateAdapters(DIRECT3D_FEATURE_LEVEL_11_0, NumAdapters, Adapters.data());

            LOG_INFO_MESSAGE("Found ", Adapters.size(), " compatible adapters");
            for (Uint32 i = 0; i < Adapters.size(); ++i)
            {
                const auto& AdapterInfo = Adapters[i];

                std::vector<DisplayModeAttribs> DisplayModes;
                if (AdapterInfo.NumOutputs > 0)
                {
                    Uint32 NumDisplayModes = 0;
                    pFactoryD3D11->EnumerateDisplayModes(DIRECT3D_FEATURE_LEVEL_11_0, i, 0, TEX_FORMAT_RGBA8_UNORM, NumDisplayModes, nullptr);
                    DisplayModes.resize(NumDisplayModes);
                    pFactoryD3D11->EnumerateDisplayModes(DIRECT3D_FEATURE_LEVEL_11_0, i, 0, TEX_FORMAT_RGBA8_UNORM, NumDisplayModes, DisplayModes.data());
                }

                PrintAdapterInfo(i, AdapterInfo, DisplayModes);
            }

            CreateInfo.AdapterId = FindAdapater(Adapters, CI.AdapterType, CI.AdapterId);

            CreateInfo.NumDeferredContexts = NumDeferredCtx;
            ppContexts.resize(1 + NumDeferredCtx);
            pFactoryD3D11->CreateDeviceAndContextsD3D11(CreateInfo, &m_pDevice, ppContexts.data());
        }
        break;
#endif

#if D3D12_SUPPORTED
        case RENDER_DEVICE_TYPE_D3D12:
        {
#    if ENGINE_DLL
            // Load the dll and import GetEngineFactoryD3D12() function
            auto GetEngineFactoryD3D12 = LoadGraphicsEngineD3D12();
            if (GetEngineFactoryD3D12 == nullptr)
            {
                LOG_ERROR_AND_THROW("Failed to load the engine");
            }
#    endif
            auto* pFactoryD3D12 = GetEngineFactoryD3D12();
            if (!pFactoryD3D12->LoadD3D12())
            {
                LOG_ERROR_AND_THROW("Failed to load d3d12 dll");
            }

            Uint32 NumAdapters = 0;
            pFactoryD3D12->EnumerateAdapters(DIRECT3D_FEATURE_LEVEL_11_0, NumAdapters, 0);
            Adapters.resize(NumAdapters);
            pFactoryD3D12->EnumerateAdapters(DIRECT3D_FEATURE_LEVEL_11_0, NumAdapters, Adapters.data());

            EngineD3D12CreateInfo CreateInfo;
            CreateInfo.DebugMessageCallback = MessageCallback;
            CreateInfo.Features             = DeviceFeatures{DEVICE_FEATURE_STATE_OPTIONAL};

            LOG_INFO_MESSAGE("Found ", Adapters.size(), " compatible adapters");
            for (Uint32 i = 0; i < Adapters.size(); ++i)
            {
                const auto& AdapterInfo = Adapters[i];

                std::vector<DisplayModeAttribs> DisplayModes;
                if (AdapterInfo.NumOutputs > 0)
                {
                    Uint32 NumDisplayModes = 0;
                    pFactoryD3D12->EnumerateDisplayModes(DIRECT3D_FEATURE_LEVEL_11_0, i, 0, TEX_FORMAT_RGBA8_UNORM, NumDisplayModes, nullptr);
                    DisplayModes.resize(NumDisplayModes);
                    pFactoryD3D12->EnumerateDisplayModes(DIRECT3D_FEATURE_LEVEL_11_0, i, 0, TEX_FORMAT_RGBA8_UNORM, NumDisplayModes, DisplayModes.data());
                }

                PrintAdapterInfo(i, AdapterInfo, DisplayModes);
            }

            CreateInfo.AdapterId = FindAdapater(Adapters, CI.AdapterType, CI.AdapterId);

            CreateInfo.EnableDebugLayer = true;
            //CreateInfo.EnableGPUBasedValidation                = true;
            CreateInfo.CPUDescriptorHeapAllocationSize[0]      = 64; // D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
            CreateInfo.CPUDescriptorHeapAllocationSize[1]      = 32; // D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
            CreateInfo.CPUDescriptorHeapAllocationSize[2]      = 16; // D3D12_DESCRIPTOR_HEAP_TYPE_RTV
            CreateInfo.CPUDescriptorHeapAllocationSize[3]      = 16; // D3D12_DESCRIPTOR_HEAP_TYPE_DSV
            CreateInfo.DynamicDescriptorAllocationChunkSize[0] = 8;  // D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
            CreateInfo.DynamicDescriptorAllocationChunkSize[1] = 8;  // D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
            ppContexts.resize(1 + NumDeferredCtx);

            CreateInfo.NumDeferredContexts = NumDeferredCtx;
            pFactoryD3D12->CreateDeviceAndContextsD3D12(CreateInfo, &m_pDevice, ppContexts.data());
        }
        break;
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
        case RENDER_DEVICE_TYPE_GL:
        case RENDER_DEVICE_TYPE_GLES:
        {
#    if EXPLICITLY_LOAD_ENGINE_GL_DLL
            // Declare function pointer
            // Load the dll and import GetEngineFactoryOpenGL() function
            auto GetEngineFactoryOpenGL = LoadGraphicsEngineOpenGL();
            if (GetEngineFactoryOpenGL == nullptr)
            {
                LOG_ERROR_AND_THROW("Failed to load the engine");
            }
#    endif
            auto* pFactoryOpenGL = GetEngineFactoryOpenGL();

            auto Window = CreateNativeWindow();

            EngineGLCreateInfo CreateInfo;
            CreateInfo.DebugMessageCallback      = MessageCallback;
            CreateInfo.Window                    = Window;
            CreateInfo.CreateDebugContext        = true;
            CreateInfo.Features                  = DeviceFeatures{DEVICE_FEATURE_STATE_OPTIONAL};
            CreateInfo.ForceNonSeparablePrograms = CI.ForceNonSeparablePrograms;
            if (NumDeferredCtx != 0)
            {
                LOG_ERROR_MESSAGE("Deferred contexts are not supported in OpenGL mode");
                NumDeferredCtx = 0;
            }
            ppContexts.resize(1 + NumDeferredCtx);
            RefCntAutoPtr<ISwapChain> pSwapChain; // We will use testing swap chain instead
            pFactoryOpenGL->CreateDeviceAndSwapChainGL(
                CreateInfo, &m_pDevice, ppContexts.data(), SCDesc, &pSwapChain);
        }
        break;
#endif

#if VULKAN_SUPPORTED
        case RENDER_DEVICE_TYPE_VULKAN:
        {
#    if EXPLICITLY_LOAD_ENGINE_VK_DLL
            // Load the dll and import GetEngineFactoryVk() function
            auto GetEngineFactoryVk = LoadGraphicsEngineVk();
            if (GetEngineFactoryVk == nullptr)
            {
                LOG_ERROR_AND_THROW("Failed to load the engine");
            }
#    endif

            EngineVkCreateInfo CreateInfo;
            CreateInfo.AdapterId                 = CI.AdapterId;
            CreateInfo.DebugMessageCallback      = MessageCallback;
            CreateInfo.EnableValidation          = true;
            CreateInfo.MainDescriptorPoolSize    = VulkanDescriptorPoolSize{64, 64, 256, 256, 64, 32, 32, 32, 32, 16};
            CreateInfo.DynamicDescriptorPoolSize = VulkanDescriptorPoolSize{64, 64, 256, 256, 64, 32, 32, 32, 32, 16};
            CreateInfo.UploadHeapPageSize        = 32 * 1024;
            //CreateInfo.DeviceLocalMemoryReserveSize = 32 << 20;
            //CreateInfo.HostVisibleMemoryReserveSize = 48 << 20;
            CreateInfo.Features = DeviceFeatures{DEVICE_FEATURE_STATE_OPTIONAL};

            CreateInfo.NumDeferredContexts = NumDeferredCtx;
            ppContexts.resize(1 + NumDeferredCtx);
            auto* pFactoryVk = GetEngineFactoryVk();
            pFactoryVk->CreateDeviceAndContextsVk(CreateInfo, &m_pDevice, ppContexts.data());
        }
        break;
#endif

#if METAL_SUPPORTED
        case RENDER_DEVICE_TYPE_METAL:
        {
            EngineMtlCreateInfo MtlAttribs;

            MtlAttribs.DebugMessageCallback = MessageCallback;
            MtlAttribs.NumDeferredContexts  = NumDeferredCtx;
            ppContexts.resize(1 + NumDeferredCtx);
            auto* pFactoryMtl = GetEngineFactoryMtl();
            pFactoryMtl->CreateDeviceAndContextsMtl(MtlAttribs, &m_pDevice, ppContexts.data());
        }
        break;
#endif

        default:
            LOG_ERROR_AND_THROW("Unknown device type");
            break;
    }
    m_pDeviceContext.Attach(ppContexts[0]);

    const auto& AdapterInfo = m_pDevice->GetDeviceCaps().AdapterInfo;
    std::string AdapterInfoStr;
    AdapterInfoStr = "Adapter description: ";
    AdapterInfoStr += AdapterInfo.Description;
    AdapterInfoStr += ". Vendor: ";
    switch (AdapterInfo.Vendor)
    {
        case ADAPTER_VENDOR_NVIDIA:
            AdapterInfoStr += "NVidia";
            break;
        case ADAPTER_VENDOR_AMD:
            AdapterInfoStr += "AMD";
            break;
        case ADAPTER_VENDOR_INTEL:
            AdapterInfoStr += "Intel";
            break;
        case ADAPTER_VENDOR_ARM:
            AdapterInfoStr += "ARM";
            break;
        case ADAPTER_VENDOR_QUALCOMM:
            AdapterInfoStr += "Qualcomm";
            break;
        case ADAPTER_VENDOR_IMGTECH:
            AdapterInfoStr += "Imagination tech";
            break;
        case ADAPTER_VENDOR_MSFT:
            AdapterInfoStr += "Microsoft";
            break;
        default:
            AdapterInfoStr += "Unknown";
    }
    AdapterInfoStr += ". Local memory: ";
    AdapterInfoStr += std::to_string(AdapterInfo.DeviceLocalMemory >> 20);
    AdapterInfoStr += " MB. Host-visible memory: ";
    AdapterInfoStr += std::to_string(AdapterInfo.HostVisibileMemory >> 20);
    AdapterInfoStr += " MB. Unified memory: ";
    AdapterInfoStr += std::to_string(AdapterInfo.UnifiedMemory >> 20);
    AdapterInfoStr += " MB.";
    LOG_INFO_MESSAGE(AdapterInfoStr);
}

TestingEnvironment::~TestingEnvironment()
{
    m_pDeviceContext->Flush();
    m_pDeviceContext->FinishFrame();
}

// Override this to define how to set up the environment.
void TestingEnvironment::SetUp()
{
}

// Override this to define how to tear down the environment.
void TestingEnvironment::TearDown()
{
}

void TestingEnvironment::ReleaseResources()
{
    // It is necessary to call Flush() to force the driver to release resources.
    // Without flushing the command buffer, the memory may not be released until sometimes
    // later causing out-of-memory error.
    m_pDeviceContext->Flush();
    m_pDeviceContext->FinishFrame();
    m_pDevice->ReleaseStaleResources();
}

void TestingEnvironment::Reset()
{
    m_pDeviceContext->Flush();
    m_pDeviceContext->FinishFrame();
    m_pDevice->IdleGPU();
    m_pDevice->ReleaseStaleResources();
    m_pDeviceContext->InvalidateState();
    m_NumAllowedErrors = 0;
}

RefCntAutoPtr<ITexture> TestingEnvironment::CreateTexture(const char* Name, TEXTURE_FORMAT Fmt, BIND_FLAGS BindFlags, Uint32 Width, Uint32 Height)
{
    TextureDesc TexDesc;

    TexDesc.Name      = Name;
    TexDesc.Type      = RESOURCE_DIM_TEX_2D;
    TexDesc.Format    = Fmt;
    TexDesc.BindFlags = BindFlags;
    TexDesc.Width     = Width;
    TexDesc.Height    = Height;

    RefCntAutoPtr<ITexture> pTexture;
    m_pDevice->CreateTexture(TexDesc, nullptr, &pTexture);
    VERIFY_EXPR(pTexture != nullptr);

    return pTexture;
}

void TestingEnvironment::SetDefaultCompiler(SHADER_COMPILER compiler)
{
    switch (m_pDevice->GetDeviceCaps().DevType)
    {
        case RENDER_DEVICE_TYPE_D3D12:
            switch (compiler)
            {
                case SHADER_COMPILER_DEFAULT:
                case SHADER_COMPILER_FXC:
                case SHADER_COMPILER_DXC:
                    m_ShaderCompiler = compiler;
                    break;

                default:
                    LOG_WARNING_MESSAGE(GetShaderCompilerTypeString(compiler), " is not supported by Direct3D12 backend. Using default compiler");
                    m_ShaderCompiler = SHADER_COMPILER_DEFAULT;
            }
            break;

        case RENDER_DEVICE_TYPE_D3D11:
            switch (compiler)
            {
                case SHADER_COMPILER_DEFAULT:
                case SHADER_COMPILER_FXC:
                    m_ShaderCompiler = compiler;
                    break;

                default:
                    LOG_WARNING_MESSAGE(GetShaderCompilerTypeString(compiler), " is not supported by Direct3D11 backend. Using default compiler");
                    m_ShaderCompiler = SHADER_COMPILER_DEFAULT;
            }
            break;

        case RENDER_DEVICE_TYPE_GL:
        case RENDER_DEVICE_TYPE_GLES:
            switch (compiler)
            {
                case SHADER_COMPILER_DEFAULT:
                    m_ShaderCompiler = compiler;
                    break;

                default:
                    LOG_WARNING_MESSAGE(GetShaderCompilerTypeString(compiler), " is not supported by OpenGL/GLES backend. Using default compiler");
                    m_ShaderCompiler = SHADER_COMPILER_DEFAULT;
            }
            break;

        case RENDER_DEVICE_TYPE_VULKAN:
            switch (compiler)
            {
                case SHADER_COMPILER_DEFAULT:
                case SHADER_COMPILER_GLSLANG:
                case SHADER_COMPILER_DXC:
                    m_ShaderCompiler = compiler;
                    break;

                default:
                    LOG_WARNING_MESSAGE(GetShaderCompilerTypeString(compiler), " is not supported by Vulkan backend. Using default compiler");
                    m_ShaderCompiler = SHADER_COMPILER_DEFAULT;
            }
            break;

        case RENDER_DEVICE_TYPE_METAL:
            switch (compiler)
            {
                case SHADER_COMPILER_DEFAULT:
                    m_ShaderCompiler = compiler;
                    break;

                default:
                    LOG_WARNING_MESSAGE(GetShaderCompilerTypeString(compiler), " is not supported by Metal backend. Using default compiler");
                    m_ShaderCompiler = SHADER_COMPILER_DEFAULT;
            }
            break;

        default:
            LOG_WARNING_MESSAGE("Unepxected device type");
            m_ShaderCompiler = SHADER_COMPILER_DEFAULT;
    }

    LOG_INFO_MESSAGE("Selected shader compiler: ", GetShaderCompilerTypeString(m_ShaderCompiler));
}

SHADER_COMPILER TestingEnvironment::GetDefaultCompiler(SHADER_SOURCE_LANGUAGE lang) const
{
    if (m_pDevice->GetDeviceCaps().DevType == RENDER_DEVICE_TYPE_VULKAN &&
        lang != SHADER_SOURCE_LANGUAGE_HLSL)
        return SHADER_COMPILER_GLSLANG;
    else
        return m_ShaderCompiler;
}

} // namespace Testing

} // namespace Diligent
