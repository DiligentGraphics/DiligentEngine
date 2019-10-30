// Copyright 2014 Intel Corporation All Rights Reserved
//
// Intel makes no representations about the suitability of this software for any purpose.  
// THIS SOFTWARE IS PROVIDED ""AS IS."" INTEL SPECIFICALLY DISCLAIMS ALL WARRANTIES,
// EXPRESS OR IMPLIED, AND ALL LIABILITY, INCLUDING CONSEQUENTIAL AND OTHER INDIRECT DAMAGES,
// FOR THE USE OF THIS SOFTWARE, INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PROPRIETARY
// RIGHTS, AND INCLUDING THE WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
// Intel does not assume any responsibility for any errors which may appear in this software
// nor any responsibility to update it.

#include <directxmath.h>
#include <math.h>

#include <iostream>
#include <algorithm>
#include <vector>
#include <future>
#include <limits>
#include <random>
#include <locale>
#include <codecvt>

#include "asteroids_DE.h"

#if D3D11_SUPPORTED
#include "EngineFactoryD3D11.h"
#endif

#if D3D12_SUPPORTED
#include "EngineFactoryD3D12.h"
#endif


#if GL_SUPPORTED
#include "EngineFactoryOpenGL.h"
#endif

#if VULKAN_SUPPORTED
#include "EngineFactoryVk.h"
#endif

#include "MapHelper.h"

#include "util.h"
#include "mesh.h"
#include "noise.h"
#include "texture.h"
#include "StringTools.h"
#include "TextureUtilities.h"

using namespace Diligent;

namespace Diligent
{
#if ENGINE_DLL
#   if D3D11_SUPPORTED
        GetEngineFactoryD3D11Type GetEngineFactoryD3D11 = nullptr;
#   endif
        
#   if D3D12_SUPPORTED
        GetEngineFactoryD3D12Type GetEngineFactoryD3D12 = nullptr;
#   endif

#   if GL_SUPPORTED
        GetEngineFactoryOpenGLType GetEngineFactoryOpenGL = nullptr;
#   endif

#   if VULKAN_SUPPORTED
        GetEngineFactoryVkType GetEngineFactoryVulkan = nullptr;
#   endif
#endif
}

namespace AsteroidsDE {

struct DrawConstantBuffer
{
    DirectX::XMFLOAT4X4 mViewProjection;
    DirectX::XMFLOAT4X4 mWorld;
    DirectX::XMFLOAT3   mSurfaceColor;
    float unused0;

