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

/// \file
/// Declaration of Diligent::ShaderResourceCacheD3D12 class

// http://diligentgraphics.com/diligent-engine/architecture/d3d12/shader-resource-cache/

// Shader resource cache stores D3D12 resources in a continuous chunk of memory:
//
//
//                                         __________________________________________________________
//  m_pMemory                             |             m_pResources, m_NumResources == m            |
//  |                                     |                                                          |
//  V                                     |                                                          V
//  |  RootTable[0]  |   ....    |  RootTable[Nrt-1]  |  Res[0]  |  ... |  Res[n-1]  |    ....     | Res[0]  |  ... |  Res[m-1]  |
//       |                                                A \
//       |                                                |  \
//       |________________________________________________|   \RefCntAutoPtr
//                    m_pResources, m_NumResources == n        \_________
//                                                             |  Object |
//                                                              ---------
//
//  Nrt = m_NumTables
//
//
// The cache is also assigned decriptor heap space to store shader visible descriptor handles (for non-dynamic resources).
//
//
//      DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
//  |   DescrptHndl[0]  ...  DescrptHndl[n-1]   |  DescrptHndl[0]  ...  DescrptHndl[m-1] |
//          A                                           A
//          |                                           |
//          | TableStartOffset                          | TableStartOffset
//          |                                           |
//   |    RootTable[0]    |    RootTable[1]    |    RootTable[2]    |     ....      |   RootTable[Nrt]   |
//                              |                                                           |
//                              | TableStartOffset                                          | InvalidDescriptorOffset
//                              |                                                           |
//                              V                                                           V
//                      |   DescrptHndl[0]  ...  DescrptHndl[n-1]   |                       X
//                       DESCRIPTOR_HEAP_TYPE_SAMPLER
//
//
//
// The allocation is inexed by the offset from the beginning of the root table
// Each root table is assigned the space to store exactly m_NumResources resources
// Dynamic resources are not assigned space in the descriptor heap allocation.
//
//
//
//   |      RootTable[i]       |       Res[0]      ...       Res[n-1]      |
//                      \
//       TableStartOffset\____
//                            \
//                             V
//                 .....       |   DescrptHndl[0]  ...  DescrptHndl[n-1]   |    ....
//

#include "DescriptorHeap.hpp"

namespace Diligent
{

enum class CachedResourceType : Int32
{
    Unknown = -1,
    CBV     = 0,
    TexSRV,
    BufSRV,
    TexUAV,
    BufUAV,
    Sampler,
    NumTypes
};

class ShaderResourceCacheD3D12
{
public:
    // This enum is used for debug purposes only
    enum DbgCacheContentType
    {
        StaticShaderResources,
        SRBResources
    };

    ShaderResourceCacheD3D12(DbgCacheContentType dbgContentType) noexcept
    // clang-format off
#ifdef DILIGENT_DEBUG
        : m_DbgContentType
    {
        dbgContentType
    }
#endif
    // clang-format on
    {
    }

    ~ShaderResourceCacheD3D12();

    static size_t GetRequiredMemorySize(Uint32 NumTables,
                                        Uint32 TableSizes[]);

    void Initialize(IMemoryAllocator& MemAllocator,
                    Uint32            NumTables,
                    Uint32            TableSizes[]);

    static constexpr Uint32 InvalidDescriptorOffset = static_cast<Uint32>(-1);

    //http://diligentgraphics.com/diligent-engine/architecture/d3d12/shader-resource-cache#Cache-Structure
    struct Resource
    {
        Resource() noexcept {}

        CachedResourceType Type = CachedResourceType::Unknown;
        // CPU descriptor handle of a cached resource in CPU-only descriptor heap
        // Note that for dynamic resources, this is the only available CPU descriptor handle
        D3D12_CPU_DESCRIPTOR_HANDLE  CPUDescriptorHandle = {0};
        RefCntAutoPtr<IDeviceObject> pObject;
    };

    class RootTable
    {
    public:
        RootTable(Uint32 NumResources, Resource* pResources) noexcept :
            // clang-format off
            m_NumResources{NumResources},
            m_pResources  {pResources  }
        // clang-format on
        {}

        inline const Resource& GetResource(Uint32 OffsetFromTableStart) const
        {
            VERIFY(OffsetFromTableStart < m_NumResources, "Root table is not large enough to store descriptor at offset ", OffsetFromTableStart);
            return m_pResources[OffsetFromTableStart];
        }

