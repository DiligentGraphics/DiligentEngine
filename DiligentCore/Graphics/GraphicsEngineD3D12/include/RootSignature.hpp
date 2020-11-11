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
/// Declaration of Diligent::RootSignature class
#include <array>
#include "ShaderResourceLayoutD3D12.hpp"
#include "BufferD3D12Impl.hpp"

namespace Diligent
{

SHADER_TYPE                ShaderTypeFromShaderVisibility(D3D12_SHADER_VISIBILITY ShaderVisibility);
D3D12_SHADER_VISIBILITY    GetShaderVisibility(SHADER_TYPE ShaderType);
D3D12_DESCRIPTOR_HEAP_TYPE dbgHeapTypeFromRangeType(D3D12_DESCRIPTOR_RANGE_TYPE RangeType);

class RootParameter
{
public:
    RootParameter(D3D12_ROOT_PARAMETER_TYPE     ParameterType,
                  Uint32                        RootIndex,
                  UINT                          Register,
                  UINT                          RegisterSpace,
                  D3D12_SHADER_VISIBILITY       Visibility,
                  SHADER_RESOURCE_VARIABLE_TYPE VarType) noexcept :
        // clang-format off
        m_RootIndex    {RootIndex},
        m_ShaderVarType{VarType  }
    // clang-format on
    {
        VERIFY(ParameterType == D3D12_ROOT_PARAMETER_TYPE_CBV || ParameterType == D3D12_ROOT_PARAMETER_TYPE_SRV || ParameterType == D3D12_ROOT_PARAMETER_TYPE_UAV, "Unexpected parameter type - verify argument list");
        m_RootParam.ParameterType             = ParameterType;
        m_RootParam.ShaderVisibility          = Visibility;
        m_RootParam.Descriptor.ShaderRegister = Register;
        m_RootParam.Descriptor.RegisterSpace  = RegisterSpace;
    }

    RootParameter(D3D12_ROOT_PARAMETER_TYPE     ParameterType,
                  Uint32                        RootIndex,
                  UINT                          Register,
                  UINT                          RegisterSpace,
                  UINT                          NumDwords,
                  D3D12_SHADER_VISIBILITY       Visibility,
                  SHADER_RESOURCE_VARIABLE_TYPE VarType) noexcept :
        // clang-format off
        m_RootIndex    {RootIndex},
        m_ShaderVarType{VarType  }
    // clang-format on
    {
        VERIFY(ParameterType == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS, "Unexpected parameter type - verify argument list");
        m_RootParam.ParameterType            = ParameterType;
        m_RootParam.ShaderVisibility         = Visibility;
        m_RootParam.Constants.Num32BitValues = NumDwords;
        m_RootParam.Constants.ShaderRegister = Register;
        m_RootParam.Constants.RegisterSpace  = RegisterSpace;
    }

    RootParameter(D3D12_ROOT_PARAMETER_TYPE     ParameterType,
                  Uint32                        RootIndex,
                  UINT                          NumRanges,
                  D3D12_DESCRIPTOR_RANGE*       pRanges,
                  D3D12_SHADER_VISIBILITY       Visibility,
                  SHADER_RESOURCE_VARIABLE_TYPE VarType) noexcept :
        // clang-format off
        m_RootIndex    {RootIndex},
        m_ShaderVarType{VarType  }
    // clang-format on
    {
        VERIFY(ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, "Unexpected parameter type - verify argument list");
        VERIFY_EXPR(pRanges != nullptr);
        m_RootParam.ParameterType                       = ParameterType;
        m_RootParam.ShaderVisibility                    = Visibility;
        m_RootParam.DescriptorTable.NumDescriptorRanges = NumRanges;
        m_RootParam.DescriptorTable.pDescriptorRanges   = pRanges;
#ifdef DILIGENT_DEBUG
        for (Uint32 r = 0; r < NumRanges; ++r)
            pRanges[r].RangeType = static_cast<D3D12_DESCRIPTOR_RANGE_TYPE>(-1);
#endif
    }

    RootParameter(const RootParameter& RP) noexcept :
        // clang-format off
        m_RootParam          {RP.m_RootParam          },
        m_DescriptorTableSize{RP.m_DescriptorTableSize},
        m_ShaderVarType      {RP.m_ShaderVarType      },
        m_RootIndex          {RP.m_RootIndex          }
    // clang-format on
    {
        VERIFY(m_RootParam.ParameterType != D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, "Use another constructor to copy descriptor table");
    }

