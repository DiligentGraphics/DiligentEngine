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

#include "../../../../DiligentCore/Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "../../../../DiligentCore/Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "../../../../DiligentCore/Graphics/GraphicsEngine/interface/Buffer.h"
#include "../../../../DiligentCore/Graphics/GraphicsEngine/interface/Texture.h"
#include "../../../../DiligentCore/Graphics/GraphicsEngine/interface/BufferView.h"
#include "../../../../DiligentCore/Graphics/GraphicsEngine/interface/TextureView.h"
#include "../../../../DiligentCore/Common/interface/RefCntAutoPtr.hpp"
#include "../../../../DiligentCore/Common/interface/BasicMath.hpp"

namespace Diligent
{

using uint = uint32_t;

#include "Shaders/Common/public/BasicStructures.fxh"
#include "Shaders/PostProcess/ToneMapping/public/ToneMappingStructures.fxh"
#include "Shaders/PostProcess/EpipolarLightScattering/public/EpipolarLightScatteringStructures.fxh"


class EpipolarLightScattering
{
public:
    struct FrameAttribs
    {
        IRenderDevice*  pDevice        = nullptr;
        IDeviceContext* pDeviceContext = nullptr;

        double dElapsedTime = 0;

        const LightAttribs*  pLightAttribs  = nullptr;
        const CameraAttribs* pCameraAttribs = nullptr;
        // If this parameter is null, the effect will use its own buffer.
        IBuffer* pcbLightAttribs = nullptr;
        // If this parameter is null, the effect will use its own buffer.
        IBuffer* pcbCameraAttribs = nullptr;

        /// Shader resource view of the source color buffer.
        ITextureView* ptex2DSrcColorBufferSRV = nullptr;

        /// Shader resource view of the source depth.
        ITextureView* ptex2DSrcDepthBufferSRV = nullptr;

        /// Render target view of the destination color buffer where final image will be rendered.
        ITextureView* ptex2DDstColorBufferRTV = nullptr;

        /// Depth-stencil view of the destination depth buffer where final image will be rendered.
        ITextureView* ptex2DDstDepthBufferDSV = nullptr;

        /// Shadow map shader resource view
        ITextureView* ptex2DShadowMapSRV = nullptr;
    };

    EpipolarLightScattering(IRenderDevice*              in_pDevice,
                            IDeviceContext*             in_pContext,
                            TEXTURE_FORMAT              BackBufferFmt,
                            TEXTURE_FORMAT              DepthBufferFmt,
                            TEXTURE_FORMAT              OffscreenBackBuffer,
                            const AirScatteringAttribs& ScatteringAttibs = AirScatteringAttribs{});
    ~EpipolarLightScattering();


    void OnWindowResize(IRenderDevice* pDevice, Uint32 uiBackBufferWidth, Uint32 uiBackBufferHeight);

    /// The method prepares internal resources and states for processing the next frame.
    /// It must be called at the beginning of every frame before any other
    /// rendering method (RenderSun, PerformPostProcessing) can be called.
    void PrepareForNewFrame(FrameAttribs&                   FrameAttribs,
                            EpipolarLightScatteringAttribs& PPAttribs);

    void ComputeSunColor(const float3& vDirectionOnSun,
                         const float4& f4ExtraterrestrialSunColor,
                         float4&       f4SunColorAtGround,
                         float4&       f4AmbientLight);

    void RenderSun(TEXTURE_FORMAT RTVFormat,
                   TEXTURE_FORMAT DSVFormat,
                   Uint8          SampleCount);

    void PerformPostProcessing();


    IBuffer*      GetMediaAttribsCB() { return m_pcbMediaAttribs; }
    ITextureView* GetPrecomputedNetDensitySRV() { return m_ptex2DOccludedNetDensityToAtmTopSRV; }
    ITextureView* GetAmbientSkyLightSRV(IRenderDevice* pDevice, IDeviceContext* pContext);

private:
    void ReconstructCameraSpaceZ();
    void RenderSliceEndpoints();
    void RenderCoordinateTexture();
    void RenderCoarseUnshadowedInctr();
    void RefineSampleLocations();
    void MarkRayMarchingSamples();
    void RenderSliceUVDirAndOrig();
    void Build1DMinMaxMipMap(int iCascadeIndex);
    void DoRayMarching(Uint32 uiMaxStepsAlongRay, int iCascadeIndex);
    void InterpolateInsctrIrradiance();
    void UnwarpEpipolarScattering(bool bRenderLuminance);
    void UpdateAverageLuminance();
    enum class EFixInscatteringMode
    {
        LuminanceOnly         = 0,
        FixInscattering       = 1,
        FullScreenRayMarching = 2
    };
    void FixInscatteringAtDepthBreaks(Uint32 uiMaxStepsAlongRay, EFixInscatteringMode Mode);
    void RenderSampleLocations();

