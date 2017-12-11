// Copyright 2014 Intel Corporation All Rights Reserved
//
// Intel makes no representations about the suitability of this software for any purpose.  
// THIS SOFTWARE IS PROVIDED ""AS IS."" INTEL SPECIFICALLY DISCLAIMS ALL WARRANTIES,
// EXPRESS OR IMPLIED, AND ALL LIABILITY, INCLUDING CONSEQUENTIAL AND OTHER INDIRECT DAMAGES,
// FOR THE USE OF THIS SOFTWARE, INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PROPRIETARY
// RIGHTS, AND INCLUDING THE WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
// Intel does not assume any responsibility for any errors which may appear in this software
// nor any responsibility to update it.

#include <d3dcompiler.h>
#include <directxmath.h>
#include <d3dx12.h>
#include <math.h>

#include <iostream>
#include <algorithm>
#include <vector>
#include <limits>
#include <random>
#include <sstream>
#include <ppl.h>

#include "asteroids_d3d12.h"
#include "util.h"
#include "mesh.h"
#include "noise.h"
#include "texture.h"

#include "asteroid_vs.h"
#include "asteroid_ps.h"

#include "skybox_vs.h"
#include "skybox_ps.h"

#include "sprite_vs.h"
#include "sprite_ps.h"
#include "font_ps.h"

using namespace DirectX;

namespace AsteroidsD3D12 {

enum RootParameters
{
    RP_DRAW_CBV,
    RP_TEX_SRV,
    RP_SMP,
};

Asteroids::Asteroids(AsteroidsSimulation* asteroids, GUI *gui, UINT minCmdLsts, IDXGIAdapter* adapter)
    : mAsteroids(asteroids)
    , mGUI(gui)
    , mFenceEventHandle(CreateEvent(NULL, FALSE, FALSE, NULL))
    , mD3D12Sprite( new GUISprite(5, 10, 140, 50, "directx12.dds") )
{
    QueryPerformanceFrequency((LARGE_INTEGER*)&mPerfCounterFreq);

    memset(&mViewPort, 0, sizeof(mViewPort));
    memset(&mScissorRect, 0, sizeof(mScissorRect));
    memset(mAsteroidTextures, 0, sizeof(mAsteroidTextures));
    memset(mIndexOffsets, 0xff, sizeof(mIndexOffsets));

#if defined(_DEBUG)
    // Enable the D3D12 debug layer.
    {
        ID3D12Debug* debugController = nullptr;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
            debugController->Release();
        }
    }
#endif

    // Create device
    {    
        ThrowIfFailed(D3D12CreateDevice(
            adapter,
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&mDevice)));
                
        D3D12_COMMAND_QUEUE_DESC QDesc;
        QDesc.NodeMask = 1;
        QDesc.Priority = 0;
        QDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        QDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        ThrowIfFailed(mDevice->CreateCommandQueue(&QDesc, IID_PPV_ARGS(&mCommandQueue)));
                                
        ThrowIfFailed(mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
    }

    // General use descriptor heaps
    mRTVDescs = new RTVDescriptorList(mDevice, NUM_SWAP_CHAIN_BUFFERS);
    mDSVDescs = new DSVDescriptorList(mDevice, 1);
    mSMPDescs = new SMPDescriptorList(mDevice, 1);
    mSRVDescs = new SRVDescriptorList(mDevice, NUM_UNIQUE_TEXTURES);

    // Filled in in Resize - just take slots for them here
    mDepthStencilView = mDSVDescs->Append();
    mRTVFormat = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    mDSVFormat = DXGI_FORMAT_D32_FLOAT;
        
    CreatePSOs();
    CreateMeshes();
    
    // Create textures
    {
        // TODO: Query simulation for this data? Defines good enough for now...
        D3D12_RESOURCE_DESC textureDesc =
            CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, TEXTURE_DIM, TEXTURE_DIM, 3, 0);

        for (UINT i = 0; i < NUM_UNIQUE_TEXTURES; ++i) {
            ThrowIfFailed(mDevice->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                D3D12_HEAP_FLAG_NONE,
                &textureDesc,
                D3D12_RESOURCE_STATE_COMMON,
                nullptr, // Clear value
                IID_PPV_ARGS(&mAsteroidTextures[i])
            ));
            textureDesc = mAsteroidTextures[i]->GetDesc();

            InitializeTexture2D(mDevice, mCommandQueue, mAsteroidTextures[i], &textureDesc, 4, mAsteroids->TextureData(i));

