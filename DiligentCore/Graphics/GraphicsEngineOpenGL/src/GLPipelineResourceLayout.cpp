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
#include <array>
#include "GLPipelineResourceLayout.hpp"
#include "Align.hpp"
#include "PlatformMisc.hpp"
#include "ShaderBase.hpp"

namespace Diligent
{


size_t GLPipelineResourceLayout::GetRequiredMemorySize(GLProgramResources*                  ProgramResources,
                                                       Uint32                               NumPrograms,
                                                       const PipelineResourceLayoutDesc&    ResourceLayout,
                                                       const SHADER_RESOURCE_VARIABLE_TYPE* AllowedVarTypes,
                                                       Uint32                               NumAllowedTypes)
{
    GLProgramResources::ResourceCounters Counters;
    for (Uint32 prog = 0; prog < NumPrograms; ++prog)
    {
        ProgramResources[prog].CountResources(ResourceLayout, AllowedVarTypes, NumAllowedTypes, Counters);
    }

    // clang-format off
    size_t RequiredSize = Counters.NumUBs           * sizeof(UniformBuffBindInfo)   + 
                          Counters.NumSamplers      * sizeof(SamplerBindInfo)       +
                          Counters.NumImages        * sizeof(ImageBindInfo)         +
                          Counters.NumStorageBlocks * sizeof(StorageBufferBindInfo) +
                          NumPrograms               * sizeof(GLProgramResources::ResourceCounters);
    // clang-format on
    return RequiredSize;
}

void GLPipelineResourceLayout::Initialize(GLProgramResources*                  ProgramResources,
                                          Uint32                               NumPrograms,
                                          PIPELINE_TYPE                        PipelineType,
                                          const PipelineResourceLayoutDesc&    ResourceLayout,
                                          const SHADER_RESOURCE_VARIABLE_TYPE* AllowedVarTypes,
                                          Uint32                               NumAllowedTypes,
                                          GLProgramResourceCache*              pResourceCache)
{
    GLProgramResources::ResourceCounters Counters;

    for (Uint32 prog = 0; prog < NumPrograms; ++prog)
    {
        ProgramResources[prog].CountResources(ResourceLayout, AllowedVarTypes, NumAllowedTypes, Counters);
    }

    // Initialize offsets
    size_t CurrentOffset = 0;

    auto AdvanceOffset = [&CurrentOffset](size_t NumBytes) //
    {
        constexpr size_t MaxOffset = std::numeric_limits<OffsetType>::max();
        VERIFY(CurrentOffset <= MaxOffset, "Current offser (", CurrentOffset, ") exceeds max allowed value (", MaxOffset, ")");
        (void)MaxOffset;
        auto Offset = static_cast<OffsetType>(CurrentOffset);
        CurrentOffset += NumBytes;
        return Offset;
    };

    // clang-format off
    auto UBOffset         = AdvanceOffset(Counters.NumUBs           * sizeof(UniformBuffBindInfo)  ); (void)UBOffset; // To suppress warning
    m_SamplerOffset       = AdvanceOffset(Counters.NumSamplers      * sizeof(SamplerBindInfo)      );
    m_ImageOffset         = AdvanceOffset(Counters.NumImages        * sizeof(ImageBindInfo)        );
    m_StorageBufferOffset = AdvanceOffset(Counters.NumStorageBlocks * sizeof(StorageBufferBindInfo));
    m_VariableEndOffset   = AdvanceOffset(0);
    // clang-format off
    m_NumPrograms         = static_cast<Uint8>(NumPrograms);
    VERIFY_EXPR(m_NumPrograms == NumPrograms);
    auto TotalMemorySize = m_VariableEndOffset + m_NumPrograms * sizeof(GLProgramResources::ResourceCounters);
    VERIFY_EXPR(TotalMemorySize == GetRequiredMemorySize(ProgramResources, NumPrograms, ResourceLayout, AllowedVarTypes, NumAllowedTypes));

    m_PipelineType = PipelineType;

    auto& ResLayoutDataAllocator = GetRawAllocator();
    if (TotalMemorySize)
    {
        auto* pRawMem = ALLOCATE_RAW(ResLayoutDataAllocator, "Raw memory buffer for shader resource layout resources", TotalMemorySize);
        m_ResourceBuffer = std::unique_ptr<void, STDDeleterRawMem<void> >(pRawMem, ResLayoutDataAllocator);
    }

    // clang-format off
    VERIFY_EXPR(Counters.NumUBs           == GetNumUBs()           );
    VERIFY_EXPR(Counters.NumSamplers      == GetNumSamplers()      );
    VERIFY_EXPR(Counters.NumImages        == GetNumImages()        );
    VERIFY_EXPR(Counters.NumStorageBlocks == GetNumStorageBuffers());
    // clang-format on

    // Current resource index for every resource type
    GLProgramResources::ResourceCounters VarCounters = {};

    Uint32 UniformBindingSlots = 0;
    Uint32 SamplerBindingSlots = 0;
    Uint32 ImageBindingSlots   = 0;
    Uint32 SSBOBindingSlots    = 0;

#ifdef DILIGENT_DEBUG
    const Uint32 DbgAllowedTypeBits = GetAllowedTypeBits(AllowedVarTypes, NumAllowedTypes);
#endif
    for (Uint32 prog = 0; prog < NumPrograms; ++prog)
    {
        const auto& Resources    = ProgramResources[prog];
        auto        ShaderStages = Resources.GetShaderStages();
        Resources.ProcessConstResources(
            [&](const GLProgramResources::UniformBufferInfo& UB) //
            {
                auto VarType = GetShaderVariableType(ShaderStages, UB.Name, ResourceLayout);
                VERIFY_EXPR(IsAllowedType(VarType, DbgAllowedTypeBits));
                auto* pUBVar = new (&GetResource<UniformBuffBindInfo>(VarCounters.NumUBs++))
                    UniformBuffBindInfo //
                    {
                        UB,
                        *this,
                        VarType //
                    };
                UniformBindingSlots = std::max(UniformBindingSlots, pUBVar->m_Attribs.Binding + pUBVar->m_Attribs.ArraySize);
            },
            [&](const GLProgramResources::SamplerInfo& Sam) //
            {
                auto VarType = GetShaderVariableType(ShaderStages, Sam.Name, ResourceLayout);
                VERIFY_EXPR(IsAllowedType(VarType, DbgAllowedTypeBits));
                Int32 ImtblSamplerIdx = -1;
                if (Sam.ResourceType == SHADER_RESOURCE_TYPE_TEXTURE_SRV)
                {
                    ImtblSamplerIdx = FindImmutableSampler(ResourceLayout.ImmutableSamplers, ResourceLayout.NumImmutableSamplers, ShaderStages,
                                                           Sam.Name, nullptr);
                }
                auto* pSamVar = new (&GetResource<SamplerBindInfo>(VarCounters.NumSamplers++))
                    SamplerBindInfo //
                    {
                        Sam,
                        *this,
                        VarType,
                        ImtblSamplerIdx //
                    };
                SamplerBindingSlots = std::max(SamplerBindingSlots, pSamVar->m_Attribs.Binding + pSamVar->m_Attribs.ArraySize);
            },
            [&](const GLProgramResources::ImageInfo& Img) //
            {
                auto VarType = GetShaderVariableType(ShaderStages, Img.Name, ResourceLayout);
                VERIFY_EXPR(IsAllowedType(VarType, DbgAllowedTypeBits));
                auto* pImgVar = new (&GetResource<ImageBindInfo>(VarCounters.NumImages++))
                    ImageBindInfo //
                    {
                        Img,
                        *this,
                        VarType //
                    };
                ImageBindingSlots = std::max(ImageBindingSlots, pImgVar->m_Attribs.Binding + pImgVar->m_Attribs.ArraySize);
            },
            [&](const GLProgramResources::StorageBlockInfo& SB) //
            {
                auto VarType = GetShaderVariableType(ShaderStages, SB.Name, ResourceLayout);
                VERIFY_EXPR(IsAllowedType(VarType, DbgAllowedTypeBits));
                auto* pSSBOVar = new (&GetResource<StorageBufferBindInfo>(VarCounters.NumStorageBlocks++))
                    StorageBufferBindInfo //
                    {
                        SB,
                        *this,
                        VarType //
                    };
                SSBOBindingSlots = std::max(SSBOBindingSlots, pSSBOVar->m_Attribs.Binding + pSSBOVar->m_Attribs.ArraySize);
            },
            &ResourceLayout,
            AllowedVarTypes,
            NumAllowedTypes //
        );

        new (&GetProgramVarEndOffsets(prog)) GLProgramResources::ResourceCounters{VarCounters};
        while (ShaderStages != SHADER_TYPE_UNKNOWN)
        {
            auto Stage                = static_cast<SHADER_TYPE>(Uint32{ShaderStages} & ~(Uint32{ShaderStages} - 1));
            auto ShaderInd            = GetShaderTypePipelineIndex(Stage, PipelineType);
            m_ProgramIndex[ShaderInd] = static_cast<Int8>(prog);
            ShaderStages              = static_cast<SHADER_TYPE>(Uint32{ShaderStages} & ~Stage);
        }
    }

    // clang-format off
    VERIFY(VarCounters.NumUBs           == GetNumUBs(),             "Not all UBs are initialized which will cause a crash when dtor is called");
    VERIFY(VarCounters.NumSamplers      == GetNumSamplers(),        "Not all Samplers are initialized which will cause a crash when dtor is called");
    VERIFY(VarCounters.NumImages        == GetNumImages(),          "Not all Images are initialized which will cause a crash when dtor is called");
    VERIFY(VarCounters.NumStorageBlocks == GetNumStorageBuffers(),  "Not all SSBOs are initialized which will cause a crash when dtor is called");
    // clang-format on

    m_pResourceCache = pResourceCache;
    if (m_pResourceCache != nullptr && !m_pResourceCache->IsInitialized())
    {
        // NOTE that here we are using max bind points required to cache only the shader variables of allowed types!
        m_pResourceCache->Initialize(UniformBindingSlots, SamplerBindingSlots, ImageBindingSlots, SSBOBindingSlots, GetRawAllocator());
    }
}

GLPipelineResourceLayout::~GLPipelineResourceLayout()
{
    // clang-format off
    HandleResources(
        [&](UniformBuffBindInfo& ub)
        {
            ub.~UniformBuffBindInfo();
        },

        [&](SamplerBindInfo& sam)
        {
            sam.~SamplerBindInfo();
        },

        [&](ImageBindInfo& img)
        {
            img.~ImageBindInfo();
        },

        [&](StorageBufferBindInfo& ssbo)
        {
            ssbo.~StorageBufferBindInfo();
        }
    );
    // clang-format on
}


void GLPipelineResourceLayout::UniformBuffBindInfo::BindResource(IDeviceObject* pBuffer,
                                                                 Uint32         ArrayIndex)
{
    DEV_CHECK_ERR(ArrayIndex < m_Attribs.ArraySize, "Array index (", ArrayIndex, ") is out of range for variable '", m_Attribs.Name, "'. Max allowed index: ", m_Attribs.ArraySize - 1);
    VERIFY(m_ParentResLayout.m_pResourceCache != nullptr, "Resource cache is not initialized");
    auto& ResourceCache = *m_ParentResLayout.m_pResourceCache;

    VERIFY_EXPR(m_Attribs.ResourceType == SHADER_RESOURCE_TYPE_CONSTANT_BUFFER);

    // We cannot use ValidatedCast<> here as the resource retrieved from the
    // resource mapping can be of wrong type
    RefCntAutoPtr<BufferGLImpl> pBuffGLImpl(pBuffer, IID_BufferGL);
#ifdef DILIGENT_DEVELOPMENT
    {
        const auto& CachedUB = ResourceCache.GetConstUB(m_Attribs.Binding + ArrayIndex);
        VerifyConstantBufferBinding(m_Attribs, GetType(), ArrayIndex, pBuffer, pBuffGLImpl.RawPtr(), CachedUB.pBuffer.RawPtr());
    }
#endif

    ResourceCache.SetUniformBuffer(m_Attribs.Binding + ArrayIndex, std::move(pBuffGLImpl));
}



void GLPipelineResourceLayout::SamplerBindInfo::BindResource(IDeviceObject* pView,
                                                             Uint32         ArrayIndex)
{
    DEV_CHECK_ERR(ArrayIndex < m_Attribs.ArraySize, "Array index (", ArrayIndex, ") is out of range for variable '", m_Attribs.Name, "'. Max allowed index: ", m_Attribs.ArraySize - 1);
    VERIFY(m_ParentResLayout.m_pResourceCache != nullptr, "Resource cache is not initialized");
    auto& ResourceCache = *m_ParentResLayout.m_pResourceCache;

    VERIFY_EXPR(m_Attribs.ResourceType == SHADER_RESOURCE_TYPE_BUFFER_SRV ||
                m_Attribs.ResourceType == SHADER_RESOURCE_TYPE_TEXTURE_SRV);

    if (m_Attribs.ResourceType == SHADER_RESOURCE_TYPE_TEXTURE_SRV)
    {
        // We cannot use ValidatedCast<> here as the resource retrieved from the
        // resource mapping can be of wrong type
        RefCntAutoPtr<TextureViewGLImpl> pViewGL(pView, IID_TextureViewGL);
#ifdef DILIGENT_DEVELOPMENT
        {
            auto& CachedTexSampler = ResourceCache.GetConstSampler(m_Attribs.Binding + ArrayIndex);
            VerifyResourceViewBinding(m_Attribs, GetType(), ArrayIndex, pView, pViewGL.RawPtr(), {TEXTURE_VIEW_SHADER_RESOURCE}, CachedTexSampler.pView.RawPtr());
            if (m_ImtblSamplerIdx >= 0)
            {
                VERIFY(CachedTexSampler.pSampler != nullptr, "Immutable samplers must be initialized by PipelineStateGLImpl::InitializeSRBResourceCache!");
            }
        }
#endif
        ResourceCache.SetTexSampler(m_Attribs.Binding + ArrayIndex, std::move(pViewGL), m_ImtblSamplerIdx < 0);
    }
    else if (m_Attribs.ResourceType == SHADER_RESOURCE_TYPE_BUFFER_SRV)
    {
        // We cannot use ValidatedCast<> here as the resource retrieved from the
        // resource mapping can be of wrong type
        RefCntAutoPtr<BufferViewGLImpl> pViewGL(pView, IID_BufferViewGL);
#ifdef DILIGENT_DEVELOPMENT
        {
            auto& CachedBuffSampler = ResourceCache.GetConstSampler(m_Attribs.Binding + ArrayIndex);
            VerifyResourceViewBinding(m_Attribs, GetType(), ArrayIndex, pView, pViewGL.RawPtr(), {BUFFER_VIEW_SHADER_RESOURCE}, CachedBuffSampler.pView.RawPtr());
            if (pViewGL != nullptr)
            {
                const auto& ViewDesc = pViewGL->GetDesc();
                const auto& BuffDesc = pViewGL->GetBuffer()->GetDesc();
                if (!((BuffDesc.Mode == BUFFER_MODE_FORMATTED && ViewDesc.Format.ValueType != VT_UNDEFINED) || BuffDesc.Mode == BUFFER_MODE_RAW))
                {
                    LOG_ERROR_MESSAGE("Error binding buffer view '", ViewDesc.Name, "' of buffer '", BuffDesc.Name, "' to shader variable '",
                                      m_Attribs.Name, ": formatted buffer view is expected.");
                }
            }
        }
#endif
        ResourceCache.SetBufSampler(m_Attribs.Binding + ArrayIndex, std::move(pViewGL));
    }
    else
    {
        UNEXPECTED("Unexpected resource type ", GetShaderResourceTypeLiteralName(m_Attribs.ResourceType), ". Texture SRV or buffer SRV is expected.");
    }
}


void GLPipelineResourceLayout::ImageBindInfo::BindResource(IDeviceObject* pView,
                                                           Uint32         ArrayIndex)
{
    DEV_CHECK_ERR(ArrayIndex < m_Attribs.ArraySize, "Array index (", ArrayIndex, ") is out of range for variable '", m_Attribs.Name, "'. Max allowed index: ", m_Attribs.ArraySize - 1);
    VERIFY(m_ParentResLayout.m_pResourceCache != nullptr, "Resource cache is not initialized");
    auto& ResourceCache = *m_ParentResLayout.m_pResourceCache;

    if (m_Attribs.ResourceType == SHADER_RESOURCE_TYPE_TEXTURE_UAV)
    {
        // We cannot use ValidatedCast<> here as the resource retrieved from the
        // resource mapping can be of wrong type
        RefCntAutoPtr<TextureViewGLImpl> pViewGL(pView, IID_TextureViewGL);
#ifdef DILIGENT_DEVELOPMENT
        {
            auto& CachedUAV = ResourceCache.GetConstImage(m_Attribs.Binding + ArrayIndex);
            VerifyResourceViewBinding(m_Attribs, GetType(), ArrayIndex, pView, pViewGL.RawPtr(), {TEXTURE_VIEW_UNORDERED_ACCESS}, CachedUAV.pView.RawPtr());
        }
#endif
        ResourceCache.SetTexImage(m_Attribs.Binding + ArrayIndex, std::move(pViewGL));
    }
    else if (m_Attribs.ResourceType == SHADER_RESOURCE_TYPE_BUFFER_UAV)
    {
        // We cannot use ValidatedCast<> here as the resource retrieved from the
        // resource mapping can be of wrong type
        RefCntAutoPtr<BufferViewGLImpl> pViewGL(pView, IID_BufferViewGL);
#ifdef DILIGENT_DEVELOPMENT
        {
            auto& CachedUAV = ResourceCache.GetConstImage(m_Attribs.Binding + ArrayIndex);
            VerifyResourceViewBinding(m_Attribs, GetType(), ArrayIndex, pView, pViewGL.RawPtr(), {BUFFER_VIEW_UNORDERED_ACCESS}, CachedUAV.pView.RawPtr());
            if (pViewGL != nullptr)
            {
                const auto& ViewDesc = pViewGL->GetDesc();
                const auto& BuffDesc = pViewGL->GetBuffer()->GetDesc();
                if (!((BuffDesc.Mode == BUFFER_MODE_FORMATTED && ViewDesc.Format.ValueType != VT_UNDEFINED) || BuffDesc.Mode == BUFFER_MODE_RAW))
                {
                    LOG_ERROR_MESSAGE("Error binding buffer view '", ViewDesc.Name, "' of buffer '", BuffDesc.Name, "' to shader variable '",
                                      m_Attribs.Name, ": formatted buffer view is expected.");
                }
            }
        }
#endif
        ResourceCache.SetBufImage(m_Attribs.Binding + ArrayIndex, std::move(pViewGL));
    }
    else
    {
        UNEXPECTED("Unexpected resource type ", GetShaderResourceTypeLiteralName(m_Attribs.ResourceType), ". Texture UAV or buffer UAV is expected.");
    }
}



void GLPipelineResourceLayout::StorageBufferBindInfo::BindResource(IDeviceObject* pView,
                                                                   Uint32         ArrayIndex)
{
    DEV_CHECK_ERR(ArrayIndex < m_Attribs.ArraySize, "Array index (", ArrayIndex, ") is out of range for variable '", m_Attribs.Name, "'. Max allowed index: ", m_Attribs.ArraySize - 1);
    VERIFY(m_ParentResLayout.m_pResourceCache != nullptr, "Resource cache is not initialized");
    auto& ResourceCache = *m_ParentResLayout.m_pResourceCache;
    VERIFY_EXPR(m_Attribs.ResourceType == SHADER_RESOURCE_TYPE_BUFFER_UAV);

    // We cannot use ValidatedCast<> here as the resource retrieved from the
    // resource mapping can be of wrong type
    RefCntAutoPtr<BufferViewGLImpl> pViewGL(pView, IID_BufferViewGL);
#ifdef DILIGENT_DEVELOPMENT
    {
        auto& CachedSSBO = ResourceCache.GetConstSSBO(m_Attribs.Binding + ArrayIndex);
        // HLSL structured buffers are mapped to SSBOs in GLSL
        VerifyResourceViewBinding(m_Attribs, GetType(), ArrayIndex, pView, pViewGL.RawPtr(), {BUFFER_VIEW_SHADER_RESOURCE, BUFFER_VIEW_UNORDERED_ACCESS}, CachedSSBO.pBufferView.RawPtr());
        if (pViewGL != nullptr)
        {
            const auto& ViewDesc = pViewGL->GetDesc();
            const auto& BuffDesc = pViewGL->GetBuffer()->GetDesc();
            if (BuffDesc.Mode != BUFFER_MODE_STRUCTURED && BuffDesc.Mode != BUFFER_MODE_RAW)
            {
                LOG_ERROR_MESSAGE("Error binding buffer view '", ViewDesc.Name, "' of buffer '", BuffDesc.Name, "' to shader variable '",
                                  m_Attribs.Name, ": structured buffer view is expected.");
            }
        }
    }
#endif
    ResourceCache.SetSSBO(m_Attribs.Binding + ArrayIndex, std::move(pViewGL));
}



// Helper template class that facilitates binding CBs, SRVs, and UAVs
class BindResourceHelper
{
public:
    BindResourceHelper(IResourceMapping& RM, SHADER_TYPE SS, Uint32 Fl) :
        // clang-format off
        ResourceMapping {RM},
        ShaderStage     {SS},
        Flags           {Fl}
    // clang-format on
    {
    }

