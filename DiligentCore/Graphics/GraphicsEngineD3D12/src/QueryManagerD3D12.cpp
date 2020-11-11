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
#include <algorithm>

#include "QueryManagerD3D12.hpp"
#include "D3D12TypeConversions.hpp"
#include "GraphicsAccessories.hpp"
#include "CommandContext.hpp"
#include "Align.hpp"

namespace Diligent
{

static Uint32 GetQueryDataSize(QUERY_TYPE QueryType)
{
    // clang-format off
    switch (QueryType)
    {
        case QUERY_TYPE_OCCLUSION:
        case QUERY_TYPE_BINARY_OCCLUSION:
        case QUERY_TYPE_TIMESTAMP:
        case QUERY_TYPE_DURATION:
            return sizeof(Uint64);
            break;

        case QUERY_TYPE_PIPELINE_STATISTICS:
            return sizeof(D3D12_QUERY_DATA_PIPELINE_STATISTICS);
            break;

        static_assert(QUERY_TYPE_NUM_TYPES == 6, "Not all QUERY_TYPE enum values are tested");

        default:
            UNEXPECTED("Unexpected query type");
            return 0;
    }
    // clang-format on
}

QueryManagerD3D12::QueryManagerD3D12(ID3D12Device* pd3d12Device,
                                     const Uint32  QueryHeapSizes[])
{
    Uint32 ResolveBufferOffset = 0;
    for (Uint32 QueryType = QUERY_TYPE_UNDEFINED + 1; QueryType < QUERY_TYPE_NUM_TYPES; ++QueryType)
    {
        // clang-format off
        static_assert(QUERY_TYPE_OCCLUSION          == 1, "Unexpected value of QUERY_TYPE_OCCLUSION. EngineD3D12CreateInfo::QueryPoolSizes must be updated");
        static_assert(QUERY_TYPE_BINARY_OCCLUSION   == 2, "Unexpected value of QUERY_TYPE_BINARY_OCCLUSION. EngineD3D12CreateInfo::QueryPoolSizes must be updated");
        static_assert(QUERY_TYPE_TIMESTAMP          == 3, "Unexpected value of QUERY_TYPE_TIMESTAMP. EngineD3D12CreateInfo::QueryPoolSizes must be updated");
        static_assert(QUERY_TYPE_PIPELINE_STATISTICS== 4, "Unexpected value of QUERY_TYPE_PIPELINE_STATISTICS. EngineD3D12CreateInfo::QueryPoolSizes must be updated");
        static_assert(QUERY_TYPE_DURATION           == 5, "Unexpected value of QUERY_TYPE_DURATION. EngineD3D12CreateInfo::QueryPoolSizes must be updated");
        static_assert(QUERY_TYPE_NUM_TYPES          == 6, "Unexpected value of QUERY_TYPE_NUM_TYPES. EngineD3D12CreateInfo::QueryPoolSizes must be updated");
        // clang-format on
        auto& HeapInfo = m_Heaps[QueryType];

        D3D12_QUERY_HEAP_DESC d3d12HeapDesc = {};

        HeapInfo.HeapSize   = QueryHeapSizes[QueryType];
        d3d12HeapDesc.Type  = QueryTypeToD3D12QueryHeapType(static_cast<QUERY_TYPE>(QueryType));
        d3d12HeapDesc.Count = HeapInfo.HeapSize;
        if (QueryType == QUERY_TYPE_DURATION)
            d3d12HeapDesc.Count *= 2;

        auto hr = pd3d12Device->CreateQueryHeap(&d3d12HeapDesc, __uuidof(HeapInfo.pd3d12QueryHeap), reinterpret_cast<void**>(&HeapInfo.pd3d12QueryHeap));
        CHECK_D3D_RESULT_THROW(hr, "Failed to create D3D12 query heap of type");

        // AlignedDestinationBufferOffset must be a multiple of 8 bytes.
        // https://microsoft.github.io/DirectX-Specs/d3d/CountersAndQueries.html#resolvequerydata
        Uint32 AlignedQueryDataSize = Align(GetQueryDataSize(static_cast<QUERY_TYPE>(QueryType)), Uint32{8});
        HeapInfo.AvailableQueries.resize(HeapInfo.HeapSize);
        HeapInfo.ResolveBufferOffsets.resize(HeapInfo.HeapSize);
        for (Uint32 i = 0; i < HeapInfo.HeapSize; ++i)
        {
            HeapInfo.AvailableQueries[i]     = i;
            HeapInfo.ResolveBufferOffsets[i] = ResolveBufferOffset;
            ResolveBufferOffset += AlignedQueryDataSize;
        }
    }

    D3D12_RESOURCE_DESC D3D12BuffDesc = {};
    D3D12BuffDesc.Dimension           = D3D12_RESOURCE_DIMENSION_BUFFER;
    D3D12BuffDesc.Alignment           = 0;
    D3D12BuffDesc.Width               = ResolveBufferOffset;
    D3D12BuffDesc.Height              = 1;
    D3D12BuffDesc.DepthOrArraySize    = 1;
    D3D12BuffDesc.MipLevels           = 1;
    D3D12BuffDesc.Format              = DXGI_FORMAT_UNKNOWN;
    D3D12BuffDesc.SampleDesc.Count    = 1;
    D3D12BuffDesc.SampleDesc.Quality  = 0;
    // Layout must be D3D12_TEXTURE_LAYOUT_ROW_MAJOR, as buffer memory layouts are
    // understood by applications and row-major texture data is commonly marshaled through buffers.
    D3D12BuffDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    D3D12BuffDesc.Flags  = D3D12_RESOURCE_FLAG_NONE;

    D3D12_HEAP_PROPERTIES HeapProps = {};
    HeapProps.Type                  = D3D12_HEAP_TYPE_READBACK;
    HeapProps.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProps.MemoryPoolPreference  = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProps.CreationNodeMask      = 1;
    HeapProps.VisibleNodeMask       = 1;

    // The destination buffer of a query resolve operation must be in the D3D12_RESOURCE_USAGE_COPY_DEST state.
    // ResolveQueryData works with all heap types (default, upload, readback).
    // https://microsoft.github.io/DirectX-Specs/d3d/CountersAndQueries.html#resolvequerydata
    auto hr = pd3d12Device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE,
                                                    &D3D12BuffDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                                    __uuidof(m_pd3d12ResolveBuffer),
                                                    reinterpret_cast<void**>(static_cast<ID3D12Resource**>(&m_pd3d12ResolveBuffer)));
    if (FAILED(hr))
        LOG_ERROR_AND_THROW("Failed to create D3D12 resolve buffer");
}

