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

#include "RootSignature.hpp"
#include "ShaderResourceLayoutD3D12.hpp"
#include "ShaderD3D12Impl.hpp"
#include "CommandContext.hpp"
#include "RenderDeviceD3D12Impl.hpp"
#include "TextureD3D12Impl.hpp"
#include "D3D12TypeConversions.hpp"
#include "HashUtils.hpp"

namespace Diligent
{

RootSignature::RootParamsManager::RootParamsManager(IMemoryAllocator& MemAllocator) :
    m_MemAllocator{MemAllocator},
    m_pMemory{nullptr, STDDeleter<void, IMemoryAllocator>(MemAllocator)}
{}

size_t RootSignature::RootParamsManager::GetRequiredMemorySize(Uint32 NumExtraRootTables,
                                                               Uint32 NumExtraRootViews,
                                                               Uint32 NumExtraDescriptorRanges) const
{
    return sizeof(RootParameter) * (m_NumRootTables + NumExtraRootTables + m_NumRootViews + NumExtraRootViews) + sizeof(D3D12_DESCRIPTOR_RANGE) * (m_TotalDescriptorRanges + NumExtraDescriptorRanges);
}

D3D12_DESCRIPTOR_RANGE* RootSignature::RootParamsManager::Extend(Uint32 NumExtraRootTables,
                                                                 Uint32 NumExtraRootViews,
                                                                 Uint32 NumExtraDescriptorRanges,
                                                                 Uint32 RootTableToAddRanges)
{
    VERIFY(NumExtraRootTables > 0 || NumExtraRootViews > 0 || NumExtraDescriptorRanges > 0, "At least one root table, root view or descriptor range must be added");
    auto MemorySize = GetRequiredMemorySize(NumExtraRootTables, NumExtraRootViews, NumExtraDescriptorRanges);
    VERIFY_EXPR(MemorySize > 0);
    auto* pNewMemory = ALLOCATE_RAW(m_MemAllocator, "Memory buffer for root tables, root views & descriptor ranges", MemorySize);
    memset(pNewMemory, 0, MemorySize);

    // Note: this order is more efficient than views->tables->ranges
    auto* pNewRootTables          = reinterpret_cast<RootParameter*>(pNewMemory);
    auto* pNewRootViews           = pNewRootTables + (m_NumRootTables + NumExtraRootTables);
    auto* pCurrDescriptorRangePtr = reinterpret_cast<D3D12_DESCRIPTOR_RANGE*>(pNewRootViews + m_NumRootViews + NumExtraRootViews);

    // Copy existing root tables to new memory
    for (Uint32 rt = 0; rt < m_NumRootTables; ++rt)
    {
        const auto& SrcTbl      = GetRootTable(rt);
        auto&       D3D12SrcTbl = static_cast<const D3D12_ROOT_PARAMETER&>(SrcTbl).DescriptorTable;
        auto        NumRanges   = D3D12SrcTbl.NumDescriptorRanges;
        if (rt == RootTableToAddRanges)
        {
            VERIFY(NumExtraRootTables == 0 || NumExtraRootTables == 1, "Up to one descriptor table can be extended at a time");
            NumRanges += NumExtraDescriptorRanges;
        }
        new (pNewRootTables + rt) RootParameter(SrcTbl, NumRanges, pCurrDescriptorRangePtr);
        pCurrDescriptorRangePtr += NumRanges;
    }

    // Copy existing root views to new memory
    for (Uint32 rv = 0; rv < m_NumRootViews; ++rv)
    {
        const auto& SrcView = GetRootView(rv);
        new (pNewRootViews + rv) RootParameter(SrcView);
    }

    m_pMemory.reset(pNewMemory);
    m_NumRootTables += NumExtraRootTables;
    m_NumRootViews += NumExtraRootViews;
    m_TotalDescriptorRanges += NumExtraDescriptorRanges;
    m_pRootTables = m_NumRootTables != 0 ? pNewRootTables : nullptr;
    m_pRootViews  = m_NumRootViews != 0 ? pNewRootViews : nullptr;

    return pCurrDescriptorRangePtr;
}

void RootSignature::RootParamsManager::AddRootView(D3D12_ROOT_PARAMETER_TYPE     ParameterType,
                                                   Uint32                        RootIndex,
                                                   UINT                          Register,
                                                   D3D12_SHADER_VISIBILITY       Visibility,
                                                   SHADER_RESOURCE_VARIABLE_TYPE VarType)
{
    auto* pRangePtr = Extend(0, 1, 0);
    VERIFY_EXPR((char*)pRangePtr == (char*)m_pMemory.get() + GetRequiredMemorySize(0, 0, 0));
    new (m_pRootViews + m_NumRootViews - 1) RootParameter(ParameterType, RootIndex, Register, 0u, Visibility, VarType);
}

void RootSignature::RootParamsManager::AddRootTable(Uint32                        RootIndex,
                                                    D3D12_SHADER_VISIBILITY       Visibility,
                                                    SHADER_RESOURCE_VARIABLE_TYPE VarType,
                                                    Uint32                        NumRangesInNewTable)
{
    auto* pRangePtr = Extend(1, 0, NumRangesInNewTable);
    VERIFY_EXPR((char*)(pRangePtr + NumRangesInNewTable) == (char*)m_pMemory.get() + GetRequiredMemorySize(0, 0, 0));
    new (m_pRootTables + m_NumRootTables - 1) RootParameter(D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, RootIndex, NumRangesInNewTable, pRangePtr, Visibility, VarType);
}

void RootSignature::RootParamsManager::AddDescriptorRanges(Uint32 RootTableInd, Uint32 NumExtraRanges)
{
    auto* pRangePtr = Extend(0, 0, NumExtraRanges, RootTableInd);
    VERIFY_EXPR((char*)pRangePtr == (char*)m_pMemory.get() + GetRequiredMemorySize(0, 0, 0));
}

bool RootSignature::RootParamsManager::operator==(const RootParamsManager& RootParams) const
{
    if (m_NumRootTables != RootParams.m_NumRootTables ||
        m_NumRootViews != RootParams.m_NumRootViews)
        return false;

    for (Uint32 rv = 0; rv < m_NumRootViews; ++rv)
    {
        const auto& RV0 = GetRootView(rv);
        const auto& RV1 = RootParams.GetRootView(rv);
        if (RV0 != RV1)
            return false;
    }

    for (Uint32 rv = 0; rv < m_NumRootTables; ++rv)
    {
        const auto& RT0 = GetRootTable(rv);
        const auto& RT1 = RootParams.GetRootTable(rv);
        if (RT0 != RT1)
            return false;
    }

    return true;
}

size_t RootSignature::RootParamsManager::GetHash() const
{
    size_t hash = ComputeHash(m_NumRootTables, m_NumRootViews);
    for (Uint32 rv = 0; rv < m_NumRootViews; ++rv)
        HashCombine(hash, GetRootView(rv).GetHash());

    for (Uint32 rv = 0; rv < m_NumRootTables; ++rv)
        HashCombine(hash, GetRootTable(rv).GetHash());

    return hash;
}

RootSignature::RootSignature() :
    m_RootParams{GetRawAllocator()},
    m_MemAllocator{GetRawAllocator()},
    m_ImmutableSamplers(STD_ALLOCATOR_RAW_MEM(ImmutableSamplerAttribs, GetRawAllocator(), "Allocator for vector<ImmutableSamplerAttribs>"))
{
    m_SrvCbvUavRootTablesMap.fill(InvalidRootTableIndex);
    m_SamplerRootTablesMap.fill(InvalidRootTableIndex);
}

// clang-format off
static constexpr D3D12_SHADER_VISIBILITY ShaderTypeInd2ShaderVisibilityMap[]
{
    D3D12_SHADER_VISIBILITY_VERTEX,        // 0
    D3D12_SHADER_VISIBILITY_PIXEL,         // 1
    D3D12_SHADER_VISIBILITY_GEOMETRY,      // 2
    D3D12_SHADER_VISIBILITY_HULL,          // 3
    D3D12_SHADER_VISIBILITY_DOMAIN,        // 4
    D3D12_SHADER_VISIBILITY_ALL,           // 5
#ifdef D3D12_H_HAS_MESH_SHADER
    D3D12_SHADER_VISIBILITY_AMPLIFICATION, // 6
    D3D12_SHADER_VISIBILITY_MESH           // 7
#endif
};
// clang-format on
D3D12_SHADER_VISIBILITY GetShaderVisibility(SHADER_TYPE ShaderType)
{
    auto ShaderInd        = GetShaderTypeIndex(ShaderType);
    auto ShaderVisibility = ShaderTypeInd2ShaderVisibilityMap[ShaderInd];
#ifdef DILIGENT_DEBUG
    switch (ShaderType)
    {
        // clang-format off
        case SHADER_TYPE_VERTEX:        VERIFY_EXPR(ShaderVisibility == D3D12_SHADER_VISIBILITY_VERTEX);        break;
        case SHADER_TYPE_PIXEL:         VERIFY_EXPR(ShaderVisibility == D3D12_SHADER_VISIBILITY_PIXEL);         break;
        case SHADER_TYPE_GEOMETRY:      VERIFY_EXPR(ShaderVisibility == D3D12_SHADER_VISIBILITY_GEOMETRY);      break;
        case SHADER_TYPE_HULL:          VERIFY_EXPR(ShaderVisibility == D3D12_SHADER_VISIBILITY_HULL);          break;
        case SHADER_TYPE_DOMAIN:        VERIFY_EXPR(ShaderVisibility == D3D12_SHADER_VISIBILITY_DOMAIN);        break;
        case SHADER_TYPE_COMPUTE:       VERIFY_EXPR(ShaderVisibility == D3D12_SHADER_VISIBILITY_ALL);           break;
#   ifdef D3D12_H_HAS_MESH_SHADER
        case SHADER_TYPE_AMPLIFICATION: VERIFY_EXPR(ShaderVisibility == D3D12_SHADER_VISIBILITY_AMPLIFICATION); break;
        case SHADER_TYPE_MESH:          VERIFY_EXPR(ShaderVisibility == D3D12_SHADER_VISIBILITY_MESH);          break;
#   endif
        // clang-format on
        default: LOG_ERROR("Unknown shader type (", ShaderType, ")"); break;
    }
#endif
    return ShaderVisibility;
}

// clang-format off
static SHADER_TYPE ShaderVisibility2ShaderTypeMap[] = 
{
    SHADER_TYPE_COMPUTE,       // D3D12_SHADER_VISIBILITY_ALL           = 0
    SHADER_TYPE_VERTEX,        // D3D12_SHADER_VISIBILITY_VERTEX        = 1
    SHADER_TYPE_HULL,          // D3D12_SHADER_VISIBILITY_HULL          = 2
    SHADER_TYPE_DOMAIN,        // D3D12_SHADER_VISIBILITY_DOMAIN        = 3
    SHADER_TYPE_GEOMETRY,      // D3D12_SHADER_VISIBILITY_GEOMETRY      = 4
    SHADER_TYPE_PIXEL,         // D3D12_SHADER_VISIBILITY_PIXEL         = 5
    SHADER_TYPE_AMPLIFICATION, // D3D12_SHADER_VISIBILITY_AMPLIFICATION = 6
    SHADER_TYPE_MESH           // D3D12_SHADER_VISIBILITY_MESH          = 7
};
// clang-format on

SHADER_TYPE ShaderTypeFromShaderVisibility(D3D12_SHADER_VISIBILITY ShaderVisibility)
{
    VERIFY_EXPR(uint32_t(ShaderVisibility) < _countof(ShaderVisibility2ShaderTypeMap));
    auto ShaderType = ShaderVisibility2ShaderTypeMap[ShaderVisibility];
#ifdef DILIGENT_DEBUG
    switch (ShaderVisibility)
    {
        // clang-format off
        case D3D12_SHADER_VISIBILITY_VERTEX:        VERIFY_EXPR(ShaderType == SHADER_TYPE_VERTEX);        break;
        case D3D12_SHADER_VISIBILITY_PIXEL:         VERIFY_EXPR(ShaderType == SHADER_TYPE_PIXEL);         break;
        case D3D12_SHADER_VISIBILITY_GEOMETRY:      VERIFY_EXPR(ShaderType == SHADER_TYPE_GEOMETRY);      break;
        case D3D12_SHADER_VISIBILITY_HULL:          VERIFY_EXPR(ShaderType == SHADER_TYPE_HULL);          break;
        case D3D12_SHADER_VISIBILITY_DOMAIN:        VERIFY_EXPR(ShaderType == SHADER_TYPE_DOMAIN);        break;
        case D3D12_SHADER_VISIBILITY_ALL:           VERIFY_EXPR(ShaderType == SHADER_TYPE_COMPUTE);       break;
#   ifdef D3D12_H_HAS_MESH_SHADER
        case D3D12_SHADER_VISIBILITY_AMPLIFICATION: VERIFY_EXPR(ShaderType == SHADER_TYPE_AMPLIFICATION); break;
        case D3D12_SHADER_VISIBILITY_MESH:          VERIFY_EXPR(ShaderType == SHADER_TYPE_MESH);          break;
#   endif
        // clang-format on
        default: LOG_ERROR("Unknown shader visibility (", ShaderVisibility, ")"); break;
    }
#endif
    return ShaderType;
}


// clang-format off
static D3D12_DESCRIPTOR_HEAP_TYPE RangeType2HeapTypeMap[]
{
    D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, //D3D12_DESCRIPTOR_RANGE_TYPE_SRV	  = 0
    D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, //D3D12_DESCRIPTOR_RANGE_TYPE_UAV	  = ( D3D12_DESCRIPTOR_RANGE_TYPE_SRV + 1 )
    D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, //D3D12_DESCRIPTOR_RANGE_TYPE_CBV	  = ( D3D12_DESCRIPTOR_RANGE_TYPE_UAV + 1 )
    D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER      //D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER = ( D3D12_DESCRIPTOR_RANGE_TYPE_CBV + 1 ) 
};
// clang-format on
D3D12_DESCRIPTOR_HEAP_TYPE HeapTypeFromRangeType(D3D12_DESCRIPTOR_RANGE_TYPE RangeType)
{
    VERIFY_EXPR(RangeType >= D3D12_DESCRIPTOR_RANGE_TYPE_SRV && RangeType <= D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER);
    auto HeapType = RangeType2HeapTypeMap[RangeType];

#ifdef DILIGENT_DEBUG
    switch (RangeType)
    {
        // clang-format off
        case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:     VERIFY_EXPR(HeapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV); break;
        case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:     VERIFY_EXPR(HeapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV); break;
        case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:     VERIFY_EXPR(HeapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV); break;
        case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER: VERIFY_EXPR(HeapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);     break;
        // clang-format on
        default: UNEXPECTED("Unexpected descriptor range type"); break;
    }
#endif
    return HeapType;
}


void RootSignature::InitImmutableSampler(SHADER_TYPE                     ShaderType,
                                         const char*                     SamplerName,
                                         const char*                     SamplerSuffix,
                                         const D3DShaderResourceAttribs& SamplerAttribs)
{
    auto ShaderVisibility = GetShaderVisibility(ShaderType);
    auto SamplerFound     = false;
    for (auto& ImtblSmplr : m_ImmutableSamplers)
    {
        if (ImtblSmplr.ShaderVisibility == ShaderVisibility &&
            StreqSuff(SamplerName, ImtblSmplr.SamplerDesc.SamplerOrTextureName, SamplerSuffix))
        {
            ImtblSmplr.ShaderRegister = SamplerAttribs.BindPoint;
            ImtblSmplr.ArraySize      = SamplerAttribs.BindCount;
            ImtblSmplr.RegisterSpace  = 0;

            SamplerFound = true;
            break;
        }
    }

    if (!SamplerFound)
    {
        LOG_ERROR("Unable to find immutable sampler \'", SamplerName, '\'');
    }
}

// http://diligentgraphics.com/diligent-engine/architecture/d3d12/shader-resource-layout#Initializing-Shader-Resource-Layouts-and-Root-Signature-in-a-Pipeline-State-Object
void RootSignature::AllocateResourceSlot(SHADER_TYPE                     ShaderType,
                                         PIPELINE_TYPE                   PipelineType,
                                         const D3DShaderResourceAttribs& ShaderResAttribs,
                                         SHADER_RESOURCE_VARIABLE_TYPE   VariableType,
                                         D3D12_DESCRIPTOR_RANGE_TYPE     RangeType,
                                         Uint32&                         RootIndex,           // Output parameter
                                         Uint32&                         OffsetFromTableStart // Output parameter
)
{
    const auto ShaderVisibility = GetShaderVisibility(ShaderType);
    if (RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_CBV && ShaderResAttribs.BindCount == 1)
    {
        // Allocate single CBV directly in the root signature

        // Get the next available root index past all allocated tables and root views
        RootIndex            = m_RootParams.GetNumRootTables() + m_RootParams.GetNumRootViews();
        OffsetFromTableStart = 0;

        // Add new root view to existing root parameters
        m_RootParams.AddRootView(D3D12_ROOT_PARAMETER_TYPE_CBV, RootIndex, ShaderResAttribs.BindPoint, ShaderVisibility, VariableType);
    }
    else
    {
        const auto ShaderInd = GetShaderTypePipelineIndex(ShaderType, PipelineType);
        // Use the same table for static and mutable resources. Treat both as static
        const auto RootTableType = (VariableType == SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC) ? SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC : SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
        const auto TableIndKey   = ShaderInd * SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES + RootTableType;
        // Get the table array index (this is not the root index!)
        auto& RootTableArrayInd = ((RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER) ? m_SamplerRootTablesMap : m_SrvCbvUavRootTablesMap)[TableIndKey];
        if (RootTableArrayInd == InvalidRootTableIndex)
        {
            // Root table has not been assigned to this combination yet

            // Get the next available root index past all allocated tables and root views
            RootIndex = m_RootParams.GetNumRootTables() + m_RootParams.GetNumRootViews();
            VERIFY_EXPR(m_RootParams.GetNumRootTables() < 255);
            RootTableArrayInd = static_cast<Uint8>(m_RootParams.GetNumRootTables());
            // Add root table with one single-descriptor range
            m_RootParams.AddRootTable(RootIndex, ShaderVisibility, RootTableType, 1);
        }
        else
        {
            // Add a new single-descriptor range to the existing table at index RootTableArrayInd
            m_RootParams.AddDescriptorRanges(RootTableArrayInd, 1);
        }

        // Reference to either existing or just added table
        auto& CurrParam = m_RootParams.GetRootTable(RootTableArrayInd);
        RootIndex       = CurrParam.GetRootIndex();

        const auto& d3d12RootParam = static_cast<const D3D12_ROOT_PARAMETER&>(CurrParam);

        VERIFY(d3d12RootParam.ShaderVisibility == ShaderVisibility, "Shader visibility is not correct");

        // Descriptors are tightly packed, so the next descriptor offset is the
        // current size of the table
        OffsetFromTableStart = CurrParam.GetDescriptorTableSize();

        // New just added range is the last range in the descriptor table
        Uint32 NewDescriptorRangeIndex = d3d12RootParam.DescriptorTable.NumDescriptorRanges - 1;
        CurrParam.SetDescriptorRange(NewDescriptorRangeIndex,
                                     RangeType,                  // Range type (CBV, SRV, UAV or SAMPLER)
                                     ShaderResAttribs.BindPoint, // Shader register
                                     ShaderResAttribs.BindCount, // Number of registers used (1 for non-array resources)
                                     0,                          // Register space. Always 0 for now
                                     OffsetFromTableStart        // Offset in descriptors from the table start
        );
    }
}


#ifdef DILIGENT_DEBUG
void RootSignature::dbgVerifyRootParameters() const
{
    Uint32 dbgTotalSrvCbvUavSlots = 0;
    Uint32 dbgTotalSamplerSlots   = 0;
    for (Uint32 rt = 0; rt < m_RootParams.GetNumRootTables(); ++rt)
    {
        auto& RootTable = m_RootParams.GetRootTable(rt);
        auto& Param     = static_cast<const D3D12_ROOT_PARAMETER&>(RootTable);
        VERIFY(Param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, "Root parameter is expected to be a descriptor table");
        auto& Table = Param.DescriptorTable;
        VERIFY(Table.NumDescriptorRanges > 0, "Descriptor table is expected to be non-empty");
        VERIFY(Table.pDescriptorRanges[0].OffsetInDescriptorsFromTableStart == 0, "Descriptor table is expected to start at 0 offset");
        bool IsResourceTable = Table.pDescriptorRanges[0].RangeType != D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
        for (Uint32 r = 0; r < Table.NumDescriptorRanges; ++r)
        {
            const auto& range = Table.pDescriptorRanges[r];
            if (IsResourceTable)
            {
                // clang-format off
                VERIFY(range.RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SRV ||
                       range.RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_CBV ||
                       range.RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
                       "Resource type is expected to be SRV, CBV or UAV");
                // clang-format on
                dbgTotalSrvCbvUavSlots += range.NumDescriptors;
            }
            else
            {
                VERIFY(range.RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, "Resource type is expected to be sampler");
                dbgTotalSamplerSlots += range.NumDescriptors;
            }

            if (r > 0)
            {
                VERIFY(Table.pDescriptorRanges[r].OffsetInDescriptorsFromTableStart == Table.pDescriptorRanges[r - 1].OffsetInDescriptorsFromTableStart + Table.pDescriptorRanges[r - 1].NumDescriptors, "Ranges in a descriptor table are expected to be consequtive");
            }
        }
    }

    Uint32 dbgTotalRootViews = 0;
    for (Uint32 rv = 0; rv < m_RootParams.GetNumRootViews(); ++rv)
    {
        auto& RootView = m_RootParams.GetRootView(rv);
        auto& Param    = static_cast<const D3D12_ROOT_PARAMETER&>(RootView);
        VERIFY(Param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_CBV, "Root parameter is expected to be a CBV");
        ++dbgTotalRootViews;
    }

    // clang-format off
    VERIFY(dbgTotalSrvCbvUavSlots == 
                m_TotalSrvCbvUavSlots[SHADER_RESOURCE_VARIABLE_TYPE_STATIC] + 
                m_TotalSrvCbvUavSlots[SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE] + 
                m_TotalSrvCbvUavSlots[SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC], "Unexpected number of SRV CBV UAV resource slots");
    VERIFY(dbgTotalSamplerSlots == 
                m_TotalSamplerSlots[SHADER_RESOURCE_VARIABLE_TYPE_STATIC] +
                m_TotalSamplerSlots[SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE] + 
                m_TotalSamplerSlots[SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC], "Unexpected number of sampler slots");
    VERIFY(dbgTotalRootViews == 
                m_TotalRootViews[SHADER_RESOURCE_VARIABLE_TYPE_STATIC] +
                m_TotalRootViews[SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE] + 
                m_TotalRootViews[SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC], "Unexpected number of root views");
    // clang-format on
}
#endif

void RootSignature::AllocateImmutableSamplers(const PipelineResourceLayoutDesc& ResourceLayout)
{
    if (ResourceLayout.NumImmutableSamplers > 0)
    {
        m_ImmutableSamplers.reserve(ResourceLayout.NumImmutableSamplers);
        for (Uint32 sam = 0; sam < ResourceLayout.NumImmutableSamplers; ++sam)
        {
            const auto& ImtblSamDesc = ResourceLayout.ImmutableSamplers[sam];
            Uint32      ShaderStages = ImtblSamDesc.ShaderStages;
            while (ShaderStages != 0)
            {
                auto Stage = ShaderStages & ~(ShaderStages - 1);
                m_ImmutableSamplers.emplace_back(ImtblSamDesc, GetShaderVisibility(static_cast<SHADER_TYPE>(Stage)));
                ShaderStages &= ~Stage;
            }
        }
    }
}

void RootSignature::Finalize(ID3D12Device* pd3d12Device)
{
    for (Uint32 rt = 0; rt < m_RootParams.GetNumRootTables(); ++rt)
    {
        const auto& RootTbl        = m_RootParams.GetRootTable(rt);
        const auto& d3d12RootParam = static_cast<const D3D12_ROOT_PARAMETER&>(RootTbl);
        VERIFY_EXPR(d3d12RootParam.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE);

        auto TableSize = RootTbl.GetDescriptorTableSize();
        VERIFY(d3d12RootParam.DescriptorTable.NumDescriptorRanges > 0 && TableSize > 0, "Unexpected empty descriptor table");
        auto IsSamplerTable = d3d12RootParam.DescriptorTable.pDescriptorRanges[0].RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
        auto VarType        = RootTbl.GetShaderVariableType();
        (IsSamplerTable ? m_TotalSamplerSlots : m_TotalSrvCbvUavSlots)[VarType] += TableSize;
    }

    for (Uint32 rv = 0; rv < m_RootParams.GetNumRootViews(); ++rv)
    {
        const auto& RootView = m_RootParams.GetRootView(rv);
        ++m_TotalRootViews[RootView.GetShaderVariableType()];
    }

#ifdef DILIGENT_DEBUG
    dbgVerifyRootParameters();
#endif

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    auto TotalParams = m_RootParams.GetNumRootTables() + m_RootParams.GetNumRootViews();

    std::vector<D3D12_ROOT_PARAMETER, STDAllocatorRawMem<D3D12_ROOT_PARAMETER>> D3D12Parameters(TotalParams, D3D12_ROOT_PARAMETER{}, STD_ALLOCATOR_RAW_MEM(D3D12_ROOT_PARAMETER, GetRawAllocator(), "Allocator for vector<D3D12_ROOT_PARAMETER>"));
    for (Uint32 rt = 0; rt < m_RootParams.GetNumRootTables(); ++rt)
    {
        const auto&                 RootTable = m_RootParams.GetRootTable(rt);
        const D3D12_ROOT_PARAMETER& SrcParam  = RootTable;
        VERIFY(SrcParam.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE && SrcParam.DescriptorTable.NumDescriptorRanges > 0, "Non-empty descriptor table is expected");
        D3D12Parameters[RootTable.GetRootIndex()] = SrcParam;
    }
    for (Uint32 rv = 0; rv < m_RootParams.GetNumRootViews(); ++rv)
    {
        const auto&                 RootView = m_RootParams.GetRootView(rv);
        const D3D12_ROOT_PARAMETER& SrcParam = RootView;
        VERIFY(SrcParam.ParameterType == D3D12_ROOT_PARAMETER_TYPE_CBV, "Root CBV is expected");
        D3D12Parameters[RootView.GetRootIndex()] = SrcParam;
    }


    rootSignatureDesc.NumParameters = static_cast<UINT>(D3D12Parameters.size());
    rootSignatureDesc.pParameters   = D3D12Parameters.size() ? D3D12Parameters.data() : nullptr;

    UINT TotalD3D12StaticSamplers = 0;
    for (const auto& ImtblSam : m_ImmutableSamplers)
        TotalD3D12StaticSamplers += ImtblSam.ArraySize;
    rootSignatureDesc.NumStaticSamplers = TotalD3D12StaticSamplers;
    rootSignatureDesc.pStaticSamplers   = nullptr;
    std::vector<D3D12_STATIC_SAMPLER_DESC, STDAllocatorRawMem<D3D12_STATIC_SAMPLER_DESC>> D3D12StaticSamplers(STD_ALLOCATOR_RAW_MEM(D3D12_STATIC_SAMPLER_DESC, GetRawAllocator(), "Allocator for vector<D3D12_STATIC_SAMPLER_DESC>"));
    D3D12StaticSamplers.reserve(TotalD3D12StaticSamplers);
    if (!m_ImmutableSamplers.empty())
    {
        for (size_t s = 0; s < m_ImmutableSamplers.size(); ++s)
        {
            const auto& ImtblSmplrDesc = m_ImmutableSamplers[s];
            const auto& SamDesc        = ImtblSmplrDesc.SamplerDesc.Desc;
            for (UINT ArrInd = 0; ArrInd < ImtblSmplrDesc.ArraySize; ++ArrInd)
            {
                D3D12StaticSamplers.emplace_back(
                    D3D12_STATIC_SAMPLER_DESC //
                    {
                        FilterTypeToD3D12Filter(SamDesc.MinFilter, SamDesc.MagFilter, SamDesc.MipFilter),
                        TexAddressModeToD3D12AddressMode(SamDesc.AddressU),
                        TexAddressModeToD3D12AddressMode(SamDesc.AddressV),
                        TexAddressModeToD3D12AddressMode(SamDesc.AddressW),
                        SamDesc.MipLODBias,
                        SamDesc.MaxAnisotropy,
                        ComparisonFuncToD3D12ComparisonFunc(SamDesc.ComparisonFunc),
                        BorderColorToD3D12StaticBorderColor(SamDesc.BorderColor),
                        SamDesc.MinLOD,
                        SamDesc.MaxLOD,
                        ImtblSmplrDesc.ShaderRegister + ArrInd,
                        ImtblSmplrDesc.RegisterSpace,
                        ImtblSmplrDesc.ShaderVisibility //
                    }                                   //
                );
            }
        }
        rootSignatureDesc.pStaticSamplers = D3D12StaticSamplers.data();

        // Release immutable samplers array, we no longer need it
        std::vector<ImmutableSamplerAttribs, STDAllocatorRawMem<ImmutableSamplerAttribs>> EmptySamplers(STD_ALLOCATOR_RAW_MEM(ImmutableSamplerAttribs, GetRawAllocator(), "Allocator for vector<ImmutableSamplerAttribs>"));
        m_ImmutableSamplers.swap(EmptySamplers);

        VERIFY_EXPR(D3D12StaticSamplers.size() == TotalD3D12StaticSamplers);
    }


    CComPtr<ID3DBlob> signature;
    CComPtr<ID3DBlob> error;
    HRESULT           hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
    hr                   = pd3d12Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), __uuidof(m_pd3d12RootSignature), reinterpret_cast<void**>(static_cast<ID3D12RootSignature**>(&m_pd3d12RootSignature)));
    CHECK_D3D_RESULT_THROW(hr, "Failed to create root signature");

    bool bHasDynamicDescriptors = m_TotalSrvCbvUavSlots[SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC] != 0 || m_TotalSamplerSlots[SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC] != 0;
    if (bHasDynamicDescriptors)
    {
        CommitDescriptorHandles              = &RootSignature::CommitDescriptorHandlesInternal_SMD<false>;
        TransitionAndCommitDescriptorHandles = &RootSignature::CommitDescriptorHandlesInternal_SMD<true>;
    }
    else
    {
        CommitDescriptorHandles              = &RootSignature::CommitDescriptorHandlesInternal_SM<false>;
        TransitionAndCommitDescriptorHandles = &RootSignature::CommitDescriptorHandlesInternal_SM<true>;
    }
}