    void PrecomputeOpticalDepthTexture(IRenderDevice* pDevice, IDeviceContext* pContext);
    void PrecomputeScatteringLUT(IRenderDevice* pDevice, IDeviceContext* pContext);
    void CreateRandomSphereSamplingTexture(IRenderDevice* pDevice);
    void ComputeAmbientSkyLightTexture(IRenderDevice* pDevice, IDeviceContext* pContext);
    void ComputeScatteringCoefficients(IDeviceContext* pDeviceCtx = nullptr);
    void CreateEpipolarTextures(IRenderDevice* pDevice);
    void CreateSliceEndPointsTexture(IRenderDevice* pDevice);
    void CreateExtinctionTexture(IRenderDevice* pDevice);
    void CreateAmbientSkyLightTexture(IRenderDevice* pDevice);
    void CreateLowResLuminanceTexture(IRenderDevice* pDevice, IDeviceContext* pDeviceCtx);
    void CreateSliceUVDirAndOriginTexture(IRenderDevice* pDevice);
    void CreateCamSpaceZTexture(IRenderDevice* pDevice);
    void CreateMinMaxShadowMap(IRenderDevice* pDevice);

    void DefineMacros(class ShaderMacroHelper& Macros);

    const TEXTURE_FORMAT m_BackBufferFmt;
    const TEXTURE_FORMAT m_DepthBufferFmt;
    const TEXTURE_FORMAT m_OffscreenBackBufferFmt;

    static constexpr TEXTURE_FORMAT PrecomputedNetDensityTexFmt = TEX_FORMAT_RG32_FLOAT;
    static constexpr TEXTURE_FORMAT CoordinateTexFmt            = TEX_FORMAT_RG32_FLOAT;
    static constexpr TEXTURE_FORMAT SliceEndpointsFmt           = TEX_FORMAT_RGBA32_FLOAT;
    static constexpr TEXTURE_FORMAT InterpolationSourceTexFmt   = TEX_FORMAT_RGBA32_UINT;
    static constexpr TEXTURE_FORMAT EpipolarCamSpaceZFmt        = TEX_FORMAT_R32_FLOAT;
    static constexpr TEXTURE_FORMAT EpipolarInsctrTexFmt        = TEX_FORMAT_RGBA16_FLOAT;
    static constexpr TEXTURE_FORMAT EpipolarImageDepthFmt0      = TEX_FORMAT_D24_UNORM_S8_UINT;
    static constexpr TEXTURE_FORMAT EpipolarImageDepthFmt1      = TEX_FORMAT_D32_FLOAT_S8X24_UINT;
    static constexpr TEXTURE_FORMAT EpipolarExtinctionFmt       = TEX_FORMAT_RGBA8_UNORM;
    static constexpr TEXTURE_FORMAT AmbientSkyLightTexFmt       = TEX_FORMAT_RGBA16_FLOAT;
    static constexpr TEXTURE_FORMAT WeightedLogLumTexFmt        = TEX_FORMAT_RG16_FLOAT;
    static constexpr TEXTURE_FORMAT AverageLuminanceTexFmt      = TEX_FORMAT_R16_FLOAT;
    static constexpr TEXTURE_FORMAT SliceUVDirAndOriginTexFmt   = TEX_FORMAT_RGBA32_FLOAT;
    static constexpr TEXTURE_FORMAT CamSpaceZFmt                = TEX_FORMAT_R32_FLOAT;


    EpipolarLightScatteringAttribs m_PostProcessingAttribs;
    FrameAttribs                   m_FrameAttribs;

    struct UserResourceIds
    {
        Int32 LightAttribs      = -1;
        Int32 CameraAttribs     = -1;
        Int32 SrcColorBufferSRV = -1;
        Int32 SrcDepthBufferSRV = -1;
        Int32 ShadowMapSRV      = -1;
    } m_UserResourceIds;