        inline const Resource& GetResource(Uint32                           OffsetFromTableStart,
                                           const D3D12_DESCRIPTOR_HEAP_TYPE dbgDescriptorHeapType,
                                           const SHADER_TYPE                dbgRefShaderType) const
        {
            VERIFY(m_dbgHeapType == dbgDescriptorHeapType, "Incosistent descriptor heap type");
            VERIFY(m_dbgShaderType == dbgRefShaderType, "Incosistent shader type");

            VERIFY(OffsetFromTableStart < m_NumResources, "Root table is not large enough to store descriptor at offset ", OffsetFromTableStart);
            return m_pResources[OffsetFromTableStart];
        }
        inline Resource& GetResource(Uint32                           OffsetFromTableStart,
                                     const D3D12_DESCRIPTOR_HEAP_TYPE dbgDescriptorHeapType,
                                     const SHADER_TYPE                dbgRefShaderType)
        {
            return const_cast<Resource&>(const_cast<const RootTable*>(this)->GetResource(OffsetFromTableStart, dbgDescriptorHeapType, dbgRefShaderType));
        }

        inline Uint32 GetSize() const { return m_NumResources; }

        // Offset from the start of the descriptor heap allocation to the start of the table
        Uint32 m_TableStartOffset = InvalidDescriptorOffset;

#ifdef DILIGENT_DEBUG
        void SetDebugAttribs(Uint32                           MaxOffset,
                             const D3D12_DESCRIPTOR_HEAP_TYPE dbgDescriptorHeapType,
                             const SHADER_TYPE                dbgRefShaderType)
        {
            VERIFY_EXPR(m_NumResources == MaxOffset);
            m_dbgHeapType   = dbgDescriptorHeapType;
            m_dbgShaderType = dbgRefShaderType;
        }

        D3D12_DESCRIPTOR_HEAP_TYPE DbgGetHeapType() const { return m_dbgHeapType; }
#endif

        const Uint32 m_NumResources = 0;

    private:
#ifdef DILIGENT_DEBUG
        D3D12_DESCRIPTOR_HEAP_TYPE m_dbgHeapType   = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
        SHADER_TYPE                m_dbgShaderType = SHADER_TYPE_UNKNOWN;
#endif

        Resource* const m_pResources = nullptr;
    };

    inline RootTable& GetRootTable(Uint32 RootIndex)
    {
        VERIFY_EXPR(RootIndex < m_NumTables);
        return reinterpret_cast<RootTable*>(m_pMemory)[RootIndex];
    }
    inline const RootTable& GetRootTable(Uint32 RootIndex) const
    {
        VERIFY_EXPR(RootIndex < m_NumTables);
        return reinterpret_cast<const RootTable*>(m_pMemory)[RootIndex];
    }

    inline Uint32 GetNumRootTables() const { return m_NumTables; }

