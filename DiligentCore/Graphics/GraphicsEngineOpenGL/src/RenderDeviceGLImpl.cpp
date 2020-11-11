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

#include "pch.h"

#include "RenderDeviceGLImpl.hpp"

#include "BufferGLImpl.hpp"
#include "ShaderGLImpl.hpp"
#include "VAOCache.hpp"
#include "Texture1D_OGL.hpp"
#include "Texture1DArray_OGL.hpp"
#include "Texture2D_OGL.hpp"
#include "Texture2DArray_OGL.hpp"
#include "Texture3D_OGL.hpp"
#include "TextureCube_OGL.hpp"
#include "TextureCubeArray_OGL.hpp"
#include "SamplerGLImpl.hpp"
#include "DeviceContextGLImpl.hpp"
#include "GLTypeConversions.hpp"
#include "PipelineStateGLImpl.hpp"
#include "ShaderResourceBindingGLImpl.hpp"
#include "FenceGLImpl.hpp"
#include "QueryGLImpl.hpp"
#include "RenderPassGLImpl.hpp"
#include "FramebufferGLImpl.hpp"
#include "EngineMemory.h"
#include "StringTools.hpp"

namespace Diligent
{

#if GL_KHR_debug
static void GLAPIENTRY openglCallbackFunction(GLenum        source,
                                              GLenum        type,
                                              GLuint        id,
                                              GLenum        severity,
                                              GLsizei       length,
                                              const GLchar* message,
                                              const void*   userParam)
{
    auto* ShowDebugOutput = reinterpret_cast<const int*>(userParam);
    if (*ShowDebugOutput == 0)
        return;

    // Note: disabling flood of notifications through glDebugMessageControl() has no effect,
    // so we have to filter them out here
    if (id == 131185 || // Buffer detailed info: Buffer object <X> (bound to GL_XXXX ... , usage hint is GL_DYNAMIC_DRAW)
                        // will use VIDEO memory as the source for buffer object operations.
        id == 131186    // Buffer object <X> (bound to GL_XXXX, usage hint is GL_DYNAMIC_DRAW) is being copied/moved from VIDEO memory to HOST memory.
    )
        return;

    std::stringstream MessageSS;

    MessageSS << "OpenGL debug message " << id << " (";
    switch (source)
    {
        // clang-format off
        case GL_DEBUG_SOURCE_API:             MessageSS << "Source: API.";             break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   MessageSS << "Source: Window System.";   break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: MessageSS << "Source: Shader Compiler."; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:     MessageSS << "Source: Third Party.";     break;
        case GL_DEBUG_SOURCE_APPLICATION:     MessageSS << "Source: Application.";     break;
        case GL_DEBUG_SOURCE_OTHER:           MessageSS << "Source: Other.";           break;
        default:                              MessageSS << "Source: Unknown (" << source << ").";
            // clang-format on
    }

    switch (type)
    {
        // clang-format off
        case GL_DEBUG_TYPE_ERROR:               MessageSS << " Type: ERROR.";                break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: MessageSS << " Type: Deprecated Behaviour."; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  MessageSS << " Type: UNDEFINED BEHAVIOUR.";  break; 
        case GL_DEBUG_TYPE_PORTABILITY:         MessageSS << " Type: Portability.";          break;
        case GL_DEBUG_TYPE_PERFORMANCE:         MessageSS << " Type: PERFORMANCE.";          break;
        case GL_DEBUG_TYPE_MARKER:              MessageSS << " Type: Marker.";               break;
        case GL_DEBUG_TYPE_PUSH_GROUP:          MessageSS << " Type: Push Group.";           break;
        case GL_DEBUG_TYPE_POP_GROUP:           MessageSS << " Type: Pop Group.";            break;
        case GL_DEBUG_TYPE_OTHER:               MessageSS << " Type: Other.";                break;
        default:                                MessageSS << " Type: Unknown (" << type << ").";
            // clang-format on
    }

    switch (severity)
    {
        // clang-format off
        case GL_DEBUG_SEVERITY_HIGH:         MessageSS << " Severity: HIGH";         break;
        case GL_DEBUG_SEVERITY_MEDIUM:       MessageSS << " Severity: Medium";       break;
        case GL_DEBUG_SEVERITY_LOW:          MessageSS << " Severity: Low";          break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: MessageSS << " Severity: Notification"; break;
        default:                             MessageSS << " Severity: Unknown (" << severity << ")"; break;
            // clang-format on
    }

    MessageSS << "): " << message;

    LOG_INFO_MESSAGE(MessageSS.str().c_str());
}
#endif // GL_KHR_debug

RenderDeviceGLImpl::RenderDeviceGLImpl(IReferenceCounters*       pRefCounters,
                                       IMemoryAllocator&         RawMemAllocator,
                                       IEngineFactory*           pEngineFactory,
                                       const EngineGLCreateInfo& InitAttribs,
                                       const SwapChainDesc*      pSCDesc) :
    // clang-format off
    TRenderDeviceBase
    {
        pRefCounters,
        RawMemAllocator,
        pEngineFactory,
        0,
        DeviceObjectSizes
        {
            sizeof(TextureBaseGL),
            sizeof(TextureViewGLImpl),
            sizeof(BufferGLImpl),
            sizeof(BufferViewGLImpl),
            sizeof(ShaderGLImpl),
            sizeof(SamplerGLImpl),
            sizeof(PipelineStateGLImpl),
            sizeof(ShaderResourceBindingGLImpl),
            sizeof(FenceGLImpl),
            sizeof(QueryGLImpl),
            sizeof(RenderPassGLImpl),
            sizeof(FramebufferGLImpl)
        }
    },
    // Device caps must be filled in before the constructor of Pipeline Cache is called!
    m_GLContext{InitAttribs, m_DeviceCaps, pSCDesc}
// clang-format on
{
    GLint NumExtensions = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &NumExtensions);
    CHECK_GL_ERROR("Failed to get the number of extensions");
    m_ExtensionStrings.reserve(NumExtensions);
    for (int Ext = 0; Ext < NumExtensions; ++Ext)
    {
        auto CurrExtension = glGetStringi(GL_EXTENSIONS, Ext);
        CHECK_GL_ERROR("Failed to get extension string #", Ext);
        m_ExtensionStrings.emplace(reinterpret_cast<const Char*>(CurrExtension));
    }

#if GL_KHR_debug
    if (InitAttribs.CreateDebugContext && glDebugMessageCallback != nullptr)
    {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(openglCallbackFunction, &m_ShowDebugGLOutput);
        GLuint unusedIds = 0;
        if (glDebugMessageControl != nullptr)
        {
            glDebugMessageControl(
                GL_DONT_CARE, // Source of debug messages to enable or disable
                GL_DONT_CARE, // Type of debug messages to enable or disable
                GL_DONT_CARE, // Severity of debug messages to enable or disable
                0,            // The length of the array ids
                &unusedIds,   // Array of unsigned integers contianing the ids of the messages to enable or disable
                GL_TRUE       // Flag determining whether the selected messages should be enabled or disabled
            );
        }
        if (glGetError() != GL_NO_ERROR)
            LOG_ERROR_MESSAGE("Failed to enable debug messages");
    }
#endif

    FlagSupportedTexFormats();

    std::basic_string<GLubyte> glstrVendor = glGetString(GL_VENDOR);
    std::string                Vendor      = StrToLower(std::string(glstrVendor.begin(), glstrVendor.end()));
    LOG_INFO_MESSAGE("GPU Vendor: ", Vendor);

    auto& AdapterInfo = m_DeviceCaps.AdapterInfo;

    for (size_t i = 0; i < _countof(AdapterInfo.Description) - 1 && i < glstrVendor.length(); ++i)
        AdapterInfo.Description[i] = glstrVendor[i];

    AdapterInfo.Type               = ADAPTER_TYPE_HARDWARE;
    AdapterInfo.VendorId           = 0;
    AdapterInfo.DeviceId           = 0;
    AdapterInfo.NumOutputs         = 0;
    AdapterInfo.DeviceLocalMemory  = 0;
    AdapterInfo.HostVisibileMemory = 0;
    AdapterInfo.UnifiedMemory      = 0;