size_t RootSignature::GetResourceCacheRequiredMemSize() const
{
    auto CacheTableSizes = GetCacheTableSizes();
    return ShaderResourceCacheD3D12::GetRequiredMemorySize(static_cast<Uint32>(CacheTableSizes.size()), CacheTableSizes.data());
}

std::vector<Uint32, STDAllocatorRawMem<Uint32>> RootSignature::GetCacheTableSizes() const
{
    // Get root table size for every root index
    // m_RootParams keeps root tables sorted by the array index, not the root index
    // Root views are treated as one-descriptor tables
    std::vector<Uint32, STDAllocatorRawMem<Uint32>> CacheTableSizes(m_RootParams.GetNumRootTables() + m_RootParams.GetNumRootViews(), 0, STD_ALLOCATOR_RAW_MEM(Uint32, GetRawAllocator(), "Allocator for vector<Uint32>"));
    for (Uint32 rt = 0; rt < m_RootParams.GetNumRootTables(); ++rt)
    {
        auto& RootParam                           = m_RootParams.GetRootTable(rt);
        CacheTableSizes[RootParam.GetRootIndex()] = RootParam.GetDescriptorTableSize();
    }

    for (Uint32 rv = 0; rv < m_RootParams.GetNumRootViews(); ++rv)
    {
        auto& RootParam                           = m_RootParams.GetRootView(rv);
        CacheTableSizes[RootParam.GetRootIndex()] = 1;
    }

    return CacheTableSizes;
}