    DirectX::XMFLOAT3 mDeepColor;
    float unused1;
};

struct AsteroidData
{
    DirectX::XMFLOAT4X4 mWorld;
    DirectX::XMFLOAT3   mSurfaceColor;
    float               unused0;
    DirectX::XMFLOAT3   mDeepColor;
    Uint32              mTextureIndex;
};

struct SkyboxConstantBuffer {
    DirectX::XMFLOAT4X4 mViewProjection;
};


// Create Direct3D device and swap chain
void Asteroids::InitDevice(HWND hWnd, DeviceType DevType)
{
    SwapChainDesc SwapChainDesc;
    SwapChainDesc.SamplesCount = 1;
    SwapChainDesc.BufferCount = NUM_SWAP_CHAIN_BUFFERS;
    SwapChainDesc.ColorBufferFormat = TEX_FORMAT_RGBA8_UNORM_SRGB;
    SwapChainDesc.DepthBufferFormat = TEX_FORMAT_D32_FLOAT;
    SwapChainDesc.DefaultDepthValue = 0.f;

    std::vector<IDeviceContext*> ppContexts(mNumSubsets);

    switch (DevType)
    {
#if D3D11_SUPPORTED
        case DeviceType::D3D11:
        {
            EngineD3D11CreateInfo EngineCI;
            EngineCI.NumDeferredContexts = mNumSubsets-1;
            EngineCI.DebugFlags = D3D11_DEBUG_FLAG_CREATE_DEBUG_DEVICE |
                                  D3D11_DEBUG_FLAG_VERIFY_COMMITTED_SHADER_RESOURCES;

#if ENGINE_DLL
            if(!GetEngineFactoryD3D11)
                LoadGraphicsEngineD3D11(GetEngineFactoryD3D11);
#endif
            auto *pFactoryD3D11 = GetEngineFactoryD3D11();
            pFactoryD3D11->CreateDeviceAndContextsD3D11( EngineCI, &mDevice, ppContexts.data() );
            pFactoryD3D11->CreateSwapChainD3D11( mDevice, ppContexts[0], SwapChainDesc, FullScreenModeDesc{}, hWnd, &mSwapChain );
        }
        break;
#endif


#if D3D12_SUPPORTED
        case DeviceType::D3D12:
        {
            EngineD3D12CreateInfo EngineCI;
            EngineCI.NumDeferredContexts = mNumSubsets-1;
            EngineCI.GPUDescriptorHeapDynamicSize[0] = 65536*4;
            EngineCI.GPUDescriptorHeapSize[0] = 65536; // For mutable mode
            EngineCI.NumCommandsToFlushCmdList = 1024;
#ifndef _DEBUG
            EngineCI.DynamicDescriptorAllocationChunkSize[0] = 8192;
#endif
#if ENGINE_DLL
            if(!GetEngineFactoryD3D12)
                LoadGraphicsEngineD3D12(GetEngineFactoryD3D12);
#endif
            auto *pFactoryD3D12 = GetEngineFactoryD3D12();
            pFactoryD3D12->CreateDeviceAndContextsD3D12( EngineCI, &mDevice, ppContexts.data() );
            pFactoryD3D12->CreateSwapChainD3D12( mDevice, ppContexts[0], SwapChainDesc, FullScreenModeDesc{}, hWnd, &mSwapChain );
        }
        break;
#endif


#if VULKAN_SUPPORTED
        case DeviceType::Vulkan:
        {
            EngineVkCreateInfo EngineCI;
            EngineCI.NumDeferredContexts = mNumSubsets-1;
            EngineCI.DynamicHeapSize = 64 << 20;
#if ENGINE_DLL
            if(!GetEngineFactoryVulkan)
                LoadGraphicsEngineVk(GetEngineFactoryVulkan);
#endif
            auto *pFactoryVk = GetEngineFactoryVulkan();
            pFactoryVk->CreateDeviceAndContextsVk( EngineCI, &mDevice, ppContexts.data() );
            pFactoryVk->CreateSwapChainVk( mDevice, ppContexts[0], SwapChainDesc, hWnd, &mSwapChain );
        }
        break;
#endif


#if GL_SUPPORTED
        case DeviceType::OpenGL:
        {
#if ENGINE_DLL
            if (GetEngineFactoryOpenGL == nullptr)
            {
                LoadGraphicsEngineOpenGL(GetEngineFactoryOpenGL);
            }
#endif
            EngineGLCreateInfo CreationAttribs;
            CreationAttribs.pNativeWndHandle = hWnd;
            GetEngineFactoryOpenGL()->CreateDeviceAndSwapChainGL(
                CreationAttribs, &mDevice, &mDeviceCtxt, SwapChainDesc, &mSwapChain);
        }
        break;
#endif


        default:
            LOG_ERROR_AND_THROW("Unknown device type");
        break;
    }

    if (DevType == DeviceType::D3D11 || DevType == DeviceType::D3D12 || DevType == DeviceType::Vulkan)
    {
        mDeviceCtxt.Attach(ppContexts[0]);
        mDeferredCtxt.resize(mNumSubsets-1);
        for(size_t ctx=0; ctx < mNumSubsets-1; ++ctx)
            mDeferredCtxt[ctx].Attach(ppContexts[1+ctx]);
    }
}

Asteroids::Asteroids(const Settings &settings, AsteroidsSimulation* asteroids, GUI* gui, HWND hWnd, Diligent::DeviceType DevType)
    : mAsteroids(asteroids)
    , mGUI(gui)
{
    QueryPerformanceFrequency((LARGE_INTEGER*)&mPerfCounterFreq);

    mNumSubsets = std::max(settings.numThreads,1);
    mNumSubsets = std::min(settings.numThreads,32);

    InitDevice(hWnd, DevType);

    m_BindingMode = static_cast<BindingMode>(settings.resourceBindingMode);
    if (m_BindingMode == BindingMode::Bindless && !mDevice->GetDeviceCaps().bBindlessSupported)
        m_BindingMode = BindingMode::TextureMutable;

    mCmdLists.resize(mDeferredCtxt.size());
    mWorkerThreads.resize(mNumSubsets-1);
    for(auto &thread : mWorkerThreads)
    {
        thread = std::thread(WorkerThreadFunc, this, (Uint32)(&thread-mWorkerThreads.data()) );
    }

    const char *spriteFile = nullptr;
    switch(DevType)
    {
        case DeviceType::D3D11: spriteFile = "DiligentD3D11.dds"; break;
        case DeviceType::D3D12: spriteFile = "DiligentD3D12.dds"; break;
        case DeviceType::OpenGL: spriteFile = "DiligentGL.dds"; break;
        case DeviceType::Vulkan: spriteFile = "DiligentVk.dds"; break;
        default: UNEXPECTED("Unexpected device type");
    }
    mSprite.reset( new GUISprite(5, 10, 140, 50, spriteFile) );
    
    RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
    mDevice->GetEngineFactory()->CreateDefaultShaderSourceStreamFactory("src", &pShaderSourceFactory);

    std::vector<StateTransitionDesc> Barriers;
    mBackBufferWidth = mSwapChain->GetDesc().Width;
    mBackBufferHeight = mSwapChain->GetDesc().Height;
    const auto MaxAsteroidsInSubset = (NUM_ASTEROIDS + mNumSubsets - 1) / mNumSubsets;

    {
        BufferDesc desc;
        desc.Name           = "Asteroids constant buffer";
        // In bindless mode we will be updating the buffer with UpdateBuffer method
        desc.Usage          = (m_BindingMode == BindingMode::Bindless) ? USAGE_DEFAULT : USAGE_DYNAMIC;
        desc.CPUAccessFlags = CPU_ACCESS_WRITE;
        desc.BindFlags      = BIND_UNIFORM_BUFFER;
        // In bindless mode, we will only write view-projection matrix
        desc.uiSizeInBytes  = static_cast<Uint32>( (m_BindingMode == BindingMode::Bindless) ? sizeof(DirectX::XMFLOAT4X4) : sizeof(DrawConstantBuffer) );
        mDevice->CreateBuffer(desc, nullptr, &mDrawConstantBuffer);
        if (m_BindingMode != BindingMode::Bindless)
            Barriers.emplace_back(mDrawConstantBuffer, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_CONSTANT_BUFFER, true);
    }

    if (m_BindingMode == BindingMode::Bindless)
    {
        {
            // In Direct3D there is no easy way to pass draw call number into the shader,
            // so we will use this auxiliary buffer that solely contains integers in ascending order
            // (0, 1, 2, ...) and we will acess it with FirstInstanceLocation.
            BufferDesc desc;
            desc.Name          = "Instance ID buffer";
            desc.Usage         = USAGE_STATIC;
            desc.BindFlags     = BIND_VERTEX_BUFFER;
            desc.uiSizeInBytes = static_cast<Uint32>(sizeof(Uint32)) * MaxAsteroidsInSubset;
            std::vector<Uint32> Ids(MaxAsteroidsInSubset);
            for (Uint32 i=0; i < Ids.size(); ++i)
                Ids[i] = i;
            BufferData Data(Ids.data(), desc.uiSizeInBytes);
            mDevice->CreateBuffer(desc, &Data, &mInstanceIDBuffer);
            Barriers.emplace_back(mInstanceIDBuffer, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_VERTEX_BUFFER, true);
        }

        {
            // Structured buffer that contains asteroid data. Evey thread needs to use
            // its own buffer.
            BufferDesc desc;
            desc.Name              = "Asteroids data buffer";
            desc.Usage             = USAGE_DYNAMIC;
            desc.BindFlags         = BIND_SHADER_RESOURCE;
            desc.Mode              = BUFFER_MODE_STRUCTURED;
            desc.CPUAccessFlags    = CPU_ACCESS_WRITE;
            desc.ElementByteStride = static_cast<Uint32>(sizeof(AsteroidData));
            desc.uiSizeInBytes = desc.ElementByteStride * MaxAsteroidsInSubset;
            mAsteroidsDataBuffers.resize(mNumSubsets);
            for(Uint32 i=0; i < mNumSubsets; ++i)
            {
                mDevice->CreateBuffer(desc, nullptr, &mAsteroidsDataBuffers[i]);
            }
        }
    }

    // create pipeline state
    {
        PipelineStateDesc PSODesc;
        LayoutElement inputDesc[] =
        {
            LayoutElement{ 0, 0, 3, VT_FLOAT32, false, 0, sizeof(Vertex)},
            LayoutElement{ 1, 0, 3, VT_FLOAT32},
            LayoutElement{ 2, 1, 1, VT_UINT32, False, LayoutElement::FREQUENCY_PER_INSTANCE}
        };

        PSODesc.GraphicsPipeline.InputLayout.LayoutElements = inputDesc;
        // In bindless mode we will use instance ID buffer as the third input
        PSODesc.GraphicsPipeline.InputLayout.NumElements = (m_BindingMode == BindingMode::Bindless) ? 3 : 2;

        PSODesc.GraphicsPipeline.DepthStencilDesc.DepthFunc = COMPARISON_FUNC_GREATER_EQUAL;

        RefCntAutoPtr<IShader> vs, ps;
        {
            ShaderCreateInfo attribs;
            attribs.Desc.ShaderType = SHADER_TYPE_VERTEX;
            attribs.Desc.Name  = "Asteroids VS";
            attribs.EntryPoint = "asteroid_vs_diligent";
            attribs.FilePath   = "asteroid_vs_diligent.hlsl";
            attribs.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
            attribs.pShaderSourceStreamFactory = pShaderSourceFactory;
            attribs.UseCombinedTextureSamplers = true;

            ShaderMacro Macros[] = {{"BINDLESS", "1"}, {}};
            if (m_BindingMode == BindingMode::Bindless)
                attribs.Macros = Macros;

            mDevice->CreateShader(attribs, &vs);
        }

        {
            ShaderCreateInfo attribs;
            attribs.Desc.ShaderType = SHADER_TYPE_PIXEL;
            attribs.Desc.Name  = "Asteroids PS";
            attribs.EntryPoint = "asteroid_ps_diligent";
            attribs.FilePath   = "asteroid_ps_diligent.hlsl";
            attribs.pShaderSourceStreamFactory = pShaderSourceFactory;
            attribs.UseCombinedTextureSamplers = true;
            attribs.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;

            ShaderMacro Macros[] = {{"BINDLESS", "1"}, {}};
            if (m_BindingMode == BindingMode::Bindless)
                attribs.Macros = Macros;

            mDevice->CreateShader(attribs, &ps);
        }

        StaticSamplerDesc samDesc;
        samDesc.ShaderStages         = SHADER_TYPE_PIXEL;
        samDesc.Desc.MagFilter       = FILTER_TYPE_ANISOTROPIC;
        samDesc.Desc.MinFilter       = FILTER_TYPE_ANISOTROPIC;
        samDesc.Desc.MipFilter       = FILTER_TYPE_ANISOTROPIC;
        samDesc.Desc.AddressU        = TEXTURE_ADDRESS_WRAP;
        samDesc.Desc.AddressV        = TEXTURE_ADDRESS_WRAP;
        samDesc.Desc.AddressW        = TEXTURE_ADDRESS_WRAP;
        samDesc.Desc.MinLOD          = -D3D11_FLOAT32_MAX;
        samDesc.Desc.MaxLOD          = D3D11_FLOAT32_MAX;
        samDesc.Desc.MipLODBias      = 0.0f;
        samDesc.Desc.MaxAnisotropy   = TEXTURE_ANISO;
        samDesc.Desc.ComparisonFunc  = COMPARISON_FUNC_NEVER;
        samDesc.SamplerOrTextureName = "Tex";

        PSODesc.ResourceLayout.StaticSamplers    = &samDesc;
        PSODesc.ResourceLayout.NumStaticSamplers = 1;

        std::vector<ShaderResourceVariableDesc> Variables =
        {
            {SHADER_TYPE_PIXEL, "Tex", m_BindingMode == BindingMode::Dynamic ? SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC : SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE}
        };
        if (m_BindingMode == BindingMode::Bindless)
            Variables.emplace_back(SHADER_TYPE_VERTEX, "g_Data", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE);

        PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
        PSODesc.ResourceLayout.Variables           = Variables.data();
        PSODesc.ResourceLayout.NumVariables        = static_cast<Uint32>(Variables.size());

        PSODesc.GraphicsPipeline.RTVFormats[0] = mSwapChain->GetDesc().ColorBufferFormat;
        PSODesc.GraphicsPipeline.NumRenderTargets = 1;
        PSODesc.GraphicsPipeline.DSVFormat = mSwapChain->GetDesc().DepthBufferFormat;
        PSODesc.GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        PSODesc.Name = "Asteroids PSO";

        PSODesc.GraphicsPipeline.pVS = vs;
        PSODesc.GraphicsPipeline.pPS = ps;
        mDevice->CreatePipelineState(PSODesc, &mAsteroidsPSO);
        mAsteroidsPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "DrawConstantBuffer")->Set(mDrawConstantBuffer);

        Uint32 NumSRBs = 0;
        if(m_BindingMode == BindingMode::Dynamic)
        {
            // Create one SRB per subset for dynamic binding mode
            NumSRBs = mNumSubsets;
        }
        else if(m_BindingMode == BindingMode::Mutable)
        {
            // Create one SRB per asteroid in mutable binding mode
            PSODesc.SRBAllocationGranularity = 1024;
            NumSRBs = NUM_ASTEROIDS;
        }
        else if(m_BindingMode == BindingMode::TextureMutable)
        {
            // Create one SRB per texture in texture-mutable binding mode
            PSODesc.SRBAllocationGranularity = NUM_UNIQUE_TEXTURES;
            NumSRBs = NUM_UNIQUE_TEXTURES;
        }
        else if(m_BindingMode == BindingMode::Bindless)
        {
            // Create one SRB per subset for bindless mode
            NumSRBs = mNumSubsets;
        }
        mAsteroidsSRBs.resize(NumSRBs);
        for(size_t srb = 0; srb < mAsteroidsSRBs.size(); ++srb)
        {
            mAsteroidsPSO->CreateShaderResourceBinding(&mAsteroidsSRBs[srb], true);
        }
    }

  
    // Load textures
    {
        TextureLoadInfo loadInfo;
        loadInfo.IsSRGB = true;
        RefCntAutoPtr<ITexture> skybox;
        CreateTextureFromFile("starbox_1024.dds", loadInfo, mDevice, &skybox);
        mSkyboxSRV = skybox->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
        Barriers.emplace_back(skybox, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_SHADER_RESOURCE, true);
    }