    RootParameter(const RootParameter&    RP,
                  UINT                    NumRanges,
                  D3D12_DESCRIPTOR_RANGE* pRanges) noexcept :
        // clang-format off
        m_RootParam          {RP.m_RootParam          },
        m_DescriptorTableSize{RP.m_DescriptorTableSize},
        m_ShaderVarType      {RP.m_ShaderVarType      },
        m_RootIndex          {RP.m_RootIndex          }
    // clang-format on
    {
        VERIFY(m_RootParam.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, "Root parameter is expected to be a descriptor table");
        VERIFY(NumRanges >= m_RootParam.DescriptorTable.NumDescriptorRanges, "New table must be larger than source one");
        auto& DstTbl               = m_RootParam.DescriptorTable;
        DstTbl.NumDescriptorRanges = NumRanges;
        DstTbl.pDescriptorRanges   = pRanges;
        const auto& SrcTbl         = RP.m_RootParam.DescriptorTable;
        memcpy(pRanges, SrcTbl.pDescriptorRanges, SrcTbl.NumDescriptorRanges * sizeof(D3D12_DESCRIPTOR_RANGE));
#ifdef DILIGENT_DEBUG
        {
            Uint32 dbgTableSize = 0;
            for (Uint32 r = 0; r < SrcTbl.NumDescriptorRanges; ++r)
            {
                const auto& Range = SrcTbl.pDescriptorRanges[r];
                dbgTableSize      = std::max(dbgTableSize, Range.OffsetInDescriptorsFromTableStart + Range.NumDescriptors);
            }
            VERIFY(dbgTableSize == m_DescriptorTableSize, "Incorrect descriptor table size");

            for (Uint32 r = SrcTbl.NumDescriptorRanges; r < DstTbl.NumDescriptorRanges; ++r)
                pRanges[r].RangeType = static_cast<D3D12_DESCRIPTOR_RANGE_TYPE>(-1);
        }
#endif
    }

    RootParameter& operator=(const RootParameter&) = delete;
    RootParameter& operator=(RootParameter&&) = delete;

    void SetDescriptorRange(UINT                        RangeIndex,
                            D3D12_DESCRIPTOR_RANGE_TYPE Type,
                            UINT                        Register,
                            UINT                        Count,
                            UINT                        Space                = 0,
                            UINT                        OffsetFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND)
    {
        VERIFY(m_RootParam.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, "Incorrect parameter table: descriptor table is expected");
        auto& Tbl = m_RootParam.DescriptorTable;
        VERIFY(RangeIndex < Tbl.NumDescriptorRanges, "Invalid descriptor range index");
        D3D12_DESCRIPTOR_RANGE& range = const_cast<D3D12_DESCRIPTOR_RANGE&>(Tbl.pDescriptorRanges[RangeIndex]);
        VERIFY(range.RangeType == static_cast<D3D12_DESCRIPTOR_RANGE_TYPE>(-1), "Descriptor range has already been initialized. m_DescriptorTableSize may be updated incorrectly");
        range.RangeType                         = Type;
        range.NumDescriptors                    = Count;
        range.BaseShaderRegister                = Register;
        range.RegisterSpace                     = Space;
        range.OffsetInDescriptorsFromTableStart = OffsetFromTableStart;
        m_DescriptorTableSize                   = std::max(m_DescriptorTableSize, OffsetFromTableStart + Count);
    }

    SHADER_RESOURCE_VARIABLE_TYPE GetShaderVariableType() const { return m_ShaderVarType; }

    Uint32 GetDescriptorTableSize() const
    {
        VERIFY(m_RootParam.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, "Incorrect parameter table: descriptor table is expected");
        return m_DescriptorTableSize;
    }

    D3D12_SHADER_VISIBILITY   GetShaderVisibility() const { return m_RootParam.ShaderVisibility; }
    D3D12_ROOT_PARAMETER_TYPE GetParameterType() const { return m_RootParam.ParameterType; }

    Uint32 GetRootIndex() const { return m_RootIndex; }

    operator const D3D12_ROOT_PARAMETER&() const { return m_RootParam; }

