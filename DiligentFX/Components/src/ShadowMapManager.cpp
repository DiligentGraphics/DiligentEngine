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

#include <cfloat>

#include "ShadowMapManager.hpp"
#include "AdvancedMath.hpp"
#include "../../../Utilities/include/DiligentFXShaderSourceStreamFactory.hpp"
#include "GraphicsUtilities.h"
#include "MapHelper.hpp"
#include "CommonlyUsedStates.h"

namespace Diligent
{

ShadowMapManager::ShadowMapManager()
{
}

void ShadowMapManager::Initialize(IRenderDevice* pDevice, const InitInfo& initInfo)
{
    VERIFY_EXPR(pDevice != nullptr);
    VERIFY(initInfo.Format != TEX_FORMAT_UNKNOWN, "Undefined shadow map format");
    VERIFY(initInfo.NumCascades != 0, "Number of cascades must not be zero");
    VERIFY(initInfo.Resolution != 0, "Shadow map resolution must not be zero");
    VERIFY(initInfo.ShadowMode != 0, "Shadow mode is not specified");

    m_pDevice    = pDevice;
    m_ShadowMode = initInfo.ShadowMode;

    TextureDesc ShadowMapDesc;
    ShadowMapDesc.Name      = "Shadow map SRV";
    ShadowMapDesc.Type      = RESOURCE_DIM_TEX_2D_ARRAY;
    ShadowMapDesc.Width     = initInfo.Resolution;
    ShadowMapDesc.Height    = initInfo.Resolution;
    ShadowMapDesc.MipLevels = 1;
    ShadowMapDesc.ArraySize = initInfo.NumCascades;
    ShadowMapDesc.Format    = initInfo.Format;
    ShadowMapDesc.BindFlags = BIND_SHADER_RESOURCE | BIND_DEPTH_STENCIL;

    RefCntAutoPtr<ITexture> ptex2DShadowMap;
    pDevice->CreateTexture(ShadowMapDesc, nullptr, &ptex2DShadowMap);

    m_pShadowMapSRV.Release();
    m_pShadowMapSRV = ptex2DShadowMap->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
    if (initInfo.pComparisonSampler != nullptr)
        m_pShadowMapSRV->SetSampler(initInfo.pComparisonSampler);

    m_pShadowMapDSVs.clear();
    m_pShadowMapDSVs.resize(ShadowMapDesc.ArraySize);
    for (Uint32 iArrSlice = 0; iArrSlice < ShadowMapDesc.ArraySize; iArrSlice++)
    {
        TextureViewDesc ShadowMapDSVDesc;
        ShadowMapDSVDesc.Name            = "Shadow map cascade DSV";
        ShadowMapDSVDesc.ViewType        = TEXTURE_VIEW_DEPTH_STENCIL;
        ShadowMapDSVDesc.FirstArraySlice = iArrSlice;
        ShadowMapDSVDesc.NumArraySlices  = 1;
        ptex2DShadowMap->CreateView(ShadowMapDSVDesc, &m_pShadowMapDSVs[iArrSlice]);
    }

    m_pFilterableShadowMapSRV.Release();
    m_pFilterableShadowMapRTVs.clear();
    m_pIntermediateSRV.Release();
    m_pIntermediateRTV.Release();
    if (initInfo.ShadowMode == SHADOW_MODE_VSM ||
        initInfo.ShadowMode == SHADOW_MODE_EVSM2 ||
        initInfo.ShadowMode == SHADOW_MODE_EVSM4)
    {
        ShadowMapDesc.BindFlags = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET;
        if (initInfo.ShadowMode == SHADOW_MODE_VSM)
            ShadowMapDesc.Format = initInfo.Is32BitFilterableFmt ? TEX_FORMAT_RG32_FLOAT : TEX_FORMAT_RG16_UNORM;
        else if (initInfo.ShadowMode == SHADOW_MODE_EVSM2)
            ShadowMapDesc.Format = initInfo.Is32BitFilterableFmt ? TEX_FORMAT_RG32_FLOAT : TEX_FORMAT_RG16_FLOAT;
        else if (initInfo.ShadowMode == SHADOW_MODE_EVSM4)
            ShadowMapDesc.Format = initInfo.Is32BitFilterableFmt ? TEX_FORMAT_RGBA32_FLOAT : TEX_FORMAT_RGBA16_FLOAT;

        RefCntAutoPtr<ITexture> ptex2DFilterableShadowMap;
        pDevice->CreateTexture(ShadowMapDesc, nullptr, &ptex2DFilterableShadowMap);
        m_pFilterableShadowMapSRV = ptex2DFilterableShadowMap->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);

        m_pFilterableShadowMapRTVs.resize(ShadowMapDesc.ArraySize);
        for (Uint32 iArrSlice = 0; iArrSlice < ShadowMapDesc.ArraySize; ++iArrSlice)
        {
            TextureViewDesc RTVDesc;
            RTVDesc.Name            = "Filterable shadow map cascade RTV";
            RTVDesc.ViewType        = TEXTURE_VIEW_RENDER_TARGET;
            RTVDesc.FirstArraySlice = iArrSlice;
            RTVDesc.NumArraySlices  = 1;
            ptex2DFilterableShadowMap->CreateView(RTVDesc, &m_pFilterableShadowMapRTVs[iArrSlice]);
        }

        ShadowMapDesc.ArraySize = 1;
        RefCntAutoPtr<ITexture> ptex2DIntermediate;
        pDevice->CreateTexture(ShadowMapDesc, nullptr, &ptex2DIntermediate);
        m_pIntermediateSRV = ptex2DIntermediate->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
        m_pIntermediateRTV = ptex2DIntermediate->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);

