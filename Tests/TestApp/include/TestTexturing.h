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

#pragma once

#include "UnitTestBase.h"

class TestTexturing : public UnitTestBase
{
public:
    TestTexturing();

    void Init(Diligent::IRenderDevice *pDevice, Diligent::IDeviceContext *pDeviceContext, Diligent::ISwapChain *pSwapChain, Diligent::TEXTURE_FORMAT TexFormat, float fMinXCoord, float fMinYCoord, float fXExtent, float fYExtent);   
    void Draw();
    static void GenerateTextureData(Diligent::IRenderDevice *pRenderDevice, std::vector<Diligent::Uint8> &Data, std::vector<Diligent::TextureSubResData> &SubResouces, const Diligent::TextureDesc &TexDesc, const float *ColorOffset = nullptr);

private:
    Diligent::RefCntAutoPtr<Diligent::IRenderDevice> m_pRenderDevice;
    Diligent::RefCntAutoPtr<Diligent::IDeviceContext> m_pDeviceContext;

    Diligent::RefCntAutoPtr<Diligent::IPipelineState> m_pPSO;
    Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> m_pSRB;
    Diligent::RefCntAutoPtr<Diligent::IBuffer> m_pVertexBuff;
    Diligent::RefCntAutoPtr<Diligent::ISampler> m_pSampler;
    Diligent::RefCntAutoPtr<Diligent::ITexture> m_pTexture;
    Diligent::RefCntAutoPtr<Diligent::IResourceMapping> m_pResourceMapping;
    const Diligent::Uint32 m_iTestTexWidth;
    const Diligent::Uint32 m_iTestTexHeight;
    const Diligent::Uint32 m_iMipLevels;
    Diligent::TEXTURE_FORMAT m_TextureFormat;
};
