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

#include <iomanip>
#include "SPIRVShaderResources.hpp"
#include "spirv_parser.hpp"
#include "spirv_cross.hpp"
#include "ShaderBase.hpp"
#include "GraphicsAccessories.hpp"
#include "StringTools.hpp"
#include "Align.hpp"

namespace Diligent
{

template <typename Type>
Type GetResourceArraySize(const diligent_spirv_cross::Compiler& Compiler,
                          const diligent_spirv_cross::Resource& Res)
{
    const auto& type    = Compiler.get_type(Res.type_id);
    uint32_t    arrSize = 1;
    if (!type.array.empty())
    {
        // https://github.com/KhronosGroup/SPIRV-Cross/wiki/Reflection-API-user-guide#querying-array-types
        VERIFY(type.array.size() == 1, "Only one-dimensional arrays are currently supported");
        arrSize = type.array[0];
    }
    VERIFY(arrSize <= std::numeric_limits<Type>::max(), "Array size exceeds maximum representable value ", std::numeric_limits<Type>::max());
    return static_cast<Type>(arrSize);
}

static RESOURCE_DIMENSION GetResourceDimension(const diligent_spirv_cross::Compiler& Compiler,
                                               const diligent_spirv_cross::Resource& Res)
{
    const auto& type = Compiler.get_type(Res.type_id);
    if (type.basetype == diligent_spirv_cross::SPIRType::BaseType::Image ||
        type.basetype == diligent_spirv_cross::SPIRType::BaseType::SampledImage)
    {
        switch (type.image.dim)
        {
            // clang-format off
            case spv::Dim1D:     return type.image.arrayed ? RESOURCE_DIM_TEX_1D_ARRAY : RESOURCE_DIM_TEX_1D;
            case spv::Dim2D:     return type.image.arrayed ? RESOURCE_DIM_TEX_2D_ARRAY : RESOURCE_DIM_TEX_2D;
            case spv::Dim3D:     return RESOURCE_DIM_TEX_3D;
            case spv::DimCube:   return type.image.arrayed ? RESOURCE_DIM_TEX_CUBE_ARRAY : RESOURCE_DIM_TEX_CUBE;
            case spv::DimBuffer: return RESOURCE_DIM_BUFFER;
            // clang-format on
            default: return RESOURCE_DIM_UNDEFINED;
        }
    }
    else
    {
        return RESOURCE_DIM_UNDEFINED;
    }
}

static bool IsMultisample(const diligent_spirv_cross::Compiler& Compiler,
                          const diligent_spirv_cross::Resource& Res)
{
    const auto& type = Compiler.get_type(Res.type_id);
    if (type.basetype == diligent_spirv_cross::SPIRType::BaseType::Image ||
        type.basetype == diligent_spirv_cross::SPIRType::BaseType::SampledImage)
    {
        return type.image.ms;
    }
    else
    {
        return RESOURCE_DIM_UNDEFINED;
    }
}

static uint32_t GetDecorationOffset(const diligent_spirv_cross::Compiler& Compiler,
                                    const diligent_spirv_cross::Resource& Res,
                                    spv::Decoration                       Decoration)
{
    VERIFY(Compiler.has_decoration(Res.id, Decoration), "Resource \'", Res.name, "\' has no requested decoration");
    uint32_t offset   = 0;
    auto     declared = Compiler.get_binary_offset_for_decoration(Res.id, Decoration, offset);
    VERIFY(declared, "Requested decoration is not declared");
    (void)declared;
    return offset;
}

SPIRVShaderResourceAttribs::SPIRVShaderResourceAttribs(const diligent_spirv_cross::Compiler& Compiler,
                                                       const diligent_spirv_cross::Resource& Res,
                                                       const char*                           _Name,
                                                       ResourceType                          _Type,
                                                       Uint32                                _SepSmplrOrImgInd) noexcept :
    // clang-format off
    Name                          {_Name},
    ArraySize                     {GetResourceArraySize<decltype(ArraySize)>(Compiler, Res)},
    Type                          {_Type},
    ResourceDim                   {Diligent::GetResourceDimension(Compiler, Res)},
    IsMS                          {Diligent::IsMultisample(Compiler, Res) ? Uint8{1} : Uint8{0}},
    SepSmplrOrImgInd              {_SepSmplrOrImgInd},
    BindingDecorationOffset       {GetDecorationOffset(Compiler, Res, spv::Decoration::DecorationBinding)},
    DescriptorSetDecorationOffset {GetDecorationOffset(Compiler, Res, spv::Decoration::DecorationDescriptorSet)}
// clang-format on
{
    VERIFY(_SepSmplrOrImgInd == SPIRVShaderResourceAttribs::InvalidSepSmplrOrImgInd ||
               (_Type == ResourceType::SeparateSampler || _Type == ResourceType::SeparateImage),
           "Only separate images or separate samplers can be assinged valid SepSmplrOrImgInd value");
}


ShaderResourceDesc SPIRVShaderResourceAttribs::GetResourceDesc() const
{
    ShaderResourceDesc ResourceDesc;
    ResourceDesc.Name      = Name;
    ResourceDesc.ArraySize = ArraySize;

    static_assert(SPIRVShaderResourceAttribs::ResourceType::NumResourceTypes == 11, "Please handle the new resource type below");
    switch (Type)
    {
        case SPIRVShaderResourceAttribs::ResourceType::UniformBuffer:
            ResourceDesc.Type = SHADER_RESOURCE_TYPE_CONSTANT_BUFFER;
            break;

        case SPIRVShaderResourceAttribs::ResourceType::ROStorageBuffer:
            // Read-only storage buffers map to buffer SRV
            // https://github.com/KhronosGroup/SPIRV-Cross/wiki/Reflection-API-user-guide#read-write-vs-read-only-resources-for-hlsl
            ResourceDesc.Type = SHADER_RESOURCE_TYPE_BUFFER_SRV;
            break;

        case SPIRVShaderResourceAttribs::ResourceType::RWStorageBuffer:
            ResourceDesc.Type = SHADER_RESOURCE_TYPE_BUFFER_UAV;
            break;

        case SPIRVShaderResourceAttribs::ResourceType::UniformTexelBuffer:
            ResourceDesc.Type = SHADER_RESOURCE_TYPE_BUFFER_SRV;
            break;

        case SPIRVShaderResourceAttribs::ResourceType::StorageTexelBuffer:
            ResourceDesc.Type = SHADER_RESOURCE_TYPE_BUFFER_UAV;
            break;

        case SPIRVShaderResourceAttribs::ResourceType::StorageImage:
            ResourceDesc.Type = SHADER_RESOURCE_TYPE_TEXTURE_UAV;
            break;

        case SPIRVShaderResourceAttribs::ResourceType::SampledImage:
            ResourceDesc.Type = SHADER_RESOURCE_TYPE_TEXTURE_SRV;
            break;

        case SPIRVShaderResourceAttribs::ResourceType::AtomicCounter:
            LOG_WARNING_MESSAGE("There is no appropriate shader resource type for atomic counter resource '", Name, "'");
            ResourceDesc.Type = SHADER_RESOURCE_TYPE_BUFFER_UAV;
            break;

        case SPIRVShaderResourceAttribs::ResourceType::SeparateImage:
            ResourceDesc.Type = SHADER_RESOURCE_TYPE_TEXTURE_SRV;
            break;

        case SPIRVShaderResourceAttribs::ResourceType::SeparateSampler:
            ResourceDesc.Type = SHADER_RESOURCE_TYPE_SAMPLER;
            break;

        case SPIRVShaderResourceAttribs::ResourceType::InputAttachment:
            ResourceDesc.Type = SHADER_RESOURCE_TYPE_INPUT_ATTACHMENT;
            break;

        default:
            UNEXPECTED("Unknown SPIRV resource type");
    }
    return ResourceDesc;
}


static spv::ExecutionModel ShaderTypeToExecutionModel(SHADER_TYPE ShaderType)
{
    static_assert(SHADER_TYPE_LAST == 0x080, "Please handle the new shader type in the switch below");
    switch (ShaderType)
    {
        // clang-format off
        case SHADER_TYPE_VERTEX:        return spv::ExecutionModelVertex;
        case SHADER_TYPE_HULL:          return spv::ExecutionModelTessellationControl;
        case SHADER_TYPE_DOMAIN:        return spv::ExecutionModelTessellationEvaluation;
        case SHADER_TYPE_GEOMETRY:      return spv::ExecutionModelGeometry;
        case SHADER_TYPE_PIXEL:         return spv::ExecutionModelFragment;
        case SHADER_TYPE_COMPUTE:       return spv::ExecutionModelGLCompute;
        case SHADER_TYPE_AMPLIFICATION: return spv::ExecutionModelTaskNV;
        case SHADER_TYPE_MESH:          return spv::ExecutionModelMeshNV;
        // clang-format on
        default:
            UNEXPECTED("Unexpected shader type");
            return spv::ExecutionModelVertex;
    }
}

const std::string& GetUBName(diligent_spirv_cross::Compiler&               Compiler,
                             const diligent_spirv_cross::Resource&         UB,
                             const diligent_spirv_cross::ParsedIR::Source& IRSource)
{
    // Consider the following HLSL constant buffer:
    //
    //    cbuffer Constants
    //    {
    //        float4x4 g_WorldViewProj;
    //    };
    //
    // glslang emits SPIRV as if the following GLSL was written:
    //
    //    uniform Constants // UB.name
    //    {
    //        float4x4 g_WorldViewProj;
    //    }; // no instance name
    //
    // DXC emits the byte code that corresponds to the following GLSL:
    //
    //    uniform type_Constants // UB.name
    //    {
    //        float4x4 g_WorldViewProj;
    //    }Constants; // get_name(UB.id)
    //
    //
    //                            |     glslang      |         DXC
    //  -------------------------------------------------------------------
    //  UB.name                   |   "Constants"    |   "type_Constants"
    //  Compiler.get_name(UB.id)  |   ""             |   "Constants"
    //
    // Note that for the byte code produced from GLSL, we must always
    // use UB.name even if the instance name is present

    const auto& instance_name = Compiler.get_name(UB.id);
    return (IRSource.hlsl && !instance_name.empty()) ? instance_name : UB.name;
}

SPIRVShaderResources::SPIRVShaderResources(IMemoryAllocator&     Allocator,
                                           IRenderDevice*        pRenderDevice,
                                           std::vector<uint32_t> spirv_binary,
                                           const ShaderDesc&     shaderDesc,
                                           const char*           CombinedSamplerSuffix,
                                           bool                  LoadShaderStageInputs,
                                           std::string&          EntryPoint) :
    m_ShaderType{shaderDesc.ShaderType}
{
    // https://github.com/KhronosGroup/SPIRV-Cross/wiki/Reflection-API-user-guide
    diligent_spirv_cross::Parser parser(move(spirv_binary));
    parser.parse();
    const auto ParsedIRSource = parser.get_parsed_ir().source;
    m_IsHLSLSource            = ParsedIRSource.hlsl;
    diligent_spirv_cross::Compiler Compiler(std::move(parser.get_parsed_ir()));

    spv::ExecutionModel ExecutionModel = ShaderTypeToExecutionModel(shaderDesc.ShaderType);
    auto                EntryPoints    = Compiler.get_entry_points_and_stages();
    for (const auto& CurrEntryPoint : EntryPoints)
    {
        if (CurrEntryPoint.execution_model == ExecutionModel)
        {
            if (!EntryPoint.empty())
            {
                LOG_WARNING_MESSAGE("More than one entry point of type ", GetShaderTypeLiteralName(shaderDesc.ShaderType), " found in SPIRV binary for shader '", shaderDesc.Name, "'. The first one ('", EntryPoint, "') will be used.");
            }
            else
            {
                EntryPoint = CurrEntryPoint.name;
            }
        }
    }
    if (EntryPoint.empty())
    {
        LOG_ERROR_AND_THROW("Unable to find entry point of type ", GetShaderTypeLiteralName(shaderDesc.ShaderType), " in SPIRV binary for shader '", shaderDesc.Name, "'");
    }
    Compiler.set_entry_point(EntryPoint, ExecutionModel);

    // The SPIR-V is now parsed, and we can perform reflection on it.
    diligent_spirv_cross::ShaderResources resources = Compiler.get_shader_resources();

    size_t ResourceNamesPoolSize = 0;
    for (const auto& ub : resources.uniform_buffers)
        ResourceNamesPoolSize += GetUBName(Compiler, ub, ParsedIRSource).length() + 1;
    static_assert(SPIRVShaderResourceAttribs::ResourceType::NumResourceTypes == 11, "Please account for the new resource type below");
    for (auto* pResType :
         {
             &resources.storage_buffers,
             &resources.storage_images,
             &resources.sampled_images,
             &resources.atomic_counters,
             &resources.separate_images,
             &resources.separate_samplers,
             &resources.subpass_inputs
             // clang-format off
         })
    // clang-format on
    {
        for (const auto& res : *pResType)
            ResourceNamesPoolSize += res.name.length() + 1;
    }

    if (CombinedSamplerSuffix != nullptr)
    {
        ResourceNamesPoolSize += strlen(CombinedSamplerSuffix) + 1;
    }

    VERIFY_EXPR(shaderDesc.Name != nullptr);
    ResourceNamesPoolSize += strlen(shaderDesc.Name) + 1;

    Uint32 NumShaderStageInputs = 0;

    if (!m_IsHLSLSource || resources.stage_inputs.empty())
        LoadShaderStageInputs = false;
    if (LoadShaderStageInputs)
    {
        const auto& Extensions         = Compiler.get_declared_extensions();
        bool        HlslFunctionality1 = false;
        for (const auto& ext : Extensions)
        {
            HlslFunctionality1 = (ext == "SPV_GOOGLE_hlsl_functionality1");
            if (HlslFunctionality1)
                break;
        }

        if (HlslFunctionality1)
        {
            for (const auto& Input : resources.stage_inputs)
            {
                if (Compiler.has_decoration(Input.id, spv::Decoration::DecorationHlslSemanticGOOGLE))
                {
                    const auto& Semantic = Compiler.get_decoration_string(Input.id, spv::Decoration::DecorationHlslSemanticGOOGLE);
                    ResourceNamesPoolSize += Semantic.length() + 1;
                    ++NumShaderStageInputs;
                }
                else
                {
                    LOG_ERROR_MESSAGE("Shader input '", Input.name, "' does not have DecorationHlslSemanticGOOGLE decoration, which is unexpected as the shader declares SPV_GOOGLE_hlsl_functionality1 extension");
                }
            }
        }
        else
        {
            LoadShaderStageInputs = false;
            if (m_IsHLSLSource)
            {
                LOG_WARNING_MESSAGE("SPIRV byte code of shader '", shaderDesc.Name,
                                    "' does not use SPV_GOOGLE_hlsl_functionality1 extension. "
                                    "As a result, it is not possible to get semantics of shader inputs and map them to proper locations. "
                                    "The shader will still work correctly if all attributes are declared in ascending order without any gaps. "
                                    "Enable SPV_GOOGLE_hlsl_functionality1 in your compiler to allow proper mapping of vertex shader inputs.");
            }
        }
    }

    ResourceCounters ResCounters;
    ResCounters.NumUBs       = static_cast<Uint32>(resources.uniform_buffers.size());
    ResCounters.NumSBs       = static_cast<Uint32>(resources.storage_buffers.size());
    ResCounters.NumImgs      = static_cast<Uint32>(resources.storage_images.size());
    ResCounters.NumSmpldImgs = static_cast<Uint32>(resources.sampled_images.size());
    ResCounters.NumACs       = static_cast<Uint32>(resources.atomic_counters.size());
    ResCounters.NumSepSmplrs = static_cast<Uint32>(resources.separate_samplers.size());
    ResCounters.NumSepImgs   = static_cast<Uint32>(resources.separate_images.size());
    ResCounters.NumInptAtts  = static_cast<Uint32>(resources.subpass_inputs.size());
    static_assert(SPIRVShaderResourceAttribs::ResourceType::NumResourceTypes == 11, "Please set the new resource type counter here");

    // Resource names pool is only needed to facilitate string allocation.
    StringPool ResourceNamesPool;
    Initialize(Allocator, ResCounters, NumShaderStageInputs, ResourceNamesPoolSize, ResourceNamesPool);

    {
        Uint32 CurrUB = 0;
        for (const auto& UB : resources.uniform_buffers)
        {
            const auto& name = GetUBName(Compiler, UB, ParsedIRSource);
            new (&GetUB(CurrUB++))
                SPIRVShaderResourceAttribs(Compiler,
                                           UB,
                                           ResourceNamesPool.CopyString(name),
                                           SPIRVShaderResourceAttribs::ResourceType::UniformBuffer);
        }
        VERIFY_EXPR(CurrUB == GetNumUBs());
    }

    {
        Uint32 CurrSB = 0;
        for (const auto& SB : resources.storage_buffers)
        {
            auto BufferFlags = Compiler.get_buffer_block_flags(SB.id);
            auto IsReadOnly  = BufferFlags.get(spv::DecorationNonWritable);
            auto ResType     = IsReadOnly ?
                SPIRVShaderResourceAttribs::ResourceType::ROStorageBuffer :
                SPIRVShaderResourceAttribs::ResourceType::RWStorageBuffer;
            new (&GetSB(CurrSB++))
                SPIRVShaderResourceAttribs(Compiler,
                                           SB,
                                           ResourceNamesPool.CopyString(SB.name),
                                           ResType);
        }
        VERIFY_EXPR(CurrSB == GetNumSBs());
    }

    {
        Uint32 CurrSmplImg = 0;
        for (const auto& SmplImg : resources.sampled_images)
        {
            const auto& type    = Compiler.get_type(SmplImg.type_id);
            auto        ResType = type.image.dim == spv::DimBuffer ?
                SPIRVShaderResourceAttribs::ResourceType::UniformTexelBuffer :
                SPIRVShaderResourceAttribs::ResourceType::SampledImage;
            new (&GetSmpldImg(CurrSmplImg++))
                SPIRVShaderResourceAttribs(Compiler,
                                           SmplImg,
                                           ResourceNamesPool.CopyString(SmplImg.name),
                                           ResType);
        }
        VERIFY_EXPR(CurrSmplImg == GetNumSmpldImgs());
    }

    {
        Uint32 CurrImg = 0;
        for (const auto& Img : resources.storage_images)
        {
            const auto& type    = Compiler.get_type(Img.type_id);
            auto        ResType = type.image.dim == spv::DimBuffer ?
                SPIRVShaderResourceAttribs::ResourceType::StorageTexelBuffer :
                SPIRVShaderResourceAttribs::ResourceType::StorageImage;
            new (&GetImg(CurrImg++))
                SPIRVShaderResourceAttribs(Compiler,
                                           Img,
                                           ResourceNamesPool.CopyString(Img.name),
                                           ResType);
        }
        VERIFY_EXPR(CurrImg == GetNumImgs());
    }

    {
        Uint32 CurrAC = 0;
        for (const auto& AC : resources.atomic_counters)
        {
            new (&GetAC(CurrAC++))
                SPIRVShaderResourceAttribs(Compiler,
                                           AC,
                                           ResourceNamesPool.CopyString(AC.name),
                                           SPIRVShaderResourceAttribs::ResourceType::AtomicCounter);
        }
        VERIFY_EXPR(CurrAC == GetNumACs());
    }

    {
        Uint32 CurrSepSmpl = 0;
        for (const auto& SepSam : resources.separate_samplers)
        {
            new (&GetSepSmplr(CurrSepSmpl++))
                SPIRVShaderResourceAttribs(Compiler,
                                           SepSam,
                                           ResourceNamesPool.CopyString(SepSam.name),
                                           SPIRVShaderResourceAttribs::ResourceType::SeparateSampler);
        }
        VERIFY_EXPR(CurrSepSmpl == GetNumSepSmplrs());
    }

    {
        Uint32 CurrSepImg = 0;
        for (const auto& SepImg : resources.separate_images)
        {
            const auto& type    = Compiler.get_type(SepImg.type_id);
            auto        ResType = type.image.dim == spv::DimBuffer ?
                SPIRVShaderResourceAttribs::ResourceType::UniformTexelBuffer :
                SPIRVShaderResourceAttribs::ResourceType::SeparateImage;

            Uint32 SamplerInd = SPIRVShaderResourceAttribs::InvalidSepSmplrOrImgInd;
            if (CombinedSamplerSuffix != nullptr)
            {
                auto NumSepSmpls = GetNumSepSmplrs();
                for (SamplerInd = 0; SamplerInd < NumSepSmpls; ++SamplerInd)
                {
                    auto& SepSmplr = GetSepSmplr(SamplerInd);
                    if (StreqSuff(SepSmplr.Name, SepImg.name.c_str(), CombinedSamplerSuffix))
                    {
                        SepSmplr.AssignSeparateImage(CurrSepImg);
                        break;
                    }
                }
                if (SamplerInd == NumSepSmpls)
                    SamplerInd = SPIRVShaderResourceAttribs::InvalidSepSmplrOrImgInd;
                else
                {
                    if (ResType == SPIRVShaderResourceAttribs::ResourceType::UniformTexelBuffer)
                    {
                        LOG_WARNING_MESSAGE("Combined image sampler assigned to uniform texel buffer '", SepImg.name, "' will be ignored");
                        SamplerInd = SPIRVShaderResourceAttribs::InvalidSepSmplrOrImgInd;
                    }
                }
            }
            auto* pNewSepImg = new (&GetSepImg(CurrSepImg++))
                SPIRVShaderResourceAttribs(Compiler,
                                           SepImg,
                                           ResourceNamesPool.CopyString(SepImg.name),
                                           ResType,
                                           SamplerInd);
            if (ResType == SPIRVShaderResourceAttribs::ResourceType::SeparateImage && pNewSepImg->IsValidSepSamplerAssigned())
            {
#ifdef DILIGENT_DEVELOPMENT
                const auto& SepSmplr = GetSepSmplr(pNewSepImg->GetAssignedSepSamplerInd());
                DEV_CHECK_ERR(SepSmplr.ArraySize == 1 || SepSmplr.ArraySize == pNewSepImg->ArraySize,
                              "Array size (", SepSmplr.ArraySize, ") of separate sampler variable '",
                              SepSmplr.Name, "' must be equal to 1 or be the same as the array size (", pNewSepImg->ArraySize,
                              ") of separate image variable '", pNewSepImg->Name, "' it is assigned to");
#endif
            }
        }
        VERIFY_EXPR(CurrSepImg == GetNumSepImgs());
    }

    {
        Uint32 CurrSubpassInput = 0;
        for (const auto& SubpassInput : resources.subpass_inputs)
        {
            new (&GetInptAtt(CurrSubpassInput++))
                SPIRVShaderResourceAttribs(Compiler,
                                           SubpassInput,
                                           ResourceNamesPool.CopyString(SubpassInput.name),
                                           SPIRVShaderResourceAttribs::ResourceType::InputAttachment);
        }
        VERIFY_EXPR(CurrSubpassInput == GetNumInptAtts());
    }

    static_assert(SPIRVShaderResourceAttribs::ResourceType::NumResourceTypes == 11, "Please initialize SPIRVShaderResourceAttribs for the new resource type here");

    if (CombinedSamplerSuffix != nullptr)
    {
        m_CombinedSamplerSuffix = ResourceNamesPool.CopyString(CombinedSamplerSuffix);
    }

    m_ShaderName = ResourceNamesPool.CopyString(shaderDesc.Name);

    if (LoadShaderStageInputs)
    {
        Uint32 CurrStageInput = 0;
        for (const auto& Input : resources.stage_inputs)
        {
            if (Compiler.has_decoration(Input.id, spv::Decoration::DecorationHlslSemanticGOOGLE))
            {
                const auto& Semantic = Compiler.get_decoration_string(Input.id, spv::Decoration::DecorationHlslSemanticGOOGLE);
                new (&GetShaderStageInputAttribs(CurrStageInput++))
                    SPIRVShaderStageInputAttribs(ResourceNamesPool.CopyString(Semantic), GetDecorationOffset(Compiler, Input, spv::Decoration::DecorationLocation));
            }
        }
        VERIFY_EXPR(CurrStageInput == GetNumShaderStageInputs());
    }

    VERIFY(ResourceNamesPool.GetRemainingSize() == 0, "Names pool must be empty");

    //LOG_INFO_MESSAGE(DumpResources());

#ifdef DILIGENT_DEVELOPMENT
    if (CombinedSamplerSuffix != nullptr)
    {
        for (Uint32 n = 0; n < GetNumSepSmplrs(); ++n)
        {
            const auto& SepSmplr = GetSepSmplr(n);
            if (!SepSmplr.IsValidSepImageAssigned())
                LOG_ERROR_MESSAGE("Shader '", shaderDesc.Name, "' uses combined texture samplers, but separate sampler '", SepSmplr.Name, "' is not assigned to any texture");
        }
    }
#endif
}

void SPIRVShaderResources::Initialize(IMemoryAllocator&       Allocator,
                                      const ResourceCounters& Counters,
                                      Uint32                  NumShaderStageInputs,
                                      size_t                  ResourceNamesPoolSize,
                                      StringPool&             ResourceNamesPool)
{
    Uint32           CurrentOffset = 0;
    constexpr Uint32 MaxOffset     = std::numeric_limits<OffsetType>::max();
    auto             AdvanceOffset = [&CurrentOffset, MaxOffset](Uint32 NumResources) {
        VERIFY(CurrentOffset <= MaxOffset, "Current offset (", CurrentOffset, ") exceeds max allowed value (", MaxOffset, ")");
        (void)MaxOffset;
        auto Offset = static_cast<OffsetType>(CurrentOffset);
        CurrentOffset += NumResources;
        return Offset;
    };

    auto UniformBufferOffset = AdvanceOffset(Counters.NumUBs);
    (void)UniformBufferOffset;
    m_StorageBufferOffset   = AdvanceOffset(Counters.NumSBs);
    m_StorageImageOffset    = AdvanceOffset(Counters.NumImgs);
    m_SampledImageOffset    = AdvanceOffset(Counters.NumSmpldImgs);
    m_AtomicCounterOffset   = AdvanceOffset(Counters.NumACs);
    m_SeparateSamplerOffset = AdvanceOffset(Counters.NumSepSmplrs);
    m_SeparateImageOffset   = AdvanceOffset(Counters.NumSepImgs);
    m_InputAttachmentOffset = AdvanceOffset(Counters.NumInptAtts);
    m_TotalResources        = AdvanceOffset(0);
    static_assert(SPIRVShaderResourceAttribs::ResourceType::NumResourceTypes == 11, "Please update the new resource type offset");

    VERIFY(NumShaderStageInputs <= MaxOffset, "Max offset exceeded");
    m_NumShaderStageInputs = static_cast<OffsetType>(NumShaderStageInputs);

    auto AlignedResourceNamesPoolSize = Align(ResourceNamesPoolSize, sizeof(void*));

    static_assert(sizeof(SPIRVShaderResourceAttribs) % sizeof(void*) == 0, "Size of SPIRVShaderResourceAttribs struct must be multiple of sizeof(void*)");
    // clang-format off
    auto MemorySize = m_TotalResources              * sizeof(SPIRVShaderResourceAttribs) + 
                      m_NumShaderStageInputs        * sizeof(SPIRVShaderStageInputAttribs) +
                      AlignedResourceNamesPoolSize  * sizeof(char);

    VERIFY_EXPR(GetNumUBs()       == Counters.NumUBs);
    VERIFY_EXPR(GetNumSBs()       == Counters.NumSBs);
    VERIFY_EXPR(GetNumImgs()      == Counters.NumImgs);
    VERIFY_EXPR(GetNumSmpldImgs() == Counters.NumSmpldImgs);
    VERIFY_EXPR(GetNumACs()       == Counters.NumACs);
    VERIFY_EXPR(GetNumSepSmplrs() == Counters.NumSepSmplrs);
    VERIFY_EXPR(GetNumSepImgs()   == Counters.NumSepImgs);
    VERIFY_EXPR(GetNumInptAtts()  == Counters.NumInptAtts);
    // clang-format on

    if (MemorySize)
    {
        auto* pRawMem   = Allocator.Allocate(MemorySize, "Memory for shader resources", __FILE__, __LINE__);
        m_MemoryBuffer  = std::unique_ptr<void, STDDeleterRawMem<void>>(pRawMem, Allocator);
        char* NamesPool = reinterpret_cast<char*>(m_MemoryBuffer.get()) +
            m_TotalResources * sizeof(SPIRVShaderResourceAttribs) +
            m_NumShaderStageInputs * sizeof(SPIRVShaderStageInputAttribs);
        ResourceNamesPool.AssignMemory(NamesPool, ResourceNamesPoolSize);
    }
}

SPIRVShaderResources::~SPIRVShaderResources()
{
    for (Uint32 n = 0; n < GetNumUBs(); ++n)
        GetUB(n).~SPIRVShaderResourceAttribs();

    for (Uint32 n = 0; n < GetNumSBs(); ++n)
        GetSB(n).~SPIRVShaderResourceAttribs();

    for (Uint32 n = 0; n < GetNumImgs(); ++n)
        GetImg(n).~SPIRVShaderResourceAttribs();

    for (Uint32 n = 0; n < GetNumSmpldImgs(); ++n)
        GetSmpldImg(n).~SPIRVShaderResourceAttribs();

    for (Uint32 n = 0; n < GetNumACs(); ++n)
        GetAC(n).~SPIRVShaderResourceAttribs();

    for (Uint32 n = 0; n < GetNumSepSmplrs(); ++n)
        GetSepSmplr(n).~SPIRVShaderResourceAttribs();

    for (Uint32 n = 0; n < GetNumSepImgs(); ++n)
        GetSepImg(n).~SPIRVShaderResourceAttribs();

    for (Uint32 n = 0; n < GetNumInptAtts(); ++n)
        GetInptAtt(n).~SPIRVShaderResourceAttribs();

    for (Uint32 n = 0; n < GetNumShaderStageInputs(); ++n)
        GetShaderStageInputAttribs(n).~SPIRVShaderStageInputAttribs();
}



std::string SPIRVShaderResources::DumpResources()
{
    std::stringstream ss;
    ss << "Shader '" << m_ShaderName << "' resource stats: total resources: " << GetTotalResources() << ":" << std::endl
       << "UBs: " << GetNumUBs() << "; SBs: " << GetNumSBs() << "; Imgs: " << GetNumImgs() << "; Smpl Imgs: " << GetNumSmpldImgs()
       << "; ACs: " << GetNumACs() << "; Sep Imgs: " << GetNumSepImgs() << "; Sep Smpls: " << GetNumSepSmplrs() << '.' << std::endl
       << "Resources:";

    Uint32 ResNum       = 0;
    auto   DumpResource = [&ss, &ResNum](const SPIRVShaderResourceAttribs& Res) {
        std::stringstream FullResNameSS;
        FullResNameSS << '\'' << Res.Name;
        if (Res.ArraySize > 1)
            FullResNameSS << '[' << Res.ArraySize << ']';
        FullResNameSS << '\'';
        ss << std::setw(32) << FullResNameSS.str();

        if (Res.Type == SPIRVShaderResourceAttribs::ResourceType::SeparateImage && Res.IsValidSepSamplerAssigned())
        {
            ss << " Assigned sep sampler ind: " << Res.GetAssignedSepSamplerInd();
        }
        else if (Res.Type == SPIRVShaderResourceAttribs::ResourceType::SeparateSampler && Res.IsValidSepImageAssigned())
        {
            ss << " Assigned sep image ind: " << Res.GetAssignedSepImageInd();
        }

        ++ResNum;
    };

    ProcessResources(
        [&](const SPIRVShaderResourceAttribs& UB, Uint32) //
        {
            VERIFY(UB.Type == SPIRVShaderResourceAttribs::ResourceType::UniformBuffer, "Unexpected resource type");
            ss << std::endl
               << std::setw(3) << ResNum << " Uniform Buffer   ";
            DumpResource(UB);
        },
        [&](const SPIRVShaderResourceAttribs& SB, Uint32) //
        {
            VERIFY(SB.Type == SPIRVShaderResourceAttribs::ResourceType::ROStorageBuffer ||
                       SB.Type == SPIRVShaderResourceAttribs::ResourceType::RWStorageBuffer,
                   "Unexpected resource type");
            ss << std::endl
               << std::setw(3) << ResNum
               << (SB.Type == SPIRVShaderResourceAttribs::ResourceType::ROStorageBuffer ? " RO Storage Buffer" : " RW Storage Buffer");
            DumpResource(SB);
        },
        [&](const SPIRVShaderResourceAttribs& Img, Uint32) //
        {
            if (Img.Type == SPIRVShaderResourceAttribs::ResourceType::StorageImage)
            {
                ss << std::endl
                   << std::setw(3) << ResNum << " Storage Image    ";
            }
            else if (Img.Type == SPIRVShaderResourceAttribs::ResourceType::StorageTexelBuffer)
            {
                ss << std::endl
                   << std::setw(3) << ResNum << " Storage Txl Buff ";
            }
            else
                UNEXPECTED("Unexpected resource type");
            DumpResource(Img);
        },
        [&](const SPIRVShaderResourceAttribs& SmplImg, Uint32) //
        {
            if (SmplImg.Type == SPIRVShaderResourceAttribs::ResourceType::SampledImage)
            {
                ss << std::endl
                   << std::setw(3) << ResNum << " Sampled Image    ";
            }
            else if (SmplImg.Type == SPIRVShaderResourceAttribs::ResourceType::UniformTexelBuffer)
            {
                ss << std::endl
                   << std::setw(3) << ResNum << " Uniform Txl Buff ";
            }
            else
                UNEXPECTED("Unexpected resource type");
            DumpResource(SmplImg);
        },
        [&](const SPIRVShaderResourceAttribs& AC, Uint32) //
        {
            VERIFY(AC.Type == SPIRVShaderResourceAttribs::ResourceType::AtomicCounter, "Unexpected resource type");
            ss << std::endl
               << std::setw(3) << ResNum << " Atomic Cntr      ";
            DumpResource(AC);
        },
        [&](const SPIRVShaderResourceAttribs& SepSmpl, Uint32) //
        {
            VERIFY(SepSmpl.Type == SPIRVShaderResourceAttribs::ResourceType::SeparateSampler, "Unexpected resource type");
            ss << std::endl
               << std::setw(3) << ResNum << " Separate Smpl    ";
            DumpResource(SepSmpl);
        },
        [&](const SPIRVShaderResourceAttribs& SepImg, Uint32) //
        {
            VERIFY(SepImg.Type == SPIRVShaderResourceAttribs::ResourceType::SeparateImage, "Unexpected resource type");
            ss << std::endl
               << std::setw(3) << ResNum << " Separate Img     ";
            DumpResource(SepImg);
        },
        [&](const SPIRVShaderResourceAttribs& InptAtt, Uint32) //
        {
            VERIFY(InptAtt.Type == SPIRVShaderResourceAttribs::ResourceType::InputAttachment, "Unexpected resource type");
            ss << std::endl
               << std::setw(3) << ResNum << " Input Attachment ";
            DumpResource(InptAtt);
        } //
    );
    VERIFY_EXPR(ResNum == GetTotalResources());

    return ss.str();
}



bool SPIRVShaderResources::IsCompatibleWith(const SPIRVShaderResources& Resources) const
{
    // clang-format off
    if( GetNumUBs()               != Resources.GetNumUBs()        ||
        GetNumSBs()               != Resources.GetNumSBs()        ||
        GetNumImgs()              != Resources.GetNumImgs()       ||
        GetNumSmpldImgs()         != Resources.GetNumSmpldImgs()  ||
        GetNumACs()               != Resources.GetNumACs()        ||
        GetNumSepImgs()           != Resources.GetNumSepImgs()    ||
        GetNumSepSmplrs()         != Resources.GetNumSepSmplrs()  ||
        GetNumInptAtts()          != Resources.GetNumInptAtts())
        return false;
    // clang-format on
    VERIFY_EXPR(GetTotalResources() == Resources.GetTotalResources());

    bool IsCompatible = true;
    ProcessResources(
        [&](const SPIRVShaderResourceAttribs& Res, Uint32 n) {
            const auto& Res2 = Resources.GetResource(n);
            if (!Res.IsCompatibleWith(Res2))
                IsCompatible = false;
        });

    return IsCompatible;
}

} // namespace Diligent