    if (Vendor.find("intel") != std::string::npos)
        AdapterInfo.Vendor = ADAPTER_VENDOR_INTEL;
    else if (Vendor.find("nvidia") != std::string::npos)
    {
        AdapterInfo.Vendor = ADAPTER_VENDOR_NVIDIA;

#ifndef GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX
        static constexpr GLenum GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX = 0x9048;
#endif

        GLint AvailableMemoryKb = 0;
        glGetIntegerv(GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX, &AvailableMemoryKb);
        if (glGetError() == GL_NO_ERROR)
        {
            AdapterInfo.DeviceLocalMemory = static_cast<Uint64>(AvailableMemoryKb) * Uint64{1024};
        }
        else
        {
            LOG_WARNING_MESSAGE("Unable to read available memory size for NVidia GPU");
        }
    }
    else if (Vendor.find("ati") != std::string::npos ||
             Vendor.find("amd") != std::string::npos)
    {
        AdapterInfo.Vendor = ADAPTER_VENDOR_AMD;

#ifndef GL_TEXTURE_FREE_MEMORY_ATI
        static constexpr GLenum GL_TEXTURE_FREE_MEMORY_ATI = 0x87FC;
#endif
        // https://www.khronos.org/registry/OpenGL/extensions/ATI/ATI_meminfo.txt
        // param[0] - total memory free in the pool
        // param[1] - largest available free block in the pool
        // param[2] - total auxiliary memory free
        // param[3] - largest auxiliary free block
        GLint MemoryParamsKb[4] = {};

        glGetIntegerv(GL_TEXTURE_FREE_MEMORY_ATI, MemoryParamsKb);
        if (glGetError() == GL_NO_ERROR)
        {
            AdapterInfo.DeviceLocalMemory = static_cast<Uint64>(MemoryParamsKb[0]) * Uint64{1024};
        }
        else
        {
            LOG_WARNING_MESSAGE("Unable to read free memory size for AMD GPU");
        }
    }
    else if (Vendor.find("qualcomm"))
        AdapterInfo.Vendor = ADAPTER_VENDOR_QUALCOMM;
    else if (Vendor.find("arm"))
        AdapterInfo.Vendor = ADAPTER_VENDOR_ARM;
    else
        AdapterInfo.Vendor = ADAPTER_VENDOR_UNKNOWN;

    auto MajorVersion = m_DeviceCaps.MajorVersion;
    auto MinorVersion = m_DeviceCaps.MinorVersion;

    auto& Features = m_DeviceCaps.Features;
    auto& TexCaps  = m_DeviceCaps.TexCaps;
    auto& SamCaps  = m_DeviceCaps.SamCaps;

    GLint MaxTextureSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &MaxTextureSize);
    CHECK_GL_ERROR("Failed to get maximum texture size");

    GLint Max3DTextureSize = 0;
    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &Max3DTextureSize);
    CHECK_GL_ERROR("Failed to get maximum 3d texture size");

    GLint MaxCubeTextureSize = 0;
    glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &MaxCubeTextureSize);
    CHECK_GL_ERROR("Failed to get maximum cubemap texture size");

    GLint MaxLayers = 0;
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &MaxLayers);
    CHECK_GL_ERROR("Failed to get maximum number of texture array layers");