            // Append a descriptor to the heap
            mSRVDescs->AppendSRV(mAsteroidTextures[i]);
        }

        ThrowIfFailed(CreateTexture2DFromDDS_XXXX8(
            mDevice, mCommandQueue, &mSkybox, "starbox_1024.dds", DXGI_FORMAT_R8G8B8A8_UNORM_SRGB));
    }
    
    CreateGUIResources();
    
    // Fill in general heaps
    { // Samplers
        D3D12_SAMPLER_DESC sampler = {};
        sampler.Filter = D3D12_FILTER_ANISOTROPIC;
        sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.MaxAnisotropy = TEXTURE_ANISO;
        sampler.MinLOD = -std::numeric_limits<float>::max();
        sampler.MaxLOD =  std::numeric_limits<float>::max();
        mSampler = mSMPDescs->Append(&sampler);
    }
    
    // Swap chain resources
    for (UINT s = 0; s < NUM_SWAP_CHAIN_BUFFERS; s++) {
        // Filled in in resize, just make a slot here
        mSwapChainBuffer[s].mRenderTargetView = mRTVDescs->Append();
    }

    // Per-frame resources
    for (UINT f = 0; f < NUM_FRAMES_TO_BUFFER; f++) {
        auto frame = &mFrame[f];
        ThrowIfFailed(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&frame->mCmdAlloc)));

        frame->mDynamicUpload = new UploadHeapT<DynamicUploadHeap>(mDevice);
        auto dynamicUploadWO = frame->mDynamicUpload->DataWO();
        auto dynamicUploadGPUVA = frame->mDynamicUpload->Heap()->GetGPUVirtualAddress();

        frame->mDrawConstantBuffersGPUVA = dynamicUploadGPUVA + offsetof(DynamicUploadHeap, mDrawConstantBuffers);

        // Set any static asteroid data now
        auto staticData = mAsteroids->StaticData();
        for (int j = 0; j < NUM_ASTEROIDS; ++j) {
            auto constants = &dynamicUploadWO->mDrawConstantBuffers[j];
            constants->mSurfaceColor = staticData[j].surfaceColor;
            constants->mDeepColor = staticData[j].deepColor;
            constants->mTextureIndex = staticData[j].textureIndex;

            auto indirectDraw = &dynamicUploadWO->mIndirectArgs[j];
            indirectDraw->mConstantBuffer = frame->mDrawConstantBuffersGPUVA + sizeof(DrawConstantBuffer) * j;
            indirectDraw->mDrawIndexed.InstanceCount = 1;
            indirectDraw->mDrawIndexed.StartInstanceLocation = 0;
            indirectDraw->mDrawIndexed.BaseVertexLocation = staticData[j].vertexStart;
        }
        
        // Dynamic sprite vertices        
        {
            frame->mSpriteVertexBufferView.BufferLocation = dynamicUploadGPUVA + offsetof(DynamicUploadHeap, mSpriteVertices);
            frame->mSpriteVertexBufferView.StrideInBytes  = sizeof(SpriteVertex);
            frame->mSpriteVertexBufferView.SizeInBytes    = sizeof(SpriteVertex) * MAX_SPRITE_VERTICES_PER_FRAME;
        }
        
        // General frame SRV descriptor heap
        {
            // We allocate a pile of extra space for dynamic descriptors (GUI, etc)
            frame->mSRVDescs = new SRVDescriptorList(mDevice, 100);

            // Skybox constants
            frame->mSkyboxConstants = dynamicUploadGPUVA + offsetof(DynamicUploadHeap, mSkyboxConstants);
                                    
            // Skybox texture
            {
                auto textureDesc = mSkybox->GetDesc();

                D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                srvDesc.Format = textureDesc.Format;
                srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;                
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
                srvDesc.TextureCube.MipLevels = 1;
                srvDesc.TextureCube.MostDetailedMip = 0;
                srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

                frame->mSkyboxTexture = frame->mSRVDescs->AppendSRV(mSkybox, &srvDesc);
            }

            // The rest of the heap we'll use for dynamically copy descriptors into, etc.
            frame->mSRVDescsDynamicStart = frame->mSRVDescs->Size();
        }
    }

    // Command Lists
    ThrowIfFailed(mDevice->CreateCommandList(1, D3D12_COMMAND_LIST_TYPE_DIRECT, mFrame[0].mCmdAlloc, mAsteroidPSO, IID_PPV_ARGS(&mPostCmdLst)));
    ThrowIfFailed(mPostCmdLst->Close()); // Avoid allocator issues... command lists really should be created in a "closed" state...
    ThrowIfFailed(mDevice->CreateCommandList(1, D3D12_COMMAND_LIST_TYPE_DIRECT, mFrame[0].mCmdAlloc, mAsteroidPSO, IID_PPV_ARGS(&mPreCmdLst)));
    ThrowIfFailed(mPreCmdLst->Close());

    // Each command list for asteroid drawing maps 1:1 with a descriptor heap
    // Change heaps is "free" at cmdlst boundaries and this greatly simplifies the code
    // Thus the expectation is that we have ~ #threads heaps for multithreaded rendering on most GPUs
    // Need at least one draw in each heap/cmd list...
    minCmdLsts = std::min(minCmdLsts, (UINT)NUM_ASTEROIDS);
    CreateSubsets(minCmdLsts);
    std::cout << "Using " << mSubsetCount << " subsets per frame." << std::endl;
    
    // Just in case
    WaitForAll();
}

