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

// Descriptor heap management utilities.
// See http://diligentgraphics.com/diligent-engine/architecture/d3d12/managing-descriptor-heaps/ for details

#pragma once

#include <mutex>
#include <vector>
#include <queue>
#include <string>
#include <unordered_set>
#include <atomic>
#include "ObjectBase.hpp"
#include "VariableSizeAllocationsManager.hpp"

namespace Diligent
{

class DescriptorHeapAllocation;
class DescriptorHeapAllocationManager;
class RenderDeviceD3D12Impl;

class IDescriptorAllocator
{
public:
    // Allocate Count descriptors
    virtual DescriptorHeapAllocation Allocate(uint32_t Count)                                         = 0;
    virtual void                     Free(DescriptorHeapAllocation&& Allocation, Uint64 CmdQueueMask) = 0;
    virtual Uint32                   GetDescriptorSize() const                                        = 0;
};


// The class represents descriptor heap allocation (continuous descriptor range in a descriptor heap)
//
//                  m_FirstCpuHandle
//                   |
//  | ~  ~  ~  ~  ~  X  X  X  X  X  X  X  ~  ~  ~  ~  ~  ~ |  D3D12 Descriptor Heap
//                   |
//                  m_FirstGpuHandle
//
class DescriptorHeapAllocation
{
public:
    // Creates null allocation
    DescriptorHeapAllocation() noexcept :
        // clang-format off
        m_NumHandles      {1      }, // One null descriptor handle
        m_pDescriptorHeap {nullptr},
        m_DescriptorSize  {0      }
    // clang-format on
    {
        m_FirstCpuHandle.ptr = 0;
        m_FirstGpuHandle.ptr = 0;
    }

    // Initializes non-null allocation
    DescriptorHeapAllocation(IDescriptorAllocator&       Allocator,
                             ID3D12DescriptorHeap*       pHeap,
                             D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle,
                             D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle,
                             Uint32                      NHandles,
                             Uint16                      AllocationManagerId) noexcept :
        // clang-format off
        m_FirstCpuHandle      {CpuHandle          }, 
        m_FirstGpuHandle      {GpuHandle          },
        m_pAllocator          {&Allocator         },
        m_NumHandles          {NHandles           },
        m_pDescriptorHeap     {pHeap              },
        m_AllocationManagerId {AllocationManagerId}
    // clang-format on
    {
        VERIFY_EXPR(m_pAllocator != nullptr && m_pDescriptorHeap != nullptr);
        auto DescriptorSize = m_pAllocator->GetDescriptorSize();
        VERIFY(DescriptorSize < std::numeric_limits<Uint16>::max(), "DescriptorSize exceeds allowed limit");
        m_DescriptorSize = static_cast<Uint16>(DescriptorSize);
    }

    // Move constructor (copy is not allowed)
    DescriptorHeapAllocation(DescriptorHeapAllocation&& Allocation) noexcept :
        // clang-format off
        m_FirstCpuHandle      {std::move(Allocation.m_FirstCpuHandle)     },
        m_FirstGpuHandle      {std::move(Allocation.m_FirstGpuHandle)     },
        m_NumHandles          {std::move(Allocation.m_NumHandles)         },
        m_pAllocator          {std::move(Allocation.m_pAllocator)         },
        m_AllocationManagerId {std::move(Allocation.m_AllocationManagerId)},
        m_pDescriptorHeap     {std::move(Allocation.m_pDescriptorHeap)    },
        m_DescriptorSize      {std::move(Allocation.m_DescriptorSize)     }
    // clang-format on
    {
        Allocation.Reset();
    }

    // Move assignment (assignment is not allowed)
    DescriptorHeapAllocation& operator=(DescriptorHeapAllocation&& Allocation) noexcept
    {
        m_FirstCpuHandle      = std::move(Allocation.m_FirstCpuHandle);
        m_FirstGpuHandle      = std::move(Allocation.m_FirstGpuHandle);
        m_NumHandles          = std::move(Allocation.m_NumHandles);
        m_pAllocator          = std::move(Allocation.m_pAllocator);
        m_AllocationManagerId = std::move(Allocation.m_AllocationManagerId);
        m_pDescriptorHeap     = std::move(Allocation.m_pDescriptorHeap);
        m_DescriptorSize      = std::move(Allocation.m_DescriptorSize);

        Allocation.Reset();

        return *this;
    }

