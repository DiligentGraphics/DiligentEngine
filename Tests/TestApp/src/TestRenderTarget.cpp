/*     Copyright 2019 Diligent Graphics LLC
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

// EngineSandbox.cpp : Defines the entry point for the application.
//

#include "pch.h"
#include "TestRenderTarget.h"
#include "GraphicsUtilities.h"
#include "ConvenienceFunctions.h"

using namespace Diligent;

TestRenderTarget::TestRenderTarget() :
    UnitTestBase("Render target test")
{
}

void TestRenderTarget::Init( IRenderDevice *pDevice, IDeviceContext *pDeviceContext, ISwapChain* pSwapChain, float fMinXCoord, float fMinYCoord, float fXExtent, float fYExtent )
{
#if PLATFORM_IOS
    SetStatus(TestResult::Skipped);
    return;
#endif
    
    m_pRenderDevice = pDevice;
    m_pDeviceContext = pDeviceContext;
    m_pRenderScript = CreateRenderScriptFromFile( "TestRenderTargets.lua", pDevice, pDeviceContext, [&]( ScriptParser *pScriptParser )
    {
        const auto* BackBufferFmt = pDevice->GetTextureFormatInfo(pSwapChain->GetDesc().ColorBufferFormat).Name;
        const auto* DepthBufferFmt = pDevice->GetTextureFormatInfo(pSwapChain->GetDesc().DepthBufferFormat).Name;
        pScriptParser->SetGlobalVariable( "extBackBufferFormat", BackBufferFmt );
        pScriptParser->SetGlobalVariable( "extDepthBufferFormat", DepthBufferFmt );

        pScriptParser->SetGlobalVariable( "MinX", fMinXCoord );
        pScriptParser->SetGlobalVariable( "MinY", fMinYCoord );
        pScriptParser->SetGlobalVariable( "XExt", fXExtent );
        pScriptParser->SetGlobalVariable( "YExt", fYExtent );
    } );
}

void TestRenderTarget::Draw()
{
    if(!m_pDeviceContext)
        return;
    
    m_pRenderScript->Run( m_pDeviceContext, "Render" );
    SetStatus(TestResult::Succeeded);
}