//http://diligentgraphics.com/diligent-engine/architecture/d3d12/shader-resource-cache#Initializing-the-Cache-for-Shader-Resource-Binding-Object
void RootSignature::InitResourceCache(RenderDeviceD3D12Impl*    pDeviceD3D12Impl,
                                      ShaderResourceCacheD3D12& ResourceCache,
                                      IMemoryAllocator&         CacheMemAllocator) const
{
    auto CacheTableSizes = GetCacheTableSizes();
    // Initialize resource cache to hold root tables
    ResourceCache.Initialize(CacheMemAllocator, static_cast<Uint32>(CacheTableSizes.size()), CacheTableSizes.data());

    // Allocate space in GPU-visible descriptor heap for static and mutable variables only
    Uint32 TotalSrvCbvUavDescriptors =
        m_TotalSrvCbvUavSlots[SHADER_RESOURCE_VARIABLE_TYPE_STATIC] +
        m_TotalSrvCbvUavSlots[SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE];
    Uint32 TotalSamplerDescriptors =
        m_TotalSamplerSlots[SHADER_RESOURCE_VARIABLE_TYPE_STATIC] +
        m_TotalSamplerSlots[SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE];

    DescriptorHeapAllocation CbcSrvUavHeapSpace, SamplerHeapSpace;
    if (TotalSrvCbvUavDescriptors)
    {
        CbcSrvUavHeapSpace = pDeviceD3D12Impl->AllocateGPUDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, TotalSrvCbvUavDescriptors);
        DEV_CHECK_ERR(!CbcSrvUavHeapSpace.IsNull(),
                      "Failed to allocate ", TotalSrvCbvUavDescriptors, " GPU-visible CBV/SRV/UAV descriptor",
                      (TotalSrvCbvUavDescriptors > 1 ? "s" : ""),
                      ". Consider increasing GPUDescriptorHeapSize[0] in EngineD3D12CreateInfo.");
    }
    VERIFY_EXPR(TotalSrvCbvUavDescriptors == 0 && CbcSrvUavHeapSpace.IsNull() || CbcSrvUavHeapSpace.GetNumHandles() == TotalSrvCbvUavDescriptors);

    if (TotalSamplerDescriptors)
    {
        SamplerHeapSpace = pDeviceD3D12Impl->AllocateGPUDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, TotalSamplerDescriptors);
        DEV_CHECK_ERR(!SamplerHeapSpace.IsNull(),
                      "Failed to allocate ", TotalSamplerDescriptors, " GPU-visible Sampler descriptor",
                      (TotalSamplerDescriptors > 1 ? "s" : ""),
                      ". Consider using immutable samplers in the Pipeline State Object or "
                      "increasing GPUDescriptorHeapSize[1] in EngineD3D12CreateInfo.");
    }
    VERIFY_EXPR(TotalSamplerDescriptors == 0 && SamplerHeapSpace.IsNull() || SamplerHeapSpace.GetNumHandles() == TotalSamplerDescriptors);

    // Iterate through all root static/mutable tables and assign start offsets. The tables are tightly packed, so
    // start offset of table N+1 is start offset of table N plus the size of table N.
    // Root tables with dynamic resources as well as root views are not assigned space in GPU-visible allocation
    // (root views are simply not processed)
    Uint32 SrvCbvUavTblStartOffset = 0;
    Uint32 SamplerTblStartOffset   = 0;
    for (Uint32 rt = 0; rt < m_RootParams.GetNumRootTables(); ++rt)
    {
        auto&       RootParam      = m_RootParams.GetRootTable(rt);
        const auto& D3D12RootParam = static_cast<const D3D12_ROOT_PARAMETER&>(RootParam);
        auto&       RootTableCache = ResourceCache.GetRootTable(RootParam.GetRootIndex());

        SHADER_TYPE dbgShaderType = SHADER_TYPE_UNKNOWN;
#ifdef DILIGENT_DEBUG
        dbgShaderType = ShaderTypeFromShaderVisibility(D3D12RootParam.ShaderVisibility);
#endif
        VERIFY_EXPR(D3D12RootParam.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE);

        auto TableSize = RootParam.GetDescriptorTableSize();
        VERIFY(TableSize > 0, "Unexpected empty descriptor table");

        auto HeapType = HeapTypeFromRangeType(D3D12RootParam.DescriptorTable.pDescriptorRanges[0].RangeType);

#ifdef DILIGENT_DEBUG
        RootTableCache.SetDebugAttribs(TableSize, HeapType, dbgShaderType);
#endif

        // Space for dynamic variables is allocated at every draw call
        if (RootParam.GetShaderVariableType() != SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC)
        {
            if (HeapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
            {
                RootTableCache.m_TableStartOffset = SrvCbvUavTblStartOffset;
                SrvCbvUavTblStartOffset += TableSize;
            }
            else
            {
                RootTableCache.m_TableStartOffset = SamplerTblStartOffset;
                SamplerTblStartOffset += TableSize;
            }
        }
        else
        {
            VERIFY_EXPR(RootTableCache.m_TableStartOffset == ShaderResourceCacheD3D12::InvalidDescriptorOffset);
        }
    }

#ifdef DILIGENT_DEBUG
    for (Uint32 rv = 0; rv < m_RootParams.GetNumRootViews(); ++rv)
    {
        auto&       RootParam      = m_RootParams.GetRootView(rv);
        const auto& D3D12RootParam = static_cast<const D3D12_ROOT_PARAMETER&>(RootParam);
        auto&       RootTableCache = ResourceCache.GetRootTable(RootParam.GetRootIndex());
        // Root views are not assigned valid table start offset
        VERIFY_EXPR(RootTableCache.m_TableStartOffset == ShaderResourceCacheD3D12::InvalidDescriptorOffset);

        SHADER_TYPE dbgShaderType = ShaderTypeFromShaderVisibility(D3D12RootParam.ShaderVisibility);
        VERIFY_EXPR(D3D12RootParam.ParameterType == D3D12_ROOT_PARAMETER_TYPE_CBV);
        RootTableCache.SetDebugAttribs(1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, dbgShaderType);
    }
#endif

    VERIFY_EXPR(SrvCbvUavTblStartOffset == TotalSrvCbvUavDescriptors);
    VERIFY_EXPR(SamplerTblStartOffset == TotalSamplerDescriptors);

    ResourceCache.SetDescriptorHeapSpace(std::move(CbcSrvUavHeapSpace), std::move(SamplerHeapSpace));
}