    template <typename ResourceType>
    void Bind(ResourceType& Res)
    {
        if ((Flags & (1 << Res.GetType())) == 0)
            return;

        for (Uint16 elem = 0; elem < Res.m_Attribs.ArraySize; ++elem)
        {
            if ((Res.m_Attribs.ShaderStages & ShaderStage) == 0)
                continue;

            if ((Flags & BIND_SHADER_RESOURCES_KEEP_EXISTING) && Res.IsBound(elem))
                continue;

            const auto*                  VarName = Res.m_Attribs.Name;
            RefCntAutoPtr<IDeviceObject> pRes;
            ResourceMapping.GetResource(VarName, &pRes, elem);
            if (pRes)
            {
                //  Call non-virtual function
                Res.BindResource(pRes, elem);
            }
            else
            {
                if ((Flags & BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED) && !Res.IsBound(elem))
                    LOG_ERROR_MESSAGE("Unable to bind resource to shader variable '", VarName, "': resource is not found in the resource mapping");
            }
        }
    }

private:
    IResourceMapping& ResourceMapping;
    const SHADER_TYPE ShaderStage;
    const Uint32      Flags;
};


void GLPipelineResourceLayout::BindResources(SHADER_TYPE ShaderStage, IResourceMapping* pResourceMapping, Uint32 Flags, const GLProgramResourceCache& dbgResourceCache)
{
    VERIFY(&dbgResourceCache == m_pResourceCache, "Resource cache does not match the cache provided at initialization");

    if (pResourceMapping == nullptr)
    {
        LOG_ERROR_MESSAGE("Failed to bind resources: resource mapping is null");
        return;
    }

    if ((Flags & BIND_SHADER_RESOURCES_UPDATE_ALL) == 0)
        Flags |= BIND_SHADER_RESOURCES_UPDATE_ALL;

    BindResourceHelper BindResHelper(*pResourceMapping, ShaderStage, Flags);

    // clang-format off
    HandleResources(
        [&](UniformBuffBindInfo& ub)
        {
            BindResHelper.Bind(ub);
        },

        [&](SamplerBindInfo& sam)
        {
            BindResHelper.Bind(sam);
        },

        [&](ImageBindInfo& img)
        {
            BindResHelper.Bind(img);
        },

        [&](StorageBufferBindInfo& ssbo)
        {
            BindResHelper.Bind(ssbo);
        }
    );
    // clang-format on
}


template <typename ResourceType>
IShaderResourceVariable* GLPipelineResourceLayout::GetResourceByName(SHADER_TYPE ShaderStage, const Char* Name)
{
    auto NumResources = GetNumResources<ResourceType>();
    for (Uint32 res = 0; res < NumResources; ++res)
    {
        auto& Resource = GetResource<ResourceType>(res);
        if ((Resource.m_Attribs.ShaderStages & ShaderStage) != 0 && strcmp(Resource.m_Attribs.Name, Name) == 0)
            return &Resource;
    }

    return nullptr;
}


IShaderResourceVariable* GLPipelineResourceLayout::GetShaderVariable(SHADER_TYPE ShaderStage, const Char* Name)
{
    VERIFY_EXPR(IsConsistentShaderType(ShaderStage, static_cast<PIPELINE_TYPE>(m_PipelineType)));

    if (auto* pUB = GetResourceByName<UniformBuffBindInfo>(ShaderStage, Name))
        return pUB;

    if (auto* pSampler = GetResourceByName<SamplerBindInfo>(ShaderStage, Name))
        return pSampler;

    if (auto* pImage = GetResourceByName<ImageBindInfo>(ShaderStage, Name))
        return pImage;

    if (auto* pSSBO = GetResourceByName<StorageBufferBindInfo>(ShaderStage, Name))
        return pSSBO;

    return nullptr;
}

Uint32 GLPipelineResourceLayout::GetNumVariables(SHADER_TYPE ShaderStage) const
{
    VERIFY_EXPR(IsConsistentShaderType(ShaderStage, static_cast<PIPELINE_TYPE>(m_PipelineType)));
    VERIFY(IsPowerOfTwo(Uint32{ShaderStage}), "Only one shader stage must be specified");
    auto ShaderInd = GetShaderTypePipelineIndex(ShaderStage, static_cast<PIPELINE_TYPE>(m_PipelineType));
    auto ProgIdx   = m_ProgramIndex[ShaderInd];

    if (ProgIdx < 0)
    {
        LOG_WARNING_MESSAGE("Unable to get the number of variables in shader stage ", GetShaderTypeLiteralName(ShaderStage),
                            " as the stage is inactive");
        return 0;
    }

    const auto& VariableEndOffset   = GetProgramVarEndOffsets(ProgIdx);
    const auto& VariableStartOffset = ProgIdx > 0 ? GetProgramVarEndOffsets(ProgIdx - 1) : GLProgramResources::ResourceCounters{};

    // clang-format off
    Uint32 NumVars = VariableEndOffset.NumUBs           - VariableStartOffset.NumUBs      + 
                     VariableEndOffset.NumSamplers      - VariableStartOffset.NumSamplers + 
                     VariableEndOffset.NumImages        - VariableStartOffset.NumImages   +
                     VariableEndOffset.NumStorageBlocks - VariableStartOffset.NumStorageBlocks;
    // clang-format on

#ifdef DILIGENT_DEBUG
    {
        Uint32 DbgNumVars = 0;
        auto   CountVar   = [&](const GLVariableBase& Var) {
            DbgNumVars += ((Var.m_Attribs.ShaderStages & ShaderStage) != 0) ? 1 : 0;
        };
        HandleConstResources(CountVar, CountVar, CountVar, CountVar);
        VERIFY_EXPR(DbgNumVars == NumVars);
    }
#endif

    return NumVars;
}

class ShaderVariableLocator
{
public:
    ShaderVariableLocator(GLPipelineResourceLayout& _Layout,
                          Uint32                    _Index) :
        // clang-format off
        Layout {_Layout},
        Index  {_Index}
    // clang-format on
    {
    }