    // Create skybox constant buffer
    {
        BufferDesc desc;
        desc.Name = "skybox constant buffer (dynamic)";
        desc.Usage = USAGE_DYNAMIC;
        desc.uiSizeInBytes = sizeof(SkyboxConstantBuffer);
        desc.BindFlags = BIND_UNIFORM_BUFFER;
        desc.CPUAccessFlags = CPU_ACCESS_WRITE;
        mDevice->CreateBuffer(desc, nullptr, &mSkyboxConstantBuffer);
        Barriers.emplace_back(mSkyboxConstantBuffer, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_CONSTANT_BUFFER, true);
    }

    // create skybox pipeline state
    {
        LayoutElement inputDesc[] = {
            LayoutElement{0, 0, 3, VT_FLOAT32},
            LayoutElement{1, 0, 3, VT_FLOAT32}
        };

        RefCntAutoPtr<IShader> vs, ps;
        ShaderCreateInfo attribs;
        attribs.Desc.ShaderType = SHADER_TYPE_VERTEX;
        attribs.Desc.Name = "Skybox VS";
        attribs.EntryPoint = "skybox_vs";
        attribs.FilePath = "skybox_vs.hlsl";
        attribs.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
        attribs.UseCombinedTextureSamplers = true;
        attribs.pShaderSourceStreamFactory = pShaderSourceFactory;
        mDevice->CreateShader(attribs, &vs);

        attribs.Desc.Name = "Skybox PS";
        attribs.EntryPoint = "skybox_ps";
        attribs.FilePath = "skybox_ps.hlsl";
        attribs.Desc.ShaderType = SHADER_TYPE_PIXEL;
        mDevice->CreateShader(attribs, &ps);

        PipelineStateDesc PSODesc;
        PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
        
        StaticSamplerDesc ssdesc;
        ssdesc.ShaderStages         = SHADER_TYPE_PIXEL;
        ssdesc.SamplerOrTextureName = "Skybox";
        ssdesc.Desc.MagFilter       = FILTER_TYPE_ANISOTROPIC;
        ssdesc.Desc.MinFilter       = FILTER_TYPE_ANISOTROPIC;
        ssdesc.Desc.MipFilter       = FILTER_TYPE_ANISOTROPIC;
        ssdesc.Desc.AddressU        = TEXTURE_ADDRESS_WRAP;
        ssdesc.Desc.AddressV        = TEXTURE_ADDRESS_WRAP;
        ssdesc.Desc.AddressW        = TEXTURE_ADDRESS_WRAP;
        ssdesc.Desc.MaxAnisotropy   = TEXTURE_ANISO;
        PSODesc.ResourceLayout.StaticSamplers = &ssdesc;
        PSODesc.ResourceLayout.NumStaticSamplers = 1;

        PSODesc.Name = "Skybox PSO";
        PSODesc.GraphicsPipeline.InputLayout.LayoutElements = inputDesc;
        PSODesc.GraphicsPipeline.InputLayout.NumElements = _countof(inputDesc);

        PSODesc.GraphicsPipeline.DepthStencilDesc.DepthFunc = COMPARISON_FUNC_GREATER_EQUAL;
        PSODesc.GraphicsPipeline.pVS = vs;
        PSODesc.GraphicsPipeline.pPS = ps;

        PSODesc.GraphicsPipeline.RTVFormats[0] = mSwapChain->GetDesc().ColorBufferFormat;
        PSODesc.GraphicsPipeline.NumRenderTargets = 1;
        PSODesc.GraphicsPipeline.DSVFormat = mSwapChain->GetDesc().DepthBufferFormat;
        PSODesc.GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        mDevice->CreatePipelineState(PSODesc, &mSkyboxPSO);
        mSkyboxPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "SkyboxConstantBuffer")->Set(mSkyboxConstantBuffer);
        mSkyboxPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "Skybox")->Set(mSkyboxSRV);
        mSkyboxPSO->CreateShaderResourceBinding(&mSkyboxSRB, true);
    }

    // Create sampler
    {
        SamplerDesc desc;
        desc.MagFilter      = FILTER_TYPE_ANISOTROPIC;
        desc.MinFilter      = FILTER_TYPE_ANISOTROPIC;
        desc.MipFilter      = FILTER_TYPE_ANISOTROPIC;
        desc.AddressU       = TEXTURE_ADDRESS_WRAP;
        desc.AddressV       = TEXTURE_ADDRESS_WRAP;
        desc.AddressW       = TEXTURE_ADDRESS_WRAP;
        desc.MinLOD         = -D3D11_FLOAT32_MAX;
        desc.MaxLOD         = D3D11_FLOAT32_MAX;
        desc.MipLODBias     = 0.0f;
        desc.MaxAnisotropy  = TEXTURE_ANISO;
        desc.ComparisonFunc = COMPARISON_FUNC_NEVER;
        mDevice->CreateSampler(desc, &mSamplerState);
    }

    CreateGUIResources();

    // Sprites and fonts
    {
        PipelineStateDesc PSODesc;
        LayoutElement inputDesc[] = {
            LayoutElement{0, 0, 2, VT_FLOAT32},
            LayoutElement{1, 0, 2, VT_FLOAT32}
        };
        PSODesc.GraphicsPipeline.InputLayout.LayoutElements = inputDesc;
        PSODesc.GraphicsPipeline.InputLayout.NumElements = _countof(inputDesc);

        auto &BlendState = PSODesc.GraphicsPipeline.BlendDesc;
        {
            // Premultiplied over blend
            BlendState.RenderTargets[0].BlendEnable = TRUE;
            BlendState.RenderTargets[0].SrcBlend    = BLEND_FACTOR_ONE;
            BlendState.RenderTargets[0].BlendOp     = BLEND_OPERATION_ADD;
            BlendState.RenderTargets[0].DestBlend   = BLEND_FACTOR_INV_SRC_ALPHA;
        }

        PSODesc.GraphicsPipeline.DepthStencilDesc.DepthEnable = false;

        PSODesc.GraphicsPipeline.RTVFormats[0] = mSwapChain->GetDesc().ColorBufferFormat;
        PSODesc.GraphicsPipeline.NumRenderTargets = 1;
        PSODesc.GraphicsPipeline.DSVFormat = mSwapChain->GetDesc().DepthBufferFormat;
        PSODesc.GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        
        RefCntAutoPtr<IShader> sprite_vs, sprite_ps, font_ps;
        {
            ShaderCreateInfo attribs;
            attribs.Desc.ShaderType = SHADER_TYPE_VERTEX;
            attribs.Desc.Name = "Sprite VS";
            attribs.EntryPoint = "sprite_vs";
            attribs.FilePath = "sprite_vs.hlsl";
            attribs.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
            attribs.UseCombinedTextureSamplers = true;
            attribs.pShaderSourceStreamFactory = pShaderSourceFactory;
            mDevice->CreateShader(attribs, &sprite_vs);
        }

        {
            ShaderCreateInfo attribs;
            attribs.Desc.ShaderType = SHADER_TYPE_PIXEL;
            attribs.Desc.Name = "Sprite PS";
            attribs.EntryPoint = "sprite_ps";
            attribs.FilePath = "sprite_ps.hlsl";
            attribs.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
            attribs.UseCombinedTextureSamplers = true;
            attribs.pShaderSourceStreamFactory = pShaderSourceFactory;
            mDevice->CreateShader(attribs, &sprite_ps);
        }

        {
            ShaderCreateInfo attribs;
            attribs.Desc.ShaderType = SHADER_TYPE_PIXEL;
            attribs.Desc.Name = "Font PS";
            attribs.EntryPoint = "font_ps";
            attribs.FilePath = "font_ps.hlsl";
            attribs.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
            attribs.UseCombinedTextureSamplers = true;
            attribs.pShaderSourceStreamFactory = pShaderSourceFactory;
            mDevice->CreateShader(attribs, &font_ps);
        }
        
        PSODesc.Name = "Sprite PSO";
        PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC;
        PSODesc.GraphicsPipeline.pVS = sprite_vs;
        PSODesc.GraphicsPipeline.pPS = sprite_ps;
        mDevice->CreatePipelineState(PSODesc, &mSpritePSO);
        mSpritePSO->CreateShaderResourceBinding(&mSpriteSRB, true);

        PSODesc.Name = "Font PSO";
        PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
        PSODesc.GraphicsPipeline.pPS = font_ps;
        mDevice->CreatePipelineState(PSODesc, &mFontPSO);
        mFontPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "Tex")->Set(mFontTextureSRV);
        mFontPSO->CreateShaderResourceBinding(&mFontSRB, true);
    }


    CreateMeshes();
    InitializeTextureData();
    if( m_BindingMode == BindingMode::Mutable )
    {
        // Bind the corresponding texture to the asteroids's SRB
        for(size_t srb = 0; srb < NUM_ASTEROIDS; ++srb)
        {
            auto staticData = &mAsteroids->StaticData()[srb];
            mAsteroidsSRBs[srb]->GetVariableByName(SHADER_TYPE_PIXEL, "Tex")->Set(mTextureSRVs[staticData->textureIndex]);
        }
    }
    else if( m_BindingMode == BindingMode::TextureMutable )
    {
        // Bind the corresponding texture to the textures's SRB
        for(size_t srb = 0; srb < NUM_UNIQUE_TEXTURES; ++srb)
        {
            mAsteroidsSRBs[srb]->GetVariableByName(SHADER_TYPE_PIXEL, "Tex")->Set(mTextureSRVs[srb]);
        }
    }
    else if( m_BindingMode == BindingMode::Bindless )
    {
        // Bind all textures to every subset's SRB. The textures will be dynamically indexed in the shader.
        IDeviceObject* SRVArray[NUM_UNIQUE_TEXTURES];
        for(Uint32 t = 0; t < NUM_UNIQUE_TEXTURES; ++t)
            SRVArray[t] = mTextureSRVs[t];
        for(Uint32 i = 0; i < mNumSubsets; ++i)
        {
            mAsteroidsSRBs[i]->GetVariableByName(SHADER_TYPE_PIXEL, "Tex")->SetArray(SRVArray, 0, NUM_UNIQUE_TEXTURES);
            mAsteroidsSRBs[i]->GetVariableByName(SHADER_TYPE_VERTEX, "g_Data")->Set(mAsteroidsDataBuffers[i]->GetDefaultView(BUFFER_VIEW_SHADER_RESOURCE));
        }
    }
    mDeviceCtxt->TransitionResourceStates(static_cast<Uint32>(Barriers.size()), Barriers.data());
}