    void Reset()
    {
        m_FirstCpuHandle.ptr  = 0;
        m_FirstGpuHandle.ptr  = 0;
        m_pAllocator          = nullptr;
        m_pDescriptorHeap     = nullptr;
        m_NumHandles          = 0;
        m_AllocationManagerId = static_cast<Uint16>(-1);
        m_DescriptorSize      = 0;
    }

    // clang-format off
    DescriptorHeapAllocation           (const DescriptorHeapAllocation&) = delete;
    DescriptorHeapAllocation& operator=(const DescriptorHeapAllocation&) = delete;
    // clang-format on


    // Destructor automatically releases this allocation through the allocator
    ~DescriptorHeapAllocation()
    {
        if (!IsNull() && m_pAllocator)
            m_pAllocator->Free(std::move(*this), ~Uint64{0});
        // Allocation must have been disposed by the allocator
        VERIFY(IsNull(), "Non-null descriptor is being destroyed");
    }

    // Returns CPU descriptor handle at the specified offset
    D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(Uint32 Offset = 0) const
    {
        VERIFY_EXPR(Offset >= 0 && Offset < m_NumHandles);

        D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle = m_FirstCpuHandle;
        CPUHandle.ptr += m_DescriptorSize * Offset;

        return CPUHandle;
    }

    // Returns GPU descriptor handle at the specified offset
    D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(Uint32 Offset = 0) const
    {
        VERIFY_EXPR(Offset >= 0 && Offset < m_NumHandles);
        D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle = m_FirstGpuHandle;
        GPUHandle.ptr += m_DescriptorSize * Offset;

        return GPUHandle;
    }

    // Returns pointer to D3D12 descriptor heap that contains this allocation
    ID3D12DescriptorHeap* GetDescriptorHeap() { return m_pDescriptorHeap; }


    // clang-format off
    size_t GetNumHandles()          const { return m_NumHandles;              }
    bool   IsNull()                 const { return m_FirstCpuHandle.ptr == 0; }
	bool   IsShaderVisible()        const { return m_FirstGpuHandle.ptr != 0; }
    size_t GetAllocationManagerId() const { return m_AllocationManagerId;     }
    UINT  GetDescriptorSize()       const { return m_DescriptorSize;          }
    // clang-format on

private:
    // First CPU descriptor handle in this allocation
    D3D12_CPU_DESCRIPTOR_HANDLE m_FirstCpuHandle = {0};

    // First GPU descriptor handle in this allocation
    D3D12_GPU_DESCRIPTOR_HANDLE m_FirstGpuHandle = {0};

    // Keep strong reference to the parent heap to make sure it is alive while allocation is alive - TOO EXPENSIVE
    //RefCntAutoPtr<IDescriptorAllocator> m_pAllocator;

    // Pointer to the descriptor heap allocator that created this allocation
    IDescriptorAllocator* m_pAllocator = nullptr;

    // Pointer to the D3D12 descriptor heap that contains descriptors in this allocation
    ID3D12DescriptorHeap* m_pDescriptorHeap = nullptr;

    // Number of descriptors in the allocation
    Uint32 m_NumHandles = 0;

    // Allocation manager ID. One allocator may support several
    // allocation managers. This field is required to identify
    // the manager within the allocator that was used to create
    // this allocation
    Uint16 m_AllocationManagerId = static_cast<Uint16>(-1);

    // Descriptor size
    Uint16 m_DescriptorSize = 0;
};


// The class performs suballocations within one D3D12 descriptor heap.
// It uses VariableSizeAllocationsManager to manage free space in the heap
//
// |  X  X  X  X  O  O  O  X  X  O  O  X  O  O  O  O  |  D3D12 descriptor heap
//
//  X - used descriptor
//  O - available descriptor
//
class DescriptorHeapAllocationManager
{
public:
    // Creates a new D3D12 descriptor heap
    DescriptorHeapAllocationManager(IMemoryAllocator&                 Allocator,
                                    RenderDeviceD3D12Impl&            DeviceD3D12Impl,
                                    IDescriptorAllocator&             ParentAllocator,
                                    size_t                            ThisManagerId,
                                    const D3D12_DESCRIPTOR_HEAP_DESC& HeapDesc);