__forceinline void TransitionResource(CommandContext&                     Ctx,
                                      ShaderResourceCacheD3D12::Resource& Res,
                                      D3D12_DESCRIPTOR_RANGE_TYPE         RangeType)
{
    switch (Res.Type)
    {
        case CachedResourceType::CBV:
        {
            VERIFY(RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_CBV, "Unexpected descriptor range type");
            // Not using QueryInterface() for the sake of efficiency
            auto* pBuffToTransition = Res.pObject.RawPtr<BufferD3D12Impl>();
            if (pBuffToTransition->IsInKnownState() && !pBuffToTransition->CheckState(RESOURCE_STATE_CONSTANT_BUFFER))
                Ctx.TransitionResource(pBuffToTransition, RESOURCE_STATE_CONSTANT_BUFFER);
        }
        break;

        case CachedResourceType::BufSRV:
        {
            VERIFY(RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SRV, "Unexpected descriptor range type");
            auto* pBuffViewD3D12    = Res.pObject.RawPtr<BufferViewD3D12Impl>();
            auto* pBuffToTransition = pBuffViewD3D12->GetBuffer<BufferD3D12Impl>();
            if (pBuffToTransition->IsInKnownState() && !pBuffToTransition->CheckState(RESOURCE_STATE_SHADER_RESOURCE))
                Ctx.TransitionResource(pBuffToTransition, RESOURCE_STATE_SHADER_RESOURCE);
        }
        break;

        case CachedResourceType::BufUAV:
        {
            VERIFY(RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_UAV, "Unexpected descriptor range type");
            auto* pBuffViewD3D12    = Res.pObject.RawPtr<BufferViewD3D12Impl>();
            auto* pBuffToTransition = pBuffViewD3D12->GetBuffer<BufferD3D12Impl>();
            if (pBuffToTransition->IsInKnownState())
            {
                // We must always call TransitionResource() even when the state is already
                // RESOURCE_STATE_UNORDERED_ACCESS as in this case UAV barrier must be executed
                Ctx.TransitionResource(pBuffToTransition, RESOURCE_STATE_UNORDERED_ACCESS);
            }
        }
        break;

        case CachedResourceType::TexSRV:
        {
            VERIFY(RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SRV, "Unexpected descriptor range type");
            auto* pTexViewD3D12    = Res.pObject.RawPtr<TextureViewD3D12Impl>();
            auto* pTexToTransition = pTexViewD3D12->GetTexture<TextureD3D12Impl>();
            if (pTexToTransition->IsInKnownState() && !pTexToTransition->CheckAnyState(RESOURCE_STATE_SHADER_RESOURCE | RESOURCE_STATE_INPUT_ATTACHMENT))
                Ctx.TransitionResource(pTexToTransition, RESOURCE_STATE_SHADER_RESOURCE);
        }
        break;

        case CachedResourceType::TexUAV:
        {
            VERIFY(RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_UAV, "Unexpected descriptor range type");
            auto* pTexViewD3D12    = Res.pObject.RawPtr<TextureViewD3D12Impl>();
            auto* pTexToTransition = pTexViewD3D12->GetTexture<TextureD3D12Impl>();
            if (pTexToTransition->IsInKnownState())
            {
                // We must always call TransitionResource() even when the state is already
                // RESOURCE_STATE_UNORDERED_ACCESS as in this case UAV barrier must be executed
                Ctx.TransitionResource(pTexToTransition, RESOURCE_STATE_UNORDERED_ACCESS);
            }
        }
        break;

        case CachedResourceType::Sampler:
            VERIFY(RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, "Unexpected descriptor range type");
            break;

        default:
            // Resource not bound
            VERIFY(Res.Type == CachedResourceType::Unknown, "Unexpected resource type");
            VERIFY(Res.pObject == nullptr && Res.CPUDescriptorHandle.ptr == 0, "Bound resource is unexpected");
    }
}