    template <typename ResourceType>
    IShaderResourceVariable* TryResource(Uint32 StartVarOffset, Uint32 EndVarOffset)
    {
        auto NumResources = EndVarOffset - StartVarOffset;
        if (Index < NumResources)
            return &Layout.GetResource<ResourceType>(StartVarOffset + Index);
        else
        {
            Index -= NumResources;
            return nullptr;
        }
    }

private:
    GLPipelineResourceLayout& Layout;
    Uint32                    Index;
};


IShaderResourceVariable* GLPipelineResourceLayout::GetShaderVariable(SHADER_TYPE ShaderStage, Uint32 Index)
{
    VERIFY(IsPowerOfTwo(Uint32{ShaderStage}), "Only one shader stage must be specified");
    auto ShaderInd = GetShaderTypePipelineIndex(ShaderStage, static_cast<PIPELINE_TYPE>(m_PipelineType));
    auto ProgIdx   = m_ProgramIndex[ShaderInd];

    if (ProgIdx < 0)
        return nullptr;

    const auto& VariableEndOffset   = GetProgramVarEndOffsets(ProgIdx);
    const auto& VariableStartOffset = ProgIdx > 0 ? GetProgramVarEndOffsets(ProgIdx - 1) : GLProgramResources::ResourceCounters{};

    ShaderVariableLocator VarLocator(*this, Index);

    if (auto* pUB = VarLocator.TryResource<UniformBuffBindInfo>(VariableStartOffset.NumUBs, VariableEndOffset.NumUBs))
        return pUB;

    if (auto* pSampler = VarLocator.TryResource<SamplerBindInfo>(VariableStartOffset.NumSamplers, VariableEndOffset.NumSamplers))
        return pSampler;

    if (auto* pImage = VarLocator.TryResource<ImageBindInfo>(VariableStartOffset.NumImages, VariableEndOffset.NumImages))
        return pImage;

    if (auto* pSSBO = VarLocator.TryResource<StorageBufferBindInfo>(VariableStartOffset.NumStorageBlocks, VariableEndOffset.NumStorageBlocks))
        return pSSBO;

    LOG_ERROR(Index, " is not a valid variable index.");
    return nullptr;
}



class ShaderVariableIndexLocator
{
public:
    ShaderVariableIndexLocator(const GLPipelineResourceLayout& _Layout, const GLPipelineResourceLayout::GLVariableBase& Variable) :
        // clang-format off
        Layout   {_Layout},
        VarOffset(reinterpret_cast<const Uint8*>(&Variable) - reinterpret_cast<const Uint8*>(_Layout.m_ResourceBuffer.get()))
    // clang-format on
    {}