#define SET_FEATURE_STATE(Feature, IsSupported, FeatureName)                           \
    do                                                                                 \
    {                                                                                  \
        switch (InitAttribs.Features.Feature)                                          \
        {                                                                              \
            case DEVICE_FEATURE_STATE_ENABLED:                                         \
            {                                                                          \
                if (!(IsSupported))                                                    \
                    LOG_ERROR_AND_THROW(FeatureName, " not supported by this device"); \
                else                                                                   \
                    Features.Feature = DEVICE_FEATURE_STATE_ENABLED;                   \
            }                                                                          \
            break;                                                                     \
            case DEVICE_FEATURE_STATE_DISABLED:                                        \
            case DEVICE_FEATURE_STATE_OPTIONAL:                                        \
                Features.Feature = (IsSupported) ?                                     \
                    DEVICE_FEATURE_STATE_ENABLED :                                     \
                    DEVICE_FEATURE_STATE_DISABLED;                                     \
                break;                                                                 \
            default: UNEXPECTED("Unexpected feature state");                           \
        }                                                                              \
    } while (false)

    SET_FEATURE_STATE(VertexPipelineUAVWritesAndAtomics, false, "Vertex pipeline UAV writes and atomics are");
    SET_FEATURE_STATE(MeshShaders, false, "Mesh shaders are");

    {
        bool WireframeFillSupported = (glPolygonMode != nullptr);
        if (WireframeFillSupported)
        {
            // Test glPolygonMode() function to check if it fails
            // (It does fail on NVidia Shield tablet, but works fine
            // on Intel hw)
            VERIFY(glGetError() == GL_NO_ERROR, "Unhandled gl error encountered");
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            if (glGetError() != GL_NO_ERROR)
                WireframeFillSupported = false;
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            if (glGetError() != GL_NO_ERROR)
                WireframeFillSupported = false;
        }
        SET_FEATURE_STATE(WireframeFill, WireframeFillSupported, "Wireframe fill is");
    }

    if (InitAttribs.ForceNonSeparablePrograms)
        LOG_INFO_MESSAGE("Forcing non-separable shader programs");

    if (m_DeviceCaps.DevType == RENDER_DEVICE_TYPE_GL)
    {
        const bool IsGL46OrAbove = (MajorVersion >= 5) || (MajorVersion == 4 && MinorVersion >= 6);
        const bool IsGL43OrAbove = (MajorVersion >= 5) || (MajorVersion == 4 && MinorVersion >= 3);
        const bool IsGL42OrAbove = (MajorVersion >= 5) || (MajorVersion == 4 && MinorVersion >= 2);
        const bool IsGL41OrAbove = (MajorVersion >= 5) || (MajorVersion == 4 && MinorVersion >= 1);

        SET_FEATURE_STATE(SeparablePrograms, !InitAttribs.ForceNonSeparablePrograms, "Separable programs are");
        SET_FEATURE_STATE(ShaderResourceQueries, Features.SeparablePrograms != DEVICE_FEATURE_STATE_DISABLED, "Shader resource queries are");
        Features.IndirectRendering = DEVICE_FEATURE_STATE_ENABLED;
        Features.WireframeFill     = DEVICE_FEATURE_STATE_ENABLED;
        // clang-format off
        SET_FEATURE_STATE(MultithreadedResourceCreation, false,                                                             "Multithreaded resource creation is");
        SET_FEATURE_STATE(ComputeShaders,                IsGL43OrAbove     || CheckExtension("GL_ARB_compute_shader"),      "Compute shaders are");
        SET_FEATURE_STATE(GeometryShaders,               MajorVersion >= 4 || CheckExtension("GL_ARB_geometry_shader4"),    "Geometry shaders are");
        SET_FEATURE_STATE(Tessellation,                  MajorVersion >= 4 || CheckExtension("GL_ARB_tessellation_shader"), "Tessellation is");
        SET_FEATURE_STATE(BindlessResources,             false,                                                             "Bindless resources are");
        // clang-format on
        Features.OcclusionQueries          = DEVICE_FEATURE_STATE_ENABLED; // Present since 3.3
        Features.BinaryOcclusionQueries    = DEVICE_FEATURE_STATE_ENABLED; // Present since 3.3
        Features.TimestampQueries          = DEVICE_FEATURE_STATE_ENABLED; // Present since 3.3
        Features.PipelineStatisticsQueries = DEVICE_FEATURE_STATE_ENABLED; // Present since 3.3
        Features.DurationQueries           = DEVICE_FEATURE_STATE_ENABLED; // Present since 3.3
        // clang-format off
        SET_FEATURE_STATE(DepthBiasClamp,            false,                                                             "Depth bias clamp is");                        // There is no depth bias clamp in OpenGL
        SET_FEATURE_STATE(DepthClamp,                MajorVersion >= 4 || CheckExtension("GL_ARB_depth_clamp"),         "Depth clamp is");
        SET_FEATURE_STATE(IndependentBlend,          true,                                                              "Independent blend is");
        SET_FEATURE_STATE(DualSourceBlend,           IsGL41OrAbove || CheckExtension("GL_ARB_blend_func_extended"),     "Dual source blend is");
        SET_FEATURE_STATE(MultiViewport,             IsGL41OrAbove || CheckExtension("GL_ARB_viewport_array"),          "Multi viewport is");
        SET_FEATURE_STATE(PixelUAVWritesAndAtomics,  IsGL42OrAbove || CheckExtension("GL_ARB_shader_image_load_store"), "Pixel UAV writes and atomics are");
        SET_FEATURE_STATE(TextureUAVExtendedFormats, false,                                                             "Texture UAV extended formats are");

        SET_FEATURE_STATE(ShaderFloat16,             CheckExtension("GL_EXT_shader_explicit_arithmetic_types_float16"), "16-bit float shader operations are");
        SET_FEATURE_STATE(ResourceBuffer16BitAccess, CheckExtension("GL_EXT_shader_16bit_storage"),                     "16-bit resoure buffer access is");
        SET_FEATURE_STATE(UniformBuffer16BitAccess,  CheckExtension("GL_EXT_shader_16bit_storage"),                     "16-bit uniform buffer access is");
        SET_FEATURE_STATE(ShaderInputOutput16,       false,                                                             "16-bit shader inputs/outputs are");
        SET_FEATURE_STATE(ShaderInt8,                CheckExtension("GL_EXT_shader_explicit_arithmetic_types_int8"),    "8-bit integer shader operations are");
        SET_FEATURE_STATE(ResourceBuffer8BitAccess,  CheckExtension("GL_EXT_shader_8bit_storage"),                      "8-bit resoure buffer access is");
        SET_FEATURE_STATE(UniformBuffer8BitAccess,   CheckExtension("GL_EXT_shader_8bit_storage"),                      "8-bit uniform buffer access is");
        // clang-format on

        TexCaps.MaxTexture1DDimension     = MaxTextureSize;
        TexCaps.MaxTexture1DArraySlices   = MaxLayers;
        TexCaps.MaxTexture2DDimension     = MaxTextureSize;
        TexCaps.MaxTexture2DArraySlices   = MaxLayers;
        TexCaps.MaxTexture3DDimension     = Max3DTextureSize;
        TexCaps.MaxTextureCubeDimension   = MaxCubeTextureSize;
        TexCaps.Texture2DMSSupported      = IsGL43OrAbove || CheckExtension("GL_ARB_texture_storage_multisample");
        TexCaps.Texture2DMSArraySupported = IsGL43OrAbove || CheckExtension("GL_ARB_texture_storage_multisample");
        TexCaps.TextureViewSupported      = IsGL43OrAbove || CheckExtension("GL_ARB_texture_view");
        TexCaps.CubemapArraysSupported    = IsGL43OrAbove || CheckExtension("GL_ARB_texture_cube_map_array");

        SamCaps.BorderSamplingModeSupported   = True;
        SamCaps.AnisotropicFilteringSupported = IsGL46OrAbove || CheckExtension("GL_ARB_texture_filter_anisotropic");
        SamCaps.LODBiasSupported              = True;
    }
    else
    {
        const auto* Extensions = (char*)glGetString(GL_EXTENSIONS);
        LOG_INFO_MESSAGE("Supported extensions: \n", Extensions);

        VERIFY(m_DeviceCaps.DevType == RENDER_DEVICE_TYPE_GLES, "Unexpected device type: OpenGLES expected");

        bool IsGLES31OrAbove = (MajorVersion >= 4) || (MajorVersion == 3 && MinorVersion >= 1);
        bool IsGLES32OrAbove = (MajorVersion >= 4) || (MajorVersion == 3 && MinorVersion >= 2);

        // clang-format off
        SET_FEATURE_STATE(SeparablePrograms,             (IsGLES31OrAbove || strstr(Extensions, "separate_shader_objects")) && !InitAttribs.ForceNonSeparablePrograms, "Separable programs are");
        SET_FEATURE_STATE(ShaderResourceQueries,         Features.SeparablePrograms != DEVICE_FEATURE_STATE_DISABLED,      "Shader resource queries are");
        SET_FEATURE_STATE(IndirectRendering,             IsGLES31OrAbove || strstr(Extensions, "draw_indirect"),           "Indirect rendering is");
        SET_FEATURE_STATE(WireframeFill,                 false, "Wireframe fill is");
        SET_FEATURE_STATE(MultithreadedResourceCreation, false, "Multithreaded resource creation is");
        SET_FEATURE_STATE(ComputeShaders,                IsGLES31OrAbove || strstr(Extensions, "compute_shader"),      "Compute shaders are");
        SET_FEATURE_STATE(GeometryShaders,               IsGLES32OrAbove || strstr(Extensions, "geometry_shader"),     "Geometry shaders are");
        SET_FEATURE_STATE(Tessellation,                  IsGLES32OrAbove || strstr(Extensions, "tessellation_shader"), "Tessellation is");
        SET_FEATURE_STATE(BindlessResources,             false, "Bindless resources are");
        SET_FEATURE_STATE(OcclusionQueries,              false, "Occlusion queries are");
        SET_FEATURE_STATE(BinaryOcclusionQueries,        true,  "Binary occlusion queries are"); // Supported in GLES3.0
#if GL_TIMESTAMP
        const bool DisjointTimerQueriesSupported = strstr(Extensions, "disjoint_timer_query");
        SET_FEATURE_STATE(TimestampQueries, DisjointTimerQueriesSupported, "Timestamp queries are");
        SET_FEATURE_STATE(DurationQueries,  DisjointTimerQueriesSupported, "Duration queries are");
#else
        SET_FEATURE_STATE(TimestampQueries, false, "Timestamp queries are");
        SET_FEATURE_STATE(DurationQueries,  false, "Duration queries are");
#endif
        SET_FEATURE_STATE(PipelineStatisticsQueries, false, "Pipeline atatistics queries are");
        SET_FEATURE_STATE(DepthBiasClamp,            false, "Depth bias clamp is"); // There is no depth bias clamp in OpenGL
        SET_FEATURE_STATE(DepthClamp,                strstr(Extensions, "depth_clamp"), "Depth clamp is");
        SET_FEATURE_STATE(IndependentBlend,          IsGLES32OrAbove,                   "Independent blend is");
        SET_FEATURE_STATE(DualSourceBlend,           strstr(Extensions, "blend_func_extended"), "Dual source blend");
        SET_FEATURE_STATE(MultiViewport,             strstr(Extensions, "viewport_array"),      "Multi viewport");
        SET_FEATURE_STATE(PixelUAVWritesAndAtomics,  IsGLES31OrAbove || strstr(Extensions, "shader_image_load_store"), "Pixel UAV writes and atomics");
        SET_FEATURE_STATE(TextureUAVExtendedFormats, false, "Texture UAV extended formats");

        SET_FEATURE_STATE(ShaderFloat16,             strstr(Extensions, "shader_explicit_arithmetic_types_float16"), "16-bit float shader operations are");
        SET_FEATURE_STATE(ResourceBuffer16BitAccess, strstr(Extensions, "shader_16bit_storage"),                     "16-bit resoure buffer access is");
        SET_FEATURE_STATE(UniformBuffer16BitAccess,  strstr(Extensions, "shader_16bit_storage"),                     "16-bit uniform buffer access is");
        SET_FEATURE_STATE(ShaderInputOutput16,       false,                                                          "16-bit shader inputs/outputs are");
        SET_FEATURE_STATE(ShaderInt8,                strstr(Extensions, "shader_explicit_arithmetic_types_int8"),    "8-bit integer shader operations are");
        SET_FEATURE_STATE(ResourceBuffer8BitAccess,  strstr(Extensions, "shader_8bit_storage"),                      "8-bit resoure buffer access is");
        SET_FEATURE_STATE(UniformBuffer8BitAccess,   strstr(Extensions, "shader_8bit_storage"),                      "8-bit uniform buffer access is");
        // clang-format on

        TexCaps.MaxTexture1DDimension     = 0; // Not supported in GLES 3.2
        TexCaps.MaxTexture1DArraySlices   = 0; // Not supported in GLES 3.2
        TexCaps.MaxTexture2DDimension     = MaxTextureSize;
        TexCaps.MaxTexture2DArraySlices   = MaxLayers;
        TexCaps.MaxTexture3DDimension     = Max3DTextureSize;
        TexCaps.MaxTextureCubeDimension   = MaxCubeTextureSize;
        TexCaps.Texture2DMSSupported      = IsGLES31OrAbove || strstr(Extensions, "texture_storage_multisample");
        TexCaps.Texture2DMSArraySupported = IsGLES32OrAbove || strstr(Extensions, "texture_storage_multisample_2d_array");
        TexCaps.TextureViewSupported      = IsGLES31OrAbove || strstr(Extensions, "texture_view");
        TexCaps.CubemapArraysSupported    = IsGLES32OrAbove || strstr(Extensions, "texture_cube_map_array");

        SamCaps.BorderSamplingModeSupported   = GL_TEXTURE_BORDER_COLOR && (IsGLES32OrAbove || strstr(Extensions, "texture_border_clamp"));
        SamCaps.AnisotropicFilteringSupported = GL_TEXTURE_MAX_ANISOTROPY_EXT && strstr(Extensions, "texture_filter_anisotropic");
        SamCaps.LODBiasSupported              = GL_TEXTURE_LOD_BIAS && IsGLES31OrAbove;
    }

    const bool bRGTC = CheckExtension("GL_ARB_texture_compression_rgtc");
    const bool bBPTC = CheckExtension("GL_ARB_texture_compression_bptc");
    const bool bS3TC = CheckExtension("GL_EXT_texture_compression_s3tc");

    SET_FEATURE_STATE(TextureCompressionBC, bRGTC && bBPTC && bS3TC, "BC texture compression is");

#undef SET_FEATURE_STATE

#if defined(_MSC_VER) && defined(_WIN64)
    static_assert(sizeof(DeviceFeatures) == 31, "Did you add a new feature to DeviceFeatures? Please handle its satus here.");
#endif
}

RenderDeviceGLImpl::~RenderDeviceGLImpl()
{
}

IMPLEMENT_QUERY_INTERFACE(RenderDeviceGLImpl, IID_RenderDeviceGL, TRenderDeviceBase)

void RenderDeviceGLImpl::InitTexRegionRender()
{
    m_pTexRegionRender.reset(new TexRegionRender(this));
}