#ifdef DILIGENT_DEVELOPMENT
void RootSignature::DvpVerifyResourceState(const ShaderResourceCacheD3D12::Resource& Res,
                                           D3D12_DESCRIPTOR_RANGE_TYPE               RangeType)
{
    switch (Res.Type)
    {
        case CachedResourceType::CBV:
        {
            VERIFY(RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_CBV, "Unexpected descriptor range type");
            // Not using QueryInterface() for the sake of efficiency
            const auto* pBufferD3D12 = Res.pObject.RawPtr<const BufferD3D12Impl>();
            if (pBufferD3D12->IsInKnownState() && !pBufferD3D12->CheckState(RESOURCE_STATE_CONSTANT_BUFFER))
            {
                LOG_ERROR_MESSAGE("Buffer '", pBufferD3D12->GetDesc().Name, "' must be in RESOURCE_STATE_CONSTANT_BUFFER state. Actual state: ",
                                  GetResourceStateString(pBufferD3D12->GetState()),
                                  ". Call IDeviceContext::TransitionShaderResources(), use RESOURCE_STATE_TRANSITION_MODE_TRANSITION "
                                  "when calling IDeviceContext::CommitShaderResources() or explicitly transition the buffer state "
                                  "with IDeviceContext::TransitionResourceStates().");
            }
        }
        break;

        case CachedResourceType::BufSRV:
        {
            VERIFY(RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SRV, "Unexpected descriptor range type");
            const auto* pBuffViewD3D12 = Res.pObject.RawPtr<const BufferViewD3D12Impl>();
            const auto* pBufferD3D12   = pBuffViewD3D12->GetBuffer<const BufferD3D12Impl>();
            if (pBufferD3D12->IsInKnownState() && !pBufferD3D12->CheckState(RESOURCE_STATE_SHADER_RESOURCE))
            {
                LOG_ERROR_MESSAGE("Buffer '", pBufferD3D12->GetDesc().Name, "' must be in RESOURCE_STATE_SHADER_RESOURCE state.  Actual state: ",
                                  GetResourceStateString(pBufferD3D12->GetState()),
                                  ". Call IDeviceContext::TransitionShaderResources(), use RESOURCE_STATE_TRANSITION_MODE_TRANSITION "
                                  "when calling IDeviceContext::CommitShaderResources() or explicitly transition the buffer state "
                                  "with IDeviceContext::TransitionResourceStates().");
            }
        }
        break;

        case CachedResourceType::BufUAV:
        {
            VERIFY(RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_UAV, "Unexpected descriptor range type");
            const auto* pBuffViewD3D12 = Res.pObject.RawPtr<const BufferViewD3D12Impl>();
            const auto* pBufferD3D12   = pBuffViewD3D12->GetBuffer<const BufferD3D12Impl>();
            if (pBufferD3D12->IsInKnownState() && !pBufferD3D12->CheckState(RESOURCE_STATE_UNORDERED_ACCESS))
            {
                LOG_ERROR_MESSAGE("Buffer '", pBufferD3D12->GetDesc().Name, "' must be in RESOURCE_STATE_UNORDERED_ACCESS state. Actual state: ",
                                  GetResourceStateString(pBufferD3D12->GetState()),
                                  ". Call IDeviceContext::TransitionShaderResources(), use RESOURCE_STATE_TRANSITION_MODE_TRANSITION "
                                  "when calling IDeviceContext::CommitShaderResources() or explicitly transition the buffer state "
                                  "with IDeviceContext::TransitionResourceStates().");
            }
        }
        break;

        case CachedResourceType::TexSRV:
        {
            VERIFY(RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SRV, "Unexpected descriptor range type");
            const auto* pTexViewD3D12 = Res.pObject.RawPtr<const TextureViewD3D12Impl>();
            const auto* pTexD3D12     = pTexViewD3D12->GetTexture<TextureD3D12Impl>();
            if (pTexD3D12->IsInKnownState() && !pTexD3D12->CheckAnyState(RESOURCE_STATE_SHADER_RESOURCE | RESOURCE_STATE_INPUT_ATTACHMENT))
            {
                LOG_ERROR_MESSAGE("Texture '", pTexD3D12->GetDesc().Name, "' must be in RESOURCE_STATE_SHADER_RESOURCE state. Actual state: ",
                                  GetResourceStateString(pTexD3D12->GetState()),
                                  ". Call IDeviceContext::TransitionShaderResources(), use RESOURCE_STATE_TRANSITION_MODE_TRANSITION "
                                  "when calling IDeviceContext::CommitShaderResources() or explicitly transition the texture state "
                                  "with IDeviceContext::TransitionResourceStates().");
            }
        }
        break;

        case CachedResourceType::TexUAV:
        {
            VERIFY(RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_UAV, "Unexpected descriptor range type");
            const auto* pTexViewD3D12 = Res.pObject.RawPtr<const TextureViewD3D12Impl>();
            const auto* pTexD3D12     = pTexViewD3D12->GetTexture<const TextureD3D12Impl>();
            if (pTexD3D12->IsInKnownState() && !pTexD3D12->CheckState(RESOURCE_STATE_UNORDERED_ACCESS))
            {
                LOG_ERROR_MESSAGE("Texture '", pTexD3D12->GetDesc().Name, "' must be in RESOURCE_STATE_UNORDERED_ACCESS state. Actual state: ",
                                  GetResourceStateString(pTexD3D12->GetState()),
                                  ". Call IDeviceContext::TransitionShaderResources(), use RESOURCE_STATE_TRANSITION_MODE_TRANSITION "
                                  "when calling IDeviceContext::CommitShaderResources() or explicitly transition the texture state "
                                  "with IDeviceContext::TransitionResourceStates().");
            }
        }
        break;

        case CachedResourceType::Sampler:
            VERIFY(RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, "Unexpected descriptor range type");
            break;

        default:
            // Resource not bound
            VERIFY(Res.Type == CachedResourceType::Unknown, "Unexpected resource type");
            VERIFY(Res.pObject == nullptr && Res.CPUDescriptorHandle.ptr == 0, "Bound resource is unexpected");
    }
}
#endif // DILIGENT_DEVELOPMENT