Asteroids::~Asteroids()
{
    mDeviceCtxt->Flush();
    mDeviceCtxt->FinishFrame();

    mUpdateSubsetsSignal.Trigger(true, -1);

    for(auto &thread : mWorkerThreads)
    {
        thread.join();
    }
}


void Asteroids::ResizeSwapChain(HWND outputWindow, unsigned int width, unsigned int height)
{
    mSwapChain->Resize(width, height);
    mBackBufferWidth = width;
    mBackBufferHeight = height;
}


void Asteroids::CreateMeshes()
{
    auto asteroidMeshes = mAsteroids->Meshes();

    std::vector<StateTransitionDesc> Barriers;
    // create vertex buffer
    {
        BufferDesc desc;
        desc.Name = "Asteroid Meshes Vertex Buffer";
        desc.uiSizeInBytes = (Uint32)asteroidMeshes->vertices.size() * sizeof(asteroidMeshes->vertices[0]);
        desc.BindFlags = BIND_VERTEX_BUFFER;
        desc.Usage = USAGE_DEFAULT;

        BufferData data;
        data.pData = asteroidMeshes->vertices.data();
        data.DataSize = desc.uiSizeInBytes;

        mDevice->CreateBuffer(desc, &data, &mVertexBuffer);
        Barriers.emplace_back(mVertexBuffer, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_VERTEX_BUFFER, true);
    }

    // create index buffer
    {
        BufferDesc desc;
        desc.Name = "Asteroid Meshes Index Buffer";
        desc.uiSizeInBytes = (Uint32)asteroidMeshes->indices.size() * sizeof(asteroidMeshes->indices[0]);
        desc.BindFlags = BIND_INDEX_BUFFER;
        desc.Usage = USAGE_DEFAULT;

        BufferData data;
        data.pData = asteroidMeshes->indices.data();
        data.DataSize = desc.uiSizeInBytes;

        mDevice->CreateBuffer(desc, &data, &mIndexBuffer);
        Barriers.emplace_back(mIndexBuffer, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_INDEX_BUFFER, true);
    }

    std::vector<SkyboxVertex> skyboxVertices;
    CreateSkyboxMesh(&skyboxVertices);


    // create skybox vertex buffer
    {
        BufferDesc desc;
        desc.Name = "Skybox Vertex Buffer";
        desc.uiSizeInBytes = (Uint32)skyboxVertices.size() * sizeof(skyboxVertices[0]);
        desc.BindFlags = BIND_VERTEX_BUFFER;
        desc.Usage = USAGE_DEFAULT;

        BufferData data;
        data.pData = skyboxVertices.data();
        data.DataSize = desc.uiSizeInBytes;

        mDevice->CreateBuffer(desc, &data, &mSkyboxVertexBuffer);
        Barriers.emplace_back(mSkyboxVertexBuffer, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_VERTEX_BUFFER, true);
    }

    // create sprite vertex buffer (dynamic)
    {
        BufferDesc desc;
        desc.Name = "sprite vertex buffer (dynamic)";
        desc.uiSizeInBytes = (Uint32)MAX_SPRITE_VERTICES_PER_FRAME * sizeof(SpriteVertex);
        desc.BindFlags = BIND_VERTEX_BUFFER;
        desc.Usage = USAGE_DYNAMIC;
        desc.CPUAccessFlags = CPU_ACCESS_WRITE;
        
        mDevice->CreateBuffer(desc, nullptr, &mSpriteVertexBuffer);
        Barriers.emplace_back(mSpriteVertexBuffer, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_VERTEX_BUFFER, true);
    }
    mDeviceCtxt->TransitionResourceStates(static_cast<Uint32>(Barriers.size()), Barriers.data());
}