void RenderDeviceGLImpl::CreateBuffer(const BufferDesc& BuffDesc, const BufferData* pBuffData, IBuffer** ppBuffer, bool bIsDeviceInternal)
{
    CreateDeviceObject(
        "buffer", BuffDesc, ppBuffer,
        [&]() //
        {
            auto spDeviceContext = GetImmediateContext();
            VERIFY(spDeviceContext, "Immediate device context has been destroyed");
            auto* pDeviceContextGL = spDeviceContext.RawPtr<DeviceContextGLImpl>();

            BufferGLImpl* pBufferOGL(NEW_RC_OBJ(m_BufObjAllocator, "BufferGLImpl instance", BufferGLImpl)(m_BuffViewObjAllocator, this, pDeviceContextGL->GetContextState(), BuffDesc, pBuffData, bIsDeviceInternal));
            pBufferOGL->QueryInterface(IID_Buffer, reinterpret_cast<IObject**>(ppBuffer));
            pBufferOGL->CreateDefaultViews();
            OnCreateDeviceObject(pBufferOGL);
        } //
    );
}

void RenderDeviceGLImpl::CreateBuffer(const BufferDesc& BuffDesc, const BufferData* BuffData, IBuffer** ppBuffer)
{
    CreateBuffer(BuffDesc, BuffData, ppBuffer, false);
}

void RenderDeviceGLImpl::CreateBufferFromGLHandle(Uint32 GLHandle, const BufferDesc& BuffDesc, RESOURCE_STATE InitialState, IBuffer** ppBuffer)
{
    VERIFY(GLHandle, "GL buffer handle must not be null");
    CreateDeviceObject(
        "buffer", BuffDesc, ppBuffer,
        [&]() //
        {
            auto spDeviceContext = GetImmediateContext();
            VERIFY(spDeviceContext, "Immediate device context has been destroyed");
            auto* pDeviceContextGL = spDeviceContext.RawPtr<DeviceContextGLImpl>();

            BufferGLImpl* pBufferOGL(NEW_RC_OBJ(m_BufObjAllocator, "BufferGLImpl instance", BufferGLImpl)(m_BuffViewObjAllocator, this, pDeviceContextGL->GetContextState(), BuffDesc, GLHandle, false));
            pBufferOGL->QueryInterface(IID_Buffer, reinterpret_cast<IObject**>(ppBuffer));
            pBufferOGL->CreateDefaultViews();
            OnCreateDeviceObject(pBufferOGL);
        } //
    );
}

void RenderDeviceGLImpl::CreateShader(const ShaderCreateInfo& ShaderCreateInfo, IShader** ppShader, bool bIsDeviceInternal)
{
    CreateDeviceObject(
        "shader", ShaderCreateInfo.Desc, ppShader,
        [&]() //
        {
            ShaderGLImpl* pShaderOGL(NEW_RC_OBJ(m_ShaderObjAllocator, "ShaderGLImpl instance", ShaderGLImpl)(this, ShaderCreateInfo, bIsDeviceInternal));
            pShaderOGL->QueryInterface(IID_Shader, reinterpret_cast<IObject**>(ppShader));

            OnCreateDeviceObject(pShaderOGL);
        } //
    );
}

void RenderDeviceGLImpl::CreateShader(const ShaderCreateInfo& ShaderCreateInfo, IShader** ppShader)
{
    CreateShader(ShaderCreateInfo, ppShader, false);
}

void RenderDeviceGLImpl::CreateTexture(const TextureDesc& TexDesc, const TextureData* pData, ITexture** ppTexture, bool bIsDeviceInternal)
{
    CreateDeviceObject(
        "texture", TexDesc, ppTexture,
        [&]() //
        {
            auto spDeviceContext = GetImmediateContext();
            VERIFY(spDeviceContext, "Immediate device context has been destroyed");
            auto& GLState = spDeviceContext.RawPtr<DeviceContextGLImpl>()->GetContextState();

            const auto& FmtInfo = GetTextureFormatInfo(TexDesc.Format);
            if (!FmtInfo.Supported)
            {
                LOG_ERROR_AND_THROW(FmtInfo.Name, " is not supported texture format");
            }

            TextureBaseGL* pTextureOGL = nullptr;
            switch (TexDesc.Type)
            {
                case RESOURCE_DIM_TEX_1D:
                    pTextureOGL = NEW_RC_OBJ(m_TexObjAllocator, "Texture1D_OGL instance", Texture1D_OGL)(m_TexViewObjAllocator, this, GLState, TexDesc, pData, bIsDeviceInternal);
                    break;

                case RESOURCE_DIM_TEX_1D_ARRAY:
                    pTextureOGL = NEW_RC_OBJ(m_TexObjAllocator, "Texture1DArray_OGL instance", Texture1DArray_OGL)(m_TexViewObjAllocator, this, GLState, TexDesc, pData, bIsDeviceInternal);
                    break;

                case RESOURCE_DIM_TEX_2D:
                    pTextureOGL = NEW_RC_OBJ(m_TexObjAllocator, "Texture2D_OGL instance", Texture2D_OGL)(m_TexViewObjAllocator, this, GLState, TexDesc, pData, bIsDeviceInternal);
                    break;

                case RESOURCE_DIM_TEX_2D_ARRAY:
                    pTextureOGL = NEW_RC_OBJ(m_TexObjAllocator, "Texture2DArray_OGL instance", Texture2DArray_OGL)(m_TexViewObjAllocator, this, GLState, TexDesc, pData, bIsDeviceInternal);
                    break;

                case RESOURCE_DIM_TEX_3D:
                    pTextureOGL = NEW_RC_OBJ(m_TexObjAllocator, "Texture3D_OGL instance", Texture3D_OGL)(m_TexViewObjAllocator, this, GLState, TexDesc, pData, bIsDeviceInternal);
                    break;

                case RESOURCE_DIM_TEX_CUBE:
                    pTextureOGL = NEW_RC_OBJ(m_TexObjAllocator, "TextureCube_OGL instance", TextureCube_OGL)(m_TexViewObjAllocator, this, GLState, TexDesc, pData, bIsDeviceInternal);
                    break;

                case RESOURCE_DIM_TEX_CUBE_ARRAY:
                    pTextureOGL = NEW_RC_OBJ(m_TexObjAllocator, "TextureCubeArray_OGL instance", TextureCubeArray_OGL)(m_TexViewObjAllocator, this, GLState, TexDesc, pData, bIsDeviceInternal);
                    break;

                default: LOG_ERROR_AND_THROW("Unknown texture type. (Did you forget to initialize the Type member of TextureDesc structure?)");
            }

            pTextureOGL->QueryInterface(IID_Texture, reinterpret_cast<IObject**>(ppTexture));
            pTextureOGL->CreateDefaultViews();
            OnCreateDeviceObject(pTextureOGL);
        } //
    );
}

void RenderDeviceGLImpl::CreateTexture(const TextureDesc& TexDesc, const TextureData* Data, ITexture** ppTexture)
{
    CreateTexture(TexDesc, Data, ppTexture, false);
}

void RenderDeviceGLImpl::CreateTextureFromGLHandle(Uint32             GLHandle,
                                                   Uint32             GLBindTarget,
                                                   const TextureDesc& TexDesc,
                                                   RESOURCE_STATE     InitialState,
                                                   ITexture**         ppTexture)
{
    VERIFY(GLHandle, "GL texture handle must not be null");
    CreateDeviceObject(
        "texture", TexDesc, ppTexture,
        [&]() //
        {
            auto spDeviceContext = GetImmediateContext();
            VERIFY(spDeviceContext, "Immediate device context has been destroyed");
            auto& GLState = spDeviceContext.RawPtr<DeviceContextGLImpl>()->GetContextState();

            TextureBaseGL* pTextureOGL = nullptr;
            switch (TexDesc.Type)
            {
                case RESOURCE_DIM_TEX_1D:
                    pTextureOGL = NEW_RC_OBJ(m_TexObjAllocator, "Texture1D_OGL instance", Texture1D_OGL)(m_TexViewObjAllocator, this, GLState, TexDesc, GLHandle, GLBindTarget);
                    break;

                case RESOURCE_DIM_TEX_1D_ARRAY:
                    pTextureOGL = NEW_RC_OBJ(m_TexObjAllocator, "Texture1DArray_OGL instance", Texture1DArray_OGL)(m_TexViewObjAllocator, this, GLState, TexDesc, GLHandle, GLBindTarget);
                    break;

                case RESOURCE_DIM_TEX_2D:
                    pTextureOGL = NEW_RC_OBJ(m_TexObjAllocator, "Texture2D_OGL instance", Texture2D_OGL)(m_TexViewObjAllocator, this, GLState, TexDesc, GLHandle, GLBindTarget);
                    break;

                case RESOURCE_DIM_TEX_2D_ARRAY:
                    pTextureOGL = NEW_RC_OBJ(m_TexObjAllocator, "Texture2DArray_OGL instance", Texture2DArray_OGL)(m_TexViewObjAllocator, this, GLState, TexDesc, GLHandle, GLBindTarget);
                    break;

                case RESOURCE_DIM_TEX_3D:
                    pTextureOGL = NEW_RC_OBJ(m_TexObjAllocator, "Texture3D_OGL instance", Texture3D_OGL)(m_TexViewObjAllocator, this, GLState, TexDesc, GLHandle, GLBindTarget);
                    break;

                case RESOURCE_DIM_TEX_CUBE:
                    pTextureOGL = NEW_RC_OBJ(m_TexObjAllocator, "TextureCube_OGL instance", TextureCube_OGL)(m_TexViewObjAllocator, this, GLState, TexDesc, GLHandle, GLBindTarget);
                    break;

                case RESOURCE_DIM_TEX_CUBE_ARRAY:
                    pTextureOGL = NEW_RC_OBJ(m_TexObjAllocator, "TextureCubeArray_OGL instance", TextureCubeArray_OGL)(m_TexViewObjAllocator, this, GLState, TexDesc, GLHandle, GLBindTarget);
                    break;

                default: LOG_ERROR_AND_THROW("Unknown texture type. (Did you forget to initialize the Type member of TextureDesc structure?)");
            }

            pTextureOGL->QueryInterface(IID_Texture, reinterpret_cast<IObject**>(ppTexture));
            pTextureOGL->CreateDefaultViews();
            OnCreateDeviceObject(pTextureOGL);
        } //
    );
}