Asteroids::~Asteroids()
{
    WaitForAll();
    ReleaseSwapChain();

    SafeRelease(&mPreCmdLst);
    SafeRelease(&mPostCmdLst);

    for (UINT i = 0; i < NUM_UNIQUE_TEXTURES; ++i) {
        SafeRelease(&mAsteroidTextures[i]);
    }
    for (auto i : mSpriteTextures) {
        i.second->Release();
    }

    SafeRelease(&mAsteroidPSO);
    SafeRelease(&mFontTexture);
    SafeRelease(&mFontPSO);
    SafeRelease(&mSpritePSO);
    SafeRelease(&mSkybox);
    SafeRelease(&mSkyboxPSO);
        
    for (UINT f = 0; f < NUM_FRAMES_TO_BUFFER; ++f) {
        auto frame = &mFrame[f];
        SafeRelease(&frame->mCmdAlloc);
        delete frame->mDynamicUpload;
        delete frame->mSRVDescs;
    }

    ReleaseSubsets();
    
    delete mMeshUpload;

    delete mRTVDescs;
    delete mDSVDescs;
    delete mSMPDescs;
    delete mSRVDescs;

    SafeRelease(&mGenericRootSignature);    
    SafeRelease(&mAsteroidsRootSignature);
    SafeRelease(&mCommandSignature);
    SafeRelease(&mFence);

    SafeRelease(&mCommandQueue);
    SafeRelease(&mDevice);

    if (mFenceEventHandle != NULL) {
        CloseHandle(mFenceEventHandle);
        mFenceEventHandle = NULL;
    }

    delete mD3D12Sprite;
}


void Asteroids::ReleaseSwapChain()
{
    WaitForAll();

    for (UINT s = 0; s < NUM_SWAP_CHAIN_BUFFERS; s++) {
        SafeRelease(&mSwapChainBuffer[s].mRenderTarget);
    }
    SafeRelease(&mDepthStencil);
    SafeRelease(&mSwapChain);
}

inline void DisableDXGIWindowChanges(IUnknown* device, HWND window)
{
    IDXGIDevice * pDXGIDevice;
    ThrowIfFailed(device->QueryInterface(IID_PPV_ARGS(&pDXGIDevice)));
    IDXGIAdapter * pDXGIAdapter;
    ThrowIfFailed(pDXGIDevice->GetParent(IID_PPV_ARGS(&pDXGIAdapter)));
    IDXGIFactory * pIDXGIFactory;
    ThrowIfFailed(pDXGIAdapter->GetParent(IID_PPV_ARGS(&pIDXGIFactory)));

    ThrowIfFailed(pIDXGIFactory->MakeWindowAssociation(window, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER));

    pIDXGIFactory->Release();
    pDXGIAdapter->Release();
    pDXGIDevice->Release();
}

void Asteroids::ResizeSwapChain(IDXGIFactory2* dxgiFactory, HWND outputWindow, unsigned int width, unsigned int height)
{
    ReleaseSwapChain();

    // Create swap chain
    {
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = width;
        swapChainDesc.Height = height;
        swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // Can create an SRGB render target view on the swap chain buffer
        swapChainDesc.Stereo = FALSE;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = NUM_SWAP_CHAIN_BUFFERS;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED; // Not used
        swapChainDesc.Flags = 0;

        IDXGISwapChain1 *swapChain1 = nullptr;
        ThrowIfFailed(dxgiFactory->CreateSwapChainForHwnd(
            mCommandQueue, outputWindow, &swapChainDesc, nullptr, nullptr, &swapChain1));
        // MakeWindowAssociation must be called after CreateSwapChain
        ThrowIfFailed(dxgiFactory->MakeWindowAssociation(outputWindow,
            DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_PRINT_SCREEN));
        ThrowIfFailed(swapChain1->QueryInterface(&mSwapChain));
        swapChain1->Release();
    }

    {
        // Create an SRGB view of the swap chain buffer
        D3D12_RENDER_TARGET_VIEW_DESC desc = {};
        desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        desc.Texture2D.MipSlice = 0;
        desc.Texture2D.PlaneSlice = 0;

        // Create render target view for each swap chain buffer
        for (UINT s = 0; s < NUM_SWAP_CHAIN_BUFFERS; s++) {
            auto buffer = &mSwapChainBuffer[s];
            ThrowIfFailed(mSwapChain->GetBuffer(s, IID_PPV_ARGS(&buffer->mRenderTarget)));
            mDevice->CreateRenderTargetView(buffer->mRenderTarget, &desc, buffer->mRenderTargetView);
        }
    }

    // create depth stencil view
    {
        D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_D32_FLOAT, width, height, 1, 1, 1, 0,
            D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL );

        // We use (1-z) so typically clear depth to 0
        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = desc.Format;
        clearValue.DepthStencil.Depth = 0.0f;
        clearValue.DepthStencil.Stencil = 0;

        ThrowIfFailed(mDevice->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &clearValue,
            IID_PPV_ARGS(&mDepthStencil)
        ));

        mDevice->CreateDepthStencilView(mDepthStencil, nullptr, mDepthStencilView);
    }    
    
    // create the viewport and scissor
    ZeroMemory(&mViewPort, sizeof(D3D11_VIEWPORT));
    mViewPort.TopLeftX = 0;
    mViewPort.TopLeftY = 0;
    mViewPort.Width = (float)width;
    mViewPort.Height = (float)height;
    mViewPort.MinDepth = 0.0f;
    mViewPort.MaxDepth = 1.0f;

    mScissorRect.left = 0;
    mScissorRect.top = 0;
    mScissorRect.right = width;
    mScissorRect.bottom = height;
}