template <class TOperation>
__forceinline void RootSignature::RootParamsManager::ProcessRootTables(TOperation Operation) const
{
    for (Uint32 rt = 0; rt < m_NumRootTables; ++rt)
    {
        auto&                       RootTable  = GetRootTable(rt);
        auto                        RootInd    = RootTable.GetRootIndex();
        const D3D12_ROOT_PARAMETER& D3D12Param = RootTable;

        VERIFY_EXPR(D3D12Param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE);

        auto& d3d12Table = D3D12Param.DescriptorTable;
        VERIFY(d3d12Table.NumDescriptorRanges > 0 && RootTable.GetDescriptorTableSize() > 0, "Unexepected empty descriptor table");
        bool                       IsResourceTable = d3d12Table.pDescriptorRanges[0].RangeType != D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
        D3D12_DESCRIPTOR_HEAP_TYPE dbgHeapType     = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
#ifdef DILIGENT_DEBUG
        dbgHeapType = IsResourceTable ? D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV : D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
#endif
        Operation(RootInd, RootTable, D3D12Param, IsResourceTable, dbgHeapType);
    }
}

template <class TOperation>
__forceinline void ProcessCachedTableResources(Uint32                      RootInd,
                                               const D3D12_ROOT_PARAMETER& D3D12Param,
                                               ShaderResourceCacheD3D12&   ResourceCache,
                                               D3D12_DESCRIPTOR_HEAP_TYPE  dbgHeapType,
                                               TOperation                  Operation)
{
    for (UINT r = 0; r < D3D12Param.DescriptorTable.NumDescriptorRanges; ++r)
    {
        const auto& range = D3D12Param.DescriptorTable.pDescriptorRanges[r];
        for (UINT d = 0; d < range.NumDescriptors; ++d)
        {
            SHADER_TYPE dbgShaderType = SHADER_TYPE_UNKNOWN;
#ifdef DILIGENT_DEBUG
            dbgShaderType = ShaderTypeFromShaderVisibility(D3D12Param.ShaderVisibility);
            VERIFY(dbgHeapType == HeapTypeFromRangeType(range.RangeType), "Mistmatch between descriptor heap type and descriptor range type");
#endif
            auto  OffsetFromTableStart = range.OffsetInDescriptorsFromTableStart + d;
            auto& Res                  = ResourceCache.GetRootTable(RootInd).GetResource(OffsetFromTableStart, dbgHeapType, dbgShaderType);

            Operation(OffsetFromTableStart, range, Res);
        }
    }
}


