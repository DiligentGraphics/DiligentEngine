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

// GLPipelineResourceLayout class manages resource bindings for all stages in a pipeline

//
//
//                                                      To            program              resource                  cache
//
//                                               A          A                  A        A              A           A              A            A
//                                               |          |                  |        |              |           |              |            |
//                                            Binding    Binding            Binding   Binding       Binding     Binding        Binding      Binding
//      ___________________                  ____|__________|__________________|________|______________|___________|______________|____________|____________
//     |                   |                |          |          |       |        |        |       |        |        |       |          |          |       |
//     |GLProgramResources |--------------->|   UB[0]  |   UB[1]  |  ...  | Sam[0] | Sam[1] |  ...  | Img[0] | Img[1] |  ...  | SSBOs[0] | SSBOs[1] |  ...  |
//     |___________________|                |__________|__________|_______|________|________|_______|________|________|_______|__________|__________|_______|
//                                                A                                    A                        A                            A
//                                                |                                    |                        |                            |
//                                               Ref                                  Ref                      Ref                          Ref
//    .-==========================-.         _____|____________________________________|________________________|____________________________|______________
//    ||                          ||        |           |           |       |            |            |       |            |         |           |          |
//  __|| GLPipelineResourceLayout ||------->| UBInfo[0] | UBInfo[1] |  ...  | SamInfo[0] | SamInfo[1] |  ...  | ImgInfo[0] |   ...   |  SSBO[0]  |   ...    |
// |  ||                          ||        |___________|___________|_______|____________|____________|_______|____________|_________|___________|__________|
// |  '-==========================-'                     /                                         \
// |                                                   Ref                                         Ref
// |                                                  /                                              \
// |    ___________________                  ________V________________________________________________V_____________________________________________________
// |   |                   |                |          |          |       |        |        |       |        |        |       |          |          |       |
// |   |GLProgramResources |--------------->|   UB[0]  |   UB[1]  |  ...  | Sam[0] | Sam[1] |  ...  | Img[0] | Img[1] |  ...  | SSBOs[0] | SSBOs[1] |  ...  |
// |   |___________________|                |__________|__________|_______|________|________|_______|________|________|_______|__________|__________|_______|
// |                                             |           |                |         |                |        |                |           |
// |                                          Binding     Binding          Binding    Binding          Binding  Binding         Binding      Binding
// |                                             |           |                |         |                |        |                |           |
// |    _______________________              ____V___________V________________V_________V________________V________V________________V___________V_____________
// |   |                       |            |                           |                           |                           |                           |
// '-->|GLProgramResourceCache |----------->|      Uinform Buffers      |          Samplers         |          Images           |       Storge Buffers      |
//     |_______________________|            |___________________________|___________________________|___________________________|___________________________|
//
//
// Note that GLProgramResources are kept by PipelineStateGLImpl. GLPipelineResourceLayout class is either part of the same PSO class,
// or part of ShaderResourceBindingGLImpl object that keeps a strong reference to the pipeline. So all references from GLVariableBase
// are always valid.

#include <array>

#include "Object.h"
#include "ShaderResourceVariableBase.hpp"
#include "GLProgramResources.hpp"
#include "GLProgramResourceCache.hpp"

namespace Diligent
{

class GLPipelineResourceLayout
{
public:
    GLPipelineResourceLayout(IObject& Owner) :
        m_Owner(Owner)
    {
        m_ProgramIndex.fill(-1);
    }

    ~GLPipelineResourceLayout();

    // No copies, only moves are allowed
    // clang-format off
    GLPipelineResourceLayout             (const GLPipelineResourceLayout&)  = delete;
    GLPipelineResourceLayout& operator = (const GLPipelineResourceLayout&)  = delete;
    GLPipelineResourceLayout             (      GLPipelineResourceLayout&&) = default;
    GLPipelineResourceLayout& operator = (      GLPipelineResourceLayout&&) = delete;
    // clang-format on

    void Initialize(GLProgramResources*                  ProgramResources,
                    Uint32                               NumPrograms,
                    PIPELINE_TYPE                        PipelineType,
                    const PipelineResourceLayoutDesc&    ResourceLayout,
                    const SHADER_RESOURCE_VARIABLE_TYPE* AllowedVarTypes,
                    Uint32                               NumAllowedTypes,
                    GLProgramResourceCache*              pResourceCache);

