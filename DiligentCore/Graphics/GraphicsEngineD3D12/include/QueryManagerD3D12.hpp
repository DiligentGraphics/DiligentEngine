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
#include <array>
#include <deque>
#include <vector>

#include "Query.h"

namespace Diligent
{

class CommandContext;

// https://microsoft.github.io/DirectX-Specs/d3d/CountersAndQueries.html#queries

class QueryManagerD3D12
{
public:
    QueryManagerD3D12(ID3D12Device* pd3d12Device,
                      const Uint32  QueryHeapSizes[]);
    ~QueryManagerD3D12();

    // clang-format off
    QueryManagerD3D12             (const QueryManagerD3D12&)  = delete;
    QueryManagerD3D12             (      QueryManagerD3D12&&) = delete;
    QueryManagerD3D12& operator = (const QueryManagerD3D12&)  = delete;
    QueryManagerD3D12& operator = (      QueryManagerD3D12&&) = delete;
    // clang-format on

    static constexpr Uint32 InvalidIndex = static_cast<Uint32>(-1);

    Uint32 AllocateQuery(QUERY_TYPE Type);
    void   ReleaseQuery(QUERY_TYPE Type, Uint32 Index);

    ID3D12QueryHeap* GetQueryHeap(QUERY_TYPE Type)
    {
        return m_Heaps[Type].pd3d12QueryHeap;
    }

    void BeginQuery(CommandContext& Ctx, QUERY_TYPE Type, Uint32 Index);
    void EndQuery(CommandContext& Ctx, QUERY_TYPE Type, Uint32 Index);
    void ReadQueryData(QUERY_TYPE Type, Uint32 Index, void* pDataPtr, Uint32 DataSize) const;

private:
    struct QueryHeapInfo
    {
        CComPtr<ID3D12QueryHeap> pd3d12QueryHeap;
        std::deque<Uint32>       AvailableQueries;
        std::vector<Uint32>      ResolveBufferOffsets;

        Uint32 HeapSize            = 0;
        Uint32 MaxAllocatedQueries = 0;
    };

    std::mutex                                      m_HeapMutex;
    std::array<QueryHeapInfo, QUERY_TYPE_NUM_TYPES> m_Heaps;

    // Readback buffer that will contain the query data.
    CComPtr<ID3D12Resource> m_pd3d12ResolveBuffer;
};

} // namespace Diligent
