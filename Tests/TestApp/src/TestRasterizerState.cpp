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

// EngineSandbox.cpp : Defines the entry point for the application.
//

#include "pch.h"
#include "TestRasterizerState.h"
#include "ConvenienceFunctions.h"

using namespace Diligent;

void TestRasterizerState::CreateTestRS()
{
    RefCntAutoPtr<IPipelineState> pPSO;
    m_PSODesc.Name = "TestRS1";
    m_pDevice->CreatePipelineState( m_PSODesc, &pPSO );
    //RSDesc.Name = "TestRS2";
    //m_pDevice->CreateRasterizerState( RSDesc, &pRS2 );
    //assert( pRS == pRS2 );
    m_pDeviceContext->SetPipelineState( pPSO );
    //m_pDeviceContext->SetRasterizerState( pRS2 );
}

TestRasterizerState::TestRasterizerState( IRenderDevice *pDevice, IDeviceContext *pContext ) :
    TestPipelineStateBase(pDevice, "Rasterizer state initialization test"),
    m_pDeviceContext(pContext)
{
    RasterizerStateDesc &RSDesc = m_PSODesc.GraphicsPipeline.RasterizerDesc;
    CreateTestRS( );

    for( auto FillMode = FILL_MODE_UNDEFINED + 1; FillMode < FILL_MODE_NUM_MODES; ++FillMode )
    {
        RSDesc.FillMode = static_cast<FILL_MODE>(FillMode);
        CreateTestRS();
    }

    for( auto CullMode = CULL_MODE_UNDEFINED + 1; CullMode < CULL_MODE_NUM_MODES; ++CullMode )
    {
        RSDesc.CullMode = static_cast<CULL_MODE>(CullMode);
        CreateTestRS();
    }

    RSDesc.FrontCounterClockwise = !RSDesc.FrontCounterClockwise;
    CreateTestRS();

    RSDesc.DepthBias = 100;
    CreateTestRS();

    RSDesc.DepthBiasClamp = 1.f;
    CreateTestRS();

    RSDesc.SlopeScaledDepthBias = 2.f;
    CreateTestRS();

    RSDesc.DepthClipEnable = !RSDesc.DepthClipEnable;
    CreateTestRS();

#if 0
    RSDesc.ScissorEnable = !RSDesc.ScissorEnable;
    CreateTestRS( RSDesc );
#endif

    RSDesc.AntialiasedLineEnable = !RSDesc.AntialiasedLineEnable;
    CreateTestRS();

    auto pScript = CreateRenderScriptFromFile( "RasterizerStateTest.lua", pDevice, pContext, [&]( Diligent::ScriptParser *pScriptParser )
    {
        m_PSODesc.Name = "PSO-TestRS";
        RSDesc = RasterizerStateDesc();
        RSDesc.FillMode = FILL_MODE_WIREFRAME;
        RSDesc.CullMode = CULL_MODE_FRONT;
        RSDesc.FrontCounterClockwise = True;
        RSDesc.DepthBias = 64;
        RSDesc.DepthBiasClamp = 98.f;
        RSDesc.SlopeScaledDepthBias = 12.5f;
        RSDesc.DepthClipEnable = False;
        RSDesc.ScissorEnable = False;
        RSDesc.AntialiasedLineEnable = True;

        RefCntAutoPtr<IPipelineState> pPSO;
        pDevice->CreatePipelineState( m_PSODesc, &pPSO);
        pScriptParser->SetGlobalVariable( "TestGlobalPSO", pPSO );
    }
    );

    {
        RefCntAutoPtr<IPipelineState> pPSOFromScript, pRSFromScript2;
        pScript->GetPipelineStateByName( "TestRS_PSO", &pPSOFromScript);
        //pScript->GetRasterizerStateByName( "TestRS2", &pRSFromScript2 );
        const auto &PSODesc = pPSOFromScript->GetDesc();

        const auto &RSDesc2 = PSODesc.GraphicsPipeline.RasterizerDesc;

        assert( RSDesc2.FillMode == FILL_MODE_WIREFRAME );
        assert( RSDesc2.CullMode == CULL_MODE_BACK );
        assert( RSDesc2.FrontCounterClockwise == True );
        assert( RSDesc2.DepthBias == 32 );
        assert( RSDesc2.DepthBiasClamp == 63.0 );
        assert( RSDesc2.SlopeScaledDepthBias == 31.25 );
        assert( RSDesc2.DepthClipEnable == True );
        assert( RSDesc2.ScissorEnable == False );
        assert( RSDesc2.AntialiasedLineEnable == True );
    }

    {
        m_PSODesc.GraphicsPipeline.RasterizerDesc = RasterizerStateDesc();
        RefCntAutoPtr<IPipelineState> pPSO;
        m_pDevice->CreatePipelineState(m_PSODesc, &pPSO );
        pScript->Run( m_pDeviceContext, "TestRSDescFunc", pPSO );
    }
    
    SetStatus(TestResult::Succeeded);
}