    static size_t GetRequiredMemorySize(GLProgramResources*                  ProgramResources,
                                        Uint32                               NumPrograms,
                                        const PipelineResourceLayoutDesc&    ResourceLayout,
                                        const SHADER_RESOURCE_VARIABLE_TYPE* AllowedVarTypes,
                                        Uint32                               NumAllowedTypes);

    void CopyResources(GLProgramResourceCache& DstCache) const;

    struct GLVariableBase : public ShaderVariableBase<GLPipelineResourceLayout>
    {
        using TBase = ShaderVariableBase<GLPipelineResourceLayout>;
        GLVariableBase(const GLProgramResources::GLResourceAttribs& ResourceAttribs,
                       GLPipelineResourceLayout&                    ParentLayout,
                       SHADER_RESOURCE_VARIABLE_TYPE                VariableType,
                       Int32                                        ImtblSamplerIdx) :
            // clang-format off
            TBase             {ParentLayout},
            m_Attribs         {ResourceAttribs },
            m_VariableType    {VariableType    },
            m_ImtblSamplerIdx {ImtblSamplerIdx}
        // clang-format on
        {
            VERIFY_EXPR(ImtblSamplerIdx < 0 || ResourceAttribs.ResourceType == SHADER_RESOURCE_TYPE_TEXTURE_SRV);
        }

        virtual SHADER_RESOURCE_VARIABLE_TYPE DILIGENT_CALL_TYPE GetType() const override final
        {
            return m_VariableType;
        }

        virtual void DILIGENT_CALL_TYPE GetResourceDesc(ShaderResourceDesc& ResourceDesc) const override final
        {
            ResourceDesc = m_Attribs.GetResourceDesc();
        }

        virtual Uint32 DILIGENT_CALL_TYPE GetIndex() const override final
        {
            return m_ParentResLayout.GetVariableIndex(*this);
        }

        const GLProgramResources::GLResourceAttribs& m_Attribs;
        const SHADER_RESOURCE_VARIABLE_TYPE          m_VariableType;
        const Int32                                  m_ImtblSamplerIdx;
    };


    struct UniformBuffBindInfo final : GLVariableBase
    {
        UniformBuffBindInfo(const GLProgramResources::GLResourceAttribs& ResourceAttribs,
                            GLPipelineResourceLayout&                    ParentResLayout,
                            SHADER_RESOURCE_VARIABLE_TYPE                VariableType) :
            GLVariableBase{ResourceAttribs, ParentResLayout, VariableType, -1}
        {}

        // Non-virtual function
        void BindResource(IDeviceObject* pObject, Uint32 ArrayIndex);

        virtual void DILIGENT_CALL_TYPE Set(IDeviceObject* pObject) override final
        {
            BindResource(pObject, 0);
        }

        virtual void DILIGENT_CALL_TYPE SetArray(IDeviceObject* const* ppObjects,
                                                 Uint32                FirstElement,
                                                 Uint32                NumElements) override final
        {
            VerifyAndCorrectSetArrayArguments(m_Attribs.Name, m_Attribs.ArraySize, FirstElement, NumElements);
            for (Uint32 elem = 0; elem < NumElements; ++elem)
                BindResource(ppObjects[elem], FirstElement + elem);
        }

        virtual bool DILIGENT_CALL_TYPE IsBound(Uint32 ArrayIndex) const override final
        {
            VERIFY_EXPR(ArrayIndex < m_Attribs.ArraySize);
            return m_ParentResLayout.m_pResourceCache->IsUBBound(m_Attribs.Binding + ArrayIndex);
        }
    };


    struct SamplerBindInfo final : GLVariableBase
    {
        SamplerBindInfo(const GLProgramResources::GLResourceAttribs& ResourceAttribs,
                        GLPipelineResourceLayout&                    ParentResLayout,
                        SHADER_RESOURCE_VARIABLE_TYPE                VariableType,
                        Int32                                        ImtblSamplerIdx) :
            GLVariableBase{ResourceAttribs, ParentResLayout, VariableType, ImtblSamplerIdx}
        {}

        // Non-virtual function
        void BindResource(IDeviceObject* pObject, Uint32 ArrayIndex);

