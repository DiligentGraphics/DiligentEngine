/*
 *  Copyright 2019-2020 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *  
 *      http://www.apache.org/licenses/LICENSE-2.0
 *  
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
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

#include <array>
#include <random>
#include "SampleBase.hpp"
#include "BasicMath.hpp"

namespace Diligent
{

class Tutorial11_ResourceUpdates final : public SampleBase
{
public:
    virtual void Initialize(const SampleInitInfo& InitInfo) override final;

    virtual void Render() override final;
    virtual void Update(double CurrTime, double ElapsedTime) override final;

    virtual const Char* GetSampleName() const override final { return "Tutorial11: Resource Updates"; }

private:
    void CreatePipelineStates();
    void CreateVertexBuffers();
    void CreateIndexBuffer();
    void LoadTextures();

    void WriteStripPattern(Uint8*, Uint32 Width, Uint32 Height, Uint32 Stride);
    void WriteDiamondPattern(Uint8*, Uint32 Width, Uint32 Height, Uint32 Stride);

    void UpdateTexture(Uint32 TexIndex);
    void MapTexture(Uint32 TexIndex, bool MapEntireTexture);
    void UpdateBuffer(Uint32 BufferIndex);
    void MapDynamicBuffer(Uint32 BufferIndex);

    RefCntAutoPtr<IPipelineState> m_pPSO, m_pPSO_NoCull;
    RefCntAutoPtr<IBuffer>        m_CubeVertexBuffer[3];
    RefCntAutoPtr<IBuffer>        m_CubeIndexBuffer;
    RefCntAutoPtr<IBuffer>        m_VSConstants;
    RefCntAutoPtr<IBuffer>        m_TextureUpdateBuffer;

    void DrawCube(const float4x4& WVPMatrix, IBuffer* pVertexBuffer, IShaderResourceBinding* pSRB);

    static constexpr const size_t NumTextures         = 4;
    static constexpr const Uint32 MaxUpdateRegionSize = 128;
    static constexpr const Uint32 MaxMapRegionSize    = 128;

    std::array<RefCntAutoPtr<ITexture>, NumTextures>               m_Textures;
    std::array<RefCntAutoPtr<IShaderResourceBinding>, NumTextures> m_SRBs;

    double       m_LastTextureUpdateTime = 0;
    double       m_LastBufferUpdateTime  = 0;
    double       m_LastMapTime           = 0;
    std::mt19937 m_gen{0}; //Use 0 as the seed to always generate the same sequence
    double       m_CurrTime;
};

} // namespace Diligent
