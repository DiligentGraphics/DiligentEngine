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

#include <sstream>
#include <math.h>
#include <iomanip>

#include "PlatformDefinitions.h"
#include "TestApp.h"
#include "Errors.h"
#include "StringTools.h"

#if D3D11_SUPPORTED
#   include "RenderDeviceFactoryD3D11.h"
#endif

#if D3D12_SUPPORTED
#   include "RenderDeviceFactoryD3D12.h"
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
#   include "RenderDeviceFactoryOpenGL.h"
#endif

#if VULKAN_SUPPORTED
#   include "RenderDeviceFactoryVk.h"
#endif

#if METAL_SUPPORTED
#   include "RenderDeviceFactoryMtl.h"
#endif

#include "FileSystem.h"
#include "MapHelper.h"
#include "RenderScriptTest.h"
#include "ScriptParser.h"
#include "Errors.h"
#include "ConvenienceFunctions.h"
#include "TestDepthStencilState.h"
#include "TestRasterizerState.h"
#include "TestBlendState.h"
#include "TestVPAndSR.h"
#include "TestPSOCompatibility.h"
#if GL_SUPPORTED || GLES_SUPPORTED
    #include "ShaderConverterTest.h"
#endif
#include "TestCopyTexData.h"
#include "PlatformMisc.h"
#include "TestBufferCreation.h"
#include "TestBrokenShader.h"
#include "TestShaderResourceLayout.h"
#include "TestShaderVarAccess.h"
#include "TestSeparateTextureSampler.h"
#include "StringTools.h"

using namespace Diligent;

TestApp::TestApp() :
    m_AppTitle("Test app")
{
    VERIFY_EXPR(PlatformMisc::GetMSB(Uint32{0}) == 32);
    for (Uint32 i = 0; i < 32; ++i)
    {
        auto MSB = PlatformMisc::GetMSB((Uint32{1} << i) | 1);
        VERIFY_EXPR(MSB == i);
    }
    
    VERIFY_EXPR(PlatformMisc::GetMSB(Uint64{0}) == 64);
    for (Uint32 i = 0; i < 64; ++i)
    {
        auto MSB = PlatformMisc::GetMSB((Uint64{1} << i) | 1);
        VERIFY_EXPR(MSB == i);
    }
    
    VERIFY_EXPR(PlatformMisc::GetLSB(Uint32{0}) == 32);
    for (Uint32 i = 0; i < 32; ++i)
    {
        auto LSB = PlatformMisc::GetLSB((Uint32{1} << i) | (Uint32{1}<<31));
        VERIFY_EXPR(LSB == i);
    }

    VERIFY_EXPR(PlatformMisc::GetLSB(Uint64{0}) == 64);
    for (Uint32 i = 0; i < 64; ++i)
    {
        auto LSB = PlatformMisc::GetLSB((Uint64{1} << i) | (Uint64{1}<<63));
        VERIFY_EXPR(LSB == i);
    }

    VERIFY_EXPR(PlatformMisc::CountOneBits(Uint32{0}) == 0);
    VERIFY_EXPR(PlatformMisc::CountOneBits(Uint64{0}) == 0);
    VERIFY_EXPR(PlatformMisc::CountOneBits(Uint32{1}) == 1);
    VERIFY_EXPR(PlatformMisc::CountOneBits(Uint64{1}) == 1);
    VERIFY_EXPR(PlatformMisc::CountOneBits(Uint32{7}) == 3);
    VERIFY_EXPR(PlatformMisc::CountOneBits(Uint64{7}) == 3);
    VERIFY_EXPR(PlatformMisc::CountOneBits( (Uint32{1}<<31) | (Uint32{1} << 15)) == 2);
    VERIFY_EXPR(PlatformMisc::CountOneBits( (Uint64{1}<<63) | (Uint32{1} << 31)) == 2);
    VERIFY_EXPR(PlatformMisc::CountOneBits( (Uint32{1}<<31) - 1) == 31);
    VERIFY_EXPR(PlatformMisc::CountOneBits( (Uint64{1}<<63) - 1) == 63);


    VERIFY_EXPR(StreqSuff("abc_def","abc", "_def"));
    VERIFY_EXPR(!StreqSuff("abc","abc", "_def"));
    VERIFY_EXPR(!StreqSuff("ab","abc", "_def"));
    VERIFY_EXPR(!StreqSuff("abc_de","abc", "_def"));
    VERIFY_EXPR(!StreqSuff("abc_def","ab", "_def"));
    VERIFY_EXPR(!StreqSuff("abc_def","abd", "_def"));
    VERIFY_EXPR(!StreqSuff("abc_def","abc", "_de"));
    VERIFY_EXPR(!StreqSuff("abc","abc", "_def"));
    VERIFY_EXPR(!StreqSuff("abc_def", "", "_def"));
    VERIFY_EXPR(!StreqSuff("abc_def", "", ""));
    
    VERIFY_EXPR(StreqSuff("abc","abc", "_def", true));
    VERIFY_EXPR(!StreqSuff("abc","abc_", "_def", true));
    VERIFY_EXPR(!StreqSuff("abc_","abc", "_def", true));
    VERIFY_EXPR(StreqSuff("abc","abc", nullptr, true));
    VERIFY_EXPR(StreqSuff("abc","abc", nullptr, false));
    VERIFY_EXPR(!StreqSuff("ab","abc", nullptr, true));
    VERIFY_EXPR(!StreqSuff("abc","ab", nullptr, false));
}