        if (initInfo.pFilterableShadowMapSampler != nullptr)
            m_pFilterableShadowMapSRV->SetSampler(initInfo.pFilterableShadowMapSampler);

        InitializeConversionTechniques(ShadowMapDesc.Format);

        InitializeResourceBindings();
    }
}

void ShadowMapManager::DistributeCascades(const DistributeCascadeInfo& Info,
                                          ShadowMapAttribs&            ShadowAttribs)
{
    VERIFY(Info.pCameraView, "Camera view matrix must not be null");
    VERIFY(Info.pCameraProj, "Camera projection matrix must not be null");
    VERIFY(Info.pLightDir, "Light direction must not be null");
    VERIFY(m_pDevice, "Shadow map manager is not initialized");

    const auto& DevCaps = m_pDevice->GetDeviceCaps();
    const auto  IsGL    = DevCaps.IsGLDevice();
    const auto& SMDesc  = m_pShadowMapSRV->GetTexture()->GetDesc();

    float2 f2ShadowMapSize = float2(static_cast<float>(SMDesc.Width), static_cast<float>(SMDesc.Height));

    ShadowAttribs.f4ShadowMapDim.x = f2ShadowMapSize.x;
    ShadowAttribs.f4ShadowMapDim.y = f2ShadowMapSize.y;
    ShadowAttribs.f4ShadowMapDim.z = 1.f / f2ShadowMapSize.x;
    ShadowAttribs.f4ShadowMapDim.w = 1.f / f2ShadowMapSize.y;

    if (m_ShadowMode == SHADOW_MODE_VSM || m_ShadowMode == SHADOW_MODE_EVSM2 || m_ShadowMode == SHADOW_MODE_EVSM4)
    {
        VERIFY_EXPR(m_pFilterableShadowMapSRV);
        const auto& FilterableSMDesc = m_pFilterableShadowMapSRV->GetTexture()->GetDesc();
        ShadowAttribs.bIs32BitEVSM   = FilterableSMDesc.Format == TEX_FORMAT_RGBA32_FLOAT || FilterableSMDesc.Format == TEX_FORMAT_RG32_FLOAT;
    }

    float3 LightSpaceX, LightSpaceY, LightSpaceZ;
    LightSpaceZ = *Info.pLightDir;
    VERIFY(length(LightSpaceZ) > 1e-5, "Light direction vector length is zero");
    LightSpaceZ = normalize(LightSpaceZ);

    auto min_cmp = std::min(std::min(std::abs(Info.pLightDir->x), std::abs(Info.pLightDir->y)), std::abs(Info.pLightDir->z));
    if (min_cmp == std::abs(Info.pLightDir->x))
        LightSpaceX = float3(1, 0, 0);
    else if (min_cmp == std::abs(Info.pLightDir->y))
        LightSpaceX = float3(0, 1, 0);
    else
        LightSpaceX = float3(0, 0, 1);

    LightSpaceY = cross(LightSpaceZ, LightSpaceX);
    LightSpaceX = cross(LightSpaceY, LightSpaceZ);
    LightSpaceX = normalize(LightSpaceX);
    LightSpaceY = normalize(LightSpaceY) * (Info.UseRightHandedLightViewTransform ? +1.f : -1.f);

    float4x4 WorldToLightViewSpaceMatr =
        float4x4::ViewFromBasis(LightSpaceX, LightSpaceY, LightSpaceZ);

    ShadowAttribs.mWorldToLightViewT = WorldToLightViewSpaceMatr.Transpose();

    const auto& CameraWorld = Info.pCameraWorld != nullptr ? *Info.pCameraWorld : Info.pCameraView->Inverse();
    //const float3 f3CameraPos = {CameraWorld._41, CameraWorld._42, CameraWorld._43};
    //const float3 f3CameraPosInLightSpace = f3CameraPos * WorldToLightViewSpaceMatr;

    float fMainCamNearPlane, fMainCamFarPlane;
    Info.pCameraProj->GetNearFarClipPlanes(fMainCamNearPlane, fMainCamFarPlane, IsGL);
    if (Info.AdjustCascadeRange)
    {
        Info.AdjustCascadeRange(-1, fMainCamNearPlane, fMainCamFarPlane);
    }

    for (int i = 0; i < MAX_CASCADES; ++i)
        ShadowAttribs.fCascadeCamSpaceZEnd[i] = +FLT_MAX;

    int iNumCascades           = SMDesc.ArraySize;
    ShadowAttribs.iNumCascades = iNumCascades;
    ShadowAttribs.fNumCascades = static_cast<float>(iNumCascades);

    m_CascadeTransforms.resize(iNumCascades);
    for (int iCascade = 0; iCascade < iNumCascades; ++iCascade)
    {
        auto&  CurrCascade   = ShadowAttribs.Cascades[iCascade];
        float  fCascadeNearZ = (iCascade == 0) ? fMainCamNearPlane : ShadowAttribs.fCascadeCamSpaceZEnd[iCascade - 1];
        float& fCascadeFarZ  = ShadowAttribs.fCascadeCamSpaceZEnd[iCascade];
        if (iCascade < iNumCascades - 1)
        {
            float ratio = fMainCamFarPlane / fMainCamNearPlane;
            float power = static_cast<float>(iCascade + 1) / static_cast<float>(iNumCascades);
            float logZ  = fMainCamNearPlane * pow(ratio, power);

            float range    = fMainCamFarPlane - fMainCamNearPlane;
            float uniformZ = fMainCamNearPlane + range * power;

            fCascadeFarZ = Info.fPartitioningFactor * (logZ - uniformZ) + uniformZ;
        }
        else
        {
            fCascadeFarZ = fMainCamFarPlane;
        }

        if (Info.AdjustCascadeRange)
        {
            Info.AdjustCascadeRange(iCascade, fCascadeNearZ, fCascadeFarZ);
        }
        VERIFY(fCascadeNearZ > 0.f, "Near plane distance can't be zero");
        CurrCascade.f4StartEndZ.x = fCascadeNearZ;
        CurrCascade.f4StartEndZ.y = fCascadeFarZ;

        // Set reference minimums and maximums for each coordinate
        float3 f3MinXYZ = float3(+FLT_MAX, +FLT_MAX, +FLT_MAX);
        float3 f3MaxXYZ = float3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

        if (Info.StabilizeExtents)
        {
            // We need to make sure that cascade extents are independent of the camera position and orientation.
            // For that, we compute the minimum bounding sphere of a cascade camera frustum.
            float3 f3MinimalSphereCenter;
            float  fMinimalSphereRadius;
            GetFrustumMinimumBoundingSphere(Info.pCameraProj->_11, Info.pCameraProj->_22, fCascadeNearZ, fCascadeFarZ, f3MinimalSphereCenter, fMinimalSphereRadius);
            auto f3CenterLightSpace = f3MinimalSphereCenter * CameraWorld * WorldToLightViewSpaceMatr;
            f3MinXYZ                = f3CenterLightSpace - float3(fMinimalSphereRadius, fMinimalSphereRadius, fMinimalSphereRadius);
            f3MaxXYZ                = f3CenterLightSpace + float3(fMinimalSphereRadius, fMinimalSphereRadius, fMinimalSphereRadius);
        }
        else
        {
            float4x4 CascadeFrustumProjMatrix = *Info.pCameraProj;
            CascadeFrustumProjMatrix.SetNearFarClipPlanes(fCascadeNearZ, fCascadeFarZ, IsGL);
            float4x4 CascadeFrustumViewProjMatr          = *Info.pCameraView * CascadeFrustumProjMatrix;
            float4x4 CascadeFrustumProjSpaceToWorldSpace = CascadeFrustumViewProjMatr.Inverse();
            float4x4 CascadeFrustumProjSpaceToLightSpace = CascadeFrustumProjSpaceToWorldSpace * WorldToLightViewSpaceMatr;
            for (int i = 0; i < 8; ++i)
            {
                float3 f3FrustumCornerProjSpace //
                    {
                        (i & 0x01) ? +1.f : -1.f,
                        (i & 0x02) ? +1.f : -1.f,
                        (i & 0x04) ? +1.f : (IsGL ? -1.f : 0.f) //
                    };
                float3 f3CornerLightSpace = f3FrustumCornerProjSpace * CascadeFrustumProjSpaceToLightSpace;

                f3MinXYZ = std::min(f3MinXYZ, f3CornerLightSpace);
                f3MaxXYZ = std::max(f3MaxXYZ, f3CornerLightSpace);
            }
        }

        float3 f3CascadeExtent = f3MaxXYZ - f3MinXYZ;
        float3 f3CascadeCenter = (f3MaxXYZ + f3MinXYZ) * 0.5f;
        if (Info.EqualizeExtents)
        {
            f3CascadeExtent.x = f3CascadeExtent.y = std::max(f3CascadeExtent.x, f3CascadeExtent.y);
        }

        float2 f2FixedMargin = (Info.SnapCascades ? float2(0.5f, 0.5f) : float2(0, 0));
        if (m_ShadowMode == SHADOW_MODE_VSM || m_ShadowMode == SHADOW_MODE_EVSM2 || m_ShadowMode == SHADOW_MODE_EVSM4)
        {
            f2FixedMargin.x += static_cast<float>(ShadowAttribs.iMaxAnisotropy) / 2.f;
            f2FixedMargin.y += static_cast<float>(ShadowAttribs.iMaxAnisotropy) / 2.f;
        }

        float2 f2FilterMargin;
        if (ShadowAttribs.iFixedFilterSize > 0)
        {
            f2FilterMargin.x += static_cast<float>(ShadowAttribs.iFixedFilterSize) / 2.f;
            f2FilterMargin.y += static_cast<float>(ShadowAttribs.iFixedFilterSize) / 2.f;
        }
        else
        {
            // Make sure that cascade is big enough so that varying filter is limited by 9x9
            constexpr float MaxVaryingFilterSize   = 9;
            constexpr float MaxVaryingFilterRadius = MaxVaryingFilterSize / 2.f;

            // First, compute non-extended cascade extent for which world-space filter size will result
            // in a 9x9 filter kernel.

            // FilterSize       = FilterWorldSize * LightSpaceScale * NDCtoUVScale
            // FilterRadius     = FilterSize / 2 * ShadowMapSize
            // LightSpaceScale  = 2 / CascadeExtent
            // CascadeExtent    = CascadeExtent0 * ShadowMapSize / (ShadowMapSize - Extension)
            // Extension        = 2 * (FixedMargin + FilterMargin)
            //     |
            //     V
            // FilterRadius     = FilterWorldSize * LightSpaceScale * NDCtoUVScale / 2 * ShadowMapSize
            //     |
            //     V
            // FilterRadius     = FilterWorldSize * (2 / CascadeExtent) * NDCtoUVScale / 2 * ShadowMapSize
            //     |
            //     V
            // FilterRadius     = FilterWorldSize * (2 * (ShadowMapSize - Extension) / (CascadeExtent0 * ShadowMapSize)) * NDCtoUVScale / 2 * ShadowMapSize
            //     |
            //     V
            // FilterRadius     = FilterWorldSize * (ShadowMapSize - Extension) / CascadeExtent0 * NDCtoUVScale
            //     |
            //     V
            // CascadeExtent0   = FilterWorldSize * (ShadowMapSize - Extension) / FilterRadius * NDCtoUVScale
            //
            // FilterRadius <-  MaxVaryingFilterRadius
            // Extension    <- (MaxVaryingFilterRadius + FixedMargin) * 2

            constexpr float NDCtoUVScale       = 0.5f;
            float2          f2MaxExtension     = 2.f * (float2(MaxVaryingFilterRadius, MaxVaryingFilterRadius) + f2FixedMargin);
            float2          f2MinCascadeExtent = ShadowAttribs.fFilterWorldSize * (f2ShadowMapSize - f2MaxExtension) / MaxVaryingFilterRadius * NDCtoUVScale;
            float3          f3NewExtent;
            f3NewExtent.x = std::max(f3CascadeExtent.x, f2MinCascadeExtent.x);
            f3NewExtent.y = std::max(f3CascadeExtent.y, f2MinCascadeExtent.y);
            // Extend Z range proportionally
            f3NewExtent.z   = f3CascadeExtent.z * std::max(f3NewExtent.x / f3CascadeExtent.x, f3NewExtent.y / f3CascadeExtent.y);
            f3CascadeExtent = f3NewExtent;


            // Second, compute filter margin such that filter radius after extension is exactly the same as the margin.

            // FilterRadius   = FilterWorldSize * (ShadowMapSize - 2 * FixedMargin - 2 * FilterMargin) / CascadeExtent0 * NDCtoUVScale
            // K             <- FilterWorldSize / CascadeExtent0 * NDCtoUVScale
            // FilterRadius   = K * (ShadowMapSize - 2 * FixedMargin - 2 * FilterMargin)
            // FilterRadius  <- FilterMargin
            // FilterMargin   = K * (ShadowMapSize - 2 * FixedMargin - 2 * FilterMargin)
            // FilterMargin   = K * (ShadowMapSize - 2 * FixedMargin) / (1 + 2 * K)
            float2 K       = float2(ShadowAttribs.fFilterWorldSize, ShadowAttribs.fFilterWorldSize) / float2(f3CascadeExtent.x, f3CascadeExtent.y) * NDCtoUVScale;
            f2FilterMargin = K * (f2ShadowMapSize - 2.f * f2FixedMargin) / (float2(1, 1) + 2.f * K);
        }

        float2 f2Margin    = f2FixedMargin + f2FilterMargin;
        float2 f2Extension = f2Margin * 2.f;

        // We need to remap the whole extent N x N to (N-ext) x (N-ext)
        VERIFY_EXPR(f2ShadowMapSize.x > f2Extension.x && f2ShadowMapSize.y > f2Extension.y);
        f3CascadeExtent.x *= f2ShadowMapSize.x / (f2ShadowMapSize.x - f2Extension.x);
        f3CascadeExtent.y *= f2ShadowMapSize.y / (f2ShadowMapSize.y - f2Extension.y);

        // Margin is defined in projection space for shader use, thus x2
        CurrCascade.f4MarginProjSpace.x = f2Margin.x * 2.f / f2ShadowMapSize.x;
        CurrCascade.f4MarginProjSpace.y = f2Margin.y * 2.f / f2ShadowMapSize.y;

        // Align cascade center with the shadow map texels to alleviate temporal aliasing
        if (Info.SnapCascades)
        {
            float fTexelXSize = f3CascadeExtent.x / f2ShadowMapSize.x;
            float fTexelYSize = f3CascadeExtent.y / f2ShadowMapSize.y;
            f3CascadeCenter.x = std::round(f3CascadeCenter.x / fTexelXSize) * fTexelXSize;
            f3CascadeCenter.y = std::round(f3CascadeCenter.y / fTexelYSize) * fTexelYSize;
        }

        // Extend cascade Z range to allow room for filtering
        float fZExtension = std::max(f2Margin.x / f2ShadowMapSize.x, f2Margin.y / f2ShadowMapSize.y) * ShadowAttribs.fReceiverPlaneDepthBiasClamp;
        fZExtension       = std::min(fZExtension, 0.25f);

        CurrCascade.f4MarginProjSpace.z = fZExtension * (IsGL ? 2.f : 1.f);
        CurrCascade.f4MarginProjSpace.w = fZExtension * (IsGL ? 2.f : 1.f);
        f3CascadeExtent.z *= 1.f / (1.f - fZExtension * 2.f);

        // Compute new cascade min/max xy coords
        f3MinXYZ = f3CascadeCenter - f3CascadeExtent / 2.f;
        f3MaxXYZ = f3CascadeCenter + f3CascadeExtent / 2.f;

        CurrCascade.f4LightSpaceScale.x = 2.f / f3CascadeExtent.x;
        CurrCascade.f4LightSpaceScale.y = 2.f / f3CascadeExtent.y;
        CurrCascade.f4LightSpaceScale.z = (IsGL ? 2.f : 1.f) / f3CascadeExtent.z;
        // Apply bias to shift the extent to [-1,1]x[-1,1]x[0,1] for DX or to [-1,1]x[-1,1]x[-1,1] for GL
        // Find bias such that f3MinXYZ -> (-1,-1,0) for DX or (-1,-1,-1) for GL
        CurrCascade.f4LightSpaceScaledBias.x = -f3MinXYZ.x * CurrCascade.f4LightSpaceScale.x - 1.f;
        CurrCascade.f4LightSpaceScaledBias.y = -f3MinXYZ.y * CurrCascade.f4LightSpaceScale.y - 1.f;
        CurrCascade.f4LightSpaceScaledBias.z = -f3MinXYZ.z * CurrCascade.f4LightSpaceScale.z + (IsGL ? -1.f : 0.f);

        float4x4 ScaleMatrix      = float4x4::Scale(CurrCascade.f4LightSpaceScale.x, CurrCascade.f4LightSpaceScale.y, CurrCascade.f4LightSpaceScale.z);
        float4x4 ScaledBiasMatrix = float4x4::Translation(CurrCascade.f4LightSpaceScaledBias.x, CurrCascade.f4LightSpaceScaledBias.y, CurrCascade.f4LightSpaceScaledBias.z);

        // Note: bias is applied after scaling!
        float4x4& CascadeProjMatr = m_CascadeTransforms[iCascade].Proj;
        CascadeProjMatr           = ScaleMatrix * ScaledBiasMatrix;

        // Adjust the world to light space transformation matrix
        float4x4& WorldToLightProjSpaceMatr = m_CascadeTransforms[iCascade].WorldToLightProjSpace;
        WorldToLightProjSpaceMatr           = WorldToLightViewSpaceMatr * CascadeProjMatr;

        const auto& NDCAttribs    = DevCaps.GetNDCAttribs();
        float4x4    ProjToUVScale = float4x4::Scale(0.5f, NDCAttribs.YtoVScale, NDCAttribs.ZtoDepthScale);
        float4x4    ProjToUVBias  = float4x4::Translation(0.5f, 0.5f, NDCAttribs.GetZtoDepthBias());

        float4x4 WorldToShadowMapUVDepthMatr              = WorldToLightProjSpaceMatr * ProjToUVScale * ProjToUVBias;
        ShadowAttribs.mWorldToShadowMapUVDepthT[iCascade] = WorldToShadowMapUVDepthMatr.Transpose();
    }
}

