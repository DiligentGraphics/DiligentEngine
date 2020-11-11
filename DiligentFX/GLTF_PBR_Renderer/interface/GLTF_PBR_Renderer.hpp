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

#include <unordered_map>
#include <functional>

#include "../../../DiligentCore/Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "../../../DiligentCore/Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "../../../DiligentCore/Common/interface/HashUtils.hpp"
#include "../../../DiligentTools/AssetLoader/interface/GLTFLoader.hpp"

namespace Diligent
{

#include "Shaders/GLTF_PBR/public/GLTF_PBR_Structures.fxh"

/// Implementation of a GLTF PBR renderer
class GLTF_PBR_Renderer
{
public:
    /// Renderer create info
    struct CreateInfo
    {
        /// Render target format.
        TEXTURE_FORMAT RTVFmt = TEX_FORMAT_UNKNOWN;

        /// Depth-buffer format.

        /// \note   If both RTV and DSV formats are TEX_FORMAT_UNKNOWN,
        ///         the renderer will not initialize PSO, uniform buffers and other
        ///         resources. It is expected that an application will use custom
        ///         render callback function.
        TEXTURE_FORMAT DSVFmt = TEX_FORMAT_UNKNOWN;

        /// Indicates if front face is CCW.
        bool FrontCCW = false;

        /// Indicates if the renderer should allow debug views.
        /// Rendering with debug views disabled is more efficient.
        bool AllowDebugView = false;

        /// Indicates whether to use IBL.
        bool UseIBL = false;

        /// Whether to use ambient occlusion texture.
        bool UseAO = true;

        /// Whether to use emissive texture.
        bool UseEmissive = true;

        /// When set to true, pipeline state will be compiled with immutable samplers.
        /// When set to false, samplers from the texture views will be used.
        bool UseImmutableSamplers = true;

        static const SamplerDesc DefaultSampler;

        /// Immutable sampler for color map texture.
        SamplerDesc ColorMapImmutableSampler = DefaultSampler;

        /// Immutable sampler for physical description map texture.
        SamplerDesc PhysDescMapImmutableSampler = DefaultSampler;

        /// Immutable sampler for normal map texture.
        SamplerDesc NormalMapImmutableSampler = DefaultSampler;

        /// Immutable sampler for AO texture.
        SamplerDesc AOMapImmutableSampler = DefaultSampler;

        /// Immutable sampler for emissive map texture.
        SamplerDesc EmissiveMapImmutableSampler = DefaultSampler;
    };

    /// Initializes the renderer
    GLTF_PBR_Renderer(IRenderDevice*    pDevice,
                      IDeviceContext*   pCtx,
                      const CreateInfo& CI);

    /// Rendering information
    struct RenderInfo
    {
        /// Model transform matrix
        float4x4 ModelTransform = float4x4::Identity();

        /// Alpha mode flags
        enum ALPHA_MODE_FLAGS : Uint32
        {
            /// Render nothing
            ALPHA_MODE_FLAG_NONE = 0,

            /// Render opaque matetrials
            ALPHA_MODE_FLAG_OPAQUE = 1 << GLTF::Material::ALPHAMODE_OPAQUE,

            /// Render alpha-masked matetrials
            ALPHA_MODE_FLAG_MASK = 1 << GLTF::Material::ALPHAMODE_MASK,

            /// Render alpha-blended matetrials
            ALPHA_MODE_FLAG_BLEND = 1 << GLTF::Material::ALPHAMODE_BLEND,

            /// Render all materials
            ALPHA_MODE_FLAG_ALL = ALPHA_MODE_FLAG_OPAQUE | ALPHA_MODE_FLAG_MASK | ALPHA_MODE_FLAG_BLEND
        };
        /// Flag indicating which alpha modes to render
        ALPHA_MODE_FLAGS AlphaModes = ALPHA_MODE_FLAG_ALL;

        /// Debug view type
        enum class DebugViewType : int
        {
            None            = 0,
            BaseColor       = 1,
            Transparency    = 2,
            NormalMap       = 3,
            Occlusion       = 4,
            Emissive        = 5,
            Metallic        = 6,
            Roughness       = 7,
            DiffuseColor    = 8,
            SpecularColor   = 9,
            Reflectance90   = 10,
            MeshNormal      = 11,
            PerturbedNormal = 12,
            NdotV           = 13,
            DiffuseIBL      = 14,
            SpecularIBL     = 15,
            NumDebugViews
        };
        DebugViewType DebugView = DebugViewType::None;

        /// Ambient occlusion strength
        float OcclusionStrength = 1;

        /// Emission scale
        float EmissionScale = 1;

        /// IBL scale
        float IBLScale = 1;