        virtual void DILIGENT_CALL_TYPE Set(IDeviceObject* pObject) override final
        {
            BindResource(pObject, 0);
        }

        virtual void DILIGENT_CALL_TYPE SetArray(IDeviceObject* const* ppObjects,
                                                 Uint32                FirstElement,
                                                 Uint32                NumElements) override final
        {
            VerifyAndCorrectSetArrayArguments(m_Attribs.Name, m_Attribs.ArraySize, FirstElement, NumElements);
            for (Uint32 elem = 0; elem < NumElements; ++elem)
                BindResource(ppObjects[elem], FirstElement + elem);
        }

        virtual bool DILIGENT_CALL_TYPE IsBound(Uint32 ArrayIndex) const override final
        {
            VERIFY_EXPR(ArrayIndex < m_Attribs.ArraySize);
            return m_ParentResLayout.m_pResourceCache->IsSamplerBound(m_Attribs.Binding + ArrayIndex, m_Attribs.ResourceType == SHADER_RESOURCE_TYPE_TEXTURE_SRV);
        }
    };


    struct ImageBindInfo final : GLVariableBase
    {
        ImageBindInfo(const GLProgramResources::GLResourceAttribs& ResourceAttribs,
                      GLPipelineResourceLayout&                    ParentResLayout,
                      SHADER_RESOURCE_VARIABLE_TYPE                VariableType) :
            GLVariableBase{ResourceAttribs, ParentResLayout, VariableType, -1}
        {}

        // Provide non-virtual function
        void BindResource(IDeviceObject* pObject, Uint32 ArrayIndex);

        virtual void DILIGENT_CALL_TYPE Set(IDeviceObject* pObject) override final
        {
            BindResource(pObject, 0);
        }

        virtual void DILIGENT_CALL_TYPE SetArray(IDeviceObject* const* ppObjects,
                                                 Uint32                FirstElement,
                                                 Uint32                NumElements) override final
        {
            VerifyAndCorrectSetArrayArguments(m_Attribs.Name, m_Attribs.ArraySize, FirstElement, NumElements);
            for (Uint32 elem = 0; elem < NumElements; ++elem)
                BindResource(ppObjects[elem], FirstElement + elem);
        }

        virtual bool DILIGENT_CALL_TYPE IsBound(Uint32 ArrayIndex) const override final
        {
            VERIFY_EXPR(ArrayIndex < m_Attribs.ArraySize);
            return m_ParentResLayout.m_pResourceCache->IsImageBound(m_Attribs.Binding + ArrayIndex, m_Attribs.ResourceType == SHADER_RESOURCE_TYPE_TEXTURE_UAV);
        }
    };


    struct StorageBufferBindInfo final : GLVariableBase
    {
        StorageBufferBindInfo(const GLProgramResources::GLResourceAttribs& ResourceAttribs,
                              GLPipelineResourceLayout&                    ParentResLayout,
                              SHADER_RESOURCE_VARIABLE_TYPE                VariableType) :
            GLVariableBase{ResourceAttribs, ParentResLayout, VariableType, -1}
        {}

        // Non-virtual function
        void BindResource(IDeviceObject* pObject, Uint32 ArrayIndex);

        virtual void DILIGENT_CALL_TYPE Set(IDeviceObject* pObject) override final
        {
            BindResource(pObject, 0);
        }

        virtual void DILIGENT_CALL_TYPE SetArray(IDeviceObject* const* ppObjects,
                                                 Uint32                FirstElement,
                                                 Uint32                NumElements) override final
        {
            VerifyAndCorrectSetArrayArguments(m_Attribs.Name, m_Attribs.ArraySize, FirstElement, NumElements);
            for (Uint32 elem = 0; elem < NumElements; ++elem)
                BindResource(ppObjects[elem], FirstElement + elem);
        }

        virtual bool DILIGENT_CALL_TYPE IsBound(Uint32 ArrayIndex) const override final
        {
            VERIFY_EXPR(ArrayIndex < m_Attribs.ArraySize);
            return m_ParentResLayout.m_pResourceCache->IsSSBOBound(m_Attribs.Binding + ArrayIndex);
        }
    };