void ShadowMapManager::InitializeConversionTechniques(TEXTURE_FORMAT FilterableShadowMapFmt)
{
    if (!m_pConversionAttribsBuffer)
    {
        CreateUniformBuffer(m_pDevice, 64, "Shadow conversion attribs CB", &m_pConversionAttribsBuffer);
    }

    RefCntAutoPtr<IShader> pScreenSizeTriVS;
    for (int mode = SHADOW_MODE_VSM; mode <= SHADOW_MODE_EVSM4; ++mode)
    {
        auto& Tech = m_ConversionTech[mode];
        if (mode == SHADOW_MODE_EVSM4)
        {
            Tech = m_ConversionTech[SHADOW_MODE_EVSM2];
            continue;
        }

        if (Tech.PSO)
        {
            if (Tech.PSO->GetGraphicsPipelineDesc().RTVFormats[0] != FilterableShadowMapFmt)
                Tech = ShadowConversionTechnique();
            else
                continue; // Already up to date
        }

        if (!pScreenSizeTriVS)
        {
            ShaderCreateInfo VertShaderCI;
            VertShaderCI.Desc.ShaderType            = SHADER_TYPE_VERTEX;
            VertShaderCI.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
            VertShaderCI.UseCombinedTextureSamplers = true;
            VertShaderCI.pShaderSourceStreamFactory = &DiligentFXShaderSourceStreamFactory::GetInstance();
            VertShaderCI.FilePath                   = "FullScreenTriangleVS.fx";
            VertShaderCI.EntryPoint                 = "FullScreenTriangleVS";
            VertShaderCI.Desc.Name                  = "FullScreenTriangleVS";
            m_pDevice->CreateShader(VertShaderCI, &pScreenSizeTriVS);
        }

        GraphicsPipelineStateCreateInfo PSOCreateInfo;
        PipelineStateDesc&              PSODesc = PSOCreateInfo.PSODesc;

        ShaderCreateInfo ShaderCI;
        ShaderCI.Desc.ShaderType            = SHADER_TYPE_PIXEL;
        ShaderCI.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
        ShaderCI.UseCombinedTextureSamplers = true;
        ShaderCI.pShaderSourceStreamFactory = &DiligentFXShaderSourceStreamFactory::GetInstance();
        ShaderCI.FilePath                   = "ShadowConversions.fx";
        if (mode == SHADOW_MODE_VSM)
        {
            ShaderCI.EntryPoint = "VSMHorzPS";
            ShaderCI.Desc.Name  = "VSM horizontal pass PS";
            PSODesc.Name        = "VSM horizontal pass";
        }
        else if (mode == SHADOW_MODE_EVSM2)
        {
            ShaderCI.EntryPoint = "EVSMHorzPS";
            ShaderCI.Desc.Name  = "EVSM horizontal pass PS";
            PSODesc.Name        = "EVSM horizontal pass";
        }
        else
        {
            UNEXPECTED("Unexpected shadow mode");
        }
        RefCntAutoPtr<IShader> pVSMHorzPS;
        m_pDevice->CreateShader(ShaderCI, &pVSMHorzPS);

        ShaderResourceVariableDesc Variables[] =
            {
                {SHADER_TYPE_PIXEL, "g_tex2DShadowMap", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE} //
            };

        ImmutableSamplerDesc ImtblSampler[] =
            {
                {SHADER_TYPE_PIXEL, "g_tex2DShadowMap", Sam_LinearClamp} //
            };

        if (m_pDevice->GetDeviceCaps().IsGLDevice())
        {
            // Even though textures are never sampled in the shader, OpenGL requires proper
            // sampler to be set even when texelFetch is used.
            PSODesc.ResourceLayout.ImmutableSamplers    = ImtblSampler;
            PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSampler);
        }
        PSODesc.ResourceLayout.Variables    = Variables;
        PSODesc.ResourceLayout.NumVariables = _countof(Variables);

        auto& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

        GraphicsPipeline.RasterizerDesc.FillMode      = FILL_MODE_SOLID;
        GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
        GraphicsPipeline.DepthStencilDesc.DepthEnable = False;
        PSOCreateInfo.pVS                             = pScreenSizeTriVS;
        PSOCreateInfo.pPS                             = pVSMHorzPS;
        GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        GraphicsPipeline.NumRenderTargets             = 1;
        GraphicsPipeline.RTVFormats[0]                = FilterableShadowMapFmt;

        m_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &Tech.PSO);
        Tech.PSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "cbConversionAttribs")->Set(m_pConversionAttribsBuffer);

        if (m_BlurVertTech.PSO && m_BlurVertTech.PSO->GetGraphicsPipelineDesc().RTVFormats[0] != FilterableShadowMapFmt)
            m_BlurVertTech.PSO.Release();

        if (!m_BlurVertTech.PSO)
        {
            ShaderCI.EntryPoint = "VertBlurPS";
            ShaderCI.Desc.Name  = "Vertical blur pass PS";
            PSODesc.Name        = "Vertical blur pass PSO";
            RefCntAutoPtr<IShader> pVertBlurPS;
            m_pDevice->CreateShader(ShaderCI, &pVertBlurPS);
            PSOCreateInfo.pPS = pVertBlurPS;
            m_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_BlurVertTech.PSO);
            m_BlurVertTech.PSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "cbConversionAttribs")->Set(m_pConversionAttribsBuffer);
        }
    }
}