TestApp::~TestApp()
{
    if(m_pMTResCreationTest)
        m_pMTResCreationTest->StopThreads();
}


void TestApp::InitializeDiligentEngine(
#if PLATFORM_LINUX
        void *display,
#endif
    void *NativeWindowHandle
    )
{
    SwapChainDesc SCDesc;
    SCDesc.SamplesCount = 1;
    Uint32 NumDeferredCtx = 0;
    std::vector<IDeviceContext*> ppContexts;
    std::vector<HardwareAdapterAttribs> Adapters;
    std::vector<std::vector<DisplayModeAttribs>> AdapterDisplayModes;
    switch (m_DeviceType)
    {
#if D3D11_SUPPORTED
        case DeviceType::D3D11:
        {
            EngineD3D11Attribs DeviceAttribs;
#if ENGINE_DLL
            GetEngineFactoryD3D11Type GetEngineFactoryD3D11 = nullptr;
            // Load the dll and import GetEngineFactoryD3D11() function
            LoadGraphicsEngineD3D11(GetEngineFactoryD3D11);
#endif
            auto *pFactoryD3D11 = GetEngineFactoryD3D11();
            Uint32 NumAdapters = 0;
            pFactoryD3D11->EnumerateHardwareAdapters(NumAdapters, 0);
            Adapters.resize(NumAdapters);
            pFactoryD3D11->EnumerateHardwareAdapters(NumAdapters, Adapters.data());

            for(Uint32 i=0; i < Adapters.size(); ++i)
            {
                Uint32 NumDisplayModes = 0;
                std::vector<DisplayModeAttribs> DisplayModes;
                pFactoryD3D11->EnumerateDisplayModes(i, 0, TEX_FORMAT_RGBA8_UNORM, NumDisplayModes, nullptr);
                DisplayModes.resize(NumDisplayModes);
                pFactoryD3D11->EnumerateDisplayModes(i, 0, TEX_FORMAT_RGBA8_UNORM, NumDisplayModes, DisplayModes.data());
                AdapterDisplayModes.emplace_back(std::move(DisplayModes));
            }

            ppContexts.resize(1 + NumDeferredCtx);
            pFactoryD3D11->CreateDeviceAndContextsD3D11(DeviceAttribs, &m_pDevice, ppContexts.data(), NumDeferredCtx);

            if(NativeWindowHandle != nullptr)
                pFactoryD3D11->CreateSwapChainD3D11(m_pDevice, ppContexts[0], SCDesc, FullScreenModeDesc{}, NativeWindowHandle, &m_pSwapChain);
        }
        break;
#endif

#if D3D12_SUPPORTED
        case DeviceType::D3D12:
        {
#if ENGINE_DLL
            GetEngineFactoryD3D12Type GetEngineFactoryD3D12 = nullptr;
            // Load the dll and import GetEngineFactoryD3D12() function
            LoadGraphicsEngineD3D12(GetEngineFactoryD3D12);
#endif
            auto *pFactoryD3D12 = GetEngineFactoryD3D12();
            Uint32 NumAdapters = 0;
            pFactoryD3D12->EnumerateHardwareAdapters(NumAdapters, 0);
            Adapters.resize(NumAdapters);
            pFactoryD3D12->EnumerateHardwareAdapters(NumAdapters, Adapters.data());

            for (Uint32 i = 0; i < Adapters.size(); ++i)
            {
                Uint32 NumDisplayModes = 0;
                std::vector<DisplayModeAttribs> DisplayModes;
                pFactoryD3D12->EnumerateDisplayModes(i, 0, TEX_FORMAT_RGBA8_UNORM, NumDisplayModes, nullptr);
                DisplayModes.resize(NumDisplayModes);
                pFactoryD3D12->EnumerateDisplayModes(i, 0, TEX_FORMAT_RGBA8_UNORM, NumDisplayModes, DisplayModes.data());
                AdapterDisplayModes.emplace_back(std::move(DisplayModes));
            }

            EngineD3D12Attribs EngD3D12Attribs;
            EngD3D12Attribs.CPUDescriptorHeapAllocationSize[0] = 64; // D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
            EngD3D12Attribs.CPUDescriptorHeapAllocationSize[1] = 32; // D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
            EngD3D12Attribs.CPUDescriptorHeapAllocationSize[2] = 16; // D3D12_DESCRIPTOR_HEAP_TYPE_RTV
            EngD3D12Attribs.CPUDescriptorHeapAllocationSize[3] = 16; // D3D12_DESCRIPTOR_HEAP_TYPE_DSV
            EngD3D12Attribs.DynamicDescriptorAllocationChunkSize[0] = 8; // D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
            EngD3D12Attribs.DynamicDescriptorAllocationChunkSize[1] = 8; // D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
            ppContexts.resize(1 + NumDeferredCtx);
            
            pFactoryD3D12->CreateDeviceAndContextsD3D12(EngD3D12Attribs, &m_pDevice, ppContexts.data(), NumDeferredCtx);

            if (!m_pSwapChain && NativeWindowHandle != nullptr)
                pFactoryD3D12->CreateSwapChainD3D12(m_pDevice, ppContexts[0], SCDesc, FullScreenModeDesc{}, NativeWindowHandle, &m_pSwapChain);
        }
        break;
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
        case DeviceType::OpenGL:
        case DeviceType::OpenGLES:
        {
#if !PLATFORM_MACOS
            VERIFY_EXPR(NativeWindowHandle != nullptr);
#endif
#if ENGINE_DLL && (PLATFORM_WIN32 || PLATFORM_UNIVERSAL_WINDOWS)
            // Declare function pointer
            GetEngineFactoryOpenGLType GetEngineFactoryOpenGL = nullptr;
            // Load the dll and import GetEngineFactoryOpenGL() function
            LoadGraphicsEngineOpenGL(GetEngineFactoryOpenGL);
#endif
            auto *pFactoryOpenGL = GetEngineFactoryOpenGL();
            EngineGLAttribs CreationAttribs;
            CreationAttribs.pNativeWndHandle = NativeWindowHandle;
#if PLATFORM_LINUX
            CreationAttribs.pDisplay = display;
#endif
            if (NumDeferredCtx != 0)
            {
                LOG_ERROR_MESSAGE("Deferred contexts are not supported in OpenGL mode");
                NumDeferredCtx = 0;
            }
            ppContexts.resize(1 + NumDeferredCtx);
            pFactoryOpenGL->CreateDeviceAndSwapChainGL(
                CreationAttribs, &m_pDevice, ppContexts.data(), SCDesc, &m_pSwapChain);
        }
        break;
#endif

#if VULKAN_SUPPORTED
        case DeviceType::Vulkan:
        {
#if ENGINE_DLL && PLATFORM_WIN32
            GetEngineFactoryVkType GetEngineFactoryVk = nullptr;
            // Load the dll and import GetEngineFactoryVk() function
            LoadGraphicsEngineVk(GetEngineFactoryVk);
#endif
            EngineVkAttribs EngVkAttribs;

            EngVkAttribs.EnableValidation = true;
            EngVkAttribs.MainDescriptorPoolSize = EngineVkAttribs::DescriptorPoolSize{ 64, 64, 256, 256, 64, 32, 32, 32, 32 };
            EngVkAttribs.DynamicDescriptorPoolSize = EngineVkAttribs::DescriptorPoolSize{ 64, 64, 256, 256, 64, 32, 32, 32, 32 };
            EngVkAttribs.UploadHeapPageSize = 32*1024;
            //EngVkAttribs.DeviceLocalMemoryReserveSize = 32 << 20;
            //EngVkAttribs.HostVisibleMemoryReserveSize = 48 << 20;

            auto& Features = EngVkAttribs.EnabledFeatures;
            Features.depthBiasClamp                 = true;
            Features.fillModeNonSolid               = true;
            Features.depthClamp                     = true;
            Features.independentBlend               = true;
            Features.samplerAnisotropy              = true;
            Features.geometryShader                 = true;
            Features.tessellationShader             = true;
            Features.dualSrcBlend                   = true;
            Features.multiViewport                  = true;
            Features.imageCubeArray                 = true;
            Features.textureCompressionBC           = true;
            Features.vertexPipelineStoresAndAtomics = true;
            Features.fragmentStoresAndAtomics       = true;

            ppContexts.resize(1 + NumDeferredCtx);
            auto *pFactoryVk = GetEngineFactoryVk();
            pFactoryVk->CreateDeviceAndContextsVk(EngVkAttribs, &m_pDevice, ppContexts.data(), NumDeferredCtx);

            if (!m_pSwapChain && NativeWindowHandle != nullptr)
                pFactoryVk->CreateSwapChainVk(m_pDevice, ppContexts[0], SCDesc, NativeWindowHandle, &m_pSwapChain);
        }
        break;
#endif

#if METAL_SUPPORTED
        case DeviceType::Metal:
        {
            EngineMtlAttribs MtlAttribs;

            ppContexts.resize(1 + NumDeferredCtx);
            auto *pFactoryMtl = GetEngineFactoryMtl();
            pFactoryMtl->CreateDeviceAndContextsMtl(MtlAttribs, &m_pDevice, ppContexts.data(), NumDeferredCtx);

            if (!m_pSwapChain && NativeWindowHandle != nullptr)
                pFactoryMtl->CreateSwapChainMtl(m_pDevice, ppContexts[0], SCDesc, NativeWindowHandle, &m_pSwapChain);
        }
        break;
#endif

        default:
            LOG_ERROR_AND_THROW("Unknown device type");
            break;
    }


    std::stringstream ss;
    ss << "Found " << Adapters.size() << " adapters:\n";
    for (Uint32 i = 0; i < Adapters.size(); ++i)
    {
        ss << Adapters[i].Description << " (" << (Adapters[i].DedicatedVideoMemory >> 20) << " MB). Num outputs: " << Adapters[i].NumOutputs << ". Display modes:\n";
        const auto &DisplayModes = AdapterDisplayModes[i];
        for(Uint32 m=0; m < DisplayModes.size(); ++m)
        {
            const auto &Mode = DisplayModes[m];
            float RefreshRate = (float)Mode.RefreshRateNumerator / (float)Mode.RefreshRateDenominator;
            ss << "   " << Mode.Width << 'x' << Mode.Height << " " << std::fixed << std::setprecision(2) << RefreshRate << " Hz\n";
        }
    }
    auto str = ss.str();
    LOG_INFO_MESSAGE(str);

    m_pImmediateContext.Attach(ppContexts[0]);
    m_pDeferredContexts.resize(NumDeferredCtx);
    for (Diligent::Uint32 ctx = 0; ctx < NumDeferredCtx; ++ctx)
        m_pDeferredContexts[ctx].Attach(ppContexts[1 + ctx]);
}