    // dbgResourceCache is only used for sanity check and as a remainder that the resource cache must be alive
    // while Layout is alive
    void BindResources(SHADER_TYPE ShaderStage, IResourceMapping* pResourceMapping, Uint32 Flags, const GLProgramResourceCache& dbgResourceCache);

#ifdef DILIGENT_DEVELOPMENT
    bool dvpVerifyBindings(const GLProgramResourceCache& ResourceCache) const;
#endif

    IShaderResourceVariable* GetShaderVariable(SHADER_TYPE ShaderStage, const Char* Name);
    IShaderResourceVariable* GetShaderVariable(SHADER_TYPE ShaderStage, Uint32 Index);

    IObject& GetOwner() { return m_Owner; }

    Uint32 GetNumVariables(SHADER_TYPE ShaderStage) const;

    // clang-format off
    Uint32 GetNumUBs()            const { return (m_SamplerOffset       - m_UBOffset)            / sizeof(UniformBuffBindInfo);    }
    Uint32 GetNumSamplers()       const { return (m_ImageOffset         - m_SamplerOffset)       / sizeof(SamplerBindInfo);        }
    Uint32 GetNumImages()         const { return (m_StorageBufferOffset - m_ImageOffset)         / sizeof(ImageBindInfo) ;         }
    Uint32 GetNumStorageBuffers() const { return (m_VariableEndOffset   - m_StorageBufferOffset) / sizeof(StorageBufferBindInfo);  }
    // clang-format on

    template <typename ResourceType> Uint32 GetNumResources() const;

    template <typename ResourceType>
    const ResourceType& GetConstResource(Uint32 ResIndex) const
    {
        VERIFY(ResIndex < GetNumResources<ResourceType>(), "Resource index (", ResIndex, ") exceeds max allowed value (", GetNumResources<ResourceType>(), ")");
        auto Offset = GetResourceOffset<ResourceType>();
        return reinterpret_cast<const ResourceType*>(reinterpret_cast<const Uint8*>(m_ResourceBuffer.get()) + Offset)[ResIndex];
    }

    Uint32 GetVariableIndex(const GLVariableBase& Var) const;

private:
    // clang-format off
/* 0*/ IObject&                                       m_Owner;
       // No need to use shared pointer, as the resource cache is either part of the same
       // ShaderGLImpl object, or ShaderResourceBindingGLImpl object
/* 8*/ GLProgramResourceCache*                        m_pResourceCache = nullptr;
/*16*/ std::unique_ptr<void, STDDeleterRawMem<void> > m_ResourceBuffer;
    
       // Offsets in bytes
       using OffsetType = Uint16;
       static constexpr OffsetType m_UBOffset = 0;
/*32*/ OffsetType m_SamplerOffset       = 0;
/*34*/ OffsetType m_ImageOffset         = 0;
/*36*/ OffsetType m_StorageBufferOffset = 0;
/*38*/ OffsetType m_VariableEndOffset   = 0;
/*40*/ std::array<Int8, MAX_SHADERS_IN_PIPELINE> m_ProgramIndex = {};
/*45*/ Uint8      m_NumPrograms         = 0;
/*46*/ Uint8      m_PipelineType        = 255u;
/*47*/
/*48*/ // End of structure
    // clang-format on

    template <typename ResourceType> OffsetType GetResourceOffset() const;

    template <typename ResourceType>
    ResourceType& GetResource(Uint32 ResIndex)
    {
        VERIFY(ResIndex < GetNumResources<ResourceType>(), "Resource index (", ResIndex, ") exceeds max allowed value (", GetNumResources<ResourceType>() - 1, ")");
        auto Offset = GetResourceOffset<ResourceType>();
        return reinterpret_cast<ResourceType*>(reinterpret_cast<Uint8*>(m_ResourceBuffer.get()) + Offset)[ResIndex];
    }

    GLProgramResources::ResourceCounters& GetProgramVarEndOffsets(Uint32 prog)
    {
        VERIFY_EXPR(prog < m_NumPrograms);
        return reinterpret_cast<GLProgramResources::ResourceCounters*>(reinterpret_cast<Uint8*>(m_ResourceBuffer.get()) + m_VariableEndOffset)[prog];
    }