    void SetDescriptorHeapSpace(DescriptorHeapAllocation&& CbcSrvUavHeapSpace, DescriptorHeapAllocation&& SamplerHeapSpace)
    {
        VERIFY(m_SamplerHeapSpace.GetCpuHandle().ptr == 0 && m_CbvSrvUavHeapSpace.GetCpuHandle().ptr == 0, "Space has already been allocated in GPU descriptor heaps");
#ifdef DILIGENT_DEBUG
        Uint32 NumSamplerDescriptors = 0, NumSrvCbvUavDescriptors = 0;
        for (Uint32 rt = 0; rt < m_NumTables; ++rt)
        {
            auto& Tbl = GetRootTable(rt);
            if (Tbl.m_TableStartOffset != InvalidDescriptorOffset)
            {
                if (Tbl.DbgGetHeapType() == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
                {
                    VERIFY(Tbl.m_TableStartOffset == NumSrvCbvUavDescriptors, "Descriptor space allocation is not continuous");
                    NumSrvCbvUavDescriptors = std::max(NumSrvCbvUavDescriptors, Tbl.m_TableStartOffset + Tbl.GetSize());
                }
                else
                {
                    VERIFY(Tbl.m_TableStartOffset == NumSamplerDescriptors, "Descriptor space allocation is not continuous");
                    NumSamplerDescriptors = std::max(NumSamplerDescriptors, Tbl.m_TableStartOffset + Tbl.GetSize());
                }
            }
        }
        VERIFY(NumSrvCbvUavDescriptors == CbcSrvUavHeapSpace.GetNumHandles() || NumSrvCbvUavDescriptors == 0 && CbcSrvUavHeapSpace.GetCpuHandle(0).ptr == 0, "Unexpected descriptor heap allocation size");
        VERIFY(NumSamplerDescriptors == SamplerHeapSpace.GetNumHandles() || NumSamplerDescriptors == 0 && SamplerHeapSpace.GetCpuHandle(0).ptr == 0, "Unexpected descriptor heap allocation size");
#endif

        m_CbvSrvUavHeapSpace = std::move(CbcSrvUavHeapSpace);
        m_SamplerHeapSpace   = std::move(SamplerHeapSpace);
    }

    ID3D12DescriptorHeap* GetSrvCbvUavDescriptorHeap() { return m_CbvSrvUavHeapSpace.GetDescriptorHeap(); }
    ID3D12DescriptorHeap* GetSamplerDescriptorHeap() { return m_SamplerHeapSpace.GetDescriptorHeap(); }

    // Returns CPU descriptor handle of a shader visible descriptor heap allocation
    template <D3D12_DESCRIPTOR_HEAP_TYPE HeapType>
    D3D12_CPU_DESCRIPTOR_HANDLE GetShaderVisibleTableCPUDescriptorHandle(Uint32 RootParamInd, Uint32 OffsetFromTableStart = 0) const
    {
        const auto& RootParam = GetRootTable(RootParamInd);
        VERIFY(HeapType == RootParam.DbgGetHeapType(), "Invalid descriptor heap type");

        D3D12_CPU_DESCRIPTOR_HANDLE CPUDescriptorHandle = {0};
        // Descriptor heap allocation is not assigned for dynamic resources or
        // in a special case when resource cache is used to store static
        // variable assignments for a shader. It is also not assigned to root views
        if (RootParam.m_TableStartOffset != InvalidDescriptorOffset)
        {
            VERIFY(OffsetFromTableStart < RootParam.m_NumResources, "Offset is out of range");
            if (HeapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
            {
                VERIFY_EXPR(!m_SamplerHeapSpace.IsNull());
                CPUDescriptorHandle = m_SamplerHeapSpace.GetCpuHandle(RootParam.m_TableStartOffset + OffsetFromTableStart);
            }
            else if (HeapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
            {
                VERIFY_EXPR(!m_CbvSrvUavHeapSpace.IsNull());
                CPUDescriptorHandle = m_CbvSrvUavHeapSpace.GetCpuHandle(RootParam.m_TableStartOffset + OffsetFromTableStart);
            }
            else
            {
                UNEXPECTED("Unexpected descriptor heap type");
            }
        }

        return CPUDescriptorHandle;
    }

    // Returns GPU descriptor handle of a shader visible descriptor table
    template <D3D12_DESCRIPTOR_HEAP_TYPE HeapType>
    D3D12_GPU_DESCRIPTOR_HANDLE GetShaderVisibleTableGPUDescriptorHandle(Uint32 RootParamInd, Uint32 OffsetFromTableStart = 0) const
    {
        const auto& RootParam = GetRootTable(RootParamInd);
        VERIFY(RootParam.m_TableStartOffset != InvalidDescriptorOffset, "GPU descriptor handle must never be requested for dynamic resources");
        VERIFY(OffsetFromTableStart < RootParam.m_NumResources, "Offset is out of range");

        D3D12_GPU_DESCRIPTOR_HANDLE GPUDescriptorHandle = {0};
        VERIFY(HeapType == RootParam.DbgGetHeapType(), "Invalid descriptor heap type");
        if (HeapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
        {
            VERIFY_EXPR(!m_SamplerHeapSpace.IsNull());
            GPUDescriptorHandle = m_SamplerHeapSpace.GetGpuHandle(RootParam.m_TableStartOffset + OffsetFromTableStart);
        }
        else if (HeapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
        {
            VERIFY_EXPR(!m_CbvSrvUavHeapSpace.IsNull());
            GPUDescriptorHandle = m_CbvSrvUavHeapSpace.GetGpuHandle(RootParam.m_TableStartOffset + OffsetFromTableStart);
        }
        else
        {
            UNEXPECTED("Unexpected descriptor heap type");
        }

        return GPUDescriptorHandle;
    }

    Uint32& GetBoundDynamicCBsCounter() { return m_NumDynamicCBsBound; }

    // Returns the number of dynamic constant buffers bound in the cache regardless of their variable types
    Uint32 GetNumDynamicCBsBound() const { return m_NumDynamicCBsBound; }

#ifdef DILIGENT_DEBUG
    // Only for debug purposes: indicates what types of resources are stored in the cache
    DbgCacheContentType DbgGetContentType() const { return m_DbgContentType; }
    void                DbgVerifyBoundDynamicCBsCounter() const;
#endif

private:
    // clang-format off
    ShaderResourceCacheD3D12             (const ShaderResourceCacheD3D12&) = delete;
    ShaderResourceCacheD3D12             (ShaderResourceCacheD3D12&&)      = delete;
    ShaderResourceCacheD3D12& operator = (const ShaderResourceCacheD3D12&) = delete;
    ShaderResourceCacheD3D12& operator = (ShaderResourceCacheD3D12&&)      = delete;
    // clang-format on

    // Allocation in a GPU-visible sampler descriptor heap
    DescriptorHeapAllocation m_SamplerHeapSpace;

    // Allocation in a GPU-visible CBV/SRV/UAV descriptor heap
    DescriptorHeapAllocation m_CbvSrvUavHeapSpace;

    IMemoryAllocator* m_pAllocator = nullptr;
    void*             m_pMemory    = nullptr;
    Uint32            m_NumTables  = 0;
    // The number of the dynamic buffers bound in the resource cache regardless of their variable type
    Uint32 m_NumDynamicCBsBound = 0;

#ifdef DILIGENT_DEBUG
    // Only for debug purposes: indicates what types of resources are stored in the cache
    const DbgCacheContentType m_DbgContentType;
#endif
};

} // namespace Diligent
