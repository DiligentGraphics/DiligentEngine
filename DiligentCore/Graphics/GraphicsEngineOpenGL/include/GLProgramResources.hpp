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

// GLProgramResources class allocates single continuous chunk of memory to store all program resources, as follows:
//
//
//       m_UniformBuffers        m_Samplers                 m_Images                   m_StorageBlocks
//        |                       |                          |                          |                         |                  |
//        |  UB[0]  ... UB[Nu-1]  |  Sam[0]  ...  Sam[Ns-1]  |  Img[0]  ...  Img[Ni-1]  |  SB[0]  ...  SB[Nsb-1]  |  Resource Names  |
//
//  Nu  - number of uniform buffers
//  Ns  - number of samplers
//  Ni  - number of images
//  Nsb - number of storage blocks

#include <vector>

#include "Object.h"
#include "StringPool.hpp"
#include "HashUtils.hpp"
#include "ShaderResourceVariableBase.hpp"

namespace Diligent
{

class GLProgramResources
{
public:
    GLProgramResources() {}
    ~GLProgramResources();
    // clang-format off
    GLProgramResources             (GLProgramResources&& Program)noexcept;

    GLProgramResources             (const GLProgramResources&)  = delete;
    GLProgramResources& operator = (const GLProgramResources&)  = delete;
    GLProgramResources& operator = (      GLProgramResources&&) = delete;
    // clang-format on

    /// Loads program uniforms and assigns bindings
    void LoadUniforms(SHADER_TYPE                           ShaderStages,
                      const GLObjectWrappers::GLProgramObj& GLProgram,
                      class GLContextState&                 State,
                      Uint32&                               UniformBufferBinding,
                      Uint32&                               SamplerBinding,
                      Uint32&                               ImageBinding,
                      Uint32&                               StorageBufferBinding);

    struct GLResourceAttribs
    {
        // clang-format off
/*  0 */    const Char*                             Name;
/*  8 */    const SHADER_TYPE                       ShaderStages;
/* 12 */    const SHADER_RESOURCE_TYPE              ResourceType;
/* 16 */    const Uint32                            Binding;
/* 20 */          Uint32                            ArraySize;
/* 24 */    // End of data
        // clang-format on

        GLResourceAttribs(const Char*          _Name,
                          SHADER_TYPE          _ShaderStages,
                          SHADER_RESOURCE_TYPE _ResourceType,
                          Uint32               _Binding,
                          Uint32               _ArraySize) noexcept :
            // clang-format off
            Name         {_Name        },
            ShaderStages {_ShaderStages},
            ResourceType {_ResourceType},
            Binding      {_Binding     },
            ArraySize    {_ArraySize   }
        // clang-format on
        {
            VERIFY(_ShaderStages != SHADER_TYPE_UNKNOWN, "At least one shader stage must be specified");
            VERIFY(_ResourceType != SHADER_RESOURCE_TYPE_UNKNOWN, "Unknown shader resource type");
            VERIFY(_ArraySize >= 1, "Array size must be greater than 1");
        }

        GLResourceAttribs(const GLResourceAttribs& Attribs,
                          StringPool&              NamesPool) noexcept :
            // clang-format off
            GLResourceAttribs
            {
                NamesPool.CopyString(Attribs.Name),
                Attribs.ShaderStages,
                Attribs.ResourceType,
                Attribs.Binding,
                Attribs.ArraySize
            }
            // clang-format on 
        {
        }

        bool IsCompatibleWith(const GLResourceAttribs& Var)const
        {
            // clang-format off
            return ShaderStages == Var.ShaderStages &&
                   ResourceType == Var.ResourceType &&
                   Binding      == Var.Binding      &&
                   ArraySize    == Var.ArraySize;
            // clang-format on
        }

        size_t GetHash() const
        {
            return ComputeHash(static_cast<Uint32>(ShaderStages), static_cast<Uint32>(ResourceType), Binding, ArraySize);
        }

        String GetPrintName(Uint32 ArrayInd) const
        {
            VERIFY_EXPR(ArrayInd < ArraySize);
            if (ArraySize > 1)
                return String(Name) + '[' + std::to_string(ArrayInd) + ']';
            else
                return Name;
        }

        ShaderResourceDesc GetResourceDesc() const
        {
            ShaderResourceDesc ResourceDesc;
            ResourceDesc.Name      = Name;
            ResourceDesc.ArraySize = ArraySize;
            ResourceDesc.Type      = ResourceType;
            return ResourceDesc;
        }

        RESOURCE_DIMENSION GetResourceDimension() const
        {
            return RESOURCE_DIM_UNDEFINED;
        }