    const GLProgramResources::ResourceCounters& GetProgramVarEndOffsets(Uint32 prog) const
    {
        VERIFY_EXPR(prog < m_NumPrograms);
        return reinterpret_cast<GLProgramResources::ResourceCounters*>(reinterpret_cast<Uint8*>(m_ResourceBuffer.get()) + m_VariableEndOffset)[prog];
    }

    template <typename ResourceType>
    IShaderResourceVariable* GetResourceByName(SHADER_TYPE ShaderStage, const Char* Name);

    template <typename THandleUB,
              typename THandleSampler,
              typename THandleImage,
              typename THandleStorageBuffer>
    void HandleResources(THandleUB            HandleUB,
                         THandleSampler       HandleSampler,
                         THandleImage         HandleImage,
                         THandleStorageBuffer HandleStorageBuffer)
    {
        for (Uint32 ub = 0; ub < GetNumResources<UniformBuffBindInfo>(); ++ub)
            HandleUB(GetResource<UniformBuffBindInfo>(ub));

        for (Uint32 s = 0; s < GetNumResources<SamplerBindInfo>(); ++s)
            HandleSampler(GetResource<SamplerBindInfo>(s));

        for (Uint32 i = 0; i < GetNumResources<ImageBindInfo>(); ++i)
            HandleImage(GetResource<ImageBindInfo>(i));

        for (Uint32 s = 0; s < GetNumResources<StorageBufferBindInfo>(); ++s)
            HandleStorageBuffer(GetResource<StorageBufferBindInfo>(s));
    }

    template <typename THandleUB,
              typename THandleSampler,
              typename THandleImage,
              typename THandleStorageBuffer>
    void HandleConstResources(THandleUB            HandleUB,
                              THandleSampler       HandleSampler,
                              THandleImage         HandleImage,
                              THandleStorageBuffer HandleStorageBuffer) const
    {
        for (Uint32 ub = 0; ub < GetNumResources<UniformBuffBindInfo>(); ++ub)
            HandleUB(GetConstResource<UniformBuffBindInfo>(ub));

        for (Uint32 s = 0; s < GetNumResources<SamplerBindInfo>(); ++s)
            HandleSampler(GetConstResource<SamplerBindInfo>(s));

        for (Uint32 i = 0; i < GetNumResources<ImageBindInfo>(); ++i)
            HandleImage(GetConstResource<ImageBindInfo>(i));

        for (Uint32 s = 0; s < GetNumResources<StorageBufferBindInfo>(); ++s)
            HandleStorageBuffer(GetConstResource<StorageBufferBindInfo>(s));
    }

    friend class ShaderVariableIndexLocator;
    friend class ShaderVariableLocator;
};



template <>
inline Uint32 GLPipelineResourceLayout::GetNumResources<GLPipelineResourceLayout::UniformBuffBindInfo>() const
{
    return GetNumUBs();
}

template <>
inline Uint32 GLPipelineResourceLayout::GetNumResources<GLPipelineResourceLayout::SamplerBindInfo>() const
{
    return GetNumSamplers();
}

template <>
inline Uint32 GLPipelineResourceLayout::GetNumResources<GLPipelineResourceLayout::ImageBindInfo>() const
{
    return GetNumImages();
}

template <>
inline Uint32 GLPipelineResourceLayout::GetNumResources<GLPipelineResourceLayout::StorageBufferBindInfo>() const
{
    return GetNumStorageBuffers();
}



template <>
inline GLPipelineResourceLayout::OffsetType GLPipelineResourceLayout::
    GetResourceOffset<GLPipelineResourceLayout::UniformBuffBindInfo>() const
{
    return m_UBOffset;
}

template <>
inline GLPipelineResourceLayout::OffsetType GLPipelineResourceLayout::
    GetResourceOffset<GLPipelineResourceLayout::SamplerBindInfo>() const
{
    return m_SamplerOffset;
}

template <>
inline GLPipelineResourceLayout::OffsetType GLPipelineResourceLayout::
    GetResourceOffset<GLPipelineResourceLayout::ImageBindInfo>() const
{
    return m_ImageOffset;
}

template <>
inline GLPipelineResourceLayout::OffsetType GLPipelineResourceLayout::
    GetResourceOffset<GLPipelineResourceLayout::StorageBufferBindInfo>() const
{
    return m_StorageBufferOffset;
}

} // namespace Diligent