void TestApp::InitializeRenderers()
{
    m_pMTResCreationTest.reset(new MTResourceCreationTest(m_pDevice, m_pImmediateContext, 7));
    
    TestSeparateTextureSampler TestSeparateTexSampler{m_pDevice, m_pImmediateContext};
    TestRasterizerState TestRS{m_pDevice, m_pImmediateContext};
    TestBlendState TestBS{m_pDevice, m_pImmediateContext};
    TestDepthStencilState TestDSS{m_pDevice, m_pImmediateContext};
    TestBufferCreation TestBuffCreation{m_pDevice, m_pImmediateContext};
    TestTextureCreation TestTexCreation{m_pDevice, m_pImmediateContext};
    TestPSOCompatibility TestPSOCompat{m_pDevice};
    TestBrokenShader TestBrknShdr{m_pDevice};
        
    m_TestGS.Init(m_pDevice, m_pImmediateContext, m_pSwapChain);
    m_TestTessellation.Init(m_pDevice, m_pImmediateContext, m_pSwapChain);
    m_pTestShaderResArrays.reset(new TestShaderResArrays(m_pDevice, m_pImmediateContext, m_pSwapChain, 0.4f, -0.9f, 0.5f, 0.5f));
        
    TestShaderVarAccess TestShaderVarAccess{m_pDevice, m_pImmediateContext, m_pSwapChain};
    TestShaderResourceLayout TestShaderResLayout{m_pDevice, m_pImmediateContext};
    
#if GL_SUPPORTED || GLES_SUPPORTED || VULKAN_SUPPORTED
    ShaderConverterTest ConverterTest{m_pDevice, m_pImmediateContext};
#endif
    
    TestSamplerCreation TestSamplers{m_pDevice};
    
    RenderScriptTest LuaTest{m_pDevice, m_pImmediateContext};
    
    const auto* BackBufferFmt = m_pDevice->GetTextureFormatInfo(m_pSwapChain->GetDesc().ColorBufferFormat).Name;
    const auto* DepthBufferFmt = m_pDevice->GetTextureFormatInfo(m_pSwapChain->GetDesc().DepthBufferFormat).Name;
    m_pRenderScript = CreateRenderScriptFromFile("TestRenderScripts.lua", m_pDevice, m_pImmediateContext, [BackBufferFmt, DepthBufferFmt](ScriptParser *pScriptParser)
    {
        pScriptParser->SetGlobalVariable( "extBackBufferFormat", BackBufferFmt );
        pScriptParser->SetGlobalVariable( "extDepthBufferFormat", DepthBufferFmt );
    });

    m_pTestDrawCommands.reset(new TestDrawCommands);
    m_pTestDrawCommands->Init(m_pDevice, m_pImmediateContext, m_pSwapChain, 0, 0, 1, 1);

    m_pTestBufferAccess.reset(new TestBufferAccess);
    m_pTestBufferAccess->Init(m_pDevice, m_pImmediateContext, m_pSwapChain, -1, 0, 0.5, 0.5);
    

    TEXTURE_FORMAT TestFormats[16] =
    {
        TEX_FORMAT_RGBA8_UNORM, TEX_FORMAT_RGBA8_UNORM_SRGB, TEX_FORMAT_RGBA32_FLOAT, TEX_FORMAT_RGBA16_UINT,
        TEX_FORMAT_RGBA8_SNORM, TEX_FORMAT_RGBA8_UINT,       TEX_FORMAT_RG8_UNORM,    TEX_FORMAT_RG8_UINT,
        TEX_FORMAT_RG32_FLOAT,  TEX_FORMAT_RG16_UINT,       TEX_FORMAT_RG8_SNORM,    TEX_FORMAT_R8_UNORM,
        TEX_FORMAT_R32_FLOAT,   TEX_FORMAT_R8_SNORM,         TEX_FORMAT_R8_UINT,      TEX_FORMAT_R16_UINT
    };

    for (int j = 0; j < 4; ++j)
        for (int i = 0; i < 4; ++i)
        {
            auto Ind = i + j * 4;
            m_pTestTexturing[Ind].reset(new TestTexturing);
            m_pTestTexturing[Ind]->Init(m_pDevice, m_pImmediateContext, m_pSwapChain, TestFormats[Ind], -1 + (float)i*1.f / 4.f, -1 + (float)j*1.f / 4.f, 0.9f / 4.f, 0.9f / 4.f);
        }

    TestCopyTexData TestCopyData(m_pDevice, m_pImmediateContext);

    TestVPAndSR TestVPAndSR(m_pDevice, m_pImmediateContext);

    m_pTestCS.reset(new TestComputeShaders);
    m_pTestCS->Init(m_pDevice, m_pImmediateContext, m_pSwapChain);

    m_pTestRT.reset(new TestRenderTarget);
    m_pTestRT->Init(m_pDevice, m_pImmediateContext, m_pSwapChain, -0.4f, 0.55f, 0.4f, 0.4f);

    m_pMTResCreationTest->StartThreads();

    float instance_offsets[] = { -0.3f, 0.0f, 0.0f, 0.0f, +0.3f, -0.3f };
    
    {
        m_pRenderScript->GetBufferByName("InstanceBuffer", &m_pInstBuff);
    }

    {
        auto BuffDesc = m_pInstBuff->GetDesc();
        BuffDesc.uiSizeInBytes = sizeof(instance_offsets);
        //BuffDesc.BindFlags = BIND_VERTEX_BUFFER;
        BuffDesc.Usage = USAGE_DEFAULT;
        //Diligent::BufferData BuffData;
        //BuffData.pData = instance_offsets;
        //BuffData.DataSize = sizeof(instance_offsets);
        m_pDevice->CreateBuffer(BuffDesc, Diligent::BufferData(), &m_pInstBuff2);
    }
    

    {
        m_pRenderScript->GetBufferByName("UnfiformBuffer1", &m_pUniformBuff);
    }

    {
        Diligent::BufferDesc BuffDesc;
        BuffDesc.Name = "cbTestBlock2";
        float UniformData[16] = { 1,1,1,1 };
        BuffDesc.uiSizeInBytes = sizeof(UniformData);
        BuffDesc.BindFlags = BIND_UNIFORM_BUFFER;
        BuffDesc.Usage = USAGE_DYNAMIC;
        BuffDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        m_pDevice->CreateBuffer(BuffDesc, BufferData(), &m_pUniformBuff2);
    }

    {
        Diligent::BufferDesc BuffDesc;
        BuffDesc.Name = "Test Constant Buffer 3";
        float UniformData[16] = { 1, 1, 1, 1 };
        BuffDesc.uiSizeInBytes = sizeof(UniformData);
        BuffDesc.BindFlags = BIND_UNIFORM_BUFFER;
        BuffDesc.Usage = USAGE_DEFAULT;
        BuffDesc.CPUAccessFlags = CPU_ACCESS_NONE;
        Diligent::BufferData BuffData;
        BuffData.pData = UniformData;
        BuffData.DataSize = sizeof(UniformData);
        m_pDevice->CreateBuffer(BuffDesc, BuffData, &m_pUniformBuff3);
    }

    {
        Diligent::BufferDesc BuffDesc;
        BuffDesc.Name = "Test Constant Buffer 4";
        float UniformData[16] = { 1, 1, 1, 1 };
        BuffDesc.uiSizeInBytes = sizeof(UniformData);
        BuffDesc.BindFlags = BIND_UNIFORM_BUFFER;
        BuffDesc.Usage = USAGE_DEFAULT;
        BuffDesc.CPUAccessFlags = CPU_ACCESS_NONE;
        Diligent::BufferData BuffData;
        BuffData.pData = UniformData;
        BuffData.DataSize = sizeof(UniformData);
        m_pDevice->CreateBuffer(BuffDesc, BuffData, &m_pUniformBuff4);
    }

    {
        RefCntAutoPtr<IResourceMapping> pResMapping;
        m_pRenderScript->GetResourceMappingByName("ResMapping", &pResMapping);
        pResMapping->AddResource("cbTestBlock3", m_pUniformBuff3, true);
        m_pRenderScript->Run("AddConstBufferToMapping", "cbTestBlock4", m_pUniformBuff4);
        m_pRenderScript->Run("AddConstBufferToMapping", "cbTestBlock2", m_pUniformBuff2);
        m_pRenderScript->Run("BindShaderResources");
    }

    {
        TextureDesc TexDesc;
        TexDesc.Type = RESOURCE_DIM_TEX_2D;
        TexDesc.Width = 1024;
        TexDesc.Height = 1024;
        TexDesc.Format = TEX_FORMAT_RGBA8_UNORM;
        TexDesc.Usage = USAGE_DEFAULT;
        TexDesc.BindFlags = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET | BIND_UNORDERED_ACCESS;
        TexDesc.Name = "UniqueTexture";

        m_pDevice->CreateTexture(TexDesc, TextureData(), &m_pTestTex);

        m_pTestTex.Release();
        m_pDevice->CreateTexture(TexDesc, TextureData(), &m_pTestTex);
    }

    {
        m_pImmediateContext->Flush();
        // This is a test for possible bug in D3D12
        TextureDesc TexDesc;
        TexDesc.Type = RESOURCE_DIM_TEX_2D;
        TexDesc.Width = 512;
        TexDesc.Height = 512;
        TexDesc.Format = m_pSwapChain->GetDesc().ColorBufferFormat;
        TexDesc.Usage = USAGE_DEFAULT;
        TexDesc.BindFlags = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET;
        TexDesc.Name = "UniqueTexture";
        RefCntAutoPtr<ITexture> pTex;
        m_pDevice->CreateTexture(TexDesc, TextureData(), &pTex);
        ITextureView *pRTVs[] = { pTex->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET) };

        TexDesc.Format = m_pSwapChain->GetDesc().DepthBufferFormat;
        RefCntAutoPtr<ITexture> pDepthTex;
        TexDesc.BindFlags = BIND_DEPTH_STENCIL;
        m_pDevice->CreateTexture(TexDesc, TextureData(), &pDepthTex);
        auto* pDSV = pDepthTex->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL);

        {
            MapHelper<float> UniformData(m_pImmediateContext, m_pUniformBuff, MAP_WRITE, MAP_FLAG_DISCARD);
            UniformData[0] = UniformData[1] = UniformData[2] = UniformData[3] = 0;
        }

        {
            MapHelper<float> UniformData(m_pImmediateContext, m_pUniformBuff2, MAP_WRITE, MAP_FLAG_DISCARD);
            UniformData[0] = (float)sin(0)*0.1f;
            UniformData[1] = (float)cos(0)*0.1f;
            UniformData[2] = (float)sin(0)*0.1f;
            UniformData[3] = 0;
        }

        m_pImmediateContext->SetRenderTargets(0, nullptr, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        float ClearColor[] = {0.1f, 0.2f, 0.4f, 1.0f};
        m_pImmediateContext->ClearRenderTarget(nullptr, ClearColor, RESOURCE_STATE_TRANSITION_MODE_VERIFY);
        DrawAttribs DrawAttrs;
        DrawAttrs.NumVertices = 3;
        DrawAttrs.Flags = DRAW_FLAG_VERIFY_STATES;
        m_pRenderScript->Run(m_pImmediateContext, "DrawTris", DrawAttrs);
        
        // This adds transition barrier for pTex1
        m_pImmediateContext->SetRenderTargets(1, pRTVs, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        m_pImmediateContext->ClearRenderTarget(pRTVs[0], ClearColor, RESOURCE_STATE_TRANSITION_MODE_VERIFY);
        m_pImmediateContext->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1, 0, RESOURCE_STATE_TRANSITION_MODE_VERIFY);
        // Generate draw command to the bound render target
        m_pImmediateContext->Draw(DrawAttrs);
        m_pImmediateContext->SetRenderTargets(0, nullptr, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        // This will destroy texture and put D3D12 resource into release queue
        pTex.Release();


        Diligent::BufferDesc BuffDesc;
        BuffDesc.Name = "cbTestBlock2";
        float Data[16] = { 1,1,1,1 };
        BuffDesc.uiSizeInBytes = sizeof(Data);
        BuffDesc.BindFlags = BIND_UNIFORM_BUFFER;
        BuffDesc.Usage = USAGE_DEFAULT;
        BuffDesc.CPUAccessFlags = CPU_ACCESS_NONE;
        Diligent::BufferData BuffData;
        BuffData.pData = Data;
        BuffData.DataSize = sizeof(Data);
        // This will result in creating and executing another command list
        RefCntAutoPtr<IBuffer> pBuff;
        m_pDevice->CreateBuffer(BuffDesc, BuffData, &pBuff);

        // This may cause D3D12 error
        m_pImmediateContext->Flush();
    }

    {
        FenceDesc fenceDesc;
        fenceDesc.Name = "Test fence";
        m_pDevice->CreateFence(fenceDesc, &m_pFence);
    }
}