void Asteroids::CreatePSOs()
{
    // Generic root signature (t0, s0, b0)
    {
        CD3DX12_DESCRIPTOR_RANGE descRanges[2];
        descRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0); // t0
        descRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0); // s0

        CD3DX12_ROOT_PARAMETER rootParams[3];
        rootParams[RP_DRAW_CBV].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL); // b0
        rootParams[RP_TEX_SRV].InitAsDescriptorTable(1, &descRanges[0], D3D12_SHADER_VISIBILITY_PIXEL); // t0
        rootParams[RP_SMP].InitAsDescriptorTable(1, &descRanges[1], D3D12_SHADER_VISIBILITY_PIXEL); // s0
                
        CD3DX12_ROOT_SIGNATURE_DESC RSLayout(ARRAYSIZE(rootParams), rootParams, 0, 0,
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

        ID3DBlob* serializedLayout = nullptr;
        D3D12SerializeRootSignature(&RSLayout, D3D_ROOT_SIGNATURE_VERSION_1, &serializedLayout, 0);

        ThrowIfFailed(mDevice->CreateRootSignature(
            1,
            serializedLayout->GetBufferPointer(), 
            serializedLayout->GetBufferSize(),
            IID_PPV_ARGS(&mGenericRootSignature)));

        serializedLayout->Release();
    }

    // Asteroids root signature (tN, s0, b0)
    {
        CD3DX12_DESCRIPTOR_RANGE descRanges[2];
        descRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, NUM_UNIQUE_TEXTURES, 0, 0); // t0...tN
        descRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0); // s0

        CD3DX12_ROOT_PARAMETER rootParams[3];
        rootParams[RP_DRAW_CBV].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL); // b0
        rootParams[RP_TEX_SRV].InitAsDescriptorTable(1, &descRanges[0], D3D12_SHADER_VISIBILITY_PIXEL); // t0
        rootParams[RP_SMP].InitAsDescriptorTable(1, &descRanges[1], D3D12_SHADER_VISIBILITY_PIXEL); // s0
        
        CD3DX12_ROOT_SIGNATURE_DESC RSLayout(ARRAYSIZE(rootParams), rootParams, 0, 0,
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

        ID3DBlob* serializedLayout = nullptr;
        D3D12SerializeRootSignature(&RSLayout, D3D_ROOT_SIGNATURE_VERSION_1, &serializedLayout, 0);

        ThrowIfFailed(mDevice->CreateRootSignature(
            1,
            serializedLayout->GetBufferPointer(),
            serializedLayout->GetBufferSize(),
            IID_PPV_ARGS(&mAsteroidsRootSignature)));

        serializedLayout->Release();
    }

    // Command signature
    {
        D3D12_INDIRECT_ARGUMENT_DESC args[2] = {};
        args[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
        args[0].ConstantBufferView.RootParameterIndex = RP_DRAW_CBV;
        args[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

        D3D12_COMMAND_SIGNATURE_DESC desc;
        desc.ByteStride = sizeof(ExecuteIndirectArgs);
        desc.NodeMask = 1;
        desc.pArgumentDescs = args;
        desc.NumArgumentDescs = ARRAYSIZE(args);

        ThrowIfFailed(mDevice->CreateCommandSignature(&desc, mGenericRootSignature, IID_PPV_ARGS(&mCommandSignature)));
    }

    // Common state for this app
    D3D12_GRAPHICS_PIPELINE_STATE_DESC defaultDesc = {};
    defaultDesc.BlendState = CD3DX12_BLEND_DESC(CD3DX12_DEFAULT());
    defaultDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(CD3DX12_DEFAULT());
    defaultDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(CD3DX12_DEFAULT());    
    defaultDesc.pRootSignature = mGenericRootSignature;
    defaultDesc.SampleMask = UINT_MAX;
    defaultDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    defaultDesc.NumRenderTargets = 1;
    defaultDesc.RTVFormats[0] = mRTVFormat;
    defaultDesc.DSVFormat = mDSVFormat;
    defaultDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
    defaultDesc.SampleDesc.Count = 1;
    
    // asteroid pipeline state
    D3D12_INPUT_ELEMENT_DESC asteroidInputDesc[] = {
            { "ATTRIB", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "ATTRIB", 1, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
    D3D12_GRAPHICS_PIPELINE_STATE_DESC asteroidDesc = defaultDesc;
    asteroidDesc.pRootSignature = mAsteroidsRootSignature;
    asteroidDesc.VS = { g_asteroid_vs, sizeof(g_asteroid_vs) };
    asteroidDesc.PS = { g_asteroid_ps, sizeof(g_asteroid_ps) };
    asteroidDesc.InputLayout = { asteroidInputDesc, ARRAYSIZE(asteroidInputDesc) };
    
    // skybox pipeline state
    D3D12_INPUT_ELEMENT_DESC skyboxInputDesc[] = {
            { "ATTRIB", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "ATTRIB", 1, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
    D3D12_GRAPHICS_PIPELINE_STATE_DESC skyboxDesc = defaultDesc;
    skyboxDesc.VS = { g_skybox_vs, sizeof(g_skybox_vs) };
    skyboxDesc.PS = { g_skybox_ps, sizeof(g_skybox_ps) };
    skyboxDesc.InputLayout = { skyboxInputDesc, ARRAYSIZE(skyboxInputDesc) };

    // sprite pipeline state
    D3D12_INPUT_ELEMENT_DESC spriteInputDesc[] = {
            { "ATTRIB", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "ATTRIB", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
    D3D12_GRAPHICS_PIPELINE_STATE_DESC spriteDesc = defaultDesc;
    spriteDesc.VS = { g_sprite_vs, sizeof(g_sprite_vs) };
    spriteDesc.PS = { g_sprite_ps, sizeof(g_sprite_ps) };
    spriteDesc.InputLayout = { spriteInputDesc, ARRAYSIZE(spriteInputDesc) };
    spriteDesc.DepthStencilState.DepthEnable = FALSE;
    // Premultiplied over blend
    spriteDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
    spriteDesc.BlendState.RenderTarget[0].SrcBlend    = D3D12_BLEND_ONE;
    spriteDesc.BlendState.RenderTarget[0].BlendOp     = D3D12_BLEND_OP_ADD;
    spriteDesc.BlendState.RenderTarget[0].DestBlend   = D3D12_BLEND_INV_SRC_ALPHA;
        
    // font pipeline state
    D3D12_GRAPHICS_PIPELINE_STATE_DESC fontDesc = spriteDesc;
    fontDesc.PS = { g_font_ps, sizeof(g_font_ps) };

    concurrency::parallel_invoke(
        [&] { ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&asteroidDesc, IID_PPV_ARGS(&mAsteroidPSO))); },
        [&] { ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&skyboxDesc,   IID_PPV_ARGS(&mSkyboxPSO)));   },
        [&] { ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&spriteDesc,   IID_PPV_ARGS(&mSpritePSO)));   },
        [&] { ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&fontDesc,     IID_PPV_ARGS(&mFontPSO)));     }
    );
}