        /// Average log luminance used by tone mapping
        float AverageLogLum = 0.3f;

        /// Middle gray value used by tone mapping
        float MiddleGray = 0.18f;

        /// White point value used by tone mapping
        float WhitePoint = 3.f;
    };

    /// GLTF node rendering info passed to the custom render callback
    struct GLTFNodeRenderInfo
    {
        /// GLTF material
        const GLTF::Material* pMaterial = nullptr;

        /// GLTF node shader transforms
        GLTFNodeShaderTransforms ShaderTransforms;

        /// GLTF material shader information
        GLTFMaterialShaderInfo MaterialShaderInfo;

        /// Index type for indexed primitives, or VT_UNDEFINED for non-indexed ones
        VALUE_TYPE IndexType = VT_UNDEFINED;
        union
        {
            /// Index count for indexed primitives
            Uint32 IndexCount;

            /// Vertex count for non-indexed primitives
            Uint32 VertexCount;
        };
        /// First index for indexed primitives
        Uint32 FirstIndex = 0;

        GLTFNodeRenderInfo() noexcept :
            IndexCount{0}
        {}
    };

    /// Renders the given GLTF model.

    /// \param [in] pCtx               - Device context to record rendering commands to.
    /// \param [in] GLTFModel          - GLTF model to render.
    /// \param [in] RenderParams       - Render parameters.
    /// \param [in] RenderNodeCallback - Optional render call back function that should be called
    ///                                  for every GLTF node instead of rendering it.
    /// \param [in] SRBTypeId          - Optional application-defined SRB type that was given to
    ///                                  CreateMaterialSRB.
    void Render(IDeviceContext*                                pCtx,
                GLTF::Model&                                   GLTFModel,
                const RenderInfo&                              RenderParams,
                std::function<void(const GLTFNodeRenderInfo&)> RenderNodeCallback = nullptr,
                size_t                                         SRBTypeId          = 0);

    /// Initializes resource bindings for a given GLTF model
    void InitializeResourceBindings(GLTF::Model& GLTFModel,
                                    IBuffer*     pCameraAttribs,
                                    IBuffer*     pLightAttribs);

    /// Releases resource bindings for a given GLTF model and SRB type
    void ReleaseResourceBindings(GLTF::Model& GLTFModel, size_t SRBTypeId = 0);

    /// Precompute cubemaps used by IBL.
    void PrecomputeCubemaps(IRenderDevice*  pDevice,
                            IDeviceContext* pCtx,
                            ITextureView*   pEnvironmentMap);

    // clang-format off
    ITextureView* GetIrradianceCubeSRV()    { return m_pIrradianceCubeSRV; }
    ITextureView* GetPrefilteredEnvMapSRV() { return m_pPrefilteredEnvMapSRV; }
    ITextureView* GetWhiteTexSRV()          { return m_pWhiteTexSRV; }
    ITextureView* GetBlackTexSRV()          { return m_pBlackTexSRV; }
    ITextureView* GetDefaultNormalMapSRV()  { return m_pDefaultNormalMapSRV; }
    // clang-format on

    /// Creates a shader resource binding for the given material.

    /// \param [in] Material       - GLTF material to create SRB for.
    /// \param [in] pCameraAttribs - Camera attributes constant buffer to set in the SRB.
    /// \param [in] pLightAttribs  - Light attributes constant buffer to set in the SRB.
    /// \param [in] pPSO           - Optional PSO object to use to create the SRB instead of the
    ///                              default PSO.
    /// \param [in] TypeId         - Optional application-defined type to associate created SRB object with.
    ///                              This type can be used when retreiving the SRB with GetMaterialSRB or
    ///                              rendering the model with Render.
    ///                              An application may use this type to differentiate e.g. shadow-pass SRBs
    ///                              from color-pass SRBs.
    /// \return                      Created shader resource binding.
    IShaderResourceBinding* CreateMaterialSRB(GLTF::Material& Material,
                                              IBuffer*        pCameraAttribs,
                                              IBuffer*        pLightAttribs,
                                              IPipelineState* pPSO   = nullptr,
                                              size_t          TypeId = 0);

    /// Finds a shader resource binding for the given material.

    /// \param [in] Material - GLTF material to find SRB for.
    /// \param [in] TypeId   - Optional application-defined type ID that was given to CreateMaterialSRB.
    /// \return                Shader resource binding.
    IShaderResourceBinding* GetMaterialSRB(const GLTF::Material* material, size_t TypeId = 0)
    {
        auto it = m_SRBCache.find(SRBCacheKey{material, TypeId});
        return it != m_SRBCache.end() ? it->second.RawPtr() : nullptr;
    }

private:
    void PrecomputeBRDF(IRenderDevice*  pDevice,
                        IDeviceContext* pCtx);