void Asteroids::InitializeTextureData()
{
    TextureDesc textureDesc;
    textureDesc.Type = RESOURCE_DIM_TEX_2D_ARRAY;
    textureDesc.Width            = TEXTURE_DIM;
    textureDesc.Height           = TEXTURE_DIM;
    textureDesc.ArraySize        = 3;
    textureDesc.MipLevels        = 0; // Full chain
    textureDesc.Format           = TEX_FORMAT_RGBA8_UNORM_SRGB;
    textureDesc.SampleCount      = 1;
    textureDesc.Usage            = USAGE_DEFAULT;
    textureDesc.BindFlags        = BIND_SHADER_RESOURCE;

    std::vector<StateTransitionDesc> Barriers;
    for (UINT t = 0; t < NUM_UNIQUE_TEXTURES; ++t) {
        std::vector<TextureSubResData> subResData(size_t{textureDesc.ArraySize} * size_t{mAsteroids->GetTextureMipLevels()});
        auto *texData = mAsteroids->TextureData(t);
        for(size_t subRes=0; subRes < subResData.size(); ++subRes)
        {
            subResData[subRes].pData = texData[subRes].pSysMem;
            subResData[subRes].Stride = texData[subRes].SysMemPitch;
            subResData[subRes].DepthStride = texData[subRes].SysMemSlicePitch;
        }
        TextureData initData;
        initData.NumSubresources = (Uint32)subResData.size();
        initData.pSubResources = subResData.data();
        mDevice->CreateTexture(textureDesc, &initData, &mTextures[t]);
        mTextureSRVs[t] = mTextures[t]->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
        mTextureSRVs[t]->SetSampler(mSamplerState);
        Barriers.emplace_back(mTextures[t], RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_SHADER_RESOURCE, true);
    }
    mDeviceCtxt->TransitionResourceStates(static_cast<Uint32>(Barriers.size()), Barriers.data());
}