template <bool PerformResourceTransitions>
void RootSignature::CommitDescriptorHandlesInternal_SMD(RenderDeviceD3D12Impl*    pRenderDeviceD3D12,
                                                        ShaderResourceCacheD3D12& ResourceCache,
                                                        CommandContext&           Ctx,
                                                        bool                      IsCompute,
                                                        bool                      ValidateStates) const
{
    auto* pd3d12Device = pRenderDeviceD3D12->GetD3D12Device();

    Uint32 NumDynamicCbvSrvUavDescriptors = m_TotalSrvCbvUavSlots[SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC];
    Uint32 NumDynamicSamplerDescriptors   = m_TotalSamplerSlots[SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC];
    VERIFY_EXPR(NumDynamicCbvSrvUavDescriptors > 0 || NumDynamicSamplerDescriptors > 0);

    DescriptorHeapAllocation DynamicCbvSrvUavDescriptors, DynamicSamplerDescriptors;
    if (NumDynamicCbvSrvUavDescriptors > 0)
    {
        DynamicCbvSrvUavDescriptors = Ctx.AllocateDynamicGPUVisibleDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, NumDynamicCbvSrvUavDescriptors);
        DEV_CHECK_ERR(DynamicCbvSrvUavDescriptors.GetDescriptorHeap() != nullptr,
                      "Failed to allocate ", NumDynamicCbvSrvUavDescriptors, " dynamic GPU-visible CBV/SRV/UAV descriptor",
                      (NumDynamicCbvSrvUavDescriptors > 1 ? "s" : ""),
                      ". Consider increasing GPUDescriptorHeapDynamicSize[0] in EngineD3D12CreateInfo "
                      "or optimizing dynamic resource utilization by using static or mutable shader resource variables instead.");
    }

    if (NumDynamicSamplerDescriptors > 0)
    {
        DynamicSamplerDescriptors = Ctx.AllocateDynamicGPUVisibleDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, NumDynamicSamplerDescriptors);
        DEV_CHECK_ERR(DynamicSamplerDescriptors.GetDescriptorHeap() != nullptr,
                      "Failed to allocate ", NumDynamicSamplerDescriptors, " dynamic GPU-visible Sampler descriptor",
                      (NumDynamicSamplerDescriptors > 1 ? "s" : ""),
                      ". Consider using immutable samplers in the Pipeline State Object, increasing GPUDescriptorHeapDynamicSize[1] in "
                      "EngineD3D12CreateInfo, or optimizing dynamic resource utilization by using static or mutable shader resource variables instead.");
    }

    CommandContext::ShaderDescriptorHeaps Heaps(ResourceCache.GetSrvCbvUavDescriptorHeap(), ResourceCache.GetSamplerDescriptorHeap());
    if (Heaps.pSamplerHeap == nullptr)
        Heaps.pSamplerHeap = DynamicSamplerDescriptors.GetDescriptorHeap();

    if (Heaps.pSrvCbvUavHeap == nullptr)
        Heaps.pSrvCbvUavHeap = DynamicCbvSrvUavDescriptors.GetDescriptorHeap();

    if (NumDynamicCbvSrvUavDescriptors > 0)
        VERIFY(DynamicCbvSrvUavDescriptors.GetDescriptorHeap() == Heaps.pSrvCbvUavHeap, "Inconsistent CbvSrvUav descriptor heaps");
    if (NumDynamicSamplerDescriptors > 0)
        VERIFY(DynamicSamplerDescriptors.GetDescriptorHeap() == Heaps.pSamplerHeap, "Inconsistent Sampler descriptor heaps");

    if (Heaps)
        Ctx.SetDescriptorHeaps(Heaps);

    // Offset to the beginning of the current dynamic CBV_SRV_UAV/SAMPLER table from
    // the start of the allocation
    Uint32 DynamicCbvSrvUavTblOffset = 0;
    Uint32 DynamicSamplerTblOffset   = 0;

    m_RootParams.ProcessRootTables(
        [&](Uint32                      RootInd,
            const RootParameter&        RootTable,
            const D3D12_ROOT_PARAMETER& D3D12Param,
            bool                        IsResourceTable,
            D3D12_DESCRIPTOR_HEAP_TYPE  dbgHeapType) //
        {
            D3D12_GPU_DESCRIPTOR_HANDLE RootTableGPUDescriptorHandle;

            bool IsDynamicTable = RootTable.GetShaderVariableType() == SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC;
            if (IsDynamicTable)
            {
                if (IsResourceTable)
                    RootTableGPUDescriptorHandle = DynamicCbvSrvUavDescriptors.GetGpuHandle(DynamicCbvSrvUavTblOffset);
                else
                    RootTableGPUDescriptorHandle = DynamicSamplerDescriptors.GetGpuHandle(DynamicSamplerTblOffset);
            }
            else
            {
                RootTableGPUDescriptorHandle = IsResourceTable ?
                    ResourceCache.GetShaderVisibleTableGPUDescriptorHandle<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>(RootInd) :
                    ResourceCache.GetShaderVisibleTableGPUDescriptorHandle<D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER>(RootInd);
                VERIFY(RootTableGPUDescriptorHandle.ptr != 0, "Unexpected null GPU descriptor handle");
            }

            if (IsCompute)
                Ctx.GetCommandList()->SetComputeRootDescriptorTable(RootInd, RootTableGPUDescriptorHandle);
            else
                Ctx.GetCommandList()->SetGraphicsRootDescriptorTable(RootInd, RootTableGPUDescriptorHandle);

            ProcessCachedTableResources(
                RootInd, D3D12Param, ResourceCache, dbgHeapType,
                [&](UINT                                OffsetFromTableStart,
                    const D3D12_DESCRIPTOR_RANGE&       range,
                    ShaderResourceCacheD3D12::Resource& Res) //
                {
                    if (PerformResourceTransitions)
                    {
                        TransitionResource(Ctx, Res, range.RangeType);
                    }
#ifdef DILIGENT_DEVELOPMENT
                    else if (ValidateStates)
                    {
                        DvpVerifyResourceState(Res, range.RangeType);
                    }
#endif

                    if (IsDynamicTable)
                    {
                        if (IsResourceTable)
                        {
                            VERIFY(DynamicCbvSrvUavTblOffset < NumDynamicCbvSrvUavDescriptors, "Not enough space in the descriptor heap allocation");

                            if (Res.CPUDescriptorHandle.ptr != 0)
                            {
                                pd3d12Device->CopyDescriptorsSimple(1, DynamicCbvSrvUavDescriptors.GetCpuHandle(DynamicCbvSrvUavTblOffset), Res.CPUDescriptorHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                            }
#ifdef DILIGENT_DEVELOPMENT
                            else
                            {
                                LOG_ERROR_MESSAGE("No valid CbvSrvUav descriptor handle found for root parameter ", RootInd, ", descriptor slot ", OffsetFromTableStart);
                            }
#endif

                            ++DynamicCbvSrvUavTblOffset;
                        }
                        else
                        {
                            VERIFY(DynamicSamplerTblOffset < NumDynamicSamplerDescriptors, "Not enough space in the descriptor heap allocation");

                            if (Res.CPUDescriptorHandle.ptr != 0)
                            {
                                pd3d12Device->CopyDescriptorsSimple(1, DynamicSamplerDescriptors.GetCpuHandle(DynamicSamplerTblOffset), Res.CPUDescriptorHandle, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
                            }
#ifdef DILIGENT_DEVELOPMENT
                            else
                            {
                                LOG_ERROR_MESSAGE("No valid sampler descriptor handle found for root parameter ", RootInd, ", descriptor slot ", OffsetFromTableStart);
                            }
#endif

                            ++DynamicSamplerTblOffset;
                        }
                    }
                } //
            );
        } //
    );

    VERIFY_EXPR(DynamicCbvSrvUavTblOffset == NumDynamicCbvSrvUavDescriptors);
    VERIFY_EXPR(DynamicSamplerTblOffset == NumDynamicSamplerDescriptors);
}

template <bool PerformResourceTransitions>
void RootSignature::CommitDescriptorHandlesInternal_SM(RenderDeviceD3D12Impl*    pRenderDeviceD3D12,
                                                       ShaderResourceCacheD3D12& ResourceCache,
                                                       CommandContext&           Ctx,
                                                       bool                      IsCompute,
                                                       bool                      ValidateStates) const
{
    VERIFY_EXPR(m_TotalSrvCbvUavSlots[SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC] == 0 && m_TotalSamplerSlots[SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC] == 0);

    CommandContext::ShaderDescriptorHeaps Heaps(ResourceCache.GetSrvCbvUavDescriptorHeap(), ResourceCache.GetSamplerDescriptorHeap());
    if (Heaps)
        Ctx.SetDescriptorHeaps(Heaps);

    m_RootParams.ProcessRootTables(
        [&](Uint32 RootInd, const RootParameter& RootTable, const D3D12_ROOT_PARAMETER& D3D12Param, bool IsResourceTable, D3D12_DESCRIPTOR_HEAP_TYPE dbgHeapType) {
            VERIFY(RootTable.GetShaderVariableType() != SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC, "Unexpected dynamic resource");

            D3D12_GPU_DESCRIPTOR_HANDLE RootTableGPUDescriptorHandle = IsResourceTable ?
                ResourceCache.GetShaderVisibleTableGPUDescriptorHandle<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>(RootInd) :
                ResourceCache.GetShaderVisibleTableGPUDescriptorHandle<D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER>(RootInd);
            VERIFY(RootTableGPUDescriptorHandle.ptr != 0, "Unexpected null GPU descriptor handle");

            if (IsCompute)
                Ctx.GetCommandList()->SetComputeRootDescriptorTable(RootInd, RootTableGPUDescriptorHandle);
            else
                Ctx.GetCommandList()->SetGraphicsRootDescriptorTable(RootInd, RootTableGPUDescriptorHandle);

            if (PerformResourceTransitions)
            {
                ProcessCachedTableResources(
                    RootInd, D3D12Param, ResourceCache, dbgHeapType,
                    [&](UINT                                OffsetFromTableStart,
                        const D3D12_DESCRIPTOR_RANGE&       range,
                        ShaderResourceCacheD3D12::Resource& Res) //
                    {
                        TransitionResource(Ctx, Res, range.RangeType);
                    } //
                );
            }
#ifdef DILIGENT_DEVELOPMENT
            else if (ValidateStates)
            {
                ProcessCachedTableResources(
                    RootInd, D3D12Param, ResourceCache, dbgHeapType,
                    [&](UINT                                      OffsetFromTableStart,
                        const D3D12_DESCRIPTOR_RANGE&             range,
                        const ShaderResourceCacheD3D12::Resource& Res) //
                    {
                        DvpVerifyResourceState(Res, range.RangeType);
                    } //
                );
            }
#endif
        } //
    );
}


void RootSignature::TransitionResources(ShaderResourceCacheD3D12& ResourceCache,
                                        CommandContext&           Ctx) const
{
    m_RootParams.ProcessRootTables(
        [&](Uint32                      RootInd,
            const RootParameter&        RootTable,
            const D3D12_ROOT_PARAMETER& D3D12Param,
            bool                        IsResourceTable,
            D3D12_DESCRIPTOR_HEAP_TYPE  dbgHeapType) //
        {
            ProcessCachedTableResources(
                RootInd, D3D12Param, ResourceCache, dbgHeapType,
                [&](UINT                                OffsetFromTableStart,
                    const D3D12_DESCRIPTOR_RANGE&       range,
                    ShaderResourceCacheD3D12::Resource& Res) //
                {
                    TransitionResource(Ctx, Res, range.RangeType);
                } //
            );
        } //
    );
}

} // namespace Diligent
