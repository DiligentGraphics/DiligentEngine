/*     Copyright 2015-2018 Egor Yusov
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

#pragma once 

#include <vector>

#include "NativeAppBase.h"
#include "RefCntAutoPtr.h"
#include "RenderDevice.h"
#include "DeviceContext.h"
#include "SwapChain.h"

#include "TestDrawCommands.h"
#include "TestBufferAccess.h"
#include "TestTextureCreation.h"
#include "TestSamplerCreation.h"
#include "TestTexturing.h"
#include "ScriptParser.h"
#include "TestComputeShaders.h"
#include "Timer.h"
#include "TestRenderTarget.h"
#include "MTResourceCreationTest.h"
#include "TestShaderResArrays.h"
#include "SmartPointerTest.h"
#include "TestGeometryShader.h"
#include "TestTessellation.h"

class TestApp : public NativeAppBase
{
public:
    TestApp();
    ~TestApp();
    virtual void ProcessCommandLine(const char *CmdLine)override final;
    virtual const char* GetAppTitle()const override final { return m_AppTitle.c_str(); }
    virtual void Update(double CurrTime, double ElapsedTime)override final;
    virtual void WindowResize(int width, int height)override final;
    virtual void Render()override;
    virtual void Present()override;

protected:
    void InitializeDiligentEngine(
#if PLATFORM_LINUX
        void *display,
#endif
        void *NativeWindowHandle
    );
    void InitializeRenderers();

    Diligent::DeviceType m_DeviceType = Diligent::DeviceType::Undefined;
    Diligent::RefCntAutoPtr<Diligent::IRenderDevice> m_pDevice;
    Diligent::RefCntAutoPtr<Diligent::IDeviceContext> m_pImmediateContext;
    std::vector<Diligent::RefCntAutoPtr<Diligent::IDeviceContext> > m_pDeferredContexts;
    Diligent::RefCntAutoPtr<Diligent::ISwapChain> m_pSwapChain;
    std::string m_AppTitle;

    std::unique_ptr<TestDrawCommands> m_pTestDrawCommands;
    std::unique_ptr<TestBufferAccess> m_pTestBufferAccess;
    std::unique_ptr<TestTexturing> m_pTestTexturing[16];
    std::unique_ptr<TestComputeShaders> m_pTestCS;
    std::unique_ptr<TestRenderTarget> m_pTestRT;
    std::unique_ptr<MTResourceCreationTest> m_pMTResCreationTest;
    std::unique_ptr<TestShaderResArrays> m_pTestShaderResArrays;
    TestGeometryShader m_TestGS;
    TestTessellation m_TestTessellation;

    Diligent::RefCntAutoPtr<Diligent::IBuffer> m_pInstBuff, m_pInstBuff2, m_pUniformBuff, m_pUniformBuff2, m_pUniformBuff3, m_pUniformBuff4;
    Diligent::RefCntAutoPtr<Diligent::ITexture> m_pTestTex;
    Diligent::RefCntAutoPtr<Diligent::ScriptParser> m_pRenderScript;
    Diligent::RefCntAutoPtr<Diligent::IFence> m_pFence;
    Diligent::Uint64 m_NextFenceValue = 1;

    SmartPointerTest m_SmartPointerTest;
    double m_CurrTime = 0;
};