    void CreatePSO(IRenderDevice* pDevice);

    void RenderGLTFNode(IDeviceContext*                                pCtx,
                        const GLTF::Node*                              node,
                        GLTF::Material::ALPHA_MODE                     AlphaMode,
                        const float4x4&                                ModelTransform,
                        std::function<void(const GLTFNodeRenderInfo&)> RenderNodeCallback,
                        size_t                                         SRBTypeId);

    struct PSOKey
    {
        PSOKey() noexcept {};
        PSOKey(GLTF::Material::ALPHA_MODE _AlphaMode, bool _DoubleSided) :
            AlphaMode{_AlphaMode},
            DoubleSided{_DoubleSided}
        {}

        GLTF::Material::ALPHA_MODE AlphaMode   = GLTF::Material::ALPHAMODE_OPAQUE;
        bool                       DoubleSided = false;
    };

    static size_t GetPSOIdx(const PSOKey& Key)
    {
        return (Key.AlphaMode == GLTF::Material::ALPHAMODE_BLEND ? 1 : 0) + (Key.DoubleSided ? 2 : 0);
    }

    void AddPSO(const PSOKey& Key, RefCntAutoPtr<IPipelineState> pPSO)
    {
        auto Idx = GetPSOIdx(Key);
        if (Idx >= m_PSOCache.size())
            m_PSOCache.resize(Idx + 1);
        VERIFY_EXPR(!m_PSOCache[Idx]);
        m_PSOCache[Idx] = std::move(pPSO);
    }

    IPipelineState* GetPSO(const PSOKey& Key)
    {
        auto Idx = GetPSOIdx(Key);
        VERIFY_EXPR(Idx < m_PSOCache.size());
        return Idx < m_PSOCache.size() ? m_PSOCache[Idx].RawPtr() : nullptr;
    }

    const CreateInfo m_Settings;

    static constexpr Uint32     BRDF_LUT_Dim = 512;
    RefCntAutoPtr<ITextureView> m_pBRDF_LUT_SRV;

    std::vector<RefCntAutoPtr<IPipelineState>> m_PSOCache;

    RefCntAutoPtr<ITextureView> m_pWhiteTexSRV;
    RefCntAutoPtr<ITextureView> m_pBlackTexSRV;
    RefCntAutoPtr<ITextureView> m_pDefaultNormalMapSRV;

    struct SRBCacheKey
    {
        const GLTF::Material* pMaterial = nullptr;
        size_t                TypeId    = 0;

        SRBCacheKey() = default;

        SRBCacheKey(const GLTF::Material* _pMaterial,
                    size_t                _TypeId) :
            pMaterial{_pMaterial},
            TypeId{_TypeId}
        {}


        bool operator==(const SRBCacheKey& Key) const
        {
            return pMaterial == Key.pMaterial && TypeId == Key.TypeId;
        }

        struct Hasher
        {
            size_t operator()(const SRBCacheKey& Key) const
            {
                return ComputeHash(Key.pMaterial, Key.TypeId);
            }
        };
    };
    std::unordered_map<SRBCacheKey, RefCntAutoPtr<IShaderResourceBinding>, SRBCacheKey::Hasher> m_SRBCache;

    static constexpr TEXTURE_FORMAT IrradianceCubeFmt    = TEX_FORMAT_RGBA32_FLOAT;
    static constexpr TEXTURE_FORMAT PrefilteredEnvMapFmt = TEX_FORMAT_RGBA16_FLOAT;
    static constexpr Uint32         IrradianceCubeDim    = 64;
    static constexpr Uint32         PrefilteredEnvMapDim = 256;

    RefCntAutoPtr<ITextureView>           m_pIrradianceCubeSRV;
    RefCntAutoPtr<ITextureView>           m_pPrefilteredEnvMapSRV;
    RefCntAutoPtr<IPipelineState>         m_pPrecomputeIrradianceCubePSO;
    RefCntAutoPtr<IPipelineState>         m_pPrefilterEnvMapPSO;
    RefCntAutoPtr<IShaderResourceBinding> m_pPrecomputeIrradianceCubeSRB;
    RefCntAutoPtr<IShaderResourceBinding> m_pPrefilterEnvMapSRB;

    RenderInfo m_RenderParams;

    RefCntAutoPtr<IBuffer> m_TransformsCB;
    RefCntAutoPtr<IBuffer> m_GLTFAttribsCB;
    RefCntAutoPtr<IBuffer> m_PrecomputeEnvMapAttribsCB;
};

DEFINE_FLAG_ENUM_OPERATORS(GLTF_PBR_Renderer::RenderInfo::ALPHA_MODE_FLAGS);

} // namespace Diligent