    // Uses subrange of descriptors in the existing D3D12 descriptor heap
    // that starts at offset FirstDescriptor and uses NumDescriptors descriptors
    DescriptorHeapAllocationManager(IMemoryAllocator&      Allocator,
                                    RenderDeviceD3D12Impl& DeviceD3D12Impl,
                                    IDescriptorAllocator&  ParentAllocator,
                                    size_t                 ThisManagerId,
                                    ID3D12DescriptorHeap*  pd3d12DescriptorHeap,
                                    Uint32                 FirstDescriptor,
                                    Uint32                 NumDescriptors);


    // = default causes compiler error when instantiating std::vector::emplace_back() in Visual Studio 2015 (Version 14.0.23107.0 D14REL)
    DescriptorHeapAllocationManager(DescriptorHeapAllocationManager&& rhs) noexcept :
        // clang-format off
        m_ParentAllocator           {rhs.m_ParentAllocator           },
        m_DeviceD3D12Impl           {rhs.m_DeviceD3D12Impl           },
        m_ThisManagerId             {rhs.m_ThisManagerId             },
        m_HeapDesc                  {rhs.m_HeapDesc                  },
        m_DescriptorSize            {rhs.m_DescriptorSize            },
        m_NumDescriptorsInAllocation{rhs.m_NumDescriptorsInAllocation},
	    m_FirstCPUHandle            {rhs.m_FirstCPUHandle            },
        m_FirstGPUHandle            {rhs.m_FirstGPUHandle            },
        m_MaxAllocatedSize          {rhs.m_MaxAllocatedSize          },
        // Mutex is not movable
        //m_FreeBlockManagerMutex     (std::move(rhs.m_FreeBlockManagerMutex))
        m_FreeBlockManager          {std::move(rhs.m_FreeBlockManager)    },
        m_pd3d12DescriptorHeap      {std::move(rhs.m_pd3d12DescriptorHeap)}
    // clang-format on
    {
        rhs.m_NumDescriptorsInAllocation = 0; // Must be set to zero so that debug check in dtor passes
        rhs.m_ThisManagerId              = static_cast<size_t>(-1);
        rhs.m_FirstCPUHandle.ptr         = 0;
        rhs.m_FirstGPUHandle.ptr         = 0;
        rhs.m_MaxAllocatedSize           = 0;
#ifdef DILIGENT_DEVELOPMENT
        m_AllocationsCounter.store(rhs.m_AllocationsCounter.load());
        rhs.m_AllocationsCounter = 0;
#endif
    }

    // clang-format off
    // No copies or move-assignments
    DescriptorHeapAllocationManager& operator = (DescriptorHeapAllocationManager&&)      = delete;
    DescriptorHeapAllocationManager             (const DescriptorHeapAllocationManager&) = delete;
    DescriptorHeapAllocationManager& operator = (const DescriptorHeapAllocationManager&) = delete;
    // clang-format on

    ~DescriptorHeapAllocationManager();

    // Allocates Count descriptors
    DescriptorHeapAllocation Allocate(uint32_t Count);
    void                     FreeAllocation(DescriptorHeapAllocation&& Allocation);

    // clang-format off
    size_t GetNumAvailableDescriptors()const { return m_FreeBlockManager.GetFreeSize(); }
	Uint32 GetMaxDescriptors()         const { return m_NumDescriptorsInAllocation;     }
    size_t GetMaxAllocatedSize()       const { return m_MaxAllocatedSize;               }
    // clang-format on

#ifdef DILIGENT_DEVELOPMENT
    int32_t DvpGetAllocationsCounter() const
    {
        return m_AllocationsCounter;
    }
#endif

private:
    IDescriptorAllocator&  m_ParentAllocator;
    RenderDeviceD3D12Impl& m_DeviceD3D12Impl;

    // External ID assigned to this descriptor allocations manager
    size_t m_ThisManagerId = static_cast<size_t>(-1);

    // Heap description
    const D3D12_DESCRIPTOR_HEAP_DESC m_HeapDesc;