void RenderDeviceGLImpl::CreateDummyTexture(const TextureDesc& TexDesc, RESOURCE_STATE InitialState, ITexture** ppTexture)
{
    CreateDeviceObject(
        "texture", TexDesc, ppTexture,
        [&]() //
        {
            TextureBaseGL* pTextureOGL = nullptr;
            switch (TexDesc.Type)
            {
                case RESOURCE_DIM_TEX_2D:
                    pTextureOGL = NEW_RC_OBJ(m_TexObjAllocator, "Dummy Texture2D_OGL instance", Texture2D_OGL)(m_TexViewObjAllocator, this, TexDesc);
                    break;

                default: LOG_ERROR_AND_THROW("Unsupported texture type.");
            }

            pTextureOGL->QueryInterface(IID_Texture, reinterpret_cast<IObject**>(ppTexture));
            pTextureOGL->CreateDefaultViews();
            OnCreateDeviceObject(pTextureOGL);
        } //
    );
}

void RenderDeviceGLImpl::CreateSampler(const SamplerDesc& SamplerDesc, ISampler** ppSampler, bool bIsDeviceInternal)
{
    CreateDeviceObject(
        "sampler", SamplerDesc, ppSampler,
        [&]() //
        {
            m_SamplersRegistry.Find(SamplerDesc, reinterpret_cast<IDeviceObject**>(ppSampler));
            if (*ppSampler == nullptr)
            {
                SamplerGLImpl* pSamplerOGL(NEW_RC_OBJ(m_SamplerObjAllocator, "SamplerGLImpl instance", SamplerGLImpl)(this, SamplerDesc, bIsDeviceInternal));
                pSamplerOGL->QueryInterface(IID_Sampler, reinterpret_cast<IObject**>(ppSampler));
                OnCreateDeviceObject(pSamplerOGL);
                m_SamplersRegistry.Add(SamplerDesc, *ppSampler);
            }
        } //
    );
}

void RenderDeviceGLImpl::CreateSampler(const SamplerDesc& SamplerDesc, ISampler** ppSampler)
{
    CreateSampler(SamplerDesc, ppSampler, false);
}

template <typename PSOCreateInfoType>
void RenderDeviceGLImpl::CreatePipelineState(const PSOCreateInfoType& PSOCreateInfo, IPipelineState** ppPipelineState, bool bIsDeviceInternal)
{
    CreateDeviceObject(
        "Pipeline state", PSOCreateInfo.PSODesc, ppPipelineState,
        [&]() //
        {
            PipelineStateGLImpl* pPipelineStateOGL(NEW_RC_OBJ(m_PSOAllocator, "PipelineStateGLImpl instance", PipelineStateGLImpl)(this, PSOCreateInfo, bIsDeviceInternal));
            pPipelineStateOGL->QueryInterface(IID_PipelineState, reinterpret_cast<IObject**>(ppPipelineState));
            OnCreateDeviceObject(pPipelineStateOGL);
        } //
    );
}

void RenderDeviceGLImpl::CreateGraphicsPipelineState(const GraphicsPipelineStateCreateInfo& PSOCreateInfo, IPipelineState** ppPipelineState, bool bIsDeviceInternal)
{
    CreatePipelineState(PSOCreateInfo, ppPipelineState, bIsDeviceInternal);
}

void RenderDeviceGLImpl::CreateComputePipelineState(const ComputePipelineStateCreateInfo& PSOCreateInfo, IPipelineState** ppPipelineState, bool bIsDeviceInternal)
{
    CreatePipelineState(PSOCreateInfo, ppPipelineState, bIsDeviceInternal);
}

void RenderDeviceGLImpl::CreateGraphicsPipelineState(const GraphicsPipelineStateCreateInfo& PSOCreateInfo, IPipelineState** ppPipelineState)
{
    return CreateGraphicsPipelineState(PSOCreateInfo, ppPipelineState, false);
}

void RenderDeviceGLImpl::CreateComputePipelineState(const ComputePipelineStateCreateInfo& PSOCreateInfo, IPipelineState** ppPipelineState)
{
    return CreateComputePipelineState(PSOCreateInfo, ppPipelineState, false);
}

void RenderDeviceGLImpl::CreateFence(const FenceDesc& Desc, IFence** ppFence)
{
    CreateDeviceObject(
        "Fence", Desc, ppFence,
        [&]() //
        {
            FenceGLImpl* pFenceOGL(NEW_RC_OBJ(m_FenceAllocator, "FenceGLImpl instance", FenceGLImpl)(this, Desc));
            pFenceOGL->QueryInterface(IID_Fence, reinterpret_cast<IObject**>(ppFence));
            OnCreateDeviceObject(pFenceOGL);
        } //
    );
}

void RenderDeviceGLImpl::CreateQuery(const QueryDesc& Desc, IQuery** ppQuery)
{
    CreateDeviceObject(
        "Query", Desc, ppQuery,
        [&]() //
        {
            QueryGLImpl* pQueryOGL(NEW_RC_OBJ(m_QueryAllocator, "QueryGLImpl instance", QueryGLImpl)(this, Desc));
            pQueryOGL->QueryInterface(IID_Query, reinterpret_cast<IObject**>(ppQuery));
            OnCreateDeviceObject(pQueryOGL);
        } //
    );
}

void RenderDeviceGLImpl::CreateRenderPass(const RenderPassDesc& Desc, IRenderPass** ppRenderPass)
{
    CreateDeviceObject(
        "RenderPass", Desc, ppRenderPass,
        [&]() //
        {
            RenderPassGLImpl* pRenderPassOGL(NEW_RC_OBJ(m_RenderPassAllocator, "RenderPassGLImpl instance", RenderPassGLImpl)(this, Desc));
            pRenderPassOGL->QueryInterface(IID_RenderPass, reinterpret_cast<IObject**>(ppRenderPass));
            OnCreateDeviceObject(pRenderPassOGL);
        } //
    );
}

void RenderDeviceGLImpl::CreateFramebuffer(const FramebufferDesc& Desc, IFramebuffer** ppFramebuffer)
{
    CreateDeviceObject("Framebuffer", Desc, ppFramebuffer,
                       [&]() //
                       {
                           auto spDeviceContext = GetImmediateContext();
                           VERIFY(spDeviceContext, "Immediate device context has been destroyed");
                           auto& GLState = spDeviceContext.RawPtr<DeviceContextGLImpl>()->GetContextState();

                           FramebufferGLImpl* pFramebufferGL(NEW_RC_OBJ(m_FramebufferAllocator, "FramebufferGLImpl instance", FramebufferGLImpl)(this, GLState, Desc));
                           pFramebufferGL->QueryInterface(IID_Framebuffer, reinterpret_cast<IObject**>(ppFramebuffer));
                           OnCreateDeviceObject(pFramebufferGL);
                       });
}

bool RenderDeviceGLImpl::CheckExtension(const Char* ExtensionString)
{
    return m_ExtensionStrings.find(ExtensionString) != m_ExtensionStrings.end();
}