    bool operator==(const RootParameter& rhs) const
    {
        if (m_ShaderVarType != rhs.m_ShaderVarType ||
            m_DescriptorTableSize != rhs.m_DescriptorTableSize ||
            m_RootIndex != rhs.m_RootIndex)
            return false;

        if (m_RootParam.ParameterType != rhs.m_RootParam.ParameterType ||
            m_RootParam.ShaderVisibility != rhs.m_RootParam.ShaderVisibility)
            return false;

        switch (m_RootParam.ParameterType)
        {
            case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
            {
                const auto& tbl0 = m_RootParam.DescriptorTable;
                const auto& tbl1 = rhs.m_RootParam.DescriptorTable;
                if (tbl0.NumDescriptorRanges != tbl1.NumDescriptorRanges)
                    return false;
                for (UINT r = 0; r < tbl0.NumDescriptorRanges; ++r)
                {
                    const auto& rng0 = tbl0.pDescriptorRanges[r];
                    const auto& rng1 = tbl1.pDescriptorRanges[r];
                    if (memcmp(&rng0, &rng1, sizeof(rng0)) != 0)
                        return false;
                }
            }
            break;

            case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
            {
                const auto& cnst0 = m_RootParam.Constants;
                const auto& cnst1 = rhs.m_RootParam.Constants;
                if (memcmp(&cnst0, &cnst1, sizeof(cnst0)) != 0)
                    return false;
            }
            break;

            case D3D12_ROOT_PARAMETER_TYPE_CBV:
            case D3D12_ROOT_PARAMETER_TYPE_SRV:
            case D3D12_ROOT_PARAMETER_TYPE_UAV:
            {
                const auto& dscr0 = m_RootParam.Descriptor;
                const auto& dscr1 = rhs.m_RootParam.Descriptor;
                if (memcmp(&dscr0, &dscr1, sizeof(dscr0)) != 0)
                    return false;
            }
            break;

            default: UNEXPECTED("Unexpected root parameter type");
        }

        return true;
    }

    bool operator!=(const RootParameter& rhs) const
    {
        return !(*this == rhs);
    }

    size_t GetHash() const
    {
        size_t hash = ComputeHash(m_ShaderVarType, m_DescriptorTableSize, m_RootIndex);
        HashCombine(hash, m_RootParam.ParameterType, m_RootParam.ShaderVisibility);

        switch (m_RootParam.ParameterType)
        {
            case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
            {
                const auto& tbl = m_RootParam.DescriptorTable;
                HashCombine(hash, tbl.NumDescriptorRanges);
                for (UINT r = 0; r < tbl.NumDescriptorRanges; ++r)
                {
                    const auto& rng = tbl.pDescriptorRanges[r];
                    HashCombine(hash, rng.BaseShaderRegister, rng.NumDescriptors, rng.OffsetInDescriptorsFromTableStart, rng.RangeType, rng.RegisterSpace);
                }
            }
            break;

            case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
            {
                const auto& cnst = m_RootParam.Constants;
                HashCombine(hash, cnst.Num32BitValues, cnst.RegisterSpace, cnst.ShaderRegister);
            }
            break;

            case D3D12_ROOT_PARAMETER_TYPE_CBV:
            case D3D12_ROOT_PARAMETER_TYPE_SRV:
            case D3D12_ROOT_PARAMETER_TYPE_UAV:
            {
                const auto& dscr = m_RootParam.Descriptor;
                HashCombine(hash, dscr.RegisterSpace, dscr.ShaderRegister);
            }
            break;

            default: UNEXPECTED("Unexpected root parameter type");
        }

        return hash;
    }

private:
    SHADER_RESOURCE_VARIABLE_TYPE m_ShaderVarType       = static_cast<SHADER_RESOURCE_VARIABLE_TYPE>(-1);
    D3D12_ROOT_PARAMETER          m_RootParam           = {};
    Uint32                        m_DescriptorTableSize = 0;
    Uint32                        m_RootIndex           = static_cast<Uint32>(-1);
};



/// Implementation of the Diligent::RootSignature class
class RootSignature
{
public:
    RootSignature();

    void AllocateImmutableSamplers(const PipelineResourceLayoutDesc& ResourceLayout);

    void Finalize(ID3D12Device* pd3d12Device);