void TestApp::ProcessCommandLine(const char *CmdLine)
{
    const auto* Key = "mode=";
    const auto *pos = strstr(CmdLine, Key);
    if (pos != nullptr)
    {
        pos += strlen(Key);
        if (_stricmp(pos, "D3D11") == 0)
        {
            m_DeviceType = DeviceType::D3D11;
        }
        else if (_stricmp(pos, "D3D12") == 0)
        {
            m_DeviceType = DeviceType::D3D12;
        }
        else if (_stricmp(pos, "GL") == 0)
        {
            m_DeviceType = DeviceType::OpenGL;
        }
        else if (_stricmp(pos, "VK") == 0)
        {
            m_DeviceType = DeviceType::Vulkan;
        }
        else
        {
            LOG_ERROR_AND_THROW("Unknown device type. Only the following types are supported: D3D11, D3D12, GL, VK");
        }
    }
    else
    {
#if D3D12_SUPPORTED
        m_DeviceType = DeviceType::D3D12;
#elif VULKAN_SUPPORTED
        m_DeviceType = DeviceType::Vulkan;
#elif D3D11_SUPPORTED
        m_DeviceType = DeviceType::D3D11;
#elif GL_SUPPORTED || GLES_SUPPORTED
        m_DeviceType = DeviceType::OpenGL;
#endif
    }

    switch (m_DeviceType)
    {
        case DeviceType::D3D11: m_AppTitle.append(" (D3D11)"); break;
        case DeviceType::D3D12: m_AppTitle.append(" (D3D12)"); break;
        case DeviceType::OpenGL: m_AppTitle.append(" (OpenGL)"); break;
        case DeviceType::Vulkan: m_AppTitle.append(" (Vulkan)"); break;
        default: UNEXPECTED("Unknown device type");
    }
}