    bool   m_bUseCombinedMinMaxTexture;
    Uint32 m_uiSampleRefinementCSThreadGroupSize;
    Uint32 m_uiSampleRefinementCSMinimumThreadGroupSize;

    static const int sm_iNumPrecomputedHeights = 1024;
    static const int sm_iNumPrecomputedAngles  = 1024;

    int m_iPrecomputedSctrUDim = 32;
    int m_iPrecomputedSctrVDim = 128;
    int m_iPrecomputedSctrWDim = 64;
    int m_iPrecomputedSctrQDim = 16;

    RefCntAutoPtr<ITextureView> m_ptex3DSingleScatteringSRV;
    RefCntAutoPtr<ITextureView> m_ptex3DHighOrderScatteringSRV;
    RefCntAutoPtr<ITextureView> m_ptex3DMultipleScatteringSRV;

    Uint32                      m_uiNumRandomSamplesOnSphere = 128;
    RefCntAutoPtr<ITextureView> m_ptex2DSphereRandomSamplingSRV;

    static const int            sm_iLowResLuminanceMips = 7; // 64x64
    RefCntAutoPtr<ITextureView> m_ptex2DLowResLuminanceRTV;  // 64 X 64 R16F
    RefCntAutoPtr<ITextureView> m_ptex2DLowResLuminanceSRV;
    RefCntAutoPtr<ITextureView> m_ptex2DAverageLuminanceRTV; // 1  X  1 R16F

    static const int            sm_iAmbientSkyLightTexDim = 1024;
    RefCntAutoPtr<ITextureView> m_ptex2DAmbientSkyLightSRV; // 1024 x 1 RGBA16F
    RefCntAutoPtr<ITextureView> m_ptex2DAmbientSkyLightRTV;
    RefCntAutoPtr<ITextureView> m_ptex2DOccludedNetDensityToAtmTopSRV; // 1024 x 1024 RG32F
    RefCntAutoPtr<ITextureView> m_ptex2DOccludedNetDensityToAtmTopRTV;

    RefCntAutoPtr<IShader> m_pFullScreenTriangleVS;

    RefCntAutoPtr<IResourceMapping> m_pResMapping;

    RefCntAutoPtr<ITextureView> m_ptex2DCoordinateTextureRTV;     // Max Samples X Num Slices   RG32F
    RefCntAutoPtr<ITextureView> m_ptex2DSliceEndpointsRTV;        // Num Slices  X 1            RGBA32F
    RefCntAutoPtr<ITextureView> m_ptex2DEpipolarCamSpaceZRTV;     // Max Samples X Num Slices   R32F
    RefCntAutoPtr<ITextureView> m_ptex2DEpipolarInscatteringRTV;  // Max Samples X Num Slices   RGBA16F
    RefCntAutoPtr<ITextureView> m_ptex2DEpipolarExtinctionRTV;    // Max Samples X Num Slices   RGBA8_UNORM
    RefCntAutoPtr<ITextureView> m_ptex2DEpipolarImageDSV;         // Max Samples X Num Slices   D24S8
    RefCntAutoPtr<ITextureView> m_ptex2DInitialScatteredLightRTV; // Max Samples X Num Slices   RGBA16F
    RefCntAutoPtr<ITextureView> m_ptex2DSliceUVDirAndOriginRTV;   // Num Slices  X Num Cascaes  RGBA32F
    RefCntAutoPtr<ITextureView> m_ptex2DCamSpaceZRTV;             // BckBfrWdth  x BckBfrHght   R32F
    RefCntAutoPtr<ITextureView> m_ptex2DMinMaxShadowMapSRV[2];    // MinMaxSMRes x Num Slices   RG32F or RG16UNORM
    RefCntAutoPtr<ITextureView> m_ptex2DMinMaxShadowMapRTV[2];

    RefCntAutoPtr<ISampler> m_pPointClampSampler, m_pLinearClampSampler;

    struct RenderTechnique
    {
        RefCntAutoPtr<IPipelineState>         PSO;
        RefCntAutoPtr<IShaderResourceBinding> SRB;
        Uint32                                PSODependencyFlags = 0;
        Uint32                                SRBDependencyFlags = 0;