    const UINT m_DescriptorSize = 0;

    // Number of descriptors in the allocation.
    // If this manager was initialized as a subrange in the existing heap,
    // this value may be different from m_HeapDesc.NumDescriptors
    Uint32 m_NumDescriptorsInAllocation = 0;

    // Allocations manager used to handle descriptor allocations within the heap
    std::mutex                     m_FreeBlockManagerMutex;
    VariableSizeAllocationsManager m_FreeBlockManager;

    // Strong reference to D3D12 descriptor heap object
    CComPtr<ID3D12DescriptorHeap> m_pd3d12DescriptorHeap;

    // First CPU descriptor handle in the available descriptor range
    D3D12_CPU_DESCRIPTOR_HANDLE m_FirstCPUHandle = {0};

    // First GPU descriptor handle in the available descriptor range
    D3D12_GPU_DESCRIPTOR_HANDLE m_FirstGPUHandle = {0};

    size_t m_MaxAllocatedSize = 0;

#ifdef DILIGENT_DEVELOPMENT
    std::atomic_int32_t m_AllocationsCounter = 0;
#endif

    // Note: when adding new members, do not forget to update move ctor
};

// CPU descriptor heap is intended to provide storage for resource view descriptor handles.
// It contains a pool of DescriptorHeapAllocationManager object instances, where every instance manages
// its own CPU-only D3D12 descriptor heap:
//
//           m_HeapPool[0]                m_HeapPool[1]                 m_HeapPool[2]
//   |  X  X  X  X  X  X  X  X |, |  X  X  X  O  O  X  X  O  |, |  X  O  O  O  O  O  O  O  |
//
//    X - used descriptor                m_AvailableHeaps = {1,2}
//    O - available descriptor
//
// Allocation routine goes through the list of managers that have available descriptors and tries to process
// the request using every manager. If there are no available managers or no manager was able to handle the request,
// the function creates a new descriptor heap manager and lets it handle the request
//
// Render device contains four CPUDescriptorHeap object instances (one for each D3D12 heap type). The heaps are accessed
// when a texture or a buffer view is created.
//
class CPUDescriptorHeap final : public IDescriptorAllocator
{
public:
    // Initializes the heap
    CPUDescriptorHeap(IMemoryAllocator&           Allocator,
                      RenderDeviceD3D12Impl&      DeviceD3D12Impl,
                      Uint32                      NumDescriptorsInHeap,
                      D3D12_DESCRIPTOR_HEAP_TYPE  Type,
                      D3D12_DESCRIPTOR_HEAP_FLAGS Flags);

    // clang-format off
    CPUDescriptorHeap             (const CPUDescriptorHeap&) = delete;
    CPUDescriptorHeap             (CPUDescriptorHeap&&)      = delete;
    CPUDescriptorHeap& operator = (const CPUDescriptorHeap&) = delete;
    CPUDescriptorHeap& operator = (CPUDescriptorHeap&&)      = delete;
    // clang-format on

    ~CPUDescriptorHeap();

    virtual DescriptorHeapAllocation Allocate(uint32_t Count) override final;
    virtual void                     Free(DescriptorHeapAllocation&& Allocation, Uint64 CmdQueueMask) override final;
    virtual Uint32                   GetDescriptorSize() const override final { return m_DescriptorSize; }

#ifdef DILIGENT_DEVELOPMENT
    int32_t DvpGetTotalAllocationCount();
#endif

private:
    void FreeAllocation(DescriptorHeapAllocation&& Allocation);

    IMemoryAllocator&      m_MemAllocator;
    RenderDeviceD3D12Impl& m_DeviceD3D12Impl;

    // Pool of descriptor heap managers
    std::mutex                                                                                        m_HeapPoolMutex;
    std::vector<DescriptorHeapAllocationManager, STDAllocatorRawMem<DescriptorHeapAllocationManager>> m_HeapPool;
    // Indices of available descriptor heap managers
    std::unordered_set<size_t, std::hash<size_t>, std::equal_to<size_t>, STDAllocatorRawMem<size_t>> m_AvailableHeaps;

    D3D12_DESCRIPTOR_HEAP_DESC m_HeapDesc;
    const UINT                 m_DescriptorSize = 0;