QueryManagerD3D12::~QueryManagerD3D12()
{
    std::stringstream QueryUsageSS;
    QueryUsageSS << "D3D12 query manager peak usage:";

    for (Uint32 QueryType = QUERY_TYPE_UNDEFINED + 1; QueryType < QUERY_TYPE_NUM_TYPES; ++QueryType)
    {
        auto& HeapInfo = m_Heaps[QueryType];
        if (HeapInfo.AvailableQueries.size() != HeapInfo.HeapSize)
        {
            auto OutstandingQueries = HeapInfo.HeapSize - HeapInfo.AvailableQueries.size();
            if (OutstandingQueries == 1)
            {
                LOG_ERROR_MESSAGE("One query of type ", GetQueryTypeString(static_cast<QUERY_TYPE>(QueryType)),
                                  " has not been returned to the query manager");
            }
            else
            {
                LOG_ERROR_MESSAGE(OutstandingQueries, " queries of type ",
                                  GetQueryTypeString(static_cast<QUERY_TYPE>(QueryType)),
                                  " have not been returned to the query manager");
            }
        }
        QueryUsageSS << std::endl
                     << std::setw(30) << std::left << GetQueryTypeString(static_cast<QUERY_TYPE>(QueryType)) << ": "
                     << std::setw(4) << std::right << HeapInfo.MaxAllocatedQueries
                     << '/' << std::setw(4) << HeapInfo.HeapSize;
    }
    LOG_INFO_MESSAGE(QueryUsageSS.str());
}

Uint32 QueryManagerD3D12::AllocateQuery(QUERY_TYPE Type)
{
    std::lock_guard<std::mutex> Lock(m_HeapMutex);

    Uint32 Index            = InvalidIndex;
    auto&  HeapInfo         = m_Heaps[Type];
    auto&  AvailableQueries = HeapInfo.AvailableQueries;
    if (!AvailableQueries.empty())
    {
        Index = AvailableQueries.front();
        AvailableQueries.pop_front();
        HeapInfo.MaxAllocatedQueries = std::max(HeapInfo.MaxAllocatedQueries, HeapInfo.HeapSize - static_cast<Uint32>(AvailableQueries.size()));
    }

    return Index;
}

void QueryManagerD3D12::ReleaseQuery(QUERY_TYPE Type, Uint32 Index)
{
    std::lock_guard<std::mutex> Lock(m_HeapMutex);

    auto& HeapInfo = m_Heaps[Type];
    VERIFY(Index < HeapInfo.HeapSize, "Query index ", Index, " is out of range");
#ifdef DILIGENT_DEBUG
    for (const auto& ind : HeapInfo.AvailableQueries)
    {
        VERIFY(ind != Index, "Index ", Index, " already present in available queries list");
    }
#endif
    HeapInfo.AvailableQueries.push_back(Index);
}

void QueryManagerD3D12::BeginQuery(CommandContext& Ctx, QUERY_TYPE Type, Uint32 Index)
{
    auto d3d12QueryType = QueryTypeToD3D12QueryType(Type);
    Ctx.BeginQuery(m_Heaps[Type].pd3d12QueryHeap, d3d12QueryType, Index);
}

void QueryManagerD3D12::EndQuery(CommandContext& Ctx, QUERY_TYPE Type, Uint32 Index)
{
    auto  d3d12QueryType = QueryTypeToD3D12QueryType(Type);
    auto& HeapInfo       = m_Heaps[Type];
    Ctx.EndQuery(HeapInfo.pd3d12QueryHeap, d3d12QueryType, Index);

    // https://microsoft.github.io/DirectX-Specs/d3d/CountersAndQueries.html#resolvequerydata
    Ctx.ResolveQueryData(HeapInfo.pd3d12QueryHeap, d3d12QueryType, Index, 1, m_pd3d12ResolveBuffer, HeapInfo.ResolveBufferOffsets[Index]);
}

void QueryManagerD3D12::ReadQueryData(QUERY_TYPE Type, Uint32 Index, void* pDataPtr, Uint32 DataSize) const
{
    auto& HeapInfo      = m_Heaps[Type];
    auto  QueryDataSize = GetQueryDataSize(Type);
    VERIFY_EXPR(QueryDataSize == DataSize);
    auto        Offset = HeapInfo.ResolveBufferOffsets[Index];
    D3D12_RANGE ReadRange;
    ReadRange.Begin = Offset;
    ReadRange.End   = Offset + QueryDataSize;

    void* pBufferData = nullptr;
    // The pointer returned by Map is never offset by any values in pReadRange.
    m_pd3d12ResolveBuffer->Map(0, &ReadRange, &pBufferData);
    memcpy(pDataPtr, reinterpret_cast<const Uint8*>(pBufferData) + Offset, QueryDataSize);
    m_pd3d12ResolveBuffer->Unmap(0, nullptr);
}

} // namespace Diligent
