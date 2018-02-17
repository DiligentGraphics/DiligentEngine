/*     Copyright 2015-2017 Egor Yusov
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
#include "TestVPAndSR.h"
#include "GraphicsUtilities.h"
#include "ConvenienceFunctions.h"

using namespace Diligent;

TestVPAndSR::TestVPAndSR(IRenderDevice *pDevice, IDeviceContext *pContext ) :
    UnitTestBase("Viewport and scissor rect test")
{
    m_pRenderDevice = pDevice;
    m_pDeviceContext = pContext;
    m_pRenderScript = CreateRenderScriptFromFile( "VPAndSRTest.lua", pDevice, pContext, []( ScriptParser *pScriptParser )
    {
        Viewport VP(16.5, 24.25, 156.125, 381.625, 0.25, 0.75);
        pScriptParser->SetGlobalVariable( "TestGlobalVP", VP );

        Rect SR(10, 30, 200, 300);
        pScriptParser->SetGlobalVariable( "TestGlobalSR", SR );
    } );

    m_pRenderScript->Run( m_pDeviceContext, "SetViewports" );
    m_pRenderScript->Run( m_pDeviceContext, "SetScissorRects" );
    
    SetStatus(TestResult::Succeeded);
}