void Asteroids::CreateSubsets(UINT numHeapsPerFrame)
{
    ReleaseSubsets();
    
    mDrawsPerSubset = (NUM_ASTEROIDS + numHeapsPerFrame - 1) / numHeapsPerFrame;
    mSubsetCount = numHeapsPerFrame;
    
    for (UINT f = 0; f < NUM_FRAMES_TO_BUFFER; f++) {
        // Per-frame data
        auto frame = &mFrame[f];
        //auto dynamicUploadGPUVA = frame->mDynamicUpload->Heap()->GetGPUVirtualAddress();

        for (UINT subsetIdx = 0; subsetIdx < mSubsetCount; ++subsetIdx) {
            void* memory = _aligned_malloc(sizeof(SubsetD3D12), 64);
            auto subset = new(memory) SubsetD3D12(mDevice, NUM_UNIQUE_TEXTURES, mAsteroidPSO);
            frame->mSubsets.push_back(subset);
        }
    }
}

void Asteroids::ReleaseSubsets()
{
    WaitForAll();

    for (UINT i = 0; i < NUM_FRAMES_TO_BUFFER; ++i) {
        for (auto j : mFrame[i].mSubsets) {
            j->~SubsetD3D12();
            _aligned_free(j);
        }
        mFrame[i].mSubsets.clear();
    }

    mSubsetCount = 0;
    mDrawsPerSubset = 0;
}


void Asteroids::CreateMeshes()
{
    auto asteroidMeshes = mAsteroids->Meshes();
    std::vector<SkyboxVertex> skyboxVertices;
    CreateSkyboxMesh(&skyboxVertices);

    // Simple linear allocate
    UINT64 asteroidVBSize = asteroidMeshes->vertices.size() * sizeof(asteroidMeshes->vertices[0]);
    UINT64 asteroidIBSize = asteroidMeshes->indices.size()  * sizeof(asteroidMeshes->indices[0]);
    UINT64 skyboxVBSize = skyboxVertices.size() * sizeof(SkyboxVertex);

    UINT64 asteroidVBOffset = 0;
    UINT64 asteroidIBOffset = asteroidVBOffset + asteroidVBSize;
    UINT64 skyboxVBOffset   = asteroidIBOffset + asteroidIBSize;    
    UINT64 totalSize = skyboxVBOffset + skyboxVBSize;
        
    mMeshUpload = new UploadHeap(mDevice, totalSize);
    auto bufferWO = (BYTE*)mMeshUpload->DataWO();
    auto gpuVA = mMeshUpload->Heap()->GetGPUVirtualAddress();

    // Asteroid vertices
    {
        memcpy(bufferWO + static_cast<size_t>(asteroidVBOffset), asteroidMeshes->vertices.data(), static_cast<size_t>(asteroidVBSize));

        mAsteroidVertexBufferView.BufferLocation = gpuVA + asteroidVBOffset;
        mAsteroidVertexBufferView.SizeInBytes    = static_cast<UINT>(asteroidVBSize);
        mAsteroidVertexBufferView.StrideInBytes  = sizeof(asteroidMeshes->vertices[0]);
    }

    // Asteroid indices
    {
        memcpy(bufferWO + static_cast<size_t>(asteroidIBOffset), asteroidMeshes->indices.data(), static_cast<size_t>(asteroidIBSize));

        mAsteroidIndexBufferView.BufferLocation = gpuVA + asteroidIBOffset;
        mAsteroidIndexBufferView.SizeInBytes    = static_cast<UINT>(asteroidIBSize);
        mAsteroidIndexBufferView.Format         = sizeof(IndexType) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
    }
    
    // Skybox vertices
    {
        memcpy(bufferWO + static_cast<size_t>(skyboxVBOffset), skyboxVertices.data(), static_cast<size_t>(skyboxVBSize));

        mSkyboxVertexBufferView.BufferLocation = gpuVA + skyboxVBOffset;
        mSkyboxVertexBufferView.SizeInBytes    = static_cast<UINT>(skyboxVBSize);
        mSkyboxVertexBufferView.StrideInBytes  = sizeof(skyboxVertices[0]);
    }
}


