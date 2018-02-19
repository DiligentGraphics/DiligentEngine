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
#include "TestBlendState.h"
#include "ConvenienceFunctions.h"

using namespace Diligent;

void TestBlendState::CreateTestBS( BlendStateDesc &BSDesc )
{
    RefCntAutoPtr<IPipelineState> pPS0;
    m_pDevice->CreatePipelineState( m_PSODesc, &pPS0 );
    m_pDeviceContext->SetPipelineState(pPS0);
#if 0
    BSDesc.Name = "TestBS2";
    m_pDevice->CreateBlendState( BSDesc, &pBS2 );
    assert( pBS == pBS2 );
    float BlendFactors[4] = { 0.8f, 0.2f, 0.3f, 0.6f };
    m_pDeviceContext->SetBlendState( pBS );
    m_pDeviceContext->SetBlendState( pBS2, BlendFactors );
    m_pDeviceContext->SetBlendState( pBS2, BlendFactors, 0x12F5BA3D );
    m_pDeviceContext->SetBlendState( pBS2, BlendFactors, 0x12F5BA3D );
#endif
}

TestBlendState::TestBlendState( IRenderDevice *pDevice, IDeviceContext *pContext ) :
    TestPipelineStateBase(pDevice, "Blend state initialization test"),
    m_pDeviceContext(pContext)
{
    // Dual-source color blending is not supported in GLES3.2, but NVidia's GPUs do support it
    const auto& DevCaps = pDevice->GetDeviceCaps();
    bool TestSRC1 = DevCaps.DevType != DeviceType::OpenGLES;// || DevCaps.Vendor == GPU_VENDOR::NVIDIA;
    std::stringstream statusss;
    statusss << "Dual-source color blending " << (TestSRC1 ? "tested." : "NOT tested.");

    m_PSODesc.Name = "PSO-TestBlendStates";
    BlendStateDesc &BSDesc = m_PSODesc.GraphicsPipeline.BlendDesc;
    BSDesc.RenderTargets[0].BlendEnable = True;
    CreateTestBS( BSDesc );

    BSDesc.AlphaToCoverageEnable = !BSDesc.AlphaToCoverageEnable;
    CreateTestBS( BSDesc );

    BSDesc.IndependentBlendEnable = !BSDesc.IndependentBlendEnable;
    CreateTestBS( BSDesc );

    BSDesc.AlphaToCoverageEnable = False;
    BSDesc.IndependentBlendEnable = True;
    
    const BLEND_FACTOR AlphaBlendFactors[] =
    {
        BLEND_FACTOR_ZERO,
        BLEND_FACTOR_ONE,
        BLEND_FACTOR_SRC_ALPHA,
        BLEND_FACTOR_INV_SRC_ALPHA,
        BLEND_FACTOR_DEST_ALPHA,
        BLEND_FACTOR_INV_DEST_ALPHA,
        BLEND_FACTOR_SRC_ALPHA_SAT,
        BLEND_FACTOR_BLEND_FACTOR,
        BLEND_FACTOR_INV_BLEND_FACTOR,
        BLEND_FACTOR_SRC1_ALPHA,
        BLEND_FACTOR_INV_SRC1_ALPHA,
    };

    // GLES is required to support 4 render targets
    int NumRenderTargetsToTest = DevCaps.DevType == DeviceType::OpenGLES ? 4 : 8;
    statusss << " Num render targets tested: " << NumRenderTargetsToTest;
    for( int i = 0; i < NumRenderTargetsToTest; ++i )
    {
        auto &RT = BSDesc.RenderTargets[i];
        RT.BlendEnable = True;
        for( auto bf = BLEND_FACTOR_UNDEFINED + 1; bf < BLEND_FACTOR_NUM_FACTORS; ++bf )
        {
            if( (!TestSRC1 || i > 0) && 
                (bf == BLEND_FACTOR_SRC1_COLOR || 
                 bf == BLEND_FACTOR_INV_SRC1_COLOR ||
                 bf == BLEND_FACTOR_SRC1_ALPHA || 
                 bf == BLEND_FACTOR_INV_SRC1_ALPHA) )
                continue;
            RT.SrcBlend = static_cast<BLEND_FACTOR>( bf );
            CreateTestBS( BSDesc );
        }

        for( auto bf = BLEND_FACTOR_UNDEFINED + 1; bf < BLEND_FACTOR_NUM_FACTORS; ++bf )
        {
            if( (!TestSRC1 || i > 0) && 
                (bf == BLEND_FACTOR_SRC1_COLOR || 
                 bf == BLEND_FACTOR_INV_SRC1_COLOR ||
                 bf == BLEND_FACTOR_SRC1_ALPHA || 
                 bf == BLEND_FACTOR_INV_SRC1_ALPHA) )
                continue;

            RT.DestBlend = static_cast<BLEND_FACTOR>( bf );
            CreateTestBS( BSDesc );
        }

        RT.SrcBlend  = BLEND_FACTOR_SRC_COLOR;
        RT.DestBlend = BLEND_FACTOR_INV_SRC_COLOR;
        RT.SrcBlendAlpha  = BLEND_FACTOR_SRC_ALPHA;
        RT.DestBlendAlpha = BLEND_FACTOR_INV_SRC_ALPHA;
        for( auto bo = BLEND_OPERATION_UNDEFINED + 1; bo < BLEND_OPERATION_NUM_OPERATIONS; ++bo )
        {
            RT.BlendOp = static_cast<BLEND_OPERATION>( bo );
            CreateTestBS( BSDesc );
        }

        for( auto bf = 0; bf < _countof(AlphaBlendFactors); ++bf )
        {
            auto AlphaBlend = AlphaBlendFactors[bf];
            if( (!TestSRC1 || i > 0) && (AlphaBlend == BLEND_FACTOR_SRC1_ALPHA || AlphaBlend == BLEND_FACTOR_INV_SRC1_ALPHA) )
                continue;

            RT.SrcBlendAlpha = AlphaBlend;
            CreateTestBS( BSDesc );
        }

        for( auto bf = 0; bf < _countof(AlphaBlendFactors); ++bf )
        {
            auto AlphaBlend = AlphaBlendFactors[bf];
            if( (!TestSRC1 || i > 0) && (AlphaBlend == BLEND_FACTOR_SRC1_ALPHA || AlphaBlend == BLEND_FACTOR_INV_SRC1_ALPHA) )
                continue;

            RT.DestBlendAlpha = AlphaBlend;
            CreateTestBS( BSDesc );
        }

        RT.SrcBlend  = BLEND_FACTOR_SRC_COLOR;
        RT.DestBlend = BLEND_FACTOR_INV_SRC_COLOR;
        RT.SrcBlendAlpha  = BLEND_FACTOR_SRC_ALPHA;
        RT.DestBlendAlpha = BLEND_FACTOR_INV_SRC_ALPHA;
        for( auto bo = BLEND_OPERATION_UNDEFINED + 1; bo < BLEND_OPERATION_NUM_OPERATIONS; ++bo )
        {
            RT.BlendOpAlpha = static_cast<BLEND_OPERATION>( bo );
            CreateTestBS( BSDesc );
        }

        RT.RenderTargetWriteMask = COLOR_MASK_BLUE;
        CreateTestBS( BSDesc );

        RT.RenderTargetWriteMask |= COLOR_MASK_RED;
        CreateTestBS( BSDesc );

        RT.RenderTargetWriteMask |= COLOR_MASK_GREEN;
        CreateTestBS( BSDesc );

        RT.RenderTargetWriteMask |= COLOR_MASK_ALPHA;
        CreateTestBS( BSDesc );
    }

    auto pScript = CreateRenderScriptFromFile( "BlendStateTest.lua", pDevice, pContext, [&]( Diligent::ScriptParser *pScriptParser )
    {
        BSDesc = BlendStateDesc();
        BSDesc.IndependentBlendEnable = True;
        BSDesc.AlphaToCoverageEnable = False;
        BSDesc.RenderTargets[0].BlendEnable = True;
        BSDesc.RenderTargets[0].SrcBlend       = BLEND_FACTOR_ZERO;
        BSDesc.RenderTargets[0].DestBlend      = BLEND_FACTOR_ONE;
        BSDesc.RenderTargets[0].BlendOp        = BLEND_OPERATION_ADD;
        BSDesc.RenderTargets[0].SrcBlendAlpha  = BLEND_FACTOR_SRC_ALPHA;
        BSDesc.RenderTargets[0].DestBlendAlpha = BLEND_FACTOR_INV_SRC_ALPHA;
        BSDesc.RenderTargets[0].BlendOpAlpha = BLEND_OPERATION_SUBTRACT;
        BSDesc.RenderTargets[0].RenderTargetWriteMask = COLOR_MASK_RED;

        BSDesc.RenderTargets[1].BlendEnable = True;
        BSDesc.RenderTargets[1].SrcBlend       = BLEND_FACTOR_SRC_ALPHA;
        BSDesc.RenderTargets[1].DestBlend      = BLEND_FACTOR_INV_SRC_ALPHA;
        BSDesc.RenderTargets[1].BlendOp        = BLEND_OPERATION_REV_SUBTRACT;
        BSDesc.RenderTargets[1].SrcBlendAlpha  = BLEND_FACTOR_DEST_ALPHA;
        BSDesc.RenderTargets[1].DestBlendAlpha = BLEND_FACTOR_INV_DEST_ALPHA;
        BSDesc.RenderTargets[1].BlendOpAlpha   = BLEND_OPERATION_MIN;
        BSDesc.RenderTargets[1].RenderTargetWriteMask = COLOR_MASK_GREEN;

        BSDesc.RenderTargets[2].BlendEnable = True;
        BSDesc.RenderTargets[2].SrcBlend       = BLEND_FACTOR_DEST_COLOR;
        BSDesc.RenderTargets[2].DestBlend      = BLEND_FACTOR_INV_DEST_COLOR;
        BSDesc.RenderTargets[2].BlendOp        = BLEND_OPERATION_MAX;
        BSDesc.RenderTargets[2].SrcBlendAlpha  = BLEND_FACTOR_SRC_ALPHA_SAT;
        BSDesc.RenderTargets[2].DestBlendAlpha = BLEND_FACTOR_BLEND_FACTOR;
        BSDesc.RenderTargets[2].BlendOpAlpha   = BLEND_OPERATION_ADD;
        BSDesc.RenderTargets[2].RenderTargetWriteMask = COLOR_MASK_BLUE;

        BSDesc.RenderTargets[3].BlendEnable = True;
        BSDesc.RenderTargets[3].SrcBlend       = BLEND_FACTOR_INV_BLEND_FACTOR;
        BSDesc.RenderTargets[3].DestBlend      = BLEND_FACTOR_SRC_COLOR;
        BSDesc.RenderTargets[3].BlendOp        = BLEND_OPERATION_MAX;
        BSDesc.RenderTargets[3].SrcBlendAlpha  = BLEND_FACTOR_INV_SRC_ALPHA;
        BSDesc.RenderTargets[3].DestBlendAlpha = BLEND_FACTOR_SRC_ALPHA;
        BSDesc.RenderTargets[3].BlendOpAlpha   = BLEND_OPERATION_ADD;
        BSDesc.RenderTargets[3].RenderTargetWriteMask = COLOR_MASK_ALPHA;

        RefCntAutoPtr<IPipelineState> pPSO;
        pDevice->CreatePipelineState( m_PSODesc, &pPSO);
        pScriptParser->SetGlobalVariable( "TestGlobalPSO", pPSO );
    }
    );
    {
        RefCntAutoPtr<IPipelineState> pPSOFromScript, pPSOFromScript2;
        pScript->GetPipelineStateByName( "PSO_TestBlendState", &pPSOFromScript );
        //pScript->GetPipelineStateByName( "TestPSO2", &pPSOFromScript2 );
        //assert( pPSOFromScript == pBSFromScript2 );
        const auto &PSODesc = pPSOFromScript->GetDesc(); 
        const auto &BSDesc = PSODesc.GraphicsPipeline.BlendDesc;

        assert( strcmp(PSODesc.Name, "TestPSO_FromScript") == 0 );
        assert( BSDesc.IndependentBlendEnable == true );
        assert( BSDesc.AlphaToCoverageEnable == false );
        const auto &RT1 = BSDesc.RenderTargets[0];
        assert( RT1.BlendEnable == true); 
        assert( RT1.SrcBlend == BLEND_FACTOR_ZERO); 
        assert( RT1.DestBlend == BLEND_FACTOR_SRC_COLOR); 
        assert( RT1.BlendOp == BLEND_OPERATION_ADD);
        assert( RT1.SrcBlendAlpha == BLEND_FACTOR_SRC_ALPHA); 
        assert( RT1.DestBlendAlpha == BLEND_FACTOR_INV_SRC_ALPHA); 
        assert( RT1.BlendOpAlpha == BLEND_OPERATION_SUBTRACT);
        assert( RT1.RenderTargetWriteMask == (COLOR_MASK_GREEN | COLOR_MASK_RED) );

        const auto& RT3 = BSDesc.RenderTargets[2];
        assert(RT3.BlendEnable == true); 
        assert(RT3.SrcBlend == BLEND_FACTOR_INV_DEST_ALPHA); 
        assert(RT3.DestBlend == BLEND_FACTOR_INV_DEST_COLOR); 
        assert(RT3.BlendOp == BLEND_OPERATION_ADD);
        assert(RT3.SrcBlendAlpha == BLEND_FACTOR_BLEND_FACTOR); 
        assert(RT3.DestBlendAlpha == BLEND_FACTOR_INV_SRC_ALPHA); 
        assert(RT3.BlendOpAlpha == BLEND_OPERATION_SUBTRACT);
        assert(RT3.RenderTargetWriteMask == (COLOR_MASK_BLUE | COLOR_MASK_ALPHA) );
    }

    {
        BSDesc = BlendStateDesc();
        RefCntAutoPtr<IPipelineState> pPSO;
        m_pDevice->CreatePipelineState(m_PSODesc, &pPSO );
        pScript->Run( m_pDeviceContext, "TestPSOArg", pPSO );
    }
    SetStatus(TestResult::Succeeded, statusss.str().c_str());
}