    ID3D12RootSignature* GetD3D12RootSignature() const { return m_pd3d12RootSignature; }

    size_t GetResourceCacheRequiredMemSize() const;

    void InitResourceCache(class RenderDeviceD3D12Impl* pDeviceD3D12Impl, class ShaderResourceCacheD3D12& ResourceCache, IMemoryAllocator& CacheMemAllocator) const;

    void InitImmutableSampler(SHADER_TYPE                     ShaderType,
                              const char*                     SamplerName,
                              const char*                     SamplerSuffix,
                              const D3DShaderResourceAttribs& ShaderResAttribs);

    void AllocateResourceSlot(SHADER_TYPE                     ShaderType,
                              PIPELINE_TYPE                   PipelineType,
                              const D3DShaderResourceAttribs& ShaderResAttribs,
                              SHADER_RESOURCE_VARIABLE_TYPE   VariableType,
                              D3D12_DESCRIPTOR_RANGE_TYPE     RangeType,
                              Uint32&                         RootIndex,
                              Uint32&                         OffsetFromTableStart);

    // This method should be thread-safe as it does not modify any object state
    void (RootSignature::*CommitDescriptorHandles)(class RenderDeviceD3D12Impl* pRenderDeviceD3D12,
                                                   ShaderResourceCacheD3D12&    ResourceCache,
                                                   class CommandContext&        Ctx,
                                                   bool                         IsCompute,
                                                   bool                         ValidateStates) const = nullptr;

    void (RootSignature::*TransitionAndCommitDescriptorHandles)(class RenderDeviceD3D12Impl* pRenderDeviceD3D12,
                                                                ShaderResourceCacheD3D12&    ResourceCache,
                                                                class CommandContext&        Ctx,
                                                                bool                         IsCompute,
                                                                bool                         ValidateStates) const = nullptr;

    void TransitionResources(ShaderResourceCacheD3D12& ResourceCache,
                             class CommandContext&     Ctx) const;

    __forceinline void CommitRootViews(ShaderResourceCacheD3D12&     ResourceCache,
                                       class CommandContext&         CmdCtx,
                                       bool                          IsCompute,
                                       Uint32                        DeviceCtxId,
                                       class DeviceContextD3D12Impl* pDeviceCtx,
                                       bool                          CommitViews,
                                       bool                          ProcessDynamicBuffers,
                                       bool                          ProcessNonDynamicBuffers,
                                       bool                          TransitionStates,
                                       bool                          ValidateStates) const;

    Uint32 GetTotalSrvCbvUavSlots(SHADER_RESOURCE_VARIABLE_TYPE VarType) const
    {
        return m_TotalSrvCbvUavSlots[VarType];
    }
    Uint32 GetTotalSamplerSlots(SHADER_RESOURCE_VARIABLE_TYPE VarType) const
    {
        return m_TotalSamplerSlots[VarType];
    }
    Uint32 GetTotalRootViews(SHADER_RESOURCE_VARIABLE_TYPE VarType) const
    {
        return m_TotalRootViews[VarType];
    }

    bool IsSameAs(const RootSignature& RS) const
    {
        return m_RootParams == RS.m_RootParams;
    }
    size_t GetHash() const
    {
        return m_RootParams.GetHash();
    }

private:
#ifdef DILIGENT_DEBUG
    void dbgVerifyRootParameters() const;
#endif

#ifdef DILIGENT_DEVELOPMENT
    static void DvpVerifyResourceState(const ShaderResourceCacheD3D12::Resource& Res,
                                       D3D12_DESCRIPTOR_RANGE_TYPE               RangeType);
#endif

    std::vector<Uint32, STDAllocatorRawMem<Uint32>> GetCacheTableSizes() const;

    std::array<Uint32, SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES> m_TotalSrvCbvUavSlots = {};
    std::array<Uint32, SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES> m_TotalSamplerSlots   = {};
    std::array<Uint32, SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES> m_TotalRootViews      = {};

    CComPtr<ID3D12RootSignature> m_pd3d12RootSignature;

    class RootParamsManager
    {
    public:
        RootParamsManager(IMemoryAllocator& MemAllocator);

        // clang-format off
        RootParamsManager           (const RootParamsManager&) = delete;
        RootParamsManager& operator=(const RootParamsManager&) = delete;
        RootParamsManager           (RootParamsManager&&)      = delete;
        RootParamsManager& operator=(RootParamsManager&&)      = delete;
        // clang-format on