        void InitializeFullScreenTriangleTechnique(IRenderDevice*                    pDevice,
                                                   const char*                       PSOName,
                                                   IShader*                          VertexShader,
                                                   IShader*                          PixelShader,
                                                   const PipelineResourceLayoutDesc& ResourceLayout,
                                                   Uint8                             NumRTVs,
                                                   TEXTURE_FORMAT                    RTVFmts[],
                                                   TEXTURE_FORMAT                    DSVFmt,
                                                   const DepthStencilStateDesc&      DSSDesc,
                                                   const BlendStateDesc&             BSDesc);

        void InitializeFullScreenTriangleTechnique(IRenderDevice*                    pDevice,
                                                   const char*                       PSOName,
                                                   IShader*                          VertexShader,
                                                   IShader*                          PixelShader,
                                                   const PipelineResourceLayoutDesc& ResourceLayout,
                                                   TEXTURE_FORMAT                    RTVFmt,
                                                   TEXTURE_FORMAT                    DSVFmt,
                                                   const DepthStencilStateDesc&      DSSDesc,
                                                   const BlendStateDesc&             BSDesc);

        void InitializeComputeTechnique(IRenderDevice*                    pDevice,
                                        const char*                       PSOName,
                                        IShader*                          ComputeShader,
                                        const PipelineResourceLayoutDesc& ResourceLayout);

        void PrepareSRB(IRenderDevice* pDevice, IResourceMapping* pResMapping, Uint32 Flags);

        void Render(IDeviceContext* pDeviceContext, Uint8 StencilRef = 0, Uint32 NumQuads = 1);

        void DispatchCompute(IDeviceContext* pDeviceContext, const DispatchComputeAttribs& DispatchAttrs);

        void CheckStaleFlags(Uint32 StalePSODependencies, Uint32 StaleSRBDependencies);
    };

    enum RENDER_TECH
    {
        RENDER_TECH_RECONSTRUCT_CAM_SPACE_Z = 0,
        RENDER_TECH_RENDER_SLICE_END_POINTS,
        RENDER_TECH_RENDER_COORD_TEXTURE,
        RENDER_TECH_RENDER_COARSE_UNSHADOWED_INSCTR,
        RENDER_TECH_REFINE_SAMPLE_LOCATIONS,
        RENDER_TECH_MARK_RAY_MARCHING_SAMPLES,
        RENDER_TECH_RENDER_SLICE_UV_DIRECTION,
        RENDER_TECH_INIT_MIN_MAX_SHADOW_MAP,
        RENDER_TECH_COMPUTE_MIN_MAX_SHADOW_MAP_LEVEL,
        RENDER_TECH_RAY_MARCH_NO_MIN_MAX_OPT,
        RENDER_TECH_RAY_MARCH_MIN_MAX_OPT,
        RENDER_TECH_INTERPOLATE_IRRADIANCE,
        RENDER_TECH_UNWARP_EPIPOLAR_SCATTERING,
        RENDER_TECH_UNWARP_AND_RENDER_LUMINANCE,
        RENDER_TECH_UPDATE_AVERAGE_LUMINANCE,
        RENDER_TECH_FIX_INSCATTERING_LUM_ONLY,
        RENDER_TECH_FIX_INSCATTERING,
        RENDER_TECH_BRUTE_FORCE_RAY_MARCHING,
        RENDER_TECH_RENDER_SUN,
        RENDER_TECH_RENDER_SAMPLE_LOCATIONS,

        // Precomputation techniques
        RENDER_TECH_PRECOMPUTE_NET_DENSITY_TO_ATM_TOP,
        RENDER_TECH_PRECOMPUTE_SINGLE_SCATTERING,
        RENDER_TECH_COMPUTE_SCATTERING_RADIANCE,
        RENDER_TECH_COMPUTE_SCATTERING_ORDER,
        RENDER_TECH_INIT_HIGH_ORDER_SCATTERING,
        RENDER_TECH_UPDATE_HIGH_ORDER_SCATTERING,
        RENDER_TECH_COMBINE_SCATTERING_ORDERS,
        RENDER_TECH_PRECOMPUTE_AMBIENT_SKY_LIGHT,

        RENDER_TECH_TOTAL_TECHNIQUES
    };

    RenderTechnique m_RenderTech[RENDER_TECH_TOTAL_TECHNIQUES];