void ShadowMapManager::InitializeResourceBindings()
{
    for (int mode = SHADOW_MODE_VSM; mode <= SHADOW_MODE_EVSM4; ++mode)
    {
        auto& Tech = m_ConversionTech[mode];
        if (mode == SHADOW_MODE_EVSM4)
        {
            Tech.SRB = m_ConversionTech[SHADOW_MODE_EVSM2].SRB;
            continue;
        }

        Tech.SRB.Release();
        Tech.PSO->CreateShaderResourceBinding(&Tech.SRB, true);
        Tech.SRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_tex2DShadowMap")->Set(GetSRV());
    }
    m_BlurVertTech.SRB.Release();
    m_BlurVertTech.PSO->CreateShaderResourceBinding(&m_BlurVertTech.SRB, true);
    m_BlurVertTech.SRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_tex2DShadowMap")->Set(m_pIntermediateSRV);
}

void ShadowMapManager::ConvertToFilterable(IDeviceContext* pCtx, const ShadowMapAttribs& ShadowAttribs)
{
    if (m_ShadowMode == SHADOW_MODE_VSM || m_ShadowMode == SHADOW_MODE_EVSM2 || m_ShadowMode == SHADOW_MODE_EVSM4)
    {
        auto&       Tech          = m_ConversionTech[m_ShadowMode];
        const auto& ShadowMapDesc = m_pShadowMapSRV->GetTexture()->GetDesc();
        VERIFY(static_cast<int>(ShadowMapDesc.ArraySize) == ShadowAttribs.iNumCascades, "Inconsistent number of cascades");
        const auto& FilterableSMDesc = m_pFilterableShadowMapSRV->GetTexture()->GetDesc();
        VERIFY(ShadowAttribs.bIs32BitEVSM == (FilterableSMDesc.Format == TEX_FORMAT_RGBA32_FLOAT || FilterableSMDesc.Format == TEX_FORMAT_RG32_FLOAT),
               "Incorrect 32-bit VSM flag");
        (void)FilterableSMDesc;

        int  iFilterRadius = (ShadowAttribs.iFixedFilterSize - 1) / 2;
        bool bSkipBlur     = ShadowAttribs.iFixedFilterSize == 2;
        for (Uint32 i = 0; i < ShadowMapDesc.ArraySize; ++i)
        {
            ITextureView* pRTVs[] = {bSkipBlur ? m_pFilterableShadowMapRTVs[i] : m_pIntermediateRTV};
            pCtx->SetRenderTargets(1, pRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            {
                struct ConversionAttribs //
                {
                    int   iCascade;
                    float fHorzFilterRadius;
                    float fVertFilterRadius;
                    float fEVSMPositiveExponent;

                    float fEVSMNegativeExponent;
                    int   Is32BitEVSM;
                };
                MapHelper<ConversionAttribs> pAttribs(pCtx, m_pConversionAttribsBuffer, MAP_WRITE, MAP_FLAG_DISCARD);
                pAttribs->iCascade = i;
                if (ShadowAttribs.iFixedFilterSize > 0)
                {
                    pAttribs->fHorzFilterRadius = static_cast<float>(iFilterRadius);
                    pAttribs->fVertFilterRadius = static_cast<float>(iFilterRadius);
                }
                else
                {
                    const auto& Cascade         = ShadowAttribs.Cascades[i];
                    float       fNDCtoUVScale   = 0.5f;
                    float       fFilterWidth    = ShadowAttribs.fFilterWorldSize * Cascade.f4LightSpaceScale.x * fNDCtoUVScale;
                    float       fFilterHeight   = ShadowAttribs.fFilterWorldSize * Cascade.f4LightSpaceScale.y * fNDCtoUVScale;
                    pAttribs->fHorzFilterRadius = fFilterWidth / 2.f * static_cast<float>(ShadowMapDesc.Width);
                    pAttribs->fVertFilterRadius = fFilterHeight / 2.f * static_cast<float>(ShadowMapDesc.Height);
                }
                pAttribs->fEVSMPositiveExponent = ShadowAttribs.fEVSMPositiveExponent;
                pAttribs->fEVSMNegativeExponent = ShadowAttribs.fEVSMNegativeExponent;
                pAttribs->Is32BitEVSM           = ShadowAttribs.bIs32BitEVSM;
            }
            pCtx->SetPipelineState(Tech.PSO);
            pCtx->CommitShaderResources(Tech.SRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            DrawAttribs drawAttribs{3, DRAW_FLAG_VERIFY_ALL};
            pCtx->Draw(drawAttribs);

            if (!bSkipBlur)
            {
                pRTVs[0] = m_pFilterableShadowMapRTVs[i];
                pCtx->SetRenderTargets(1, pRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
                pCtx->SetPipelineState(m_BlurVertTech.PSO);
                pCtx->CommitShaderResources(m_BlurVertTech.SRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
                pCtx->Draw(drawAttribs);
            }
        }
    }
}

} // namespace Diligent