        Uint32 GetNumRootTables() const { return m_NumRootTables; }
        Uint32 GetNumRootViews() const { return m_NumRootViews; }

        const RootParameter& GetRootTable(Uint32 TableInd) const
        {
            VERIFY_EXPR(TableInd < m_NumRootTables);
            return m_pRootTables[TableInd];
        }

        RootParameter& GetRootTable(Uint32 TableInd)
        {
            VERIFY_EXPR(TableInd < m_NumRootTables);
            return m_pRootTables[TableInd];
        }

        const RootParameter& GetRootView(Uint32 ViewInd) const
        {
            VERIFY_EXPR(ViewInd < m_NumRootViews);
            return m_pRootViews[ViewInd];
        }

        RootParameter& GetRootView(Uint32 ViewInd)
        {
            VERIFY_EXPR(ViewInd < m_NumRootViews);
            return m_pRootViews[ViewInd];
        }

        void AddRootView(D3D12_ROOT_PARAMETER_TYPE     ParameterType,
                         Uint32                        RootIndex,
                         UINT                          Register,
                         D3D12_SHADER_VISIBILITY       Visibility,
                         SHADER_RESOURCE_VARIABLE_TYPE VarType);

        void AddRootTable(Uint32                        RootIndex,
                          D3D12_SHADER_VISIBILITY       Visibility,
                          SHADER_RESOURCE_VARIABLE_TYPE VarType,
                          Uint32                        NumRangesInNewTable = 1);

        void AddDescriptorRanges(Uint32 RootTableInd, Uint32 NumExtraRanges = 1);

        template <class TOperation>
        void ProcessRootTables(TOperation) const;

        bool   operator==(const RootParamsManager& RootParams) const;
        size_t GetHash() const;

    private:
        size_t GetRequiredMemorySize(Uint32 NumExtraRootTables,
                                     Uint32 NumExtraRootViews,
                                     Uint32 NumExtraDescriptorRanges) const;

        D3D12_DESCRIPTOR_RANGE* Extend(Uint32 NumExtraRootTables,
                                       Uint32 NumExtraRootViews,
                                       Uint32 NumExtraDescriptorRanges,
                                       Uint32 RootTableToAddRanges = static_cast<Uint32>(-1));

        IMemoryAllocator&                                         m_MemAllocator;
        std::unique_ptr<void, STDDeleter<void, IMemoryAllocator>> m_pMemory;
        Uint32                                                    m_NumRootTables         = 0;
        Uint32                                                    m_NumRootViews          = 0;
        Uint32                                                    m_TotalDescriptorRanges = 0;
        RootParameter*                                            m_pRootTables           = nullptr;
        RootParameter*                                            m_pRootViews            = nullptr;
    };

    static constexpr Uint8 InvalidRootTableIndex = static_cast<Uint8>(-1);

    // The array below contains array index of a CBV/SRV/UAV root table
    // in m_RootParams (NOT the Root Index!), for every variable type
    // (static, mutable, dynamic) and every shader type,
    // or -1, if the table is not yet assigned to the combination
    std::array<Uint8, SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES* MAX_SHADERS_IN_PIPELINE> m_SrvCbvUavRootTablesMap = {};
    // This array contains the same data for Sampler root table
    std::array<Uint8, SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES* MAX_SHADERS_IN_PIPELINE> m_SamplerRootTablesMap = {};

    RootParamsManager m_RootParams;

    struct ImmutableSamplerAttribs
    {
        ImmutableSamplerDesc    SamplerDesc;
        UINT                    ShaderRegister   = static_cast<UINT>(-1);
        UINT                    ArraySize        = 0;
        UINT                    RegisterSpace    = 0;
        D3D12_SHADER_VISIBILITY ShaderVisibility = static_cast<D3D12_SHADER_VISIBILITY>(-1);

        ImmutableSamplerAttribs() noexcept {}
        ImmutableSamplerAttribs(const ImmutableSamplerDesc& SamDesc, D3D12_SHADER_VISIBILITY Visibility) noexcept :
            SamplerDesc(SamDesc),
            ShaderVisibility(Visibility)
        {}
    };
    // Note: sizeof(m_ImmutableSamplers) == 56 (MS compiler, release x64)
    std::vector<ImmutableSamplerAttribs, STDAllocatorRawMem<ImmutableSamplerAttribs>> m_ImmutableSamplers;