void RenderDeviceGLImpl::FlagSupportedTexFormats()
{
    const auto& DeviceCaps   = GetDeviceCaps();
    bool        bGL33OrAbove = DeviceCaps.DevType == RENDER_DEVICE_TYPE_GL &&
        (DeviceCaps.MajorVersion >= 4 || (DeviceCaps.MajorVersion == 3 && DeviceCaps.MinorVersion >= 3));

    bool bRGTC      = CheckExtension("GL_ARB_texture_compression_rgtc");
    bool bBPTC      = CheckExtension("GL_ARB_texture_compression_bptc");
    bool bS3TC      = CheckExtension("GL_EXT_texture_compression_s3tc");
    bool bTexNorm16 = CheckExtension("GL_EXT_texture_norm16"); // Only for ES3.1+

#define FLAG_FORMAT(Fmt, IsSupported) \
    m_TextureFormatsInfo[Fmt].Supported = IsSupported

    // The formats marked by true below are required in GL 3.3+ and GLES 3.0+
    // Note that GLES2.0 does not specify any required formats

    // clang-format off
    FLAG_FORMAT(TEX_FORMAT_RGBA32_TYPELESS,            true       );
    FLAG_FORMAT(TEX_FORMAT_RGBA32_FLOAT,               true       );
    FLAG_FORMAT(TEX_FORMAT_RGBA32_UINT,                true       );
    FLAG_FORMAT(TEX_FORMAT_RGBA32_SINT,                true       );
    FLAG_FORMAT(TEX_FORMAT_RGB32_TYPELESS,             true       );
    FLAG_FORMAT(TEX_FORMAT_RGB32_FLOAT,                true       );
    FLAG_FORMAT(TEX_FORMAT_RGB32_UINT,                 true       );
    FLAG_FORMAT(TEX_FORMAT_RGB32_SINT,                 true       );
    FLAG_FORMAT(TEX_FORMAT_RGBA16_TYPELESS,            true       );
    FLAG_FORMAT(TEX_FORMAT_RGBA16_FLOAT,               true       );
    FLAG_FORMAT(TEX_FORMAT_RGBA16_UNORM,               bGL33OrAbove || bTexNorm16 );
    FLAG_FORMAT(TEX_FORMAT_RGBA16_UINT,                true         );
    FLAG_FORMAT(TEX_FORMAT_RGBA16_SNORM,               bGL33OrAbove || bTexNorm16 );
    FLAG_FORMAT(TEX_FORMAT_RGBA16_SINT,                true         );
    FLAG_FORMAT(TEX_FORMAT_RG32_TYPELESS,              true       );
    FLAG_FORMAT(TEX_FORMAT_RG32_FLOAT,                 true       );
    FLAG_FORMAT(TEX_FORMAT_RG32_UINT,                  true       );
    FLAG_FORMAT(TEX_FORMAT_RG32_SINT,                  true       );
    FLAG_FORMAT(TEX_FORMAT_R32G8X24_TYPELESS,          true       );
    FLAG_FORMAT(TEX_FORMAT_D32_FLOAT_S8X24_UINT,       true       );
    FLAG_FORMAT(TEX_FORMAT_R32_FLOAT_X8X24_TYPELESS,   true       );
    FLAG_FORMAT(TEX_FORMAT_X32_TYPELESS_G8X24_UINT,    false      );
    FLAG_FORMAT(TEX_FORMAT_RGB10A2_TYPELESS,           true       );
    FLAG_FORMAT(TEX_FORMAT_RGB10A2_UNORM,              true       );
    FLAG_FORMAT(TEX_FORMAT_RGB10A2_UINT,               true       );
    FLAG_FORMAT(TEX_FORMAT_R11G11B10_FLOAT,            true       );
    FLAG_FORMAT(TEX_FORMAT_RGBA8_TYPELESS,             true       );
    FLAG_FORMAT(TEX_FORMAT_RGBA8_UNORM,                true       );
    FLAG_FORMAT(TEX_FORMAT_RGBA8_UNORM_SRGB,           true       );
    FLAG_FORMAT(TEX_FORMAT_RGBA8_UINT,                 true       );
    FLAG_FORMAT(TEX_FORMAT_RGBA8_SNORM,                true       );
    FLAG_FORMAT(TEX_FORMAT_RGBA8_SINT,                 true       );
    FLAG_FORMAT(TEX_FORMAT_RG16_TYPELESS,              true       );
    FLAG_FORMAT(TEX_FORMAT_RG16_FLOAT,                 true       );
    FLAG_FORMAT(TEX_FORMAT_RG16_UNORM,                 bGL33OrAbove || bTexNorm16 );
    FLAG_FORMAT(TEX_FORMAT_RG16_UINT,                  true         );
    FLAG_FORMAT(TEX_FORMAT_RG16_SNORM,                 bGL33OrAbove || bTexNorm16 );
    FLAG_FORMAT(TEX_FORMAT_RG16_SINT,                  true         );
    FLAG_FORMAT(TEX_FORMAT_R32_TYPELESS,               true       );
    FLAG_FORMAT(TEX_FORMAT_D32_FLOAT,                  true       );
    FLAG_FORMAT(TEX_FORMAT_R32_FLOAT,                  true       );
    FLAG_FORMAT(TEX_FORMAT_R32_UINT,                   true       );
    FLAG_FORMAT(TEX_FORMAT_R32_SINT,                   true       );
    FLAG_FORMAT(TEX_FORMAT_R24G8_TYPELESS,             true       );
    FLAG_FORMAT(TEX_FORMAT_D24_UNORM_S8_UINT,          true       );
    FLAG_FORMAT(TEX_FORMAT_R24_UNORM_X8_TYPELESS,      true       );
    FLAG_FORMAT(TEX_FORMAT_X24_TYPELESS_G8_UINT,       false      );
    FLAG_FORMAT(TEX_FORMAT_RG8_TYPELESS,               true       );
    FLAG_FORMAT(TEX_FORMAT_RG8_UNORM,                  true       );
    FLAG_FORMAT(TEX_FORMAT_RG8_UINT,                   true       );
    FLAG_FORMAT(TEX_FORMAT_RG8_SNORM,                  true       );
    FLAG_FORMAT(TEX_FORMAT_RG8_SINT,                   true       );
    FLAG_FORMAT(TEX_FORMAT_R16_TYPELESS,               true       );
    FLAG_FORMAT(TEX_FORMAT_R16_FLOAT,                  true       );
    FLAG_FORMAT(TEX_FORMAT_D16_UNORM,                  true       );
    FLAG_FORMAT(TEX_FORMAT_R16_UNORM,                  bGL33OrAbove || bTexNorm16 );
    FLAG_FORMAT(TEX_FORMAT_R16_UINT,                   true       );
    FLAG_FORMAT(TEX_FORMAT_R16_SNORM,                  bGL33OrAbove || bTexNorm16 );
    FLAG_FORMAT(TEX_FORMAT_R16_SINT,                   true       );
    FLAG_FORMAT(TEX_FORMAT_R8_TYPELESS,                true       );
    FLAG_FORMAT(TEX_FORMAT_R8_UNORM,                   true       );
    FLAG_FORMAT(TEX_FORMAT_R8_UINT,                    true       );
    FLAG_FORMAT(TEX_FORMAT_R8_SNORM,                   true       );
    FLAG_FORMAT(TEX_FORMAT_R8_SINT,                    true       );
    FLAG_FORMAT(TEX_FORMAT_A8_UNORM,                   false      ); // Not supported in OpenGL
    FLAG_FORMAT(TEX_FORMAT_R1_UNORM,                   false      ); // Not supported in OpenGL
    FLAG_FORMAT(TEX_FORMAT_RGB9E5_SHAREDEXP,           true       );
    FLAG_FORMAT(TEX_FORMAT_RG8_B8G8_UNORM,             false      ); // Not supported in OpenGL
    FLAG_FORMAT(TEX_FORMAT_G8R8_G8B8_UNORM,            false      ); // Not supported in OpenGL

    FLAG_FORMAT(TEX_FORMAT_BC1_TYPELESS,               bS3TC );
    FLAG_FORMAT(TEX_FORMAT_BC1_UNORM,                  bS3TC );
    FLAG_FORMAT(TEX_FORMAT_BC1_UNORM_SRGB,             bS3TC );
    FLAG_FORMAT(TEX_FORMAT_BC2_TYPELESS,               bS3TC );
    FLAG_FORMAT(TEX_FORMAT_BC2_UNORM,                  bS3TC );
    FLAG_FORMAT(TEX_FORMAT_BC2_UNORM_SRGB,             bS3TC );
    FLAG_FORMAT(TEX_FORMAT_BC3_TYPELESS,               bS3TC );
    FLAG_FORMAT(TEX_FORMAT_BC3_UNORM,                  bS3TC );
    FLAG_FORMAT(TEX_FORMAT_BC3_UNORM_SRGB,             bS3TC );

    FLAG_FORMAT(TEX_FORMAT_BC4_TYPELESS,               bRGTC );
    FLAG_FORMAT(TEX_FORMAT_BC4_UNORM,                  bRGTC );
    FLAG_FORMAT(TEX_FORMAT_BC4_SNORM,                  bRGTC );
    FLAG_FORMAT(TEX_FORMAT_BC5_TYPELESS,               bRGTC );
    FLAG_FORMAT(TEX_FORMAT_BC5_UNORM,                  bRGTC );
    FLAG_FORMAT(TEX_FORMAT_BC5_SNORM,                  bRGTC );

    FLAG_FORMAT(TEX_FORMAT_B5G6R5_UNORM,               false       ); // Not supported in OpenGL
    FLAG_FORMAT(TEX_FORMAT_B5G5R5A1_UNORM,             false       ); // Not supported in OpenGL
    FLAG_FORMAT(TEX_FORMAT_BGRA8_UNORM,                false       ); // Not supported in OpenGL
    FLAG_FORMAT(TEX_FORMAT_BGRX8_UNORM,                false       ); // Not supported in OpenGL
    FLAG_FORMAT(TEX_FORMAT_R10G10B10_XR_BIAS_A2_UNORM, false       ); // Not supported in OpenGL
    FLAG_FORMAT(TEX_FORMAT_BGRA8_TYPELESS,             false       ); // Not supported in OpenGL
    FLAG_FORMAT(TEX_FORMAT_BGRA8_UNORM_SRGB,           false       ); // Not supported in OpenGL
    FLAG_FORMAT(TEX_FORMAT_BGRX8_TYPELESS,             false       ); // Not supported in OpenGL
    FLAG_FORMAT(TEX_FORMAT_BGRX8_UNORM_SRGB,           false       ); // Not supported in OpenGL
    
    FLAG_FORMAT(TEX_FORMAT_BC6H_TYPELESS,              bBPTC );
    FLAG_FORMAT(TEX_FORMAT_BC6H_UF16,                  bBPTC );
    FLAG_FORMAT(TEX_FORMAT_BC6H_SF16,                  bBPTC );
    FLAG_FORMAT(TEX_FORMAT_BC7_TYPELESS,               bBPTC );
    FLAG_FORMAT(TEX_FORMAT_BC7_UNORM,                  bBPTC );
    FLAG_FORMAT(TEX_FORMAT_BC7_UNORM_SRGB,             bBPTC );
    // clang-format on

#ifdef DILIGENT_DEBUG
    bool bGL43OrAbove = DeviceCaps.DevType == RENDER_DEVICE_TYPE_GL &&
        (DeviceCaps.MajorVersion >= 5 || (DeviceCaps.MajorVersion == 4 && DeviceCaps.MinorVersion >= 3));

    constexpr int      TestTextureDim = 8;
    constexpr int      MaxTexelSize   = 16;
    std::vector<Uint8> ZeroData(TestTextureDim * TestTextureDim * MaxTexelSize);

    // Go through all formats and try to create small 2D texture to check if the format is supported
    for (auto FmtInfo = m_TextureFormatsInfo.begin(); FmtInfo != m_TextureFormatsInfo.end(); ++FmtInfo)
    {
        if (FmtInfo->Format == TEX_FORMAT_UNKNOWN)
            continue;

        auto GLFmt = TexFormatToGLInternalTexFormat(FmtInfo->Format);
        if (GLFmt == 0)
        {
            VERIFY(!FmtInfo->Supported, "Format should be marked as unsupported");
            continue;
        }

#    if GL_ARB_internalformat_query2
        // Only works on GL4.3+
        if (bGL43OrAbove)
        {
            GLint params = 0;
            glGetInternalformativ(GL_TEXTURE_2D, GLFmt, GL_INTERNALFORMAT_SUPPORTED, 1, &params);
            CHECK_GL_ERROR("glGetInternalformativ() failed");
            VERIFY(FmtInfo->Supported == (params == GL_TRUE), "This internal format should be supported");
        }
#    endif

        // Check that the format is indeed supported
        if (FmtInfo->Supported)
        {
            GLObjectWrappers::GLTextureObj TestGLTex(true);
            // Immediate context has not been created yet, so use raw GL functions
            glBindTexture(GL_TEXTURE_2D, TestGLTex);
            CHECK_GL_ERROR("Failed to bind texture");
            glTexStorage2D(GL_TEXTURE_2D, 1, GLFmt, TestTextureDim, TestTextureDim);
            if (glGetError() == GL_NO_ERROR)
            {
                // It turned out it is not enough to only allocate texture storage
                // For some reason glTexStorage2D() may succeed, but upload operation
                // will later fail. So we need to additionally try to upload some
                // data to the texture
                const auto& TransferAttribs = GetNativePixelTransferAttribs(FmtInfo->Format);
                if (TransferAttribs.IsCompressed)
                {
                    const auto& FmtAttribs = GetTextureFormatAttribs(FmtInfo->Format);
                    static_assert((TestTextureDim & (TestTextureDim - 1)) == 0, "Test texture dim must be power of two!");
                    auto BlockBytesInRow = (TestTextureDim / int{FmtAttribs.BlockWidth}) * int{FmtAttribs.ComponentSize};
                    glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, // mip level
                                              0, 0, TestTextureDim, TestTextureDim,
                                              GLFmt,
                                              (TestTextureDim / int{FmtAttribs.BlockHeight}) * BlockBytesInRow,
                                              ZeroData.data());
                }
                else
                {
                    glTexSubImage2D(GL_TEXTURE_2D, 0, // mip level
                                    0, 0, TestTextureDim, TestTextureDim,
                                    TransferAttribs.PixelFormat, TransferAttribs.DataType,
                                    ZeroData.data());
                }

                if (glGetError() != GL_NO_ERROR)
                {
                    LOG_WARNING_MESSAGE("Failed to upload data to a test ", TestTextureDim, "x", TestTextureDim, " ", FmtInfo->Name, " texture. "
                                                                                                                                     "This likely indicates that the format is not supported despite being reported so by the device.");
                    FmtInfo->Supported = false;
                }
            }
            else
            {
                LOG_WARNING_MESSAGE("Failed to allocate storage for a test ", TestTextureDim, "x", TestTextureDim, " ", FmtInfo->Name, " texture. "
                                                                                                                                       "This likely indicates that the format is not supported despite being reported so by the device.");
                FmtInfo->Supported = false;
            }
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }
#endif
}