        bool IsMultisample() const
        {
            return false;
        }
    };

    struct UniformBufferInfo final : GLResourceAttribs
    {
        // clang-format off
        UniformBufferInfo            (const UniformBufferInfo&)  = delete;
        UniformBufferInfo& operator= (const UniformBufferInfo&)  = delete;
        UniformBufferInfo            (      UniformBufferInfo&&) = default;
        UniformBufferInfo& operator= (      UniformBufferInfo&&) = delete;

        UniformBufferInfo(const Char*           _Name, 
                          SHADER_TYPE           _ShaderStages,
                          SHADER_RESOURCE_TYPE  _ResourceType,
                          Uint32                _Binding,
                          Uint32                _ArraySize,
                          GLuint                _UBIndex)noexcept :
            GLResourceAttribs{_Name, _ShaderStages, _ResourceType, _Binding, _ArraySize},
            UBIndex          {_UBIndex}
        {}

        UniformBufferInfo(const UniformBufferInfo& UB,
                          StringPool&              NamesPool)noexcept :
            GLResourceAttribs{UB, NamesPool},
            UBIndex          {UB.UBIndex   }
        {}
        // clang-format on

        bool IsCompatibleWith(const UniformBufferInfo& UB) const
        {
            return UBIndex == UB.UBIndex &&
                GLResourceAttribs::IsCompatibleWith(UB);
        }

        size_t GetHash() const
        {
            return ComputeHash(UBIndex, GLResourceAttribs::GetHash());
        }

        const GLuint UBIndex;
    };
    static_assert((sizeof(UniformBufferInfo) % sizeof(void*)) == 0, "sizeof(UniformBufferInfo) must be multiple of sizeof(void*)");


    struct SamplerInfo final : GLResourceAttribs
    {
        // clang-format off
        SamplerInfo            (const SamplerInfo&)  = delete;
        SamplerInfo& operator= (const SamplerInfo&)  = delete;
        SamplerInfo            (      SamplerInfo&&) = default;
        SamplerInfo& operator= (      SamplerInfo&&) = delete;

        SamplerInfo(const Char*             _Name, 
                    SHADER_TYPE             _ShaderStages,
                    SHADER_RESOURCE_TYPE    _ResourceType,
                    Uint32                  _Binding,
                    Uint32                  _ArraySize,
                    GLint                   _Location,
                    GLenum                  _SamplerType)noexcept :
            GLResourceAttribs{_Name, _ShaderStages, _ResourceType, _Binding, _ArraySize},
            Location         {_Location   },
            SamplerType      {_SamplerType}
        {}

        SamplerInfo(const SamplerInfo& Sam,
                    StringPool&        NamesPool)noexcept :
            GLResourceAttribs{Sam, NamesPool},
            Location         {Sam.Location   },
            SamplerType      {Sam.SamplerType}
        {}

        bool IsCompatibleWith(const SamplerInfo& Sam)const
        {
            return Location       == Sam.Location    &&
                    SamplerType    == Sam.SamplerType &&
                    GLResourceAttribs::IsCompatibleWith(Sam);
        }
        // clang-format on

        size_t GetHash() const
        {
            return ComputeHash(Location, SamplerType, GLResourceAttribs::GetHash());
        }

        const GLint  Location;
        const GLenum SamplerType;
    };
    static_assert((sizeof(SamplerInfo) % sizeof(void*)) == 0, "sizeof(SamplerInfo) must be multiple of sizeof(void*)");


    struct ImageInfo final : GLResourceAttribs
    {
        // clang-format off
        ImageInfo            (const ImageInfo&)  = delete;
        ImageInfo& operator= (const ImageInfo&)  = delete;
        ImageInfo            (      ImageInfo&&) = default;
        ImageInfo& operator= (      ImageInfo&&) = delete;

        ImageInfo(const Char*           _Name, 
                  SHADER_TYPE           _ShaderStages,
                  SHADER_RESOURCE_TYPE  _ResourceType,
                  Uint32                _Binding,
                  Uint32                _ArraySize,
                  GLint                 _Location,
                  GLenum                _ImageType)noexcept :
            GLResourceAttribs{_Name,  _ShaderStages, _ResourceType, _Binding, _ArraySize},
            Location         {_Location },
            ImageType        {_ImageType}
        {}

        ImageInfo(const ImageInfo& Img, 
                  StringPool&      NamesPool)noexcept :
            GLResourceAttribs{Img, NamesPool},
            Location         {Img.Location },
            ImageType        {Img.ImageType}
        {}