    IMemoryAllocator& m_MemAllocator;

    // Commits descriptor handles for static and mutable variables
    template <bool PerformResourceTransitions>
    void CommitDescriptorHandlesInternal_SM(class RenderDeviceD3D12Impl* pRenderDeviceD3D12,
                                            ShaderResourceCacheD3D12&    ResourceCache,
                                            class CommandContext&        Ctx,
                                            bool                         IsCompute,
                                            bool                         ValidateStates) const;
    template <bool PerformResourceTransitions>
    // Commits descriptor handles for static, mutable, and dynamic variables
    void CommitDescriptorHandlesInternal_SMD(class RenderDeviceD3D12Impl* pRenderDeviceD3D12,
                                             ShaderResourceCacheD3D12&    ResourceCache,
                                             class CommandContext&        Ctx,
                                             bool                         IsCompute,
                                             bool                         ValidateStates) const;
};

void RootSignature::CommitRootViews(ShaderResourceCacheD3D12& ResourceCache,
                                    CommandContext&           CmdCtx,
                                    bool                      IsCompute,
                                    Uint32                    DeviceCtxId,
                                    DeviceContextD3D12Impl*   pDeviceCtx,
                                    bool                      CommitViews,
                                    bool                      ProcessDynamicBuffers,
                                    bool                      ProcessNonDynamicBuffers,
                                    bool                      TransitionStates,
                                    bool                      ValidateStates) const
{
    for (Uint32 rv = 0; rv < m_RootParams.GetNumRootViews(); ++rv)
    {
        auto& RootView = m_RootParams.GetRootView(rv);
        auto  RootInd  = RootView.GetRootIndex();

        SHADER_TYPE dbgShaderType = SHADER_TYPE_UNKNOWN;
#ifdef DILIGENT_DEBUG
        {
            auto& Param = static_cast<const D3D12_ROOT_PARAMETER&>(RootView);
            VERIFY_EXPR(Param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_CBV);
            dbgShaderType = ShaderTypeFromShaderVisibility(Param.ShaderVisibility);
        }
#endif

        auto& Res = ResourceCache.GetRootTable(RootInd).GetResource(0, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, dbgShaderType);
        if (auto* pBuffToTransition = Res.pObject.RawPtr<BufferD3D12Impl>())
        {
            bool IsDynamic = pBuffToTransition->GetDesc().Usage == USAGE_DYNAMIC;
            if (IsDynamic && ProcessDynamicBuffers || !IsDynamic && ProcessNonDynamicBuffers)
            {
                if (IsDynamic)
                {
#ifdef DILIGENT_DEBUG
                    if (pBuffToTransition->IsInKnownState())
                    {
                        VERIFY(pBuffToTransition->CheckState(RESOURCE_STATE_CONSTANT_BUFFER),
                               "Dynamic buffers must always have RESOURCE_STATE_CONSTANT_BUFFER state flag set");
                    }
#endif
                }
                else
                {
                    if (TransitionStates)
                    {
                        if (pBuffToTransition->IsInKnownState() && !pBuffToTransition->CheckState(RESOURCE_STATE_CONSTANT_BUFFER))
                        {
                            CmdCtx.TransitionResource(pBuffToTransition, RESOURCE_STATE_CONSTANT_BUFFER);
                        }
                    }
#ifdef DILIGENT_DEVELOPMENT
                    else if (ValidateStates)
                    {

                        DvpVerifyResourceState(Res, D3D12_DESCRIPTOR_RANGE_TYPE_CBV);
                    }
#endif
                }

                if (CommitViews)
                {
                    D3D12_GPU_VIRTUAL_ADDRESS CBVAddress = pBuffToTransition->GetGPUAddress(DeviceCtxId, pDeviceCtx);
                    if (IsCompute)
                        CmdCtx.GetCommandList()->SetComputeRootConstantBufferView(RootInd, CBVAddress);
                    else
                        CmdCtx.GetCommandList()->SetGraphicsRootConstantBufferView(RootInd, CBVAddress);
                }
            }
        }
    }
}

} // namespace Diligent
