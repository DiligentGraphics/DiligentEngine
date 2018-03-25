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

// EngineSandbox.cpp : Defines the entry point for the application.
//

#include "pch.h"
#include "TestDepthStencilState.h"
#include "ConvenienceFunctions.h"

using namespace Diligent;

void TestDepthStencilState::CreateTestDSS( DepthStencilStateDesc &DSSDesc )
{
    RefCntAutoPtr<IPipelineState> pPSO;
    m_pDevice->CreatePipelineState( m_PSODesc, &pPSO );
    m_pDeviceContext->SetPipelineState( pPSO );
#if 0
    DSSDesc.Name = "TestDSS2";
    m_pDevice->CreateDepthStencilState( DSSDesc, &pDSState2 );
    assert( pDSState == pDSState2 );
    m_pDeviceContext->SetDepthStencilState( pDSState );
    m_pDeviceContext->SetDepthStencilState( pDSState2 );
    m_pDeviceContext->SetDepthStencilState( pDSState2, 2 );
#endif
}

TestDepthStencilState::TestDepthStencilState( IRenderDevice *pDevice, IDeviceContext *pContext ) :
    TestPipelineStateBase(pDevice, "Depth-stencil state initialization test"),
    m_pDeviceContext(pContext)
{
    DepthStencilStateDesc &DSSDesc = m_PSODesc.GraphicsPipeline.DepthStencilDesc;

    DSSDesc.DepthEnable = False;
    DSSDesc.DepthWriteEnable = False;
    CreateTestDSS( DSSDesc );

    DSSDesc.DepthEnable = True;
    CreateTestDSS( DSSDesc );

    DSSDesc.DepthWriteEnable = True;
    CreateTestDSS( DSSDesc );

    for( auto CmpFunc = COMPARISON_FUNC_UNKNOWN + 1; CmpFunc < COMPARISON_FUNC_NUM_FUNCTIONS; ++CmpFunc )
    {
        DSSDesc.DepthFunc = static_cast<COMPARISON_FUNCTION>(CmpFunc);
        CreateTestDSS( DSSDesc );
    }

    DSSDesc.StencilEnable = True;
    CreateTestDSS( DSSDesc );

    DSSDesc.StencilReadMask = 0xA9;
    CreateTestDSS( DSSDesc );

    DSSDesc.StencilWriteMask = 0xB8;
    CreateTestDSS( DSSDesc );

    for( int Face = 0; Face < 2; ++Face )
    {
        auto &FaceOp = Face == 0 ? DSSDesc.FrontFace : DSSDesc.BackFace;
        for( auto StOp = STENCIL_OP_UNDEFINED + 1; StOp < STENCIL_OP_NUM_OPS; ++StOp )
        {
            FaceOp.StencilFailOp = static_cast<STENCIL_OP>(StOp);
            CreateTestDSS( DSSDesc );
        }

        for( auto StOp = STENCIL_OP_UNDEFINED + 1; StOp < STENCIL_OP_NUM_OPS; ++StOp )
        {
            FaceOp.StencilDepthFailOp = static_cast<STENCIL_OP>(StOp);
            CreateTestDSS( DSSDesc );
        }

        for( auto StOp = STENCIL_OP_UNDEFINED + 1; StOp < STENCIL_OP_NUM_OPS; ++StOp )
        {
            FaceOp.StencilPassOp = static_cast<STENCIL_OP>(StOp);
            CreateTestDSS( DSSDesc );
        }

        for( auto CmpFunc = COMPARISON_FUNC_UNKNOWN + 1; CmpFunc < COMPARISON_FUNC_NUM_FUNCTIONS; ++CmpFunc )
        {
            FaceOp.StencilFunc = static_cast<COMPARISON_FUNCTION>(CmpFunc);
            CreateTestDSS( DSSDesc );
        }
    }

    auto pScript = CreateRenderScriptFromFile( "DepthStencilStateTest.lua", pDevice, pContext, [&]( Diligent::ScriptParser *pScriptParser )
    {
        DSSDesc = DepthStencilStateDesc();
        DSSDesc.DepthEnable = True;
        DSSDesc.DepthWriteEnable = True;
        DSSDesc.DepthFunc = COMPARISON_FUNC_NEVER;
        DSSDesc.StencilEnable = True;
        DSSDesc.StencilReadMask = 0xFA;
        DSSDesc.StencilWriteMask = 0xFF;

        DSSDesc.FrontFace.StencilFailOp = STENCIL_OP_DECR_SAT;
        DSSDesc.FrontFace.StencilDepthFailOp = STENCIL_OP_DECR_WRAP;
        DSSDesc.FrontFace.StencilPassOp = STENCIL_OP_INCR_WRAP;
        DSSDesc.FrontFace.StencilFunc = COMPARISON_FUNC_NOT_EQUAL;

        DSSDesc.BackFace.StencilFailOp = STENCIL_OP_INVERT;
        DSSDesc.BackFace.StencilDepthFailOp = STENCIL_OP_REPLACE;
        DSSDesc.BackFace.StencilPassOp = STENCIL_OP_INCR_SAT;
        DSSDesc.BackFace.StencilFunc = COMPARISON_FUNC_EQUAL;
        RefCntAutoPtr<IPipelineState> pPSO;
        pDevice->CreatePipelineState( m_PSODesc, &pPSO );
        pScriptParser->SetGlobalVariable( "TestGlobalPSO", pPSO );
    }
    );

    {
        RefCntAutoPtr<IPipelineState> pPSOFromScript, pDSStateFromScript2;
        pScript->GetPipelineStateByName( "TestPSO", &pPSOFromScript);
        //pScript->GetDepthStencilStateByName( "TestDSS2", &pDSStateFromScript2 );
        //assert( pDSStateFromScript == pDSStateFromScript2 );
        const auto &DSSDesc = pPSOFromScript->GetDesc().GraphicsPipeline.DepthStencilDesc;

        assert( DSSDesc.DepthEnable == True );
        assert( DSSDesc.DepthWriteEnable == True );
        assert( DSSDesc.DepthFunc == COMPARISON_FUNC_LESS );
        assert( DSSDesc.StencilEnable == True );
        assert( DSSDesc.StencilReadMask == 0xF8 );
        assert( DSSDesc.StencilWriteMask == 0xF1 );
        assert( DSSDesc.FrontFace.StencilFailOp == STENCIL_OP_KEEP );
        assert( DSSDesc.FrontFace.StencilDepthFailOp == STENCIL_OP_ZERO );
        assert( DSSDesc.FrontFace.StencilPassOp == STENCIL_OP_REPLACE );
        assert( DSSDesc.FrontFace.StencilFunc == COMPARISON_FUNC_EQUAL );
        assert( DSSDesc.BackFace.StencilFailOp == STENCIL_OP_INCR_SAT );
        assert( DSSDesc.BackFace.StencilDepthFailOp == STENCIL_OP_DECR_SAT );
        assert( DSSDesc.BackFace.StencilPassOp == STENCIL_OP_INVERT );
        assert( DSSDesc.BackFace.StencilFunc == COMPARISON_FUNC_NOT_EQUAL );
    }

    {
        m_PSODesc.GraphicsPipeline.DepthStencilDesc = DepthStencilStateDesc();
        RefCntAutoPtr<IPipelineState> pPSO;
        m_pDevice->CreatePipelineState( m_PSODesc, &pPSO );
        pScript->Run( m_pDeviceContext, "TestDSSDesc", pPSO );
    }
    
    SetStatus(TestResult::Succeeded);
}