void Asteroids::CreateGUIResources()
{
    auto font = mGUI->Font();
    TextureDesc textureDesc;
    textureDesc.Type = RESOURCE_DIM_TEX_2D;
    textureDesc.Format = TEX_FORMAT_R8_UNORM;
    textureDesc.Width = font->BitmapWidth();
    textureDesc.Height = font->BitmapHeight();
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.BindFlags = BIND_SHADER_RESOURCE;
    textureDesc.Usage = USAGE_DEFAULT;

    TextureData initialData;
    TextureSubResData subresources[] = {
        TextureSubResData((void*)font->Pixels(), font->BitmapWidth())
    };
    initialData.pSubResources = subresources;
    initialData.NumSubresources = _countof(subresources);

    RefCntAutoPtr<ITexture> texture;
    mDevice->CreateTexture(textureDesc, &initialData, &texture);
    mFontTextureSRV = texture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
    mFontTextureSRV->SetSampler(mSamplerState);

    // Load any GUI sprite textures
    for (int i = -1; i < (int)mGUI->size(); ++i) {
        auto control = i >= 0 ? (*mGUI)[i] : mSprite.get();
        if (control->TextureFile().length() > 0 && mSpriteTextures.find(control->TextureFile()) == mSpriteTextures.end()) {
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;            
            auto path = NarrowString(converter.from_bytes(control->TextureFile()).c_str());
            TextureLoadInfo loadInfo;
            loadInfo.IsSRGB = true;
            RefCntAutoPtr<ITexture> spriteTexture;
            CreateTextureFromFile(path.c_str(), loadInfo, mDevice, &spriteTexture);
            auto *pSRV = spriteTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
            pSRV->SetSampler(mSamplerState);
            mSpriteTextures[control->TextureFile()] = pSRV;
        }
    }
}

static_assert(sizeof(IndexType) == 2, "Expecting 16-bit index buffer");

void Asteroids::WorkerThreadFunc(Asteroids *pThis, Diligent::Uint32 ThreadNum)
{
    for (;;)
    {
        // Wait for UpdateSubsets signal
        auto SignalledValue = pThis->mUpdateSubsetsSignal.Wait();
        if(SignalledValue < 0)
            return;

        auto SubsetSize = NUM_ASTEROIDS / pThis->mNumSubsets;
        auto SubsetStart = SubsetSize * (ThreadNum+1);
        auto &FrameAttribs = pThis->mFrameAttribs;
        
        pThis->mAsteroids->Update(FrameAttribs.frameTime, FrameAttribs.camera->Eye(), *FrameAttribs.settings, SubsetStart, SubsetSize);
        
        // Increment number of completed threads
        ++pThis->m_NumThreadsCompleted;


        // Wait for RenderSubsets signal
        pThis->mRenderSubsetsSignal.Wait();

        pThis->RenderSubset(1+ThreadNum, pThis->mDeferredCtxt[ThreadNum], *FrameAttribs.camera, SubsetStart, SubsetSize);

        RefCntAutoPtr<ICommandList> pCmdList;
        pThis->mCmdLists[ThreadNum].Release();
        pThis->mDeferredCtxt[ThreadNum]->FinishCommandList(&pThis->mCmdLists[ThreadNum]);
        
        // Increment number of completed threads
        ++pThis->m_NumThreadsCompleted;
    }
}

