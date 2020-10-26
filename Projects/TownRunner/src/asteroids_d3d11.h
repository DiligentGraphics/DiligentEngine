// Copyright 2014 Intel Corporation All Rights Reserved
//
// Intel makes no representations about the suitability of this software for any purpose.  
// THIS SOFTWARE IS PROVIDED ""AS IS."" INTEL SPECIFICALLY DISCLAIMS ALL WARRANTIES,
// EXPRESS OR IMPLIED, AND ALL LIABILITY, INCLUDING CONSEQUENTIAL AND OTHER INDIRECT DAMAGES,
// FOR THE USE OF THIS SOFTWARE, INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PROPRIETARY
// RIGHTS, AND INCLUDING THE WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
// Intel does not assume any responsibility for any errors which may appear in this software
// nor any responsibility to update it.

#pragma once

#include <d3d11.h>
#include <dxgi1_2.h>
#include <directxmath.h>
#include <random>
#include <map>

#include "camera.h"
#include "settings.h"
#include "simulation.h"
#include "util.h"
#include "gui.h"

namespace AsteroidsD3D11 {

struct DrawConstantBuffer {
    DirectX::XMFLOAT4X4 mWorld;
    DirectX::XMFLOAT4X4 mViewProjection;
    DirectX::XMFLOAT3 mSurfaceColor;
    float unused0;
    DirectX::XMFLOAT3 mDeepColor;
    float unused1;
    UINT unused2[4];
};

struct SkyboxConstantBuffer {
    DirectX::XMFLOAT4X4 mViewProjection;
};

class Asteroids {
public:
    Asteroids(AsteroidsSimulation* asteroids, GUI* gui, bool warp);
    ~Asteroids();

    void Render(float frameTime, const OrbitCamera& camera, const Settings& settings);

    void ReleaseSwapChain();
    void ResizeSwapChain(IDXGIFactory2* dxgiFactory, HWND outputWindow, unsigned int width, unsigned int height);

    void GetPerfCounters(float &UpdateTime, float &RenderTime);

private:
    void CreateMeshes();
    void InitializeTextureData();
    void CreateGUIResources();

    AsteroidsSimulation*        mAsteroids = nullptr;
    GUI*                        mGUI = nullptr;

    IDXGISwapChain1*            mSwapChain = nullptr;
    ID3D11Device*               mDevice = nullptr;
    ID3D11DeviceContext*        mDeviceCtxt = nullptr;

    // These depend on the swap chain buffer size
    ID3D11Texture2D*            mRenderTarget = nullptr;
    ID3D11RenderTargetView*     mRenderTargetView = nullptr;
    ID3D11DepthStencilView*     mDepthStencilView = nullptr;
    D3D11_VIEWPORT              mViewPort;
    D3D11_RECT                  mScissorRect;

    ID3D11DepthStencilState*    mDepthStencilState = nullptr;
    ID3D11BlendState*           mBlendState = nullptr;
    ID3D11BlendState*           mSpriteBlendState = nullptr;

    ID3D11InputLayout*          mInputLayout = nullptr;
    ID3D11Buffer*               mIndexBuffer = nullptr;
    ID3D11Buffer*               mVertexBuffer = nullptr;
    ID3D11VertexShader*         mVertexShader = nullptr;
    ID3D11PixelShader*          mPixelShader = nullptr;
    ID3D11Buffer*               mDrawConstantBuffer = nullptr;

    ID3D11VertexShader*         mSpriteVertexShader = nullptr;
    ID3D11PixelShader*          mSpritePixelShader = nullptr;
    ID3D11InputLayout*          mSpriteInputLayout = nullptr;
    ID3D11Buffer*               mSpriteVertexBuffer = nullptr;
    std::map<std::string, ID3D11ShaderResourceView*> mSpriteTextures;

    ID3D11PixelShader*          mFontPixelShader = nullptr;
    ID3D11ShaderResourceView*   mFontTextureSRV = nullptr;

    ID3D11VertexShader*         mSkyboxVertexShader = nullptr;
    ID3D11PixelShader*          mSkyboxPixelShader = nullptr;
    ID3D11Buffer*               mSkyboxConstantBuffer = nullptr;
    ID3D11Buffer*               mSkyboxVertexBuffer = nullptr;
    ID3D11InputLayout*          mSkyboxInputLayout = nullptr;
    ID3D11ShaderResourceView*   mSkyboxSRV = nullptr;

    ID3D11Texture2D*            mTextures[NUM_UNIQUE_TEXTURES];
    ID3D11ShaderResourceView*   mTextureSRVs[NUM_UNIQUE_TEXTURES];
    ID3D11SamplerState*         mSamplerState = nullptr;

    GUISprite*                  mD3D11Sprite = nullptr;

    UINT64 mPerfCounterFreq = 0;
    volatile LONG64 mTotalUpdateTicks = 0, mTotalRenderTicks = 0;
};

} // namespace AsteroidsD3D11
