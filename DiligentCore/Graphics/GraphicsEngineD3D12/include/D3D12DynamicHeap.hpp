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

#include <mutex>
#include <map>
#include <deque>
#include <atomic>

namespace Diligent
{

class RenderDeviceD3D12Impl;

struct D3D12DynamicAllocation
{
    D3D12DynamicAllocation() noexcept {}
    D3D12DynamicAllocation(ID3D12Resource*           pBuff,
                           Uint64                    _Offset,
                           Uint64                    _Size,
                           void*                     _CPUAddress,
                           D3D12_GPU_VIRTUAL_ADDRESS _GPUAddress
#ifdef DILIGENT_DEVELOPMENT
                           ,
                           Uint64 _DvpCtxFrameNumber
#endif
                           ) noexcept :
        // clang-format off
        pBuffer    {pBuff       }, 
        Offset     {_Offset     },
        Size       {_Size       },
        CPUAddress {_CPUAddress },
        GPUAddress {_GPUAddress }
#ifdef DILIGENT_DEVELOPMENT
      , DvpCtxFrameNumber(_DvpCtxFrameNumber)
#endif
    // clang-format on
    {}

    ID3D12Resource*           pBuffer    = nullptr; // The D3D buffer associated with this memory.
    Uint64                    Offset     = 0;       // Offset from start of buffer resource
    Uint64                    Size       = 0;       // Reserved size of this allocation
    void*                     CPUAddress = nullptr; // The CPU-writeable address
    D3D12_GPU_VIRTUAL_ADDRESS GPUAddress = 0;       // The GPU-visible address
#ifdef DILIGENT_DEVELOPMENT
    Uint64 DvpCtxFrameNumber = static_cast<Uint64>(-1);
#endif
};


class D3D12DynamicPage
{
public:
    D3D12DynamicPage(ID3D12Device* pd3d12Device, Uint64 Size);

    // clang-format off
    D3D12DynamicPage            (const D3D12DynamicPage&)  = delete;
    D3D12DynamicPage            (      D3D12DynamicPage&&) = default;
    D3D12DynamicPage& operator= (const D3D12DynamicPage&)  = delete;
    D3D12DynamicPage& operator= (      D3D12DynamicPage&&) = delete;
    // clang-format on

    void* GetCPUAddress(Uint64 Offset)
    {
        VERIFY_EXPR(m_pd3d12Buffer);
        VERIFY(Offset < GetSize(), "Offset (", Offset, ") exceeds buffer size (", GetSize(), ")");
        return reinterpret_cast<Uint8*>(m_CPUVirtualAddress) + Offset;
    }

    D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress(Uint64 Offset)
    {
        VERIFY_EXPR(m_pd3d12Buffer);
        VERIFY(Offset < GetSize(), "Offset (", Offset, ") exceeds buffer size (", GetSize(), ")");
        return m_GPUVirtualAddress + Offset;
    }

    ID3D12Resource* GetD3D12Buffer()
    {
        return m_pd3d12Buffer;
    }

    Uint64 GetSize() const
    {
        VERIFY_EXPR(m_pd3d12Buffer);
        return m_pd3d12Buffer->GetDesc().Width;
    }

    bool IsValid() const { return m_pd3d12Buffer != nullptr; }

private:
    CComPtr<ID3D12Resource>   m_pd3d12Buffer;
    void*                     m_CPUVirtualAddress = nullptr; // The CPU-writeable address
    D3D12_GPU_VIRTUAL_ADDRESS m_GPUVirtualAddress = 0;       // The GPU-visible address
};


class D3D12DynamicMemoryManager
{
public:
    D3D12DynamicMemoryManager(IMemoryAllocator&      Allocator,
                              RenderDeviceD3D12Impl& DeviceD3D12Impl,
                              Uint32                 NumPagesToReserve,
                              Uint64                 PageSize);
    ~D3D12DynamicMemoryManager();

    // clang-format off
    D3D12DynamicMemoryManager            (const D3D12DynamicMemoryManager&)  = delete;
    D3D12DynamicMemoryManager            (      D3D12DynamicMemoryManager&&) = delete;
    D3D12DynamicMemoryManager& operator= (const D3D12DynamicMemoryManager&)  = delete;
    D3D12DynamicMemoryManager& operator= (      D3D12DynamicMemoryManager&&) = delete;
    // clang-format on

    void ReleasePages(std::vector<D3D12DynamicPage>& Pages, Uint64 QueueMask);

    void Destroy();

    D3D12DynamicPage AllocatePage(Uint64 SizeInBytes);

#ifdef DILIGENT_DEVELOPMENT
    int32_t GetAllocatedPageCounter() const
    {
        return m_AllocatedPageCounter;
    }
#endif

private:
    RenderDeviceD3D12Impl& m_DeviceD3D12Impl;

    std::mutex m_AvailablePagesMtx;
    using AvailablePagesMapElemType = std::pair<const Uint64, D3D12DynamicPage>;
    std::multimap<Uint64, D3D12DynamicPage, std::less<Uint64>, STDAllocatorRawMem<AvailablePagesMapElemType>> m_AvailablePages;

#ifdef DILIGENT_DEVELOPMENT
    std::atomic_int32_t m_AllocatedPageCounter = 0;
#endif
};


class D3D12DynamicHeap
{
public:
    D3D12DynamicHeap(D3D12DynamicMemoryManager& DynamicMemMgr, std::string HeapName, Uint64 PageSize) :
        m_GlobalDynamicMemMgr{DynamicMemMgr},
        m_HeapName{std::move(HeapName)},
        m_PageSize{PageSize}
    {}

    // clang-format off
    D3D12DynamicHeap            (const D3D12DynamicHeap&) = delete;
    D3D12DynamicHeap            (D3D12DynamicHeap&&)      = delete;
    D3D12DynamicHeap& operator= (const D3D12DynamicHeap&) = delete;
    D3D12DynamicHeap& operator= (D3D12DynamicHeap&&)      = delete;
    // clang-format on

    ~D3D12DynamicHeap();

    D3D12DynamicAllocation Allocate(Uint64 SizeInBytes, Uint64 Alignment, Uint64 DvpCtxFrameNumber);
    void                   ReleaseAllocatedPages(Uint64 QueueMask);

    static constexpr Uint64 InvalidOffset = static_cast<Uint64>(-1);

    size_t GetAllocatedPagesCount() const { return m_AllocatedPages.size(); }

private:
    D3D12DynamicMemoryManager& m_GlobalDynamicMemMgr;
    const std::string          m_HeapName;

    std::vector<D3D12DynamicPage> m_AllocatedPages;

    const Uint64 m_PageSize;

    Uint64 m_CurrOffset    = InvalidOffset;
    Uint64 m_AvailableSize = 0;

    Uint64 m_CurrAllocatedSize = 0;
    Uint64 m_CurrUsedSize      = 0;
    Uint64 m_CurrAlignedSize   = 0;
    Uint64 m_PeakAllocatedSize = 0;
    Uint64 m_PeakUsedSize      = 0;
    Uint64 m_PeakAlignedSize   = 0;
};

} // namespace Diligent
