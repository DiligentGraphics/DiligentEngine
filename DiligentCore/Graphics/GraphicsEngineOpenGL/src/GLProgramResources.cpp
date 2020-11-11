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
#include <unordered_set>
#include "GLContextState.hpp"
#include "GLProgramResources.hpp"
#include "RenderDeviceGLImpl.hpp"
#include "ShaderResourceBindingBase.hpp"
#include "ShaderResourceVariableBase.hpp"
#include "Align.hpp"

namespace Diligent
{

GLProgramResources::GLProgramResources(GLProgramResources&& Program) noexcept :
    // clang-format off
    m_ShaderStages     {Program.m_ShaderStages         },
    m_UniformBuffers   {Program.m_UniformBuffers       },
    m_Samplers         {Program.m_Samplers             },
    m_Images           {Program.m_Images               },
    m_StorageBlocks    {Program.m_StorageBlocks        },
    m_NumUniformBuffers{Program.m_NumUniformBuffers    },
    m_NumSamplers      {Program.m_NumSamplers          },
    m_NumImages        {Program.m_NumImages            },        
    m_NumStorageBlocks {Program.m_NumStorageBlocks     }
// clang-format on
{
    Program.m_UniformBuffers = nullptr;
    Program.m_Samplers       = nullptr;
    Program.m_Images         = nullptr;
    Program.m_StorageBlocks  = nullptr;

    Program.m_NumUniformBuffers = 0;
    Program.m_NumSamplers       = 0;
    Program.m_NumImages         = 0;
    Program.m_NumStorageBlocks  = 0;
}

inline void RemoveArrayBrackets(char* Str)
{
    auto* OpenBacketPtr = strchr(Str, '[');
    if (OpenBacketPtr != nullptr)
        *OpenBacketPtr = 0;
}

void GLProgramResources::AllocateResources(std::vector<UniformBufferInfo>& UniformBlocks,
                                           std::vector<SamplerInfo>&       Samplers,
                                           std::vector<ImageInfo>&         Images,
                                           std::vector<StorageBlockInfo>&  StorageBlocks)
{
    VERIFY(m_UniformBuffers == nullptr, "Resources have already been allocated!");

    m_NumUniformBuffers = static_cast<Uint32>(UniformBlocks.size());
    m_NumSamplers       = static_cast<Uint32>(Samplers.size());
    m_NumImages         = static_cast<Uint32>(Images.size());
    m_NumStorageBlocks  = static_cast<Uint32>(StorageBlocks.size());

    size_t StringPoolDataSize = 0;
    for (const auto& ub : UniformBlocks)
    {
        StringPoolDataSize += strlen(ub.Name) + 1;
    }

    for (const auto& sam : Samplers)
    {
        StringPoolDataSize += strlen(sam.Name) + 1;
    }

    for (const auto& img : Images)
    {
        StringPoolDataSize += strlen(img.Name) + 1;
    }

    for (const auto& sb : StorageBlocks)
    {
        StringPoolDataSize += strlen(sb.Name) + 1;
    }

    auto AlignedStringPoolDataSize = Align(StringPoolDataSize, sizeof(void*));

    // clang-format off
    size_t TotalMemorySize = 
        m_NumUniformBuffers * sizeof(UniformBufferInfo) + 
        m_NumSamplers       * sizeof(SamplerInfo) +
        m_NumImages         * sizeof(ImageInfo) +
        m_NumStorageBlocks  * sizeof(StorageBlockInfo);
    // clang-format on

    if (TotalMemorySize == 0)
    {
        m_UniformBuffers = nullptr;
        m_Samplers       = nullptr;
        m_Images         = nullptr;
        m_StorageBlocks  = nullptr;

        m_NumUniformBuffers = 0;
        m_NumSamplers       = 0;
        m_NumImages         = 0;
        m_NumStorageBlocks  = 0;

        return;
    }

    TotalMemorySize += AlignedStringPoolDataSize * sizeof(Char);

    auto& MemAllocator = GetRawAllocator();
    void* RawMemory    = ALLOCATE_RAW(MemAllocator, "Memory buffer for GLProgramResources", TotalMemorySize);

    // clang-format off
    m_UniformBuffers = reinterpret_cast<UniformBufferInfo*>(RawMemory);
    m_Samplers       = reinterpret_cast<SamplerInfo*>     (m_UniformBuffers + m_NumUniformBuffers);
    m_Images         = reinterpret_cast<ImageInfo*>       (m_Samplers       + m_NumSamplers);
    m_StorageBlocks  = reinterpret_cast<StorageBlockInfo*>(m_Images         + m_NumImages);
    void* EndOfResourceData =                              m_StorageBlocks + m_NumStorageBlocks;
    Char* StringPoolData = reinterpret_cast<Char*>(EndOfResourceData);
    // clang-format on

    // The pool is only needed to facilitate string allocation.
    StringPool TmpStringPool;
    TmpStringPool.AssignMemory(StringPoolData, StringPoolDataSize);

    for (Uint32 ub = 0; ub < m_NumUniformBuffers; ++ub)
    {
        auto& SrcUB = UniformBlocks[ub];
        new (m_UniformBuffers + ub) UniformBufferInfo{SrcUB, TmpStringPool};
    }

    for (Uint32 s = 0; s < m_NumSamplers; ++s)
    {
        auto& SrcSam = Samplers[s];
        new (m_Samplers + s) SamplerInfo{SrcSam, TmpStringPool};
    }

    for (Uint32 img = 0; img < m_NumImages; ++img)
    {
        auto& SrcImg = Images[img];
        new (m_Images + img) ImageInfo{SrcImg, TmpStringPool};
    }

    for (Uint32 sb = 0; sb < m_NumStorageBlocks; ++sb)
    {
        auto& SrcSB = StorageBlocks[sb];
        new (m_StorageBlocks + sb) StorageBlockInfo{SrcSB, TmpStringPool};
    }

    VERIFY_EXPR(TmpStringPool.GetRemainingSize() == 0);
}

GLProgramResources::~GLProgramResources()
{
    // clang-format off
    ProcessResources(
        [&](UniformBufferInfo& UB)
        {
            UB.~UniformBufferInfo();
        },
        [&](SamplerInfo& Sam)
        {
            Sam.~SamplerInfo();
        },
        [&](ImageInfo& Img)
        {
            Img.~ImageInfo();
        },
        [&](StorageBlockInfo& SB)
        {
            SB.~StorageBlockInfo();
        }
    );
    // clang-format on

    void* RawMemory = m_UniformBuffers;
    if (RawMemory != nullptr)
    {
        auto& MemAllocator = GetRawAllocator();
        MemAllocator.Free(RawMemory);
    }
}


void GLProgramResources::LoadUniforms(SHADER_TYPE                           ShaderStages,
                                      const GLObjectWrappers::GLProgramObj& GLProgram,
                                      GLContextState&                       State,
                                      Uint32&                               UniformBufferBinding,
                                      Uint32&                               SamplerBinding,
                                      Uint32&                               ImageBinding,
                                      Uint32&                               StorageBufferBinding)
{
    // Load uniforms to temporary arrays. We will then pack all variables into a single chunk of memory.
    std::vector<UniformBufferInfo> UniformBlocks;
    std::vector<SamplerInfo>       Samplers;
    std::vector<ImageInfo>         Images;
    std::vector<StorageBlockInfo>  StorageBlocks;
    std::unordered_set<String>     NamesPool;

    VERIFY(GLProgram != 0, "Null GL program");
    State.SetProgram(GLProgram);

    m_ShaderStages = ShaderStages;

    GLint numActiveUniforms = 0;
    glGetProgramiv(GLProgram, GL_ACTIVE_UNIFORMS, &numActiveUniforms);
    CHECK_GL_ERROR_AND_THROW("Unable to get the number of active uniforms\n");

    // Query the maximum name length of the active uniform (including null terminator)
    GLint activeUniformMaxLength = 0;
    glGetProgramiv(GLProgram, GL_ACTIVE_UNIFORM_MAX_LENGTH, &activeUniformMaxLength);
    CHECK_GL_ERROR_AND_THROW("Unable to get the maximum uniform name length\n");

    GLint numActiveUniformBlocks = 0;
    glGetProgramiv(GLProgram, GL_ACTIVE_UNIFORM_BLOCKS, &numActiveUniformBlocks);
    CHECK_GL_ERROR_AND_THROW("Unable to get the number of active uniform blocks\n");

    //
    // #### This parameter is currently unsupported by Intel OGL drivers.
    //
    // Query the maximum name length of the active uniform block (including null terminator)
    GLint activeUniformBlockMaxLength = 0;
    // On Intel driver, this call might fail:
    glGetProgramiv(GLProgram, GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH, &activeUniformBlockMaxLength);
    //CHECK_GL_ERROR_AND_THROW("Unable to get the maximum uniform block name length\n");
    if (glGetError() != GL_NO_ERROR)
    {
        LOG_WARNING_MESSAGE("Unable to get the maximum uniform block name length. Using 1024 as a workaround\n");
        activeUniformBlockMaxLength = 1024;
    }

    auto MaxNameLength = std::max(activeUniformMaxLength, activeUniformBlockMaxLength);

#if GL_ARB_program_interface_query
    GLint numActiveShaderStorageBlocks = 0;
    if (glGetProgramInterfaceiv)
    {
        glGetProgramInterfaceiv(GLProgram, GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_RESOURCES, &numActiveShaderStorageBlocks);
        CHECK_GL_ERROR_AND_THROW("Unable to get the number of shader storage blocks blocks\n");

        // Query the maximum name length of the active shader storage block (including null terminator)
        GLint MaxShaderStorageBlockNameLen = 0;
        glGetProgramInterfaceiv(GLProgram, GL_SHADER_STORAGE_BLOCK, GL_MAX_NAME_LENGTH, &MaxShaderStorageBlockNameLen);
        CHECK_GL_ERROR_AND_THROW("Unable to get the maximum shader storage block name length\n");
        MaxNameLength = std::max(MaxNameLength, MaxShaderStorageBlockNameLen);
    }
#endif

    MaxNameLength = std::max(MaxNameLength, 512);
    std::vector<GLchar> Name(MaxNameLength + 1);
    for (int i = 0; i < numActiveUniforms; i++)
    {
        GLenum dataType = 0;
        GLint  size     = 0;
        GLint  NameLen  = 0;
        // If one or more elements of an array are active, the name of the array is returned in 'name',
        // the type is returned in 'type', and the 'size' parameter returns the highest array element index used,
        // plus one, as determined by the compiler and/or linker.
        // Only one active uniform variable will be reported for a uniform array.
        // Uniform variables other than arrays will have a size of 1
        glGetActiveUniform(GLProgram, i, MaxNameLength, &NameLen, &size, &dataType, Name.data());
        CHECK_GL_ERROR_AND_THROW("Unable to get active uniform\n");
        VERIFY(NameLen < MaxNameLength && static_cast<size_t>(NameLen) == strlen(Name.data()), "Incorrect uniform name");
        VERIFY(size >= 1, "Size is expected to be at least 1");
        // Note that
        // glGetActiveUniform( program, index, bufSize, length, size, type, name );
        //
        // is equivalent to
        //
        // const enum props[] = { ARRAY_SIZE, TYPE };
        // glGetProgramResourceName( program, UNIFORM, index, bufSize, length, name );
        // glGetProgramResourceiv( program, GL_UNIFORM, index, 1, &props[0], 1, NULL, size );
        // glGetProgramResourceiv( program, GL_UNIFORM, index, 1, &props[1], 1, NULL, (int *)type );
        //
        // The latter is only available in GL 4.4 and GLES 3.1

        switch (dataType)
        {
            case GL_SAMPLER_1D:
            case GL_SAMPLER_2D:
            case GL_SAMPLER_3D:
            case GL_SAMPLER_CUBE:
            case GL_SAMPLER_1D_SHADOW:
            case GL_SAMPLER_2D_SHADOW:

            case GL_SAMPLER_1D_ARRAY:
            case GL_SAMPLER_2D_ARRAY:
            case GL_SAMPLER_1D_ARRAY_SHADOW:
            case GL_SAMPLER_2D_ARRAY_SHADOW:
            case GL_SAMPLER_CUBE_SHADOW:

            case GL_SAMPLER_EXTERNAL_OES:

            case GL_INT_SAMPLER_1D:
            case GL_INT_SAMPLER_2D:
            case GL_INT_SAMPLER_3D:
            case GL_INT_SAMPLER_CUBE:
            case GL_INT_SAMPLER_1D_ARRAY:
            case GL_INT_SAMPLER_2D_ARRAY:
            case GL_UNSIGNED_INT_SAMPLER_1D:
            case GL_UNSIGNED_INT_SAMPLER_2D:
            case GL_UNSIGNED_INT_SAMPLER_3D:
            case GL_UNSIGNED_INT_SAMPLER_CUBE:
            case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY:
            case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:

            case GL_SAMPLER_CUBE_MAP_ARRAY:
            case GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW:
            case GL_INT_SAMPLER_CUBE_MAP_ARRAY:
            case GL_UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY:

            case GL_SAMPLER_2D_MULTISAMPLE:
            case GL_INT_SAMPLER_2D_MULTISAMPLE:
            case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE:
            case GL_SAMPLER_2D_MULTISAMPLE_ARRAY:
            case GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
            case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:

            case GL_SAMPLER_BUFFER:
            case GL_INT_SAMPLER_BUFFER:
            case GL_UNSIGNED_INT_SAMPLER_BUFFER:
            {
                auto UniformLocation = glGetUniformLocation(GLProgram, Name.data());
                // Note that glGetUniformLocation(program, name) is equivalent to
                // glGetProgramResourceLocation(program, GL_UNIFORM, name);
                // The latter is only available in GL 4.4 and GLES 3.1

                // clang-format off
                const auto ResourceType = 
                        dataType == GL_SAMPLER_BUFFER     ||
                        dataType == GL_INT_SAMPLER_BUFFER ||
                        dataType == GL_UNSIGNED_INT_SAMPLER_BUFFER ?
                    SHADER_RESOURCE_TYPE_BUFFER_SRV :
                    SHADER_RESOURCE_TYPE_TEXTURE_SRV;
                // clang-format on

                RemoveArrayBrackets(Name.data());

                Samplers.emplace_back(
                    NamesPool.emplace(Name.data()).first->c_str(),
                    ShaderStages,
                    ResourceType,
                    SamplerBinding,
                    static_cast<Uint32>(size),
                    UniformLocation,
                    dataType //
                );

                for (GLint arr_ind = 0; arr_ind < size; ++arr_ind)
                {
                    // glProgramUniform1i is not available in GLES3.0
                    glUniform1i(UniformLocation + arr_ind, SamplerBinding++);
                    CHECK_GL_ERROR("Failed to set binding point for sampler uniform '", Name.data(), '\'');
                }

                break;
            }

#if GL_ARB_shader_image_load_store
            case GL_IMAGE_1D:
            case GL_IMAGE_2D:
            case GL_IMAGE_3D:
            case GL_IMAGE_2D_RECT:
            case GL_IMAGE_CUBE:
            case GL_IMAGE_BUFFER:
            case GL_IMAGE_1D_ARRAY:
            case GL_IMAGE_2D_ARRAY:
            case GL_IMAGE_CUBE_MAP_ARRAY:
            case GL_IMAGE_2D_MULTISAMPLE:
            case GL_IMAGE_2D_MULTISAMPLE_ARRAY:
            case GL_INT_IMAGE_1D:
            case GL_INT_IMAGE_2D:
            case GL_INT_IMAGE_3D:
            case GL_INT_IMAGE_2D_RECT:
            case GL_INT_IMAGE_CUBE:
            case GL_INT_IMAGE_BUFFER:
            case GL_INT_IMAGE_1D_ARRAY:
            case GL_INT_IMAGE_2D_ARRAY:
            case GL_INT_IMAGE_CUBE_MAP_ARRAY:
            case GL_INT_IMAGE_2D_MULTISAMPLE:
            case GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY:
            case GL_UNSIGNED_INT_IMAGE_1D:
            case GL_UNSIGNED_INT_IMAGE_2D:
            case GL_UNSIGNED_INT_IMAGE_3D:
            case GL_UNSIGNED_INT_IMAGE_2D_RECT:
            case GL_UNSIGNED_INT_IMAGE_CUBE:
            case GL_UNSIGNED_INT_IMAGE_BUFFER:
            case GL_UNSIGNED_INT_IMAGE_1D_ARRAY:
            case GL_UNSIGNED_INT_IMAGE_2D_ARRAY:
            case GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY:
            case GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE:
            case GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY:
            {
                auto UniformLocation = glGetUniformLocation(GLProgram, Name.data());

                // clang-format off
                const auto ResourceType = 
                        dataType == GL_IMAGE_BUFFER     ||
                        dataType == GL_INT_IMAGE_BUFFER ||
                        dataType == GL_UNSIGNED_INT_IMAGE_BUFFER ?
                    SHADER_RESOURCE_TYPE_BUFFER_UAV :
                    SHADER_RESOURCE_TYPE_TEXTURE_UAV;
                // clang-format on

                RemoveArrayBrackets(Name.data());

                Images.emplace_back(
                    NamesPool.emplace(Name.data()).first->c_str(),
                    ShaderStages,
                    ResourceType,
                    ImageBinding,
                    static_cast<Uint32>(size),
                    UniformLocation,
                    dataType //
                );

                for (GLint arr_ind = 0; arr_ind < size; ++arr_ind)
                {
                    // glUniform1i for image uniforms is not supported in at least GLES3.2.
                    // glProgramUniform1i is not available in GLES3.0
                    glUniform1i(UniformLocation + arr_ind, ImageBinding);
                    if (glGetError() != GL_NO_ERROR)
                    {
                        if (size > 1)
                        {
                            LOG_WARNING_MESSAGE("Failed to set binding for image uniform '", Name.data(), "'[", arr_ind,
                                                "]. Expected binding: ", ImageBinding,
                                                ". Make sure that this binding is explicitly assigned in shader source code."
                                                " Note that if the source code is converted from HLSL and if images are only used"
                                                " by a single shader stage, then bindings automatically assigned by HLSL->GLSL"
                                                " converter will work fine.");
                        }
                        else
                        {
                            LOG_WARNING_MESSAGE("Failed to set binding for image uniform '", Name.data(), "'."
                                                                                                          " Expected binding: ",
                                                ImageBinding,
                                                ". Make sure that this binding is explicitly assigned in shader source code."
                                                " Note that if the source code is converted from HLSL and if images are only used"
                                                " by a single shader stage, then bindings automatically assigned by HLSL->GLSL"
                                                " converter will work fine.");
                        }
                    }
                    ++ImageBinding;
                }

                break;
            }
#endif
            default:
                // Some other uniform type like scalar, matrix etc.
                break;
        }
    }

    for (int i = 0; i < numActiveUniformBlocks; i++)
    {
        // In contrast to shader uniforms, every element in uniform block array is enumerated individually
        GLsizei NameLen = 0;
        glGetActiveUniformBlockName(GLProgram, i, MaxNameLength, &NameLen, Name.data());
        CHECK_GL_ERROR_AND_THROW("Unable to get active uniform block name\n");
        VERIFY(NameLen < MaxNameLength && static_cast<size_t>(NameLen) == strlen(Name.data()), "Incorrect uniform block name");

        // glGetActiveUniformBlockName( program, uniformBlockIndex, bufSize, length, uniformBlockName );
        // is equivalent to
        // glGetProgramResourceName(program, GL_UNIFORM_BLOCK, uniformBlockIndex, bufSize, length, uniformBlockName);

        auto UniformBlockIndex = glGetUniformBlockIndex(GLProgram, Name.data());
        CHECK_GL_ERROR_AND_THROW("Unable to get active uniform block index\n");
        // glGetUniformBlockIndex( program, uniformBlockName );
        // is equivalent to
        // glGetProgramResourceIndex( program, GL_UNIFORM_BLOCK, uniformBlockName );

        bool IsNewBlock = true;

        GLint ArraySize     = 1;
        auto* OpenBacketPtr = strchr(Name.data(), '[');
        if (OpenBacketPtr != nullptr)
        {
            auto Ind       = atoi(OpenBacketPtr + 1);
            ArraySize      = std::max(ArraySize, Ind + 1);
            *OpenBacketPtr = 0;
            if (!UniformBlocks.empty())
            {
                // Look at previous uniform block to check if it is the same array
                auto& LastBlock = UniformBlocks.back();
                if (strcmp(LastBlock.Name, Name.data()) == 0)
                {
                    ArraySize = std::max(ArraySize, static_cast<GLint>(LastBlock.ArraySize));
                    VERIFY(UniformBlockIndex == LastBlock.UBIndex + Ind, "Uniform block indices are expected to be continuous");
                    LastBlock.ArraySize = ArraySize;
                    IsNewBlock          = false;
                }
                else
                {
#ifdef DILIGENT_DEBUG
                    for (const auto& ub : UniformBlocks)
                        VERIFY(strcmp(ub.Name, Name.data()) != 0, "Uniform block with the name '", ub.Name, "' has already been enumerated");
#endif
                }
            }
        }

        if (IsNewBlock)
        {
            UniformBlocks.emplace_back(
                NamesPool.emplace(Name.data()).first->c_str(),
                ShaderStages,
                SHADER_RESOURCE_TYPE_CONSTANT_BUFFER,
                UniformBufferBinding,
                static_cast<Uint32>(ArraySize),
                UniformBlockIndex //
            );
        }

        glUniformBlockBinding(GLProgram, UniformBlockIndex, UniformBufferBinding++);
        CHECK_GL_ERROR("glUniformBlockBinding() failed");
    }

#if GL_ARB_shader_storage_buffer_object
    for (int i = 0; i < numActiveShaderStorageBlocks; ++i)
    {
        GLsizei Length = 0;
        glGetProgramResourceName(GLProgram, GL_SHADER_STORAGE_BLOCK, i, MaxNameLength, &Length, Name.data());
        CHECK_GL_ERROR_AND_THROW("Unable to get shader storage block name\n");
        VERIFY(Length < MaxNameLength && static_cast<size_t>(Length) == strlen(Name.data()), "Incorrect shader storage block name");

        auto SBIndex = glGetProgramResourceIndex(GLProgram, GL_SHADER_STORAGE_BLOCK, Name.data());
        CHECK_GL_ERROR_AND_THROW("Unable to get shader storage block index\n");

        bool  IsNewBlock    = true;
        Int32 ArraySize     = 1;
        auto* OpenBacketPtr = strchr(Name.data(), '[');
        if (OpenBacketPtr != nullptr)
        {
            auto Ind       = atoi(OpenBacketPtr + 1);
            ArraySize      = std::max(ArraySize, Ind + 1);
            *OpenBacketPtr = 0;
            if (!StorageBlocks.empty())
            {
                // Look at previous storage block to check if it is the same array
                auto& LastBlock = StorageBlocks.back();
                if (strcmp(LastBlock.Name, Name.data()) == 0)
                {
                    ArraySize = std::max(ArraySize, static_cast<GLint>(LastBlock.ArraySize));
                    VERIFY(static_cast<GLint>(SBIndex) == LastBlock.SBIndex + Ind, "Storage block indices are expected to be continuous");
                    LastBlock.ArraySize = ArraySize;
                    IsNewBlock          = false;
                }
                else
                {
#    ifdef DILIGENT_DEBUG
                    for (const auto& sb : StorageBlocks)
                        VERIFY(strcmp(sb.Name, Name.data()) != 0, "Storage block with the name \"", sb.Name, "\" has already been enumerated");
#    endif
                }
            }
        }

        if (IsNewBlock)
        {
            StorageBlocks.emplace_back(
                NamesPool.emplace(Name.data()).first->c_str(),
                ShaderStages,
                SHADER_RESOURCE_TYPE_BUFFER_UAV,
                StorageBufferBinding,
                static_cast<Uint32>(ArraySize),
                SBIndex //
            );
        }

        if (glShaderStorageBlockBinding)
        {
            glShaderStorageBlockBinding(GLProgram, SBIndex, StorageBufferBinding);
            CHECK_GL_ERROR("glShaderStorageBlockBinding() failed");
        }
        else
        {
            LOG_WARNING_MESSAGE("glShaderStorageBlockBinding is not available on this device and "
                                "the engine is unable to automatically assign shader storage block bindindg for '",
                                Name.data(), "' variable. Expected binding: ", StorageBufferBinding,
                                ". Make sure that this binding is explicitly assigned in shader source code."
                                " Note that if the source code is converted from HLSL and if storage blocks are only used"
                                " by a single shader stage, then bindings automatically assigned by HLSL->GLSL"
                                " converter will work fine.");
        }
        ++StorageBufferBinding;
    }
#endif

    State.SetProgram(GLObjectWrappers::GLProgramObj::Null());

    AllocateResources(UniformBlocks, Samplers, Images, StorageBlocks);
}

ShaderResourceDesc GLProgramResources::GetResourceDesc(Uint32 Index) const
{
    if (Index < m_NumUniformBuffers)
        return GetUniformBuffer(Index).GetResourceDesc();
    else
        Index -= m_NumUniformBuffers;

    if (Index < m_NumSamplers)
        return GetSampler(Index).GetResourceDesc();
    else
        Index -= m_NumSamplers;

    if (Index < m_NumImages)
        return GetImage(Index).GetResourceDesc();
    else
        Index -= m_NumImages;

    if (Index < m_NumStorageBlocks)
        return GetStorageBlock(Index).GetResourceDesc();
    else
        Index -= m_NumStorageBlocks;

    LOG_ERROR_MESSAGE("Resource index ", Index + GetVariableCount(), " is invalid");
    return ShaderResourceDesc{};
}

void GLProgramResources::CountResources(const PipelineResourceLayoutDesc&    ResourceLayout,
                                        const SHADER_RESOURCE_VARIABLE_TYPE* AllowedVarTypes,
                                        Uint32                               NumAllowedTypes,
                                        ResourceCounters&                    Counters) const
{
    // clang-format off
    ProcessConstResources(
        [&](const GLProgramResources::UniformBufferInfo& UB)
        {
            ++Counters.NumUBs;
        },
        [&](const GLProgramResources::SamplerInfo& Sam)
        {
            ++Counters.NumSamplers;
        },
        [&](const GLProgramResources::ImageInfo& Img)
        {
            ++Counters.NumImages;
        },
        [&](const GLProgramResources::StorageBlockInfo& SB)
        {
            ++Counters.NumStorageBlocks;
        },
        &ResourceLayout,
        AllowedVarTypes,
        NumAllowedTypes
    );
    // clang-format on
}

bool GLProgramResources::IsCompatibleWith(const GLProgramResources& Res) const
{
    // clang-format off
    if (GetNumUniformBuffers() != Res.GetNumUniformBuffers() ||
        GetNumSamplers()       != Res.GetNumSamplers()       ||
        GetNumImages()         != Res.GetNumImages()         ||
        GetNumStorageBlocks()  != Res.GetNumStorageBlocks())
        return false;
    // clang-format on

    for (Uint32 ub = 0; ub < GetNumUniformBuffers(); ++ub)
    {
        const auto& UB0 = GetUniformBuffer(ub);
        const auto& UB1 = Res.GetUniformBuffer(ub);
        if (!UB0.IsCompatibleWith(UB1))
            return false;
    }

    for (Uint32 sam = 0; sam < GetNumSamplers(); ++sam)
    {
        const auto& Sam0 = GetSampler(sam);
        const auto& Sam1 = Res.GetSampler(sam);
        if (!Sam0.IsCompatibleWith(Sam1))
            return false;
    }

    for (Uint32 img = 0; img < GetNumImages(); ++img)
    {
        const auto& Img0 = GetImage(img);
        const auto& Img1 = Res.GetImage(img);
        if (!Img0.IsCompatibleWith(Img1))
            return false;
    }

    for (Uint32 sb = 0; sb < GetNumStorageBlocks(); ++sb)
    {
        const auto& SB0 = GetStorageBlock(sb);
        const auto& SB1 = Res.GetStorageBlock(sb);
        if (!SB0.IsCompatibleWith(SB1))
            return false;
    }

    return true;
}


size_t GLProgramResources::GetHash() const
{
    size_t hash = ComputeHash(GetNumUniformBuffers(), GetNumSamplers(), GetNumImages(), GetNumStorageBlocks());

    // clang-format off
    ProcessConstResources(
        [&](const UniformBufferInfo& UB)
        {
            HashCombine(hash, UB.GetHash());
        },
        [&](const SamplerInfo& Sam)
        {
            HashCombine(hash, Sam.GetHash());
        },
        [&](const ImageInfo& Img)
        {
            HashCombine(hash, Img.GetHash());
        },
        [&](const StorageBlockInfo& SB)
        {
            HashCombine(hash, SB.GetHash());
        }
    );
    // clang-format on

    return hash;
}

} // namespace Diligent
