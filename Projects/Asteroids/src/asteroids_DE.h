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

#include "RenderDevice.h"
#include "SwapChain.h"
#include "DeviceContext.h"
#include "RefCntAutoPtr.h"
#include "LockHelper.h"
#include <map>
#include <mutex>
#include <atomic>

#include "camera.h"
#include "settings.h"
#include "simulation.h"
#include "util.h"
#include "gui.h"


namespace AsteroidsDE {

struct DrawConstantBuffer {
    DirectX::XMFLOAT4X4 mWorld;
    DirectX::XMFLOAT4X4 mViewProjection;
    DirectX::XMFLOAT3 mSurfaceColor;
    float unused0;
    DirectX::XMFLOAT3 mDeepColor;
    float unused1;
};

struct SkyboxConstantBuffer {
    DirectX::XMFLOAT4X4 mViewProjection;
};

class Asteroids {
public:
    Asteroids(const Settings &settings, AsteroidsSimulation* asteroids, GUI* gui, HWND hWnd, Diligent::DeviceType DevType);
    ~Asteroids();

    void Render(float frameTime, const OrbitCamera& camera, const Settings& settings);

    void ResizeSwapChain(HWND outputWindow, unsigned int width, unsigned int height);

    void GetPerfCounters(float &UpdateTime, float &RenderTime);

private:
    void CreateMeshes();
    void InitializeTextureData();
    void CreateGUIResources();
    void RenderSubset(Diligent::Uint32 SubsetNum, Diligent::IDeviceContext *pCtx, const OrbitCamera& camera, Diligent::Uint32 startIdx, Diligent::Uint32 numAsteroids);
    void InitDevice(HWND hWnd, Diligent::DeviceType DevType);

    enum class BindingMode
    {
        Dynamic = 0,
        Mutable = 1,
        TextureMutable = 2
    }m_BindingMode = BindingMode::TextureMutable;

    AsteroidsSimulation*        mAsteroids = nullptr;
    GUI*                        mGUI = nullptr;

    Diligent::RefCntAutoPtr<Diligent::ISwapChain> mSwapChain;
    Diligent::RefCntAutoPtr<Diligent::IRenderDevice>  mDevice;
    Diligent::RefCntAutoPtr<Diligent::IDeviceContext>  mDeviceCtxt;
    std::vector< Diligent::RefCntAutoPtr<Diligent::IDeviceContext> > mDeferredCtxt;
    std::vector< Diligent::RefCntAutoPtr<Diligent::ICommandList> > mCmdLists;
    
    Diligent::Uint32 mBackBufferWidth, mBackBufferHeight;
    Diligent::Uint32 mNumSubsets = 0;
    std::vector<std::thread> mWorkerThreads;
    
    std::mutex mMutex;
    std::condition_variable mCondVar;
    ThreadingTools::Signal mUpdateSubsetsSignal;
    ThreadingTools::Signal mRenderSubsetsSignal;
    std::atomic_int m_NumThreadsCompleted;
    static void WorkerThreadFunc(Asteroids *pThis, Diligent::Uint32 ThreadNum);

    struct FrameAttribs
    {
        float frameTime;
        const OrbitCamera* camera;
        const Settings* settings;
    }mFrameAttribs;

    Diligent::RefCntAutoPtr<Diligent::IBuffer>  mIndexBuffer;
    Diligent::RefCntAutoPtr<Diligent::IBuffer>  mVertexBuffer;
    Diligent::RefCntAutoPtr<Diligent::IBuffer>  mDrawConstantBuffer;
    Diligent::RefCntAutoPtr<Diligent::IBuffer>  mSpriteVertexBuffer;
    Diligent::RefCntAutoPtr<Diligent::IBuffer>  mSkyboxConstantBuffer;
    Diligent::RefCntAutoPtr<Diligent::IBuffer>  mSkyboxVertexBuffer;

    Diligent::RefCntAutoPtr<Diligent::IPipelineState>  mAsteroidsPSO;
    std::vector< Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> > mAsteroidsSRBs;
    
    Diligent::RefCntAutoPtr<Diligent::IPipelineState>  mFontPSO;
    Diligent::RefCntAutoPtr<Diligent::IPipelineState>  mSpritePSO;
    Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> mSpriteSRB;

    Diligent::RefCntAutoPtr<Diligent::IPipelineState>  mSkyboxPSO;

    std::map<std::string, Diligent::RefCntAutoPtr<Diligent::ITextureView>> mSpriteTextures;
    Diligent::RefCntAutoPtr<Diligent::ITextureView> mSkyboxSRV;
    Diligent::RefCntAutoPtr<Diligent::ITextureView> mFontTextureSRV;
    Diligent::RefCntAutoPtr<Diligent::ITexture> mTextures[NUM_UNIQUE_TEXTURES];
    Diligent::RefCntAutoPtr<Diligent::ITextureView> mTextureSRVs[NUM_UNIQUE_TEXTURES];
    Diligent::RefCntAutoPtr<Diligent::ISampler> mSamplerState;

    std::unique_ptr<GUISprite> mSprite;
    UINT64 mPerfCounterFreq = 0;
    volatile LONG64 mUpdateTicks = 0, mRenderTicks = 0;
};

} // namespace AsteroidsD3D11