void Asteroids::RenderSubset(Diligent::Uint32 SubsetNum,
                             IDeviceContext *pCtx, 
                             const OrbitCamera& camera, 
                             Uint32 startIdx, 
                             Uint32 numAsteroids)
{
    pCtx->SetRenderTargets(0, nullptr, nullptr, RESOURCE_STATE_TRANSITION_MODE_VERIFY);
    
    // Frame data
    auto staticAsteroidData = mAsteroids->StaticData();
    auto dynamicAsteroidData = mAsteroids->DynamicData();
    
    pCtx->SetPipelineState(mAsteroidsPSO);

    {
        IBuffer* ia_buffers[] = { mVertexBuffer, mInstanceIDBuffer };
        Uint32 ia_offsets[_countof(ia_buffers)] = {};
        // Bind instance data buffer in bindless mode
        pCtx->SetVertexBuffers(0, (m_BindingMode == BindingMode::Bindless) ? 2 : 1, ia_buffers, ia_offsets, RESOURCE_STATE_TRANSITION_MODE_VERIFY, SET_VERTEX_BUFFERS_FLAG_NONE);
        pCtx->SetIndexBuffer(mIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_VERIFY);
    }

    if (m_BindingMode == BindingMode::Bindless)
    {
        {
            // Update asteroid data buffer
            MapHelper<AsteroidData> asteroidData(pCtx, mAsteroidsDataBuffers[SubsetNum], MAP_WRITE, MAP_FLAG_DISCARD);
            UINT i=0;
            for (UINT drawIdx = startIdx; drawIdx < startIdx+numAsteroids; ++drawIdx, ++i)
            {
                const auto staticData = &staticAsteroidData[drawIdx];
                const auto dynamicData = &dynamicAsteroidData[drawIdx];

                XMStoreFloat4x4(&asteroidData[i].mWorld, dynamicData->world);
                asteroidData[i].mSurfaceColor = staticData->surfaceColor;
                asteroidData[i].mDeepColor    = staticData->deepColor;
                asteroidData[i].mTextureIndex = staticData->textureIndex;
            }
        }

        StateTransitionDesc Barrier{mAsteroidsDataBuffers[SubsetNum], RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_SHADER_RESOURCE, true};
        pCtx->TransitionResourceStates(1, &Barrier);

        // Commit and verify resources
        pCtx->CommitShaderResources(mAsteroidsSRBs[SubsetNum], RESOURCE_STATE_TRANSITION_MODE_VERIFY);
    }
       
    const auto& viewProjection = camera.ViewProjection();
    auto pVar = m_BindingMode == BindingMode::Dynamic ? mAsteroidsSRBs[SubsetNum]->GetVariableByName(SHADER_TYPE_PIXEL, "Tex") : nullptr;
    for (UINT drawIdx = startIdx; drawIdx < startIdx+numAsteroids; ++drawIdx)
    {
        const auto staticData = &staticAsteroidData[drawIdx];
        const auto dynamicData = &dynamicAsteroidData[drawIdx];

        if( m_BindingMode != BindingMode::Bindless )
        {
            MapHelper<DrawConstantBuffer> drawConstants(pCtx, mDrawConstantBuffer, MAP_WRITE, MAP_FLAG_DISCARD);
            XMStoreFloat4x4(&drawConstants->mWorld,          dynamicData->world);
            XMStoreFloat4x4(&drawConstants->mViewProjection, viewProjection);
            drawConstants->mSurfaceColor = staticData->surfaceColor;
            drawConstants->mDeepColor    = staticData->deepColor;
        }
        // No need to update the buffer in bindless mode

        if( m_BindingMode == BindingMode::Dynamic )
        {
            pVar->Set(mTextureSRVs[staticData->textureIndex]);
            pCtx->CommitShaderResources(mAsteroidsSRBs[SubsetNum], RESOURCE_STATE_TRANSITION_MODE_VERIFY);
        }
        else if( m_BindingMode == BindingMode::Mutable )
        {
            pCtx->CommitShaderResources(mAsteroidsSRBs[drawIdx], RESOURCE_STATE_TRANSITION_MODE_VERIFY);
        }
        else if( m_BindingMode == BindingMode::TextureMutable )
        {
            pCtx->CommitShaderResources(mAsteroidsSRBs[staticData->textureIndex], RESOURCE_STATE_TRANSITION_MODE_VERIFY);
        }

        DrawIndexedAttribs attribs(dynamicData->indexCount, VT_UINT16, DRAW_FLAG_VERIFY_ALL);
        attribs.FirstIndexLocation = dynamicData->indexStart;
        attribs.BaseVertex         = staticData->vertexStart;

        if( m_BindingMode == BindingMode::Bindless )
        {
            // It is very important to speciy this flag to make sure the engine does not do extra
            // work processing buffers that stay intact.
            attribs.Flags |= DRAW_FLAG_DYNAMIC_RESOURCE_BUFFERS_INTACT;
            attribs.FirstInstanceLocation = drawIdx - startIdx;
        }

        pCtx->DrawIndexed(attribs);
    }
}