void Asteroids::CreateGUIResources()
{
    auto font = mGUI->Font();
    auto textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_R8_UNORM, font->BitmapWidth(), font->BitmapHeight(), 1, 1);
    
    ThrowIfFailed(mDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&mFontTexture)
    ));

    D3D11_SUBRESOURCE_DATA initialData = {};
    initialData.pSysMem = font->Pixels();
    initialData.SysMemPitch = font->BitmapWidth();

    InitializeTexture2D(mDevice, mCommandQueue, mFontTexture, &textureDesc, 1, &initialData);

    // Load any GUI sprite textures
    for (int i = -1; i < (int)mGUI->size(); ++i) {
        auto control = i>=0 ? (*mGUI)[i] : mD3D12Sprite;
        if (control->TextureFile().length() > 0 && mSpriteTextures.find(control->TextureFile()) == mSpriteTextures.end()) {
            ID3D12Resource* texture = nullptr;
            ThrowIfFailed(CreateTexture2DFromDDS_XXXX8(
                mDevice, mCommandQueue, &texture, control->TextureFile().c_str(), DXGI_FORMAT_B8G8R8A8_UNORM_SRGB));
            mSpriteTextures[control->TextureFile()] = texture;
        }
    }
}

void Asteroids::WaitForAll()
{
    ThrowIfFailed(mCommandQueue->Signal(mFence, ++mCurrentFence));
    ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFence, mFenceEventHandle));
    WaitForSingleObject(mFenceEventHandle, INFINITE);
}

void Asteroids::WaitForReadyToRender()
{
    // Wait for both the GPU to be done with our per-frame resources
    HANDLE handles[] = { mFenceEventHandle };
    
    ThrowIfFailed(mFence->SetEventOnCompletion(mFrame[mCurrentFrameIndex].mFrameCompleteFence, mFenceEventHandle));
    WaitForMultipleObjects(ARRAYSIZE(handles), handles, TRUE, INFINITE);
}

void Asteroids::RenderSubset(
    D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView,
    size_t frameIndex, float frameTime,
    SubsetD3D12* subset, UINT subsetIdx,
    XMVECTOR cameraEye, XMMATRIX viewProjection,
    const Settings& settings)
{
    UINT drawStart = mDrawsPerSubset * subsetIdx;
    UINT drawEnd = std::min(drawStart + mDrawsPerSubset, (UINT)NUM_ASTEROIDS);
    assert(drawStart < drawEnd);
    auto staticAsteroidData = mAsteroids->StaticData();
    auto dynamicAsteroidData = mAsteroids->DynamicData();
    // Frame data
    auto frame = &mFrame[frameIndex];
    auto drawConstantBuffers = frame->mDynamicUpload->DataWO()->mDrawConstantBuffers;
    auto indirectArgs = frame->mDynamicUpload->DataWO()->mIndirectArgs;
    
    auto cmdLst = subset->Begin(mAsteroidPSO);

    // Root signature and common bindings
    cmdLst->SetGraphicsRootSignature(mAsteroidsRootSignature);
    ID3D12DescriptorHeap* heaps[2] = {mSRVDescs->Heap(), mSMPDescs->Heap()};
    cmdLst->SetDescriptorHeaps(ARRAYSIZE(heaps), heaps);
        
    // Common state
    cmdLst->IASetIndexBuffer(&mAsteroidIndexBufferView);
    cmdLst->IASetVertexBuffers(0, 1, &mAsteroidVertexBufferView);
    cmdLst->RSSetViewports(1, &mViewPort);
    cmdLst->RSSetScissorRects(1, &mScissorRect);
    cmdLst->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdLst->OMSetRenderTargets(1, &renderTargetView, true, &mDepthStencilView);
        
    // Set textures (all as a single descriptor table) and samplers
    cmdLst->SetGraphicsRootDescriptorTable(RP_TEX_SRV, mSRVDescs->GPU(0));
    cmdLst->SetGraphicsRootDescriptorTable(RP_SMP, mSampler);
        
    if (!settings.executeIndirect)
    {
        // Standard draw path
        auto constantsPointer = frame->mDrawConstantBuffersGPUVA + sizeof(DrawConstantBuffer) * drawStart;
        for (UINT drawIdx = drawStart; drawIdx < drawEnd; ++drawIdx)
        {
            auto staticData = &staticAsteroidData[drawIdx];
            auto dynamicData = &dynamicAsteroidData[drawIdx];

            XMStoreFloat4x4(&drawConstantBuffers[drawIdx].mWorld, dynamicData->world);
            XMStoreFloat4x4(&drawConstantBuffers[drawIdx].mViewProjection, viewProjection);

            // Set root cbuffer
            //cmdLst->SetGraphicsRootDescriptorTable(RP_TEX_SRV, mSRVDescs->GPU(0));
            cmdLst->SetGraphicsRootConstantBufferView(RP_DRAW_CBV, constantsPointer);
            constantsPointer += sizeof(DrawConstantBuffer);
                    
            cmdLst->DrawIndexedInstanced(dynamicData->indexCount, 1, dynamicData->indexStart, staticData->vertexStart, 0);
        }
    }
    else
    {
        // ExecuteIndirect path
        for (UINT drawIdx = drawStart; drawIdx < drawEnd; ++drawIdx)
        {
            auto dynamicData = &dynamicAsteroidData[drawIdx];

            XMStoreFloat4x4(&drawConstantBuffers[drawIdx].mWorld, dynamicData->world);
            XMStoreFloat4x4(&drawConstantBuffers[drawIdx].mViewProjection, viewProjection);

            auto drawIndexed = &indirectArgs[drawIdx].mDrawIndexed;
            drawIndexed->IndexCountPerInstance = dynamicData->indexCount;
            drawIndexed->StartIndexLocation = dynamicData->indexStart;
        }

        UINT64 offset = (BYTE*)(&indirectArgs[drawStart]) - (BYTE*)frame->mDynamicUpload->DataWO();
        cmdLst->ExecuteIndirect(mCommandSignature, drawEnd - drawStart,
                                frame->mDynamicUpload->Heap(), offset,
                                nullptr, 0);
    }

    subset->End();
}