        bool IsCompatibleWith(const ImageInfo& Img)const
        {
            return Location  == Img.Location  &&
                   ImageType == Img.ImageType &&
                   GLResourceAttribs::IsCompatibleWith(Img);
        }
        // clang-format on

        size_t GetHash() const
        {
            return ComputeHash(Location, ImageType, GLResourceAttribs::GetHash());
        }

        const GLint  Location;
        const GLenum ImageType;
    };
    static_assert((sizeof(ImageInfo) % sizeof(void*)) == 0, "sizeof(ImageInfo) must be multiple of sizeof(void*)");


    struct StorageBlockInfo final : GLResourceAttribs
    {
        // clang-format off
        StorageBlockInfo            (const StorageBlockInfo&)  = delete;
        StorageBlockInfo& operator= (const StorageBlockInfo&)  = delete;
        StorageBlockInfo            (      StorageBlockInfo&&) = default;
        StorageBlockInfo& operator= (      StorageBlockInfo&&) = delete;

        StorageBlockInfo(const Char*            _Name, 
                         SHADER_TYPE            _ShaderStages,
                         SHADER_RESOURCE_TYPE   _ResourceType,
                         Uint32                 _Binding,
                         Uint32                 _ArraySize,
                         GLint                  _SBIndex)noexcept :
            GLResourceAttribs{_Name, _ShaderStages, _ResourceType, _Binding, _ArraySize},
            SBIndex          {_SBIndex}
        {}

        StorageBlockInfo(const StorageBlockInfo& SB,
                         StringPool&             NamesPool)noexcept :
            GLResourceAttribs{SB, NamesPool},
            SBIndex          {SB.SBIndex}
        {}

        bool IsCompatibleWith(const StorageBlockInfo& SB)const
        {
            return SBIndex == SB.SBIndex &&
                   GLResourceAttribs::IsCompatibleWith(SB);
        }
        // clang-format on

        size_t GetHash() const
        {
            return ComputeHash(SBIndex, GLResourceAttribs::GetHash());
        }

        const GLint SBIndex;
    };
    static_assert((sizeof(StorageBlockInfo) % sizeof(void*)) == 0, "sizeof(StorageBlockInfo) must be multiple of sizeof(void*)");


    // clang-format off
    Uint32 GetNumUniformBuffers()const { return m_NumUniformBuffers; }
    Uint32 GetNumSamplers()      const { return m_NumSamplers;       }
    Uint32 GetNumImages()        const { return m_NumImages;         }
    Uint32 GetNumStorageBlocks() const { return m_NumStorageBlocks;  }
    // clang-format on

    UniformBufferInfo& GetUniformBuffer(Uint32 Index)
    {
        VERIFY(Index < m_NumUniformBuffers, "Uniform buffer index (", Index, ") is out of range");
        return m_UniformBuffers[Index];
    }

    SamplerInfo& GetSampler(Uint32 Index)
    {
        VERIFY(Index < m_NumSamplers, "Sampler index (", Index, ") is out of range");
        return m_Samplers[Index];
    }

    ImageInfo& GetImage(Uint32 Index)
    {
        VERIFY(Index < m_NumImages, "Image index (", Index, ") is out of range");
        return m_Images[Index];
    }

    StorageBlockInfo& GetStorageBlock(Uint32 Index)
    {
        VERIFY(Index < m_NumStorageBlocks, "Storage block index (", Index, ") is out of range");
        return m_StorageBlocks[Index];
    }


    const UniformBufferInfo& GetUniformBuffer(Uint32 Index) const
    {
        VERIFY(Index < m_NumUniformBuffers, "Uniform buffer index (", Index, ") is out of range");
        return m_UniformBuffers[Index];
    }

    const SamplerInfo& GetSampler(Uint32 Index) const
    {
        VERIFY(Index < m_NumSamplers, "Sampler index (", Index, ") is out of range");
        return m_Samplers[Index];
    }

    const ImageInfo& GetImage(Uint32 Index) const
    {
        VERIFY(Index < m_NumImages, "Image index (", Index, ") is out of range");
        return m_Images[Index];
    }

    const StorageBlockInfo& GetStorageBlock(Uint32 Index) const
    {
        VERIFY(Index < m_NumStorageBlocks, "Storage block index (", Index, ") is out of range");
        return m_StorageBlocks[Index];
    }

    Uint32 GetVariableCount() const
    {
        return m_NumUniformBuffers + m_NumSamplers + m_NumImages + m_NumStorageBlocks;
    }

    ShaderResourceDesc GetResourceDesc(Uint32 Index) const;

