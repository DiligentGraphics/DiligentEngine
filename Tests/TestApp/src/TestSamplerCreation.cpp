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

#include "pch.h"
#include "TestSamplerCreation.h"
#include "RenderDevice.h"

using namespace Diligent;

TestSamplerCreation::TestSamplerCreation(IRenderDevice *pDevice) :
    UnitTestBase("Test sampler creation"),
    m_pDevice(pDevice)
{
    // Test different filters
    for(int Min = 0; Min < 2; ++Min)
        for(int Mag = 0; Mag < 2; ++Mag)
            for(int Mip = 0; Mip < 2; ++Mip)
            {
                SamplerDesc SamplerDesc;
                SamplerDesc.MinFilter = Min ? FILTER_TYPE_LINEAR : FILTER_TYPE_POINT;
                SamplerDesc.MagFilter = Mag ? FILTER_TYPE_LINEAR : FILTER_TYPE_POINT;
                SamplerDesc.MipFilter = Mip ? FILTER_TYPE_LINEAR : FILTER_TYPE_POINT;
                RefCntAutoPtr<ISampler> pSampler;
                m_pDevice->CreateSampler(SamplerDesc, &pSampler);
            }

    {
        SamplerDesc SamplerDesc;
        SamplerDesc.MinFilter = FILTER_TYPE_ANISOTROPIC;
        SamplerDesc.MagFilter = FILTER_TYPE_ANISOTROPIC;
        SamplerDesc.MipFilter = FILTER_TYPE_ANISOTROPIC;
        SamplerDesc.MaxAnisotropy = 4;
        RefCntAutoPtr<ISampler> pSampler;
        m_pDevice->CreateSampler(SamplerDesc, &pSampler);
    }

    for(int Min = 0; Min < 2; ++Min)
    for(int Mag = 0; Mag < 2; ++Mag)
        for(int Mip = 0; Mip < 2; ++Mip)
        {
            SamplerDesc SamplerDesc;
            SamplerDesc.MinFilter = Min ? FILTER_TYPE_COMPARISON_LINEAR : FILTER_TYPE_COMPARISON_POINT;
            SamplerDesc.MagFilter = Mag ? FILTER_TYPE_COMPARISON_LINEAR : FILTER_TYPE_COMPARISON_POINT;
            SamplerDesc.MipFilter = Mip ? FILTER_TYPE_COMPARISON_LINEAR : FILTER_TYPE_COMPARISON_POINT;
            RefCntAutoPtr<ISampler> pSampler;
            m_pDevice->CreateSampler(SamplerDesc, &pSampler);
        }

    {
        SamplerDesc SamplerDesc;
        SamplerDesc.MinFilter = FILTER_TYPE_COMPARISON_ANISOTROPIC;
        SamplerDesc.MagFilter = FILTER_TYPE_COMPARISON_ANISOTROPIC;
        SamplerDesc.MipFilter = FILTER_TYPE_COMPARISON_ANISOTROPIC;
        SamplerDesc.MaxAnisotropy = 4;
        RefCntAutoPtr<ISampler> pSampler;
        m_pDevice->CreateSampler(SamplerDesc, &pSampler);
    }

    // Test address modes
    TEXTURE_ADDRESS_MODE AddrModes[] = 
    {
        TEXTURE_ADDRESS_WRAP,
	    TEXTURE_ADDRESS_MIRROR,
	    TEXTURE_ADDRESS_CLAMP,
	    TEXTURE_ADDRESS_BORDER
	    //TEXTURE_ADDRESS_MIRROR_ONCE // This mode is not supported on Intel HW
    };

    for(int AddrMode = 0; AddrMode < _countof(AddrModes); ++AddrMode)
    {
        SamplerDesc SamplerDesc;
        SamplerDesc.MinFilter = FILTER_TYPE_LINEAR;
        SamplerDesc.MagFilter = FILTER_TYPE_LINEAR;
        SamplerDesc.MipFilter = FILTER_TYPE_LINEAR;
        SamplerDesc.AddressU = SamplerDesc.AddressV = SamplerDesc.AddressW = AddrModes[AddrMode];
        RefCntAutoPtr<ISampler> pSampler;
        m_pDevice->CreateSampler(SamplerDesc, &pSampler);
    }

    // Test comparison funcs
    COMPARISON_FUNCTION CmpFuncs[] = 
    {
        COMPARISON_FUNC_NEVER,
	    COMPARISON_FUNC_LESS,
	    COMPARISON_FUNC_EQUAL,
	    COMPARISON_FUNC_LESS_EQUAL,
	    COMPARISON_FUNC_GREATER,
	    COMPARISON_FUNC_NOT_EQUAL,
	    COMPARISON_FUNC_GREATER_EQUAL,
	    COMPARISON_FUNC_ALWAYS
    };

    for(int CmpFunc=0; CmpFunc < _countof(CmpFuncs); ++CmpFunc)
    {
        SamplerDesc SamplerDesc;
        SamplerDesc.MinFilter = FILTER_TYPE_LINEAR;
        SamplerDesc.MagFilter = FILTER_TYPE_LINEAR;
        SamplerDesc.MipFilter = FILTER_TYPE_LINEAR;
        SamplerDesc.ComparisonFunc = CmpFuncs[CmpFunc];
        RefCntAutoPtr<ISampler> pSampler1, pSampler2;
        SamplerDesc.Name = "Sam1";
        m_pDevice->CreateSampler(SamplerDesc, &pSampler1);
        SamplerDesc.Name = "Sam2";
        m_pDevice->CreateSampler(SamplerDesc, &pSampler2);
        assert(pSampler1 == pSampler2);
    }
    
    SetStatus(TestResult::Succeeded);
}
