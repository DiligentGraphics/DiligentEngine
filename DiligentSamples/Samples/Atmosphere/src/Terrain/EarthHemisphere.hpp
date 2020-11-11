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

// This file is derived from the open source project provided by Intel Corportaion that
// requires the following notice to be kept:
//--------------------------------------------------------------------------------------
// Copyright 2013 Intel Corporation
// All Rights Reserved
//
// Permission is granted to use, copy, distribute and prepare derivative works of this
// software for any purpose and without fee, provided, that the above copyright notice
// and this statement appear in all copies.  Intel makes no representations about the
// suitability of this software for any purpose.  THIS SOFTWARE IS PROVIDED "AS IS."
// INTEL SPECIFICALLY DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, AND ALL LIABILITY,
// INCLUDING CONSEQUENTIAL AND OTHER INDIRECT DAMAGES, FOR THE USE OF THIS SOFTWARE,
// INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PROPRIETARY RIGHTS, AND INCLUDING THE
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  Intel does not
// assume any responsibility for any errors which may appear in this software nor any
// responsibility to update it.
//--------------------------------------------------------------------------------------

#pragma once

#include <vector>

#include "RenderDevice.h"
#include "DeviceContext.h"
#include "Buffer.h"
#include "Texture.h"
#include "BufferView.h"
#include "TextureView.h"
#include "GraphicsTypes.h"
#include "RefCntAutoPtr.hpp"

#include "AdvancedMath.hpp"

namespace Diligent
{

// Include structures in Diligent namespace
#include "../../assets/shaders/HostSharedTerrainStructs.fxh"

// Structure describing terrain rendering parameters
struct RenderingParams
{
    TerrainAttribs m_TerrainAttribs;

    enum TEXTURING_MODE
    {
        TM_HEIGHT_BASED     = 0,
        TM_MATERIAL_MASK    = 1,
        TM_MATERIAL_MASK_NM = 2
    };

    // Patch rendering params
    TEXTURING_MODE m_TexturingMode  = TM_MATERIAL_MASK_NM;
    int            m_iRingDimension = 65;
    int            m_iNumRings      = 15;

    int            m_iNumShadowCascades         = 6;
    int            m_bBestCascadeSearch         = 1;
    int            m_FixedShadowFilterSize      = 5;
    bool           m_FilterAcrossShadowCascades = true;
    int            m_iColOffset                 = 1356;
    int            m_iRowOffset                 = 924;
    TEXTURE_FORMAT DstRTVFormat                 = TEX_FORMAT_R11G11B10_FLOAT;
    TEXTURE_FORMAT ShadowMapFormat              = TEX_FORMAT_D32_FLOAT;
};

struct RingSectorMesh
{
    RefCntAutoPtr<IBuffer> pIndBuff;
    Uint32                 uiNumIndices;
    BoundBox               BndBox;
    RingSectorMesh() :
        uiNumIndices(0) {}
};

// This class renders the adaptive model using DX11 API
class EarthHemsiphere
{
public:
    EarthHemsiphere(void) :
        m_ValidShaders(0) {}

    // clang-format off
    EarthHemsiphere             (const EarthHemsiphere&) = delete;
    EarthHemsiphere& operator = (const EarthHemsiphere&) = delete;
    EarthHemsiphere             (EarthHemsiphere&&)      = delete;
    EarthHemsiphere& operator = (EarthHemsiphere&&)      = delete;
    // clang-format on

    // Renders the model
    void Render(IDeviceContext*        pContext,
                const RenderingParams& NewParams,
                const float3&          vCameraPosition,
                const float4x4&        CameraViewProjMatrix,
                ITextureView*          pShadowMapSRV,
                ITextureView*          pPrecomputedNetDensitySRV,
                ITextureView*          pAmbientSkylightSRV,
                bool                   bZOnlyPass);

    // Creates device resources
    void Create(class ElevationDataSource* pDataSource,
                const RenderingParams&     Params,
                IRenderDevice*             pDevice,
                IDeviceContext*            pContext,
                const char*                MaterialMaskPath,
                const char*                TileTexturePath[],
                const char*                TileNormalMapPath[],
                IBuffer*                   pcbCameraAttribs,
                IBuffer*                   pcbLightAttribs,
                IBuffer*                   pcMediaScatteringParams);

    enum
    {
        NUM_TILE_TEXTURES = 1 + 4
    }; // One base material + 4 masked materials

private:
    void RenderNormalMap(IRenderDevice*  pd3dDevice,
                         IDeviceContext* pd3dImmediateContext,
                         const Uint16*   pHeightMap,
                         size_t          HeightMapPitch,
                         int             HeightMapDim,
                         ITexture*       ptex2DNormalMap);

    RenderingParams m_Params;

    RefCntAutoPtr<IRenderDevice> m_pDevice;

    RefCntAutoPtr<IBuffer>      m_pcbTerrainAttribs;
    RefCntAutoPtr<IBuffer>      m_pVertBuff;
    RefCntAutoPtr<ITextureView> m_ptex2DNormalMapSRV, m_ptex2DMtrlMaskSRV;

    RefCntAutoPtr<ITextureView> m_ptex2DTilesSRV[NUM_TILE_TEXTURES];
    RefCntAutoPtr<ITextureView> m_ptex2DTilNormalMapsSRV[NUM_TILE_TEXTURES];

    RefCntAutoPtr<IResourceMapping> m_pResMapping;
    RefCntAutoPtr<IShader>          m_pHemisphereVS;

    RefCntAutoPtr<IPipelineState>         m_pHemisphereZOnlyPSO;
    RefCntAutoPtr<IShaderResourceBinding> m_pHemisphereZOnlySRB;
    RefCntAutoPtr<IPipelineState>         m_pHemispherePSO;
    RefCntAutoPtr<IShaderResourceBinding> m_pHemisphereSRB;
    RefCntAutoPtr<ISampler>               m_pComparisonSampler;

    std::vector<RingSectorMesh> m_SphereMeshes;

    Uint32 m_ValidShaders;
};

} // namespace Diligent