    // Maximum heap size during the application lifetime - for statistic purposes
    Uint32 m_MaxSize     = 0;
    Uint32 m_CurrentSize = 0;
};

// GPU descriptor heap provides storage for shader-visible descriptors
// The heap contains single D3D12 descriptor heap that is split into two parts.
// The first part stores static and mutable resource descriptor handles.
// The second part is intended to provide temporary storage for dynamic resources.
// Space for dynamic resources is allocated in chunks, and then descriptors are suballocated within every
// chunk. DynamicSuballocationsManager facilitates this process.
//
//
//     static and mutable handles      ||                 dynamic space
//                                     ||    chunk 0     chunk 1     chunk 2     unused
//  | X O O X X O X O O O O X X X X O  ||  | X X X O | | X X O O | | O O O O |  O O O O  ||
//                                               |         |
//                                     suballocation       suballocation
//                                    within chunk 0       within chunk 1
//
// Render device contains two GPUDescriptorHeap instances (CBV_SRV_UAV and SAMPLER). The heaps
// are used to allocate GPU-visible descriptors for shader resource binding objects. The heaps
// are also used by the command contexts (through DynamicSuballocationsManager to allocated dynamic descriptors)
//
//  _______________________________________________________________________________________________________________________________
// | Render Device                                                                                                                 |
// |                                                                                                                               |
// | m_CPUDescriptorHeaps[CBV_SRV_UAV] |  X  X  X  X  X  X  X  X  |, |  X  X  X  X  X  X  X  X  |, |  X  O  O  X  O  O  O  O  |    |
// | m_CPUDescriptorHeaps[SAMPLER]     |  X  X  X  X  O  O  O  X  |, |  X  O  O  X  O  O  O  O  |                                  |
// | m_CPUDescriptorHeaps[RTV]         |  X  X  X  O  O  O  O  O  |, |  O  O  O  O  O  O  O  O  |                                  |
// | m_CPUDescriptorHeaps[DSV]         |  X  X  X  O  X  O  X  O  |                                                                |
// |                                                                               ctx1        ctx2                                |
// | m_GPUDescriptorHeaps[CBV_SRV_UAV]  | X O O X X O X O O O O X X X X O  ||  | X X X O | | X X O O | | O O O O |  O O O O  ||    |
// | m_GPUDescriptorHeaps[SAMPLER]      | X X O O X O X X X O O X O O O O  ||  | X X O O | | X O O O | | O O O O |  O O O O  ||    |
// |                                                                                                                               |
// |_______________________________________________________________________________________________________________________________|
//
//  ________________________________________________               ________________________________________________
// |Device Context 1                                |             |Device Context 2                                |
// |                                                |             |                                                |
// | m_DynamicGPUDescriptorAllocator[CBV_SRV_UAV]   |             | m_DynamicGPUDescriptorAllocator[CBV_SRV_UAV]   |
// | m_DynamicGPUDescriptorAllocator[SAMPLER]       |             | m_DynamicGPUDescriptorAllocator[SAMPLER]       |
// |________________________________________________|             |________________________________________________|
//
class GPUDescriptorHeap final : public IDescriptorAllocator
{
public:
    GPUDescriptorHeap(IMemoryAllocator&           Allocator,
                      RenderDeviceD3D12Impl&      Device,
                      Uint32                      NumDescriptorsInHeap,
                      Uint32                      NumDynamicDescriptors,
                      D3D12_DESCRIPTOR_HEAP_TYPE  Type,
                      D3D12_DESCRIPTOR_HEAP_FLAGS Flags);

    // clang-format off
    GPUDescriptorHeap             (const GPUDescriptorHeap&) = delete;
    GPUDescriptorHeap             (GPUDescriptorHeap&&)      = delete;
    GPUDescriptorHeap& operator = (const GPUDescriptorHeap&) = delete;
    GPUDescriptorHeap& operator = (GPUDescriptorHeap&&)      = delete;
    // clang-format on

    ~GPUDescriptorHeap();

    virtual DescriptorHeapAllocation Allocate(uint32_t Count) override final
    {
        return m_HeapAllocationManager.Allocate(Count);
    }