    bool   IsCompatibleWith(const GLProgramResources& Res) const;
    size_t GetHash() const;

    SHADER_TYPE GetShaderStages() const { return m_ShaderStages; }

    template <typename THandleUB,
              typename THandleSampler,
              typename THandleImg,
              typename THandleSB>
    void ProcessConstResources(THandleUB                            HandleUB,
                               THandleSampler                       HandleSampler,
                               THandleImg                           HandleImg,
                               THandleSB                            HandleSB,
                               const PipelineResourceLayoutDesc*    pResourceLayout = nullptr,
                               const SHADER_RESOURCE_VARIABLE_TYPE* AllowedVarTypes = nullptr,
                               Uint32                               NumAllowedTypes = 0) const
    {
        const Uint32 AllowedTypeBits = GetAllowedTypeBits(AllowedVarTypes, NumAllowedTypes);

        auto CheckResourceType = [&](const char* Name) //
        {
            if (pResourceLayout == nullptr)
                return true;
            else
            {
                auto VarType = GetShaderVariableType(m_ShaderStages, Name, *pResourceLayout);
                return IsAllowedType(VarType, AllowedTypeBits);
            }
        };

        for (Uint32 ub = 0; ub < m_NumUniformBuffers; ++ub)
        {
            const auto& UB = GetUniformBuffer(ub);
            if (CheckResourceType(UB.Name))
                HandleUB(UB);
        }

        for (Uint32 s = 0; s < m_NumSamplers; ++s)
        {
            const auto& Sam = GetSampler(s);
            if (CheckResourceType(Sam.Name))
                HandleSampler(Sam);
        }

        for (Uint32 img = 0; img < m_NumImages; ++img)
        {
            const auto& Img = GetImage(img);
            if (CheckResourceType(Img.Name))
                HandleImg(Img);
        }

        for (Uint32 sb = 0; sb < m_NumStorageBlocks; ++sb)
        {
            const auto& SB = GetStorageBlock(sb);
            if (CheckResourceType(SB.Name))
                HandleSB(SB);
        }
    }

    template <typename THandleUB,
              typename THandleSampler,
              typename THandleImg,
              typename THandleSB>
    void ProcessResources(THandleUB      HandleUB,
                          THandleSampler HandleSampler,
                          THandleImg     HandleImg,
                          THandleSB      HandleSB)
    {
        for (Uint32 ub = 0; ub < m_NumUniformBuffers; ++ub)
            HandleUB(GetUniformBuffer(ub));

        for (Uint32 s = 0; s < m_NumSamplers; ++s)
            HandleSampler(GetSampler(s));

        for (Uint32 img = 0; img < m_NumImages; ++img)
            HandleImg(GetImage(img));

        for (Uint32 sb = 0; sb < m_NumStorageBlocks; ++sb)
            HandleSB(GetStorageBlock(sb));
    }

    struct ResourceCounters
    {
        Uint32 NumUBs           = 0;
        Uint32 NumSamplers      = 0;
        Uint32 NumImages        = 0;
        Uint32 NumStorageBlocks = 0;
    };
    void CountResources(const PipelineResourceLayoutDesc&    ResourceLayout,
                        const SHADER_RESOURCE_VARIABLE_TYPE* AllowedVarTypes,
                        Uint32                               NumAllowedTypes,
                        ResourceCounters&                    Counters) const;


private:
    void AllocateResources(std::vector<UniformBufferInfo>& UniformBlocks,
                           std::vector<SamplerInfo>&       Samplers,
                           std::vector<ImageInfo>&         Images,
                           std::vector<StorageBlockInfo>&  StorageBlocks);

    // clang-format off
    // There could be more than one stage if using non-separable programs
    SHADER_TYPE         m_ShaderStages = SHADER_TYPE_UNKNOWN; 

    // Memory layout:
    // 
    //  |  Uniform buffers  |   Samplers  |   Images   |   Storage Blocks   |    String Pool Data   |
    //

    UniformBufferInfo*  m_UniformBuffers = nullptr;
    SamplerInfo*        m_Samplers       = nullptr;
    ImageInfo*          m_Images         = nullptr;
    StorageBlockInfo*   m_StorageBlocks  = nullptr;

    Uint32              m_NumUniformBuffers = 0;
    Uint32              m_NumSamplers       = 0;
    Uint32              m_NumImages         = 0;
    Uint32              m_NumStorageBlocks  = 0;
    // clang-format on
    // When adding new member DO NOT FORGET TO UPDATE GLProgramResources( GLProgramResources&& ProgramResources )!!!
};

} // namespace Diligent