void Asteroids::Render(float frameTime, const OrbitCamera& camera, const Settings& settings)
{
    mFrameAttribs.frameTime = frameTime;
    mFrameAttribs.camera = &camera;
    mFrameAttribs.settings = &settings;
    
    // Clear the render target
    float clearcol[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    mDeviceCtxt->SetRenderTargets(0, nullptr, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    mDeviceCtxt->ClearRenderTarget(nullptr, clearcol, RESOURCE_STATE_TRANSITION_MODE_VERIFY);
    mDeviceCtxt->ClearDepthStencil(nullptr, CLEAR_DEPTH_FLAG, 0.0f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    LONG64 currCounter;
    QueryPerformanceCounter((LARGE_INTEGER*)&currCounter);
    mUpdateTicks = currCounter;
        
    auto SubsetSize = NUM_ASTEROIDS / mNumSubsets;
    
    if (m_BindingMode == BindingMode::Bindless)
    {
        // Write view-projection matrix into the buffer
        const auto& viewProjection = camera.ViewProjection();
        mDeviceCtxt->UpdateBuffer(mDrawConstantBuffer, 0, sizeof(DirectX::XMFLOAT4X4), (void*)&viewProjection, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        // Explicitly transition the buffer to CONSTANT_BUFFER state
        StateTransitionDesc Barrier{mDrawConstantBuffer, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_CONSTANT_BUFFER, true};
        mDeviceCtxt->TransitionResourceStates(1, &Barrier);
    }

    if (settings.multithreadedRendering)
    {
        m_NumThreadsCompleted = 0;
        mUpdateSubsetsSignal.Trigger(true);
    }

    // Update all subsets in this thread when multithreadedRendering is false
    for(Uint32 i=0; i < (!settings.multithreadedRendering ? mNumSubsets : 1); ++i)
        mAsteroids->Update(frameTime, camera.Eye(), settings, SubsetSize * i, SubsetSize);

    if (settings.multithreadedRendering)
    {
        // Wait for worker threads to finish
        while(m_NumThreadsCompleted < (int)mNumSubsets-1)
            std::this_thread::yield();
        // Reset mUpdateSubsetsSignal while all threads are waiting for mRenderSubsetsSignal
        mUpdateSubsetsSignal.Reset();
    }

    QueryPerformanceCounter((LARGE_INTEGER*)&currCounter);
    mUpdateTicks = currCounter-mUpdateTicks;

    mRenderTicks = currCounter;

    if (settings.multithreadedRendering)
    {
        // Signal RenderSubsets
        m_NumThreadsCompleted = 0;
        mRenderSubsetsSignal.Trigger(true);
    }

    // Render all subsets in this thread when multithreadedRendering is false
    for(Uint32 i=0; i < (!settings.multithreadedRendering ? mNumSubsets : 1); ++i)
        RenderSubset(i, mDeviceCtxt, camera, SubsetSize * i, SubsetSize);

    if (settings.multithreadedRendering)
    {
        // Wait for worker threads to finish
        while(m_NumThreadsCompleted < (int)mNumSubsets-1)
            std::this_thread::yield();
        // Reset mRenderSubsetsSignal while all threads are waiting for mUpdateSubsetsSignal
        mRenderSubsetsSignal.Reset();

        for(auto &cmdList : mCmdLists)
        {
            mDeviceCtxt->ExecuteCommandList(cmdList);
            // Release command lists now to release all outstanding references
            // In d3d11 mode, command lists hold references to the swap chain's back buffer 
            // that cause swap chain resize to fail
            cmdList.Release();
        }
    }

    // Call FinishFrame() to release dynamic resources allocated by deferred contexts
    // IMPORTANT: we must wait until the command lists are submitted for execution
    // because FinishFrame() invalidates all dynamic resources
    for(auto& ctx : mDeferredCtxt)
        ctx->FinishFrame();

    QueryPerformanceCounter((LARGE_INTEGER*)&currCounter);
    mRenderTicks = currCounter-mRenderTicks;

    mDeviceCtxt->SetRenderTargets(0, nullptr, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    // Draw skybox
    {
        {
            MapHelper<SkyboxConstantBuffer> skyboxConstants(mDeviceCtxt, mSkyboxConstantBuffer, MAP_WRITE, MAP_FLAG_DISCARD);
            XMStoreFloat4x4(&skyboxConstants->mViewProjection, camera.ViewProjection());
        }

        IBuffer* ia_buffers[] = { mSkyboxVertexBuffer };
        UINT ia_offsets[] = { 0 };
        mDeviceCtxt->SetVertexBuffers(0, 1, ia_buffers, ia_offsets, RESOURCE_STATE_TRANSITION_MODE_VERIFY, SET_VERTEX_BUFFERS_FLAG_NONE);

        mDeviceCtxt->SetPipelineState(mSkyboxPSO);
        mDeviceCtxt->CommitShaderResources(mSkyboxSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        DrawAttribs DrawAttrs(6*6, DRAW_FLAG_VERIFY_ALL);
        mDeviceCtxt->Draw(DrawAttrs);
    }

    // Draw sprites and fonts
    {
        // Fill in vertices (TODO: could move this vector to be a member - not a big deal)
        std::vector<UINT> controlVertices;
        controlVertices.reserve(mGUI->size());

        {
            MapHelper<SpriteVertex> vertexBase(mDeviceCtxt, mSpriteVertexBuffer, MAP_WRITE, MAP_FLAG_DISCARD);
            auto vertexEnd = (SpriteVertex*)vertexBase;
            
            for (int i = -1; i < (int)mGUI->size(); ++i) {
                auto control = i >= 0 ? (*mGUI)[i] : mSprite.get();
                controlVertices.push_back((UINT)(control->Draw((float)mBackBufferWidth, (float)mBackBufferHeight, vertexEnd) - vertexEnd));
                vertexEnd += controlVertices.back();
            }
        }

        IBuffer* ia_buffers[] = { mSpriteVertexBuffer };
        Uint32 ia_offsets[] = { 0 };
        mDeviceCtxt->SetVertexBuffers(0, 1, ia_buffers, ia_offsets, RESOURCE_STATE_TRANSITION_MODE_VERIFY, SET_VERTEX_BUFFERS_FLAG_NONE);

        // Draw
        UINT vertexStart = 0;
        for (int i = -1; i < (int)mGUI->size(); ++i) {
            auto control = i >= 0 ? (*mGUI)[i] : mSprite.get();
            if (control->Visible()) {
                if (control->TextureFile().length() == 0) { // Font
                    mDeviceCtxt->SetPipelineState(mFontPSO);
                    mDeviceCtxt->CommitShaderResources(mFontSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
                } else { // Sprite
                    auto textureSRV = mSpriteTextures[control->TextureFile()];
                    mDeviceCtxt->SetPipelineState(mSpritePSO);
                    mSpriteSRB->GetVariableByName(SHADER_TYPE_PIXEL, "Tex")->Set(textureSRV);
                    mDeviceCtxt->CommitShaderResources(mSpriteSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
                }
                DrawAttribs DrawAttrs(controlVertices[1+i], DRAW_FLAG_VERIFY_ALL);
                DrawAttrs.StartVertexLocation = vertexStart;
                mDeviceCtxt->Draw(DrawAttrs);
            }
            vertexStart += controlVertices[1+i];
        }
    }

    mSwapChain->Present(settings.vsync ? 1 : 0);
}

void Asteroids::GetPerfCounters(float &UpdateTime, float &RenderTime)
{
    UpdateTime = (float)mUpdateTicks/(float)mPerfCounterFreq;
    RenderTime = (float)mRenderTicks/(float)mPerfCounterFreq;
}

} // namespace AsteroidsDE