void TestApp::WindowResize(int width, int height)
{
    if (m_pSwapChain)
    {
        m_pSwapChain->Resize(width, height);
        //auto SCWidth = m_pSwapChain->GetDesc().Width;
        //auto SCHeight = m_pSwapChain->GetDesc().Height;
        

    }
}

void TestApp::Update(double CurrTime, double ElapsedTime)
{
    m_SmartPointerTest.RunConcurrencyTest();
    m_CurrTime = CurrTime;
}

void TestApp::Render()
{
    m_pImmediateContext->SetRenderTargets(0, nullptr, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    float ClearColor[] = {0.1f, 0.2f, 0.4f, 1.0f};
    m_pImmediateContext->ClearRenderTarget(nullptr, ClearColor, RESOURCE_STATE_TRANSITION_MODE_VERIFY);
    m_pImmediateContext->ClearDepthStencil(nullptr, CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_VERIFY);
    
    double dCurrTime = m_CurrTime;
    
    float instance_offsets[] = { -0.3f, (float)sin(dCurrTime + 0.5)*0.1f, 0.0f, (float)sin(dCurrTime)*0.1f, +0.3f, -0.3f + (float)cos(dCurrTime)*0.1f };
    m_pImmediateContext->UpdateBuffer(m_pInstBuff2, sizeof(float) * 1, sizeof(float) * 5, &instance_offsets[1], RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    m_pImmediateContext->CopyBuffer(m_pInstBuff2, sizeof(float) * 2, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, m_pInstBuff, sizeof(float) * 2, sizeof(float) * 4, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    
    {
        MapHelper<float> UniformData(m_pImmediateContext, m_pUniformBuff, MAP_WRITE, MAP_FLAG_DISCARD);
        UniformData[0] = UniformData[1] = UniformData[2] = UniformData[3] = (float)fabs(sin(dCurrTime));
    }
    
    {
        MapHelper<float> UniformData(m_pImmediateContext, m_pUniformBuff2, MAP_WRITE, MAP_FLAG_DISCARD);
        UniformData[0] = (float)sin(dCurrTime*3.8)*0.1f;
        UniformData[1] = (float)cos(dCurrTime*3.2)*0.1f;
        UniformData[2] = (float)sin(dCurrTime*3.9)*0.1f;
        UniformData[3] = 0;
    }

    DrawAttribs DrawAttrs;
    DrawAttrs.NumVertices = 3;
    DrawAttrs.Flags = DRAW_FLAG_VERIFY_STATES;
    m_pRenderScript->Run(m_pImmediateContext, "DrawTris", DrawAttrs);

    DrawAttrs.IsIndexed = true;
    DrawAttrs.NumIndices = 3;
    DrawAttrs.IndexType = VT_UINT32;
    DrawAttrs.NumInstances = 3;
    DrawAttrs.Flags = DRAW_FLAG_VERIFY_STATES;
    m_pRenderScript->Run(m_pImmediateContext, "DrawTris", DrawAttrs);
    m_pTestDrawCommands->Draw();
    m_pTestBufferAccess->Draw((float)dCurrTime);

    for (int i = 0; i<_countof(m_pTestTexturing); ++i)
    {
        m_pTestTexturing[i]->Draw();
    }
    m_pTestCS->Draw();
    m_pTestRT->Draw();
    m_pTestShaderResArrays->Draw();
    m_TestGS.Draw();
    m_TestTessellation.Draw();
    
    auto CompletedFenceValue = m_pFence->GetCompletedValue();
    VERIFY_EXPR(CompletedFenceValue < m_NextFenceValue);
    m_pImmediateContext->SignalFence(m_pFence, m_NextFenceValue++);

    m_pImmediateContext->Flush();
    m_pImmediateContext->InvalidateState();
    
    CompletedFenceValue = m_pFence->GetCompletedValue();
    VERIFY_EXPR(CompletedFenceValue < m_NextFenceValue);
}

void TestApp::Present()
{
    m_pSwapChain->Present(0);
}