    enum PSO_DEPENDENCY_FLAGS
    {
        PSO_DEPENDENCY_INITIAL_SAMPLE_STEP       = 0x00001,
        PSO_DEPENDENCY_EPIPOLE_SAMPLING_DENSITY  = 0x00002,
        PSO_DEPENDENCY_CORRECT_SCATTERING        = 0x00004,
        PSO_DEPENDENCY_OPTIMIZE_SAMPLE_LOCATIONS = 0x00008,
        PSO_DEPENDENCY_ENABLE_LIGHT_SHAFTS       = 0x00010,
        PSO_DEPENDENCY_USE_1D_MIN_MAX_TREE       = 0x00020,
        PSO_DEPENDENCY_USE_COMBINED_MIN_MAX_TEX  = 0x00040,
        PSO_DEPENDENCY_LIGHT_SCTR_TECHNIQUE      = 0x00080,
        PSO_DEPENDENCY_CASCADE_PROCESSING_MODE   = 0x00100,
        PSO_DEPENDENCY_REFINEMENT_CRITERION      = 0x00200,
        PSO_DEPENDENCY_IS_32_BIT_MIN_MAX_TREE    = 0x00400,
        PSO_DEPENDENCY_MULTIPLE_SCATTERING_MODE  = 0x00800,
        PSO_DEPENDENCY_SINGLE_SCATTERING_MODE    = 0x01000,
        PSO_DEPENDENCY_AUTO_EXPOSURE             = 0x02000,
        PSO_DEPENDENCY_TONE_MAPPING_MODE         = 0x04000,
        PSO_DEPENDENCY_LIGHT_ADAPTATION          = 0x08000,
        PSO_DEPENDENCY_EXTINCTION_EVAL_MODE      = 0x10000
    };

    enum SRB_DEPENDENCY_FLAGS
    {
        SRB_DEPENDENCY_LIGHT_ATTRIBS            = 0x00001,
        SRB_DEPENDENCY_CAMERA_ATTRIBS           = 0x00002,
        SRB_DEPENDENCY_SRC_COLOR_BUFFER         = 0x00004,
        SRB_DEPENDENCY_SRC_DEPTH_BUFFER         = 0x00008,
        SRB_DEPENDENCY_SHADOW_MAP               = 0x00010,
        SRB_DEPENDENCY_MIN_MAX_SHADOW_MAP       = 0x00020,
        SRB_DEPENDENCY_COORDINATE_TEX           = 0x00040,
        SRB_DEPENDENCY_INTERPOLATION_SOURCE_TEX = 0x00080,
        SRB_DEPENDENCY_SLICE_END_POINTS_TEX     = 0x00100,
        SRB_DEPENDENCY_EPIPOLAR_CAM_SPACE_Z_TEX = 0x00200,
        SRB_DEPENDENCY_EPIPOLAR_INSCTR_TEX      = 0x00400,
        SRB_DEPENDENCY_EPIPOLAR_EXTINCTION_TEX  = 0x00800,
        SRB_DEPENDENCY_EPIPOLAR_IMAGE_DEPTH     = 0x01000,
        SRB_DEPENDENCY_INITIAL_SCTR_LIGHT_TEX   = 0x02000,
        SRB_DEPENDENCY_AVERAGE_LUMINANCE_TEX    = 0x04000,
        SRB_DEPENDENCY_SLICE_UV_DIR_TEX         = 0x08000,
        SRB_DEPENDENCY_CAM_SPACE_Z_TEX          = 0x10000
    };

    RefCntAutoPtr<IShaderResourceBinding> m_pComputeMinMaxSMLevelSRB[2];

    RefCntAutoPtr<ITexture> m_ptex3DHighOrderSctr, m_ptex3DHighOrderSctr2;

    RefCntAutoPtr<IBuffer> m_pcbPostProcessingAttribs;
    RefCntAutoPtr<IBuffer> m_pcbMediaAttribs;
    RefCntAutoPtr<IBuffer> m_pcbMiscParams;
    RefCntAutoPtr<IBuffer> m_pcbLightAttribs;
    RefCntAutoPtr<IBuffer> m_pcbCameraAttribs;

    Uint32 m_uiBackBufferWidth  = 0;
    Uint32 m_uiBackBufferHeight = 0;

    //const float m_fTurbidity = 1.02f;
    AirScatteringAttribs m_MediaParams;

    enum UpToDateResourceFlags
    {
        PrecomputedOpticalDepthTex = 0x01,
        AmbientSkyLightTex         = 0x02,
        PrecomputedIntegralsTex    = 0x04
    };
    Uint32 m_uiUpToDateResourceFlags;
};

} // namespace Diligent
