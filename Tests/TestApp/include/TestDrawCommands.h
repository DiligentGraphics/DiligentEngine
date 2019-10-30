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

#pragma once

#include "UnitTestBase.h"

class TestDrawCommands : public UnitTestBase
{
public:
    TestDrawCommands() : UnitTestBase("Draw commands test"){}
    
    static const int TriGridSize = 16;

    void Init(Diligent::IRenderDevice *pDevice, Diligent::IDeviceContext *pDeviceContext, Diligent::ISwapChain *pSwapChain, float fMinXCoord, float fMinYCoord, float fXExtent, float fYExtent);   
    void Draw();

private:
    void Define2DVertex(std::vector<float> &VertexData, float fX, float fY, float fR, float fG, float fB);

    Diligent::RefCntAutoPtr<Diligent::IRenderDevice> m_pRenderDevice;
    Diligent::RefCntAutoPtr<Diligent::IDeviceContext> m_pDeviceContext;

    Diligent::RefCntAutoPtr<Diligent::IPipelineState> m_pPSO, m_pPSO_2xStride, m_pPSOInst;
    Diligent::RefCntAutoPtr<Diligent::IBuffer> m_pVertexBuff, m_pVertexBuff2, m_pIndexBuff, m_pInstanceData, m_pIndirectDrawArgs, m_pIndexedIndirectDrawArgs;
    Diligent::RefCntAutoPtr<Diligent::IResourceMapping> m_pResMapping;
    Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> m_pSRB, m_pSRBInst;
};