template <typename CreateFuncType>
bool CreateTestGLTexture(GLContextState& GlCtxState, GLenum BindTarget, const GLObjectWrappers::GLTextureObj& GLTexObj, CreateFuncType CreateFunc)
{
    GlCtxState.BindTexture(-1, BindTarget, GLTexObj);
    CreateFunc();
    bool bSuccess = glGetError() == GL_NO_ERROR;
    GlCtxState.BindTexture(-1, BindTarget, GLObjectWrappers::GLTextureObj(false));
    return bSuccess;
}

template <typename CreateFuncType>
bool CreateTestGLTexture(GLContextState& GlCtxState, GLenum BindTarget, CreateFuncType CreateFunc)
{
    GLObjectWrappers::GLTextureObj GLTexObj{true};
    return CreateTestGLTexture(GlCtxState, BindTarget, GLTexObj, CreateFunc);
}

void RenderDeviceGLImpl::TestTextureFormat(TEXTURE_FORMAT TexFormat)
{
    auto& TexFormatInfo = m_TextureFormatsInfo[TexFormat];
    VERIFY(TexFormatInfo.Supported, "Texture format is not supported");

    auto GLFmt = TexFormatToGLInternalTexFormat(TexFormat);
    VERIFY(GLFmt != 0, "Incorrect internal GL format");

    auto spDeviceContext = GetImmediateContext();
    VERIFY(spDeviceContext, "Immediate device context has been destroyed");
    auto* pContextGL   = spDeviceContext.RawPtr<DeviceContextGLImpl>();
    auto& ContextState = pContextGL->GetContextState();

    const int TestTextureDim   = 32;
    const int TestArraySlices  = 8;
    const int TestTextureDepth = 8;

    TexFormatInfo.BindFlags  = BIND_SHADER_RESOURCE;
    TexFormatInfo.Dimensions = RESOURCE_DIMENSION_SUPPORT_NONE;

    // Disable debug messages - errors are exepcted
    m_ShowDebugGLOutput = 0;

    // Create test texture 1D
    if (m_DeviceCaps.TexCaps.MaxTexture1DDimension != 0 &&
        TexFormatInfo.ComponentType != COMPONENT_TYPE_COMPRESSED)
    {
        if (CreateTestGLTexture(ContextState, GL_TEXTURE_1D,
                                [&]() //
                                {
                                    glTexStorage1D(GL_TEXTURE_1D, 1, GLFmt, TestTextureDim);
                                }))

        {
            TexFormatInfo.Dimensions |= RESOURCE_DIMENSION_SUPPORT_TEX_1D;

            if (CreateTestGLTexture(ContextState, GL_TEXTURE_1D_ARRAY,
                                    [&]() //
                                    {
                                        glTexStorage2D(GL_TEXTURE_1D_ARRAY, 1, GLFmt, TestTextureDim, TestArraySlices);
                                    }))

            {
                TexFormatInfo.Dimensions |= RESOURCE_DIMENSION_SUPPORT_TEX_1D_ARRAY;
            }
        }
    }

    // Create test texture 2D
    {
        GLObjectWrappers::GLTextureObj TestGLTex2D{true};
        if (CreateTestGLTexture(ContextState, GL_TEXTURE_2D, TestGLTex2D,
                                [&]() //
                                {
                                    glTexStorage2D(GL_TEXTURE_2D, 1, GLFmt, TestTextureDim, TestTextureDim);
                                }))
        {
            TexFormatInfo.Dimensions |= RESOURCE_DIMENSION_SUPPORT_TEX_2D;

            if (CreateTestGLTexture(ContextState, GL_TEXTURE_2D_ARRAY,
                                    [&]() //
                                    {
                                        glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GLFmt, TestTextureDim, TestTextureDim, TestArraySlices);
                                    }))
            {
                TexFormatInfo.Dimensions |= RESOURCE_DIMENSION_SUPPORT_TEX_2D_ARRAY;
            }
        }

        if (TexFormatInfo.Dimensions & RESOURCE_DIMENSION_SUPPORT_TEX_2D)
        {
            if (CreateTestGLTexture(
                    ContextState, GL_TEXTURE_CUBE_MAP,
                    [&]() //
                    {
                        glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, GLFmt, TestTextureDim, TestTextureDim);
                    }))
            {
                TexFormatInfo.Dimensions |= RESOURCE_DIMENSION_SUPPORT_TEX_CUBE;

                if (CreateTestGLTexture(
                        ContextState, GL_TEXTURE_CUBE_MAP_ARRAY,
                        [&]() //
                        {
                            glTexStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 1, GLFmt, TestTextureDim, TestTextureDim, 6);
                        }))
                {
                    TexFormatInfo.Dimensions |= RESOURCE_DIMENSION_SUPPORT_TEX_CUBE_ARRAY;
                }
            }

            bool bTestDepthAttachment =
                TexFormatInfo.ComponentType == COMPONENT_TYPE_DEPTH ||
                TexFormatInfo.ComponentType == COMPONENT_TYPE_DEPTH_STENCIL;
            bool bTestColorAttachment = !bTestDepthAttachment && TexFormatInfo.ComponentType != COMPONENT_TYPE_COMPRESSED;

            GLObjectWrappers::GLFrameBufferObj NewFBO{false};

            GLint CurrentFramebuffer = -1;
            if (bTestColorAttachment || bTestDepthAttachment)
            {
                glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &CurrentFramebuffer);
                CHECK_GL_ERROR("Failed to get current framebuffer");

                NewFBO.Create();
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, NewFBO);
                CHECK_GL_ERROR("Failed to bind the framebuffer");
            }

            if (bTestDepthAttachment)
            {
                GLenum Attachment = TexFormatInfo.ComponentType == COMPONENT_TYPE_DEPTH ? GL_DEPTH_ATTACHMENT : GL_DEPTH_STENCIL_ATTACHMENT;
                glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, Attachment, GL_TEXTURE_2D, TestGLTex2D, 0);
                if (glGetError() == GL_NO_ERROR)
                {
                    // Create dummy texture2D since some older version do not allow depth only
                    // attachments
                    GLObjectWrappers::GLTextureObj ColorTex(true);

                    bool Success = CreateTestGLTexture(
                        ContextState, GL_TEXTURE_2D, ColorTex,
                        [&]() //
                        {
                            glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, TestTextureDim, TestTextureDim);
                        } //
                    );
                    VERIFY(Success, "Failed to create dummy render target texture");
                    (void)Success;
                    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ColorTex, 0);
                    CHECK_GL_ERROR("Failed to set bind dummy render target to framebuffer");

                    static const GLenum DrawBuffers[] = {GL_COLOR_ATTACHMENT0};
                    glDrawBuffers(_countof(DrawBuffers), DrawBuffers);
                    CHECK_GL_ERROR("Failed to set draw buffers via glDrawBuffers()");

                    GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
                    if ((glGetError() == GL_NO_ERROR) && (Status == GL_FRAMEBUFFER_COMPLETE))
                        TexFormatInfo.BindFlags |= BIND_DEPTH_STENCIL;
                }
            }
            else if (bTestColorAttachment)
            {
                glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, TestGLTex2D, 0);
                if (glGetError() == GL_NO_ERROR)
                {
                    static const GLenum DrawBuffers[] = {GL_COLOR_ATTACHMENT0};
                    glDrawBuffers(_countof(DrawBuffers), DrawBuffers);
                    CHECK_GL_ERROR("Failed to set draw buffers via glDrawBuffers()");

                    GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
                    if ((glGetError() == GL_NO_ERROR) && (Status == GL_FRAMEBUFFER_COMPLETE))
                        TexFormatInfo.BindFlags |= BIND_RENDER_TARGET;
                }
            }

            if (bTestColorAttachment || bTestDepthAttachment)
            {
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, CurrentFramebuffer);
                CHECK_GL_ERROR("Failed to bind the framebuffer");
            }
        }