void Asteroids::Render(float frameTime, const OrbitCamera& camera, const Settings& settings)
{
    // Pick the right swap chain buffer based on where DXGI says we are...
    auto backBufferIndex = mSwapChain->GetCurrentBackBufferIndex();
    assert(backBufferIndex < NUM_SWAP_CHAIN_BUFFERS);
    auto swapChainBuffer = &mSwapChainBuffer[backBufferIndex];

    // And the right frame data based on our own rotation/fences
    assert(mCurrentFrameIndex < NUM_FRAMES_TO_BUFFER);
    auto frame = &mFrame[mCurrentFrameIndex];

    QueryPerformanceCounter((LARGE_INTEGER*)&mTotalUpdateTicks);
    // Update asteroid simulation
    if (settings.multithreadedRendering)
    {
        concurrency::parallel_for<UINT>(0, mSubsetCount, [&](UINT subsetIdx) {
            UINT drawStart = mDrawsPerSubset * subsetIdx;
            UINT drawEnd = std::min(drawStart + mDrawsPerSubset, (UINT)NUM_ASTEROIDS);
            mAsteroids->Update(frameTime, camera.Eye(), settings, drawStart, drawEnd - drawStart);
        });
    }
    else
    {
        mAsteroids->Update(frameTime, camera.Eye(), settings, 0, (UINT)NUM_ASTEROIDS);
    }
    LONG64 currCounter;
    QueryPerformanceCounter((LARGE_INTEGER*)&currCounter);
    mTotalUpdateTicks = currCounter - mTotalUpdateTicks;

    mTotalRenderTicks = currCounter;
    // Generate command lists
    if (settings.multithreadedRendering)
    {
        concurrency::parallel_for<UINT>(0, mSubsetCount, [&](UINT subsetIdx) {
            RenderSubset(swapChainBuffer->mRenderTargetView, mCurrentFrameIndex, frameTime,
                frame->mSubsets[subsetIdx], subsetIdx, camera.Eye(), camera.ViewProjection(), settings);
        });
    }
    else
    {
        for (unsigned int subsetIdx = 0; subsetIdx < mSubsetCount; ++subsetIdx) {
            RenderSubset(swapChainBuffer->mRenderTargetView, mCurrentFrameIndex, frameTime,
                frame->mSubsets[subsetIdx], subsetIdx, camera.Eye(), camera.ViewProjection(), settings);
        }
    }
    QueryPerformanceCounter((LARGE_INTEGER*)&currCounter);
    mTotalRenderTicks = currCounter - mTotalRenderTicks;

    LONG64 prevCounter = currCounter;
    // Set up pre and post commands
    {
        auto cmdAlloc = frame->mCmdAlloc;
        auto dynamicUploadWO = frame->mDynamicUpload->DataWO();
        ThrowIfFailed(cmdAlloc->Reset());

        // Set resource states for rendering
        ResourceBarrier rb;
        rb.AddTransition(swapChainBuffer->mRenderTarget, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

        // Pre
        {
            ThrowIfFailed(mPreCmdLst->Reset(cmdAlloc, mAsteroidPSO));
            rb.Submit(mPreCmdLst);

            // Don't need to clear color at the moment - skybox overwrites it all and no MSAA
            //float clearcol[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            //mPreCmdLst->ClearRenderTargetView(swapChainBuffer->mRenderTargetView, clearcol, 0, 0);

            // Clear depth
            mPreCmdLst->ClearDepthStencilView(mDepthStencilView, D3D12_CLEAR_FLAG_DEPTH, 0.0f, 0, 0, nullptr);

            ThrowIfFailed(mPreCmdLst->Close());
        }

        rb.ReverseTransitions();

        // Post
        {
            ThrowIfFailed(mPostCmdLst->Reset(cmdAlloc, mSkyboxPSO));

            // Root signature and descriptor heaps
            mPostCmdLst->SetGraphicsRootSignature(mGenericRootSignature);
            ID3D12DescriptorHeap* heaps[2] = { frame->mSRVDescs->Heap(), mSMPDescs->Heap() };
            mPostCmdLst->SetDescriptorHeaps(2, heaps);

            mPostCmdLst->SetGraphicsRootDescriptorTable(RP_SMP, mSampler);

            // Common state
            mPostCmdLst->RSSetViewports(1, &mViewPort);
            mPostCmdLst->RSSetScissorRects(1, &mScissorRect);
            mPostCmdLst->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            mPostCmdLst->OMSetRenderTargets(1, &swapChainBuffer->mRenderTargetView, true, &mDepthStencilView);
            
            // Draw skybox
            {
                auto constants = &frame->mDynamicUpload->DataWO()->mSkyboxConstants;
                XMStoreFloat4x4(&constants->mViewProjection, camera.ViewProjection());

                mPostCmdLst->IASetVertexBuffers(0, 1, &mSkyboxVertexBufferView);

                mPostCmdLst->SetGraphicsRootConstantBufferView(RP_DRAW_CBV, frame->mSkyboxConstants);
                mPostCmdLst->SetGraphicsRootDescriptorTable(RP_TEX_SRV, frame->mSkyboxTexture);
                mPostCmdLst->DrawInstanced(6 * 6, 1, 0, 0);
            }

            // Draw sprites
            {
                auto vertexBase = dynamicUploadWO->mSpriteVertices;
                auto vertexEnd = vertexBase;

                // Drop off any dynamic descriptors from the last frame
                auto descHeap = frame->mSRVDescs;
                descHeap->Resize(frame->mSRVDescsDynamicStart);

                mPostCmdLst->IASetVertexBuffers(0, 1, &frame->mSpriteVertexBufferView);

                for (int i = -1; i < (int)mGUI->size(); ++i) {
                    auto control = i >=0 ? (*mGUI)[i] : mD3D12Sprite;
                    if (!control->Visible()) continue;

                    UINT numVertices = (UINT)(control->Draw(mViewPort.Width, mViewPort.Height, vertexEnd) - vertexEnd);
                    ID3D12Resource* texture = nullptr;

                    // TODO: Could eliminate redundant state setting and cache descriptors... meh for now.
                    if (control->TextureFile().length() == 0) { // Font
                        mPostCmdLst->SetPipelineState(mFontPSO);
                        texture = mFontTexture;
                    }
                    else { // Sprite
                        mPostCmdLst->SetPipelineState(mSpritePSO);
                        texture = mSpriteTextures[control->TextureFile()];
                    }

                    if (texture) {
                        mPostCmdLst->SetGraphicsRootDescriptorTable(RP_TEX_SRV, descHeap->AppendSRV(texture));
                    }

                    mPostCmdLst->DrawInstanced(numVertices, 1, (UINT)(vertexEnd - vertexBase), 0);
                    vertexEnd += numVertices;
                }
            }

            // Final resource state transitions
            rb.Submit(mPostCmdLst);
            ThrowIfFailed(mPostCmdLst->Close());
        }
    }
    
    // Set up command lists for submission
    mCmdListsToSubmit.resize(0);
    mCmdListsToSubmit.push_back(mPreCmdLst);
    if (settings.submitRendering) {
        for (auto &i :frame->mSubsets)
            mCmdListsToSubmit.push_back(i->mCmdLst);
    }
    mCmdListsToSubmit.push_back(mPostCmdLst);

    mCommandQueue->ExecuteCommandLists(
        (UINT)mCmdListsToSubmit.size(),
        CommandListCast(mCmdListsToSubmit.data()));

    if (settings.vsync)
        ThrowIfFailed(mSwapChain->Present(1, 0));
    else
        ThrowIfFailed(mSwapChain->Present(0, 0));
    
    ThrowIfFailed(mCommandQueue->Signal(mFence, ++mCurrentFence));
    frame->mFrameCompleteFence = mCurrentFence;
    mCurrentFrameIndex = (mCurrentFrameIndex + 1) % NUM_FRAMES_TO_BUFFER;

    //LONG64 currCounter;
    QueryPerformanceCounter((LARGE_INTEGER*)&currCounter);
    mTotalRenderTicks += currCounter - prevCounter;
}

void Asteroids::GetPerfCounters(float &UpdateTime, float &RenderTime)
{
    UpdateTime = (float)mTotalUpdateTicks/(float)mPerfCounterFreq;
    RenderTime = (float)mTotalRenderTicks/(float)mPerfCounterFreq;
}

} // namespace AsteroidsD3D12