    virtual void   Free(DescriptorHeapAllocation&& Allocation, Uint64 CmdQueueMask) override final;
    virtual Uint32 GetDescriptorSize() const override final { return m_DescriptorSize; }

    DescriptorHeapAllocation AllocateDynamic(uint32_t Count)
    {
        return m_DynamicAllocationsManager.Allocate(Count);
    }

    const D3D12_DESCRIPTOR_HEAP_DESC& GetHeapDesc() const { return m_HeapDesc; }
    Uint32                            GetMaxStaticDescriptors() const { return m_HeapAllocationManager.GetMaxDescriptors(); }
    Uint32                            GetMaxDynamicDescriptors() const { return m_DynamicAllocationsManager.GetMaxDescriptors(); }

#ifdef DILIGENT_DEVELOPMENT
    int32_t DvpGetTotalAllocationCount() const
    {
        return m_HeapAllocationManager.DvpGetAllocationsCounter() +
            m_DynamicAllocationsManager.DvpGetAllocationsCounter();
    }
#endif

protected:
    RenderDeviceD3D12Impl& m_DeviceD3D12Impl;

    const D3D12_DESCRIPTOR_HEAP_DESC m_HeapDesc;
    CComPtr<ID3D12DescriptorHeap>    m_pd3d12DescriptorHeap;

    const UINT m_DescriptorSize;

    // Allocation manager for static/mutable part
    DescriptorHeapAllocationManager m_HeapAllocationManager;

    // Allocation manager for dynamic part
    DescriptorHeapAllocationManager m_DynamicAllocationsManager;
};


// The class facilitates allocation of dynamic descriptor handles. It requests a chunk of heap
// from the master GPU descriptor heap and then performs linear suballocation within the chunk
// At the end of the frame all allocations are disposed.

//     static and mutable handles     ||                 dynamic space
//                                    ||    chunk 0                 chunk 2
//  |                                 ||  | X X X O |             | O O O O |           || GPU Descriptor Heap
//                                        |                       |
//                                        m_Suballocations[0]     m_Suballocations[1]
//
class DynamicSuballocationsManager final : public IDescriptorAllocator
{
public:
    DynamicSuballocationsManager(IMemoryAllocator&  Allocator,
                                 GPUDescriptorHeap& ParentGPUHeap,
                                 Uint32             DynamicChunkSize,
                                 String             ManagerName);

    // clang-format off
    DynamicSuballocationsManager             (const DynamicSuballocationsManager&) = delete;
    DynamicSuballocationsManager             (DynamicSuballocationsManager&&)      = delete;
    DynamicSuballocationsManager& operator = (const DynamicSuballocationsManager&) = delete;
    DynamicSuballocationsManager& operator = (DynamicSuballocationsManager&&)      = delete;
    // clang-format on

    ~DynamicSuballocationsManager();

    void ReleaseAllocations(Uint64 CmdQueueMask);

    virtual DescriptorHeapAllocation Allocate(Uint32 Count) override final;
    virtual void                     Free(DescriptorHeapAllocation&& Allocation, Uint64 CmdQueueMask) override final
    {
        // Do nothing. Dynamic allocations are not disposed individually, but as whole chunks
        // at the end of the frame by ReleaseAllocations()
        Allocation.Reset();
    }

    virtual Uint32 GetDescriptorSize() const override final { return m_ParentGPUHeap.GetDescriptorSize(); }

    size_t GetSuballocationCount() const { return m_Suballocations.size(); }

private:
    // Parent GPU descriptor heap that is used to allocate chunks
    GPUDescriptorHeap& m_ParentGPUHeap;
    const String       m_ManagerName;

    // List of chunks allocated from the master GPU descriptor heap. All chunks are disposed at the end
    // of the frame
    std::vector<DescriptorHeapAllocation, STDAllocatorRawMem<DescriptorHeapAllocation>> m_Suballocations;

    Uint32 m_CurrentSuballocationOffset = 0;
    Uint32 m_DynamicChunkSize           = 0;

    Uint32 m_CurrDescriptorCount         = 0;
    Uint32 m_PeakDescriptorCount         = 0;
    Uint32 m_CurrSuballocationsTotalSize = 0;
    Uint32 m_PeakSuballocationsTotalSize = 0;
};

} // namespace Diligent