    template <typename ResourceType>
    bool TryResource(Uint32 NextResourceTypeOffset, Uint32 FirstVarOffset, Uint32 LastVarOffset)
    {
        if (VarOffset < NextResourceTypeOffset)
        {
            auto RelativeOffset = VarOffset - Layout.GetResourceOffset<ResourceType>();
            DEV_CHECK_ERR(RelativeOffset % sizeof(ResourceType) == 0, "Offset is not multiple of resource type (", sizeof(ResourceType), ")");
            RelativeOffset /= sizeof(ResourceType);
            VERIFY(RelativeOffset >= FirstVarOffset && RelativeOffset < LastVarOffset,
                   "Relative offset is out of bounds which either means the variable does not belong to this SRB or "
                   "there is a bug in varaible offsets");
            Index += static_cast<Uint32>(RelativeOffset) - FirstVarOffset;
            return true;
        }
        else
        {
            Index += LastVarOffset - FirstVarOffset;
            return false;
        }
    }

    Uint32 GetIndex() const { return Index; }

private:
    const GLPipelineResourceLayout& Layout;
    const size_t                    VarOffset;
    Uint32                          Index = 0;
};


Uint32 GLPipelineResourceLayout::GetVariableIndex(const GLVariableBase& Var) const
{
    if (!m_ResourceBuffer)
    {
        LOG_ERROR("This shader resource layout does not have resources");
        return static_cast<Uint32>(-1);
    }

    auto FirstStage = static_cast<SHADER_TYPE>(Uint32{Var.m_Attribs.ShaderStages} & ~(Uint32{Var.m_Attribs.ShaderStages} - 1));
    auto ProgIdx    = m_ProgramIndex[GetShaderTypePipelineIndex(FirstStage, static_cast<PIPELINE_TYPE>(m_PipelineType))];
    VERIFY(ProgIdx >= 0, "This shader stage is not initialized in the resource layout");

    const auto& VariableEndOffset   = GetProgramVarEndOffsets(ProgIdx);
    const auto& VariableStartOffset = ProgIdx > 0 ? GetProgramVarEndOffsets(ProgIdx - 1) : GLProgramResources::ResourceCounters{};

    ShaderVariableIndexLocator IdxLocator(*this, Var);

    if (IdxLocator.TryResource<UniformBuffBindInfo>(m_SamplerOffset, VariableStartOffset.NumUBs, VariableEndOffset.NumUBs))
        return IdxLocator.GetIndex();

    if (IdxLocator.TryResource<SamplerBindInfo>(m_ImageOffset, VariableStartOffset.NumSamplers, VariableEndOffset.NumSamplers))
        return IdxLocator.GetIndex();

    if (IdxLocator.TryResource<ImageBindInfo>(m_StorageBufferOffset, VariableStartOffset.NumImages, VariableEndOffset.NumImages))
        return IdxLocator.GetIndex();

    if (IdxLocator.TryResource<StorageBufferBindInfo>(m_VariableEndOffset, VariableStartOffset.NumStorageBlocks, VariableEndOffset.NumStorageBlocks))
        return IdxLocator.GetIndex();

    LOG_ERROR("Failed to get variable index. The variable ", &Var, " does not belong to this shader resource layout");
    return static_cast<Uint32>(-1);
}

void GLPipelineResourceLayout::CopyResources(GLProgramResourceCache& DstCache) const
{
    VERIFY_EXPR(m_pResourceCache != nullptr);
    const auto& SrcCache = *m_pResourceCache;
    // clang-format off
    VERIFY( DstCache.GetUBCount()      >= SrcCache.GetUBCount(),      "Dst cache is not large enough to contain all CBs" );
    VERIFY( DstCache.GetSamplerCount() >= SrcCache.GetSamplerCount(), "Dst cache is not large enough to contain all SRVs" );
    VERIFY( DstCache.GetImageCount()   >= SrcCache.GetImageCount(),   "Dst cache is not large enough to contain all samplers" );
    VERIFY( DstCache.GetSSBOCount()    >= SrcCache.GetSSBOCount(),    "Dst cache is not large enough to contain all UAVs" );
    // clang-format on

    HandleConstResources(
        [&](const UniformBuffBindInfo& UB) //
        {
            for (auto binding = UB.m_Attribs.Binding; binding < UB.m_Attribs.Binding + UB.m_Attribs.ArraySize; ++binding)
            {
                auto pBufferGL = SrcCache.GetConstUB(binding).pBuffer;
                DstCache.SetUniformBuffer(binding, std::move(pBufferGL));
            }
        },

        [&](const SamplerBindInfo& Sam) //
        {
            for (auto binding = Sam.m_Attribs.Binding; binding < Sam.m_Attribs.Binding + Sam.m_Attribs.ArraySize; ++binding)
            {
                const auto& SrcSam = SrcCache.GetConstSampler(binding);
                DstCache.CopySampler(binding, SrcSam);
            }
        },

        [&](const ImageBindInfo& Img) //
        {
            for (auto binding = Img.m_Attribs.Binding; binding < Img.m_Attribs.Binding + Img.m_Attribs.ArraySize; ++binding)
            {
                const auto& SrcImg = SrcCache.GetConstImage(binding);
                DstCache.CopyImage(binding, SrcImg);
            }
        },

        [&](const StorageBufferBindInfo& SSBO) //
        {
            for (auto binding = SSBO.m_Attribs.Binding; binding < SSBO.m_Attribs.Binding + SSBO.m_Attribs.ArraySize; ++binding)
            {
                auto pBufferView = SrcCache.GetConstSSBO(binding).pBufferView;
                DstCache.SetSSBO(binding, std::move(pBufferView));
            }
        } //
    );
}

#ifdef DILIGENT_DEVELOPMENT
bool GLPipelineResourceLayout::dvpVerifyBindings(const GLProgramResourceCache& ResourceCache) const
{
#    define LOG_MISSING_BINDING(VarType, BindInfo, BindPt) LOG_ERROR_MESSAGE("No resource is bound to ", VarType, " variable '", BindInfo.m_Attribs.GetPrintName(BindPt - BindInfo.m_Attribs.Binding), "'")

    bool BindingsOK = true;
    HandleConstResources(
        [&](const UniformBuffBindInfo& ub) //
        {
            for (Uint32 BindPoint = ub.m_Attribs.Binding; BindPoint < Uint32{ub.m_Attribs.Binding} + ub.m_Attribs.ArraySize; ++BindPoint)
            {
                if (!ResourceCache.IsUBBound(BindPoint))
                {
                    LOG_MISSING_BINDING("constant buffer", ub, BindPoint);
                    BindingsOK = false;
                }
            }
        },

        [&](const SamplerBindInfo& sam) //
        {
            for (Uint32 BindPoint = sam.m_Attribs.Binding; BindPoint < Uint32{sam.m_Attribs.Binding} + sam.m_Attribs.ArraySize; ++BindPoint)
            {
                VERIFY_EXPR(sam.m_Attribs.ResourceType == SHADER_RESOURCE_TYPE_TEXTURE_SRV ||
                            sam.m_Attribs.ResourceType == SHADER_RESOURCE_TYPE_BUFFER_SRV);
                if (!ResourceCache.IsSamplerBound(BindPoint, sam.m_Attribs.ResourceType == SHADER_RESOURCE_TYPE_TEXTURE_SRV))
                {
                    LOG_MISSING_BINDING("texture", sam, BindPoint);
                    BindingsOK = false;
                }
                else
                {
                    const auto& CachedSampler = ResourceCache.GetConstSampler(BindPoint);
                    if (sam.m_ImtblSamplerIdx >= 0 && CachedSampler.pSampler == nullptr)
                    {
                        LOG_ERROR_MESSAGE("Immutable sampler is not initialized for texture '", sam.m_Attribs.Name, "'");
                        BindingsOK = false;
                    }
                }
            }
        },

        [&](const ImageBindInfo& img) //
        {
            for (Uint32 BindPoint = img.m_Attribs.Binding; BindPoint < Uint32{img.m_Attribs.Binding} + img.m_Attribs.ArraySize; ++BindPoint)
            {
                VERIFY_EXPR(img.m_Attribs.ResourceType == SHADER_RESOURCE_TYPE_TEXTURE_UAV ||
                            img.m_Attribs.ResourceType == SHADER_RESOURCE_TYPE_BUFFER_UAV);
                if (!ResourceCache.IsImageBound(BindPoint, img.m_Attribs.ResourceType == SHADER_RESOURCE_TYPE_TEXTURE_UAV))
                {
                    LOG_MISSING_BINDING("texture UAV", img, BindPoint);
                    BindingsOK = false;
                }
            }
        },

        [&](const StorageBufferBindInfo& ssbo) //
        {
            for (Uint32 BindPoint = ssbo.m_Attribs.Binding; BindPoint < Uint32{ssbo.m_Attribs.Binding} + ssbo.m_Attribs.ArraySize; ++BindPoint)
            {
                if (!ResourceCache.IsSSBOBound(BindPoint))
                {
                    LOG_MISSING_BINDING("buffer", ssbo, BindPoint);
                    BindingsOK = false;
                }
            }
        });
#    undef LOG_MISSING_BINDING

    return BindingsOK;
}

#endif

} // namespace Diligent