#if GL_ARB_shader_image_load_store
        if (m_DeviceCaps.Features.PixelUAVWritesAndAtomics)
        {
            GLuint    CurrentImg     = 0;
            GLint     CurrentLevel   = 0;
            GLboolean CurrentLayered = 0;
            GLint     CurrentLayer   = 0;
            GLenum    CurrenAccess   = 0;
            GLenum    CurrenFormat   = 0;
            ContextState.GetBoundImage(0, CurrentImg, CurrentLevel, CurrentLayered, CurrentLayer, CurrenAccess, CurrenFormat);

            glBindImageTexture(0, TestGLTex2D, 0, GL_FALSE, 0, GL_READ_WRITE, GLFmt);
            if (glGetError() == GL_NO_ERROR)
                TexFormatInfo.BindFlags |= BIND_UNORDERED_ACCESS;

            glBindImageTexture(0, CurrentImg, CurrentLevel, CurrentLayered, CurrentLayer, CurrenAccess, CurrenFormat);
            CHECK_GL_ERROR("Failed to restore original image");
        }
#endif
    }

    TexFormatInfo.SampleCounts = 0x01;
    if (TexFormatInfo.ComponentType != COMPONENT_TYPE_COMPRESSED &&
        m_DeviceCaps.TexCaps.Texture2DMSSupported)
    {
#if GL_ARB_texture_storage_multisample
        for (GLsizei SampleCount = 2; SampleCount <= 8; SampleCount *= 2)
        {
            GLObjectWrappers::GLTextureObj TestGLTex(true);

            auto SampleCountSupported = CreateTestGLTexture(
                ContextState, GL_TEXTURE_2D_MULTISAMPLE, TestGLTex,
                [&]() //
                {
                    glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, SampleCount, GLFmt, TestTextureDim, TestTextureDim, GL_TRUE);
                } //
            );
            if (SampleCountSupported)
                TexFormatInfo.SampleCounts |= SampleCount;
        }
#endif
    }

    // Create test texture 3D.
    // 3D textures do not support depth formats.
    if (!(TexFormatInfo.ComponentType == COMPONENT_TYPE_DEPTH ||
          TexFormatInfo.ComponentType == COMPONENT_TYPE_DEPTH_STENCIL))
    {
        if (CreateTestGLTexture(
                ContextState, GL_TEXTURE_3D,
                [&]() //
                {
                    glTexStorage3D(GL_TEXTURE_3D, 1, GLFmt, TestTextureDim, TestTextureDim, TestTextureDepth);
                }))
        {
            TexFormatInfo.Dimensions |= RESOURCE_DIMENSION_SUPPORT_TEX_3D;
        }
    }

    // Enable debug messages
    m_ShowDebugGLOutput = 1;
}

FBOCache& RenderDeviceGLImpl::GetFBOCache(GLContext::NativeGLContextType Context)
{
    ThreadingTools::LockHelper FBOCacheLock(m_FBOCacheLockFlag);
    return m_FBOCache[Context];
}

void RenderDeviceGLImpl::OnReleaseTexture(ITexture* pTexture)
{
    ThreadingTools::LockHelper FBOCacheLock(m_FBOCacheLockFlag);
    for (auto& FBOCacheIt : m_FBOCache)
        FBOCacheIt.second.OnReleaseTexture(pTexture);
}

VAOCache& RenderDeviceGLImpl::GetVAOCache(GLContext::NativeGLContextType Context)
{
    ThreadingTools::LockHelper VAOCacheLock(m_VAOCacheLockFlag);
    return m_VAOCache[Context];
}

void RenderDeviceGLImpl::OnDestroyPSO(IPipelineState* pPSO)
{
    ThreadingTools::LockHelper VAOCacheLock(m_VAOCacheLockFlag);
    for (auto& VAOCacheIt : m_VAOCache)
        VAOCacheIt.second.OnDestroyPSO(pPSO);
}

void RenderDeviceGLImpl::OnDestroyBuffer(IBuffer* pBuffer)
{
    ThreadingTools::LockHelper VAOCacheLock(m_VAOCacheLockFlag);
    for (auto& VAOCacheIt : m_VAOCache)
        VAOCacheIt.second.OnDestroyBuffer(pBuffer);
}

void RenderDeviceGLImpl::IdleGPU()
{
    glFinish();
}

} // namespace Diligent
