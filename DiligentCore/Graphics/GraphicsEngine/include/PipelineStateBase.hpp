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
/// Implementation of the Diligent::PipelineStateBase template class

#include <array>
#include <vector>

#include "PipelineState.h"
#include "DeviceObjectBase.hpp"
#include "STDAllocator.hpp"
#include "EngineMemory.h"
#include "GraphicsAccessories.hpp"
#include "LinearAllocator.hpp"

namespace Diligent
{

void ValidateGraphicsPipelineCreateInfo(const GraphicsPipelineStateCreateInfo& CreateInfo) noexcept(false);
void ValidateComputePipelineCreateInfo(const ComputePipelineStateCreateInfo& CreateInfo) noexcept(false);

void CorrectGraphicsPipelineDesc(GraphicsPipelineDesc& GraphicsPipeline) noexcept;

/// Template class implementing base functionality for a pipeline state object.

/// \tparam BaseInterface - base interface that this class will inheret
///                         (Diligent::IPipelineStateD3D11, Diligent::IPipelineStateD3D12,
///                          Diligent::IPipelineStateGL or Diligent::IPipelineStateVk).
/// \tparam RenderDeviceImplType - type of the render device implementation
///                                (Diligent::RenderDeviceD3D11Impl, Diligent::RenderDeviceD3D12Impl,
///                                 Diligent::RenderDeviceGLImpl, or Diligent::RenderDeviceVkImpl)
template <class BaseInterface, class RenderDeviceImplType>
class PipelineStateBase : public DeviceObjectBase<BaseInterface, RenderDeviceImplType, PipelineStateDesc>
{
public:
    using TDeviceObjectBase = DeviceObjectBase<BaseInterface, RenderDeviceImplType, PipelineStateDesc>;

    /// \param pRefCounters      - Reference counters object that controls the lifetime of this PSO
    /// \param pDevice           - Pointer to the device.
    /// \param CreateInfo        - Pipeline state create info.
    /// \param bIsDeviceInternal - Flag indicating if the pipeline state is an internal device object and
    ///							   must not keep a strong reference to the device.
    PipelineStateBase(IReferenceCounters*      pRefCounters,
                      RenderDeviceImplType*    pDevice,
                      const PipelineStateDesc& PSODesc,
                      bool                     bIsDeviceInternal = false) :
        TDeviceObjectBase{pRefCounters, pDevice, PSODesc, bIsDeviceInternal}
    {
        Uint64 DeviceQueuesMask = pDevice->GetCommandQueueMask();
        DEV_CHECK_ERR((this->m_Desc.CommandQueueMask & DeviceQueuesMask) != 0,
                      "No bits in the command queue mask (0x", std::hex, this->m_Desc.CommandQueueMask,
                      ") correspond to one of ", pDevice->GetCommandQueueCount(), " available device command queues");
        this->m_Desc.CommandQueueMask &= DeviceQueuesMask;
    }

    /// \param pRefCounters       - Reference counters object that controls the lifetime of this PSO
    /// \param pDevice            - Pointer to the device.
    /// \param GraphicsPipelineCI - Graphics pipeline create information.
    /// \param bIsDeviceInternal  - Flag indicating if the pipeline state is an internal device object and
    ///							    must not keep a strong reference to the device.
    PipelineStateBase(IReferenceCounters*                    pRefCounters,
                      RenderDeviceImplType*                  pDevice,
                      const GraphicsPipelineStateCreateInfo& GraphicsPipelineCI,
                      bool                                   bIsDeviceInternal = false) :
        PipelineStateBase{pRefCounters, pDevice, GraphicsPipelineCI.PSODesc, bIsDeviceInternal}
    {
        ValidateGraphicsPipelineCreateInfo(GraphicsPipelineCI);
    }

    /// \param pRefCounters       - Reference counters object that controls the lifetime of this PSO
    /// \param pDevice            - Pointer to the device.
    /// \param ComputePipelineCI  - Compute pipeline create information.
    /// \param bIsDeviceInternal  - Flag indicating if the pipeline state is an internal device object and
    ///							    must not keep a strong reference to the device.
    PipelineStateBase(IReferenceCounters*                   pRefCounters,
                      RenderDeviceImplType*                 pDevice,
                      const ComputePipelineStateCreateInfo& ComputePipelineCI,
                      bool                                  bIsDeviceInternal = false) :
        PipelineStateBase{pRefCounters, pDevice, ComputePipelineCI.PSODesc, bIsDeviceInternal}
    {
        ValidateComputePipelineCreateInfo(ComputePipelineCI);
    }


    ~PipelineStateBase()
    {
        /*
        /// \note Destructor cannot directly remove the object from the registry as this may cause a  
        ///       deadlock at the point where StateObjectsRegistry::Find() locks the weak pointer: if we
        ///       are in dtor, the object is locked by Diligent::RefCountedObject::Release() and 
        ///       StateObjectsRegistry::Find() will wait for that lock to be released.
        ///       A the same time this thread will be waiting for the other thread to unlock the registry.\n
        ///       Thus destructor only notifies the registry that there is a deleted object.
        ///       The reference to the object will be removed later.
        auto &PipelineStateRegistry = static_cast<TRenderDeviceBase*>(this->GetDevice())->GetBSRegistry();
        auto &RasterizerStateRegistry = static_cast<TRenderDeviceBase*>(this->GetDevice())->GetRSRegistry();
        auto &DSSRegistry = static_cast<TRenderDeviceBase*>(this->GetDevice())->GetDSSRegistry();
        // StateObjectsRegistry::ReportDeletedObject() does not lock the registry, but only 
        // atomically increments the outstanding deleted objects counter.
        PipelineStateRegistry.ReportDeletedObject();
        RasterizerStateRegistry.ReportDeletedObject();
        DSSRegistry.ReportDeletedObject();
        */
    }

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_PipelineState, TDeviceObjectBase)

    Uint32 GetBufferStride(Uint32 BufferSlot) const
    {
        return BufferSlot < m_BufferSlotsUsed ? m_pStrides[BufferSlot] : 0;
    }

    Uint32 GetNumBufferSlotsUsed() const
    {
        return m_BufferSlotsUsed;
    }

    SHADER_TYPE GetShaderStageType(Uint32 Stage) const { return m_ShaderStageTypes[Stage]; }
    Uint32      GetNumShaderStages() const { return m_NumShaderStages; }

    // This function only compares shader resource layout hashes, so
    // it can potentially give false negatives
    bool IsIncompatibleWith(const IPipelineState* pPSO) const
    {
        return m_ShaderResourceLayoutHash != ValidatedCast<const PipelineStateBase>(pPSO)->m_ShaderResourceLayoutHash;
    }

    virtual const GraphicsPipelineDesc& DILIGENT_CALL_TYPE GetGraphicsPipelineDesc() const override final
    {
        VERIFY_EXPR(this->m_Desc.IsAnyGraphicsPipeline());
        VERIFY_EXPR(m_pGraphicsPipelineDesc != nullptr);
        return *m_pGraphicsPipelineDesc;
    }

protected:
    Int8 GetStaticVariableCountHelper(SHADER_TYPE ShaderType, const std::array<Int8, MAX_SHADERS_IN_PIPELINE>& ResourceLayoutIndex) const
    {
        if (!IsConsistentShaderType(ShaderType, this->m_Desc.PipelineType))
        {
            LOG_WARNING_MESSAGE("Unable to get the number of static variables in shader stage ", GetShaderTypeLiteralName(ShaderType),
                                " as the stage is invalid for ", GetPipelineTypeString(this->m_Desc.PipelineType), " pipeline '", this->m_Desc.Name, "'");
            return -1;
        }

        const auto ShaderTypeInd = GetShaderTypePipelineIndex(ShaderType, this->m_Desc.PipelineType);
        const auto LayoutInd     = ResourceLayoutIndex[ShaderTypeInd];
        if (LayoutInd < 0)
        {
            LOG_WARNING_MESSAGE("Unable to get the number of static variables in shader stage ", GetShaderTypeLiteralName(ShaderType),
                                " as the stage is inactive in PSO '", this->m_Desc.Name, "'");
        }

        return LayoutInd;
    }

    Int8 GetStaticVariableByNameHelper(SHADER_TYPE ShaderType, const Char* Name, const std::array<Int8, MAX_SHADERS_IN_PIPELINE>& ResourceLayoutIndex) const
    {
        if (!IsConsistentShaderType(ShaderType, this->m_Desc.PipelineType))
        {
            LOG_WARNING_MESSAGE("Unable to find static variable '", Name, "' in shader stage ", GetShaderTypeLiteralName(ShaderType),
                                " as the stage is invalid for ", GetPipelineTypeString(this->m_Desc.PipelineType), " pipeline '", this->m_Desc.Name, "'");
            return -1;
        }

        const auto ShaderTypeInd = GetShaderTypePipelineIndex(ShaderType, this->m_Desc.PipelineType);
        const auto LayoutInd     = ResourceLayoutIndex[ShaderTypeInd];
        if (LayoutInd < 0)
        {
            LOG_WARNING_MESSAGE("Unable to find static variable '", Name, "' in shader stage ", GetShaderTypeLiteralName(ShaderType),
                                " as the stage is inactive in PSO '", this->m_Desc.Name, "'");
        }

        return LayoutInd;
    }

    Int8 GetStaticVariableByIndexHelper(SHADER_TYPE ShaderType, Uint32 Index, const std::array<Int8, MAX_SHADERS_IN_PIPELINE>& ResourceLayoutIndex) const
    {
        if (!IsConsistentShaderType(ShaderType, this->m_Desc.PipelineType))
        {
            LOG_WARNING_MESSAGE("Unable to get static variable at index ", Index, " in shader stage ", GetShaderTypeLiteralName(ShaderType),
                                " as the stage is invalid for ", GetPipelineTypeString(this->m_Desc.PipelineType), " pipeline '", this->m_Desc.Name, "'");
            return -1;
        }

        const auto ShaderTypeInd = GetShaderTypePipelineIndex(ShaderType, this->m_Desc.PipelineType);
        const auto LayoutInd     = ResourceLayoutIndex[ShaderTypeInd];
        if (LayoutInd < 0)
        {
            LOG_WARNING_MESSAGE("Unable to get static variable at index ", Index, " in shader stage ", GetShaderTypeLiteralName(ShaderType),
                                " as the stage is inactive in PSO '", this->m_Desc.Name, "'");
        }

        return LayoutInd;
    }


    void ReserveSpaceForPipelineDesc(const GraphicsPipelineStateCreateInfo& CreateInfo,
                                     LinearAllocator&                       MemPool) noexcept
    {
        MemPool.AddSpace<GraphicsPipelineDesc>();
        ReserveResourceLayout(CreateInfo.PSODesc.ResourceLayout, MemPool);

        const auto& InputLayout = CreateInfo.GraphicsPipeline.InputLayout;
        MemPool.AddSpace<LayoutElement>(InputLayout.NumElements);
        for (Uint32 i = 0; i < InputLayout.NumElements; ++i)
        {
            auto& LayoutElem = InputLayout.LayoutElements[i];
            MemPool.AddSpaceForString(LayoutElem.HLSLSemantic);
            m_BufferSlotsUsed = std::max(m_BufferSlotsUsed, static_cast<Uint8>(LayoutElem.BufferSlot + 1));
        }

        MemPool.AddSpace<Uint32>(m_BufferSlotsUsed);
    }

    void ReserveSpaceForPipelineDesc(const ComputePipelineStateCreateInfo& CreateInfo,
                                     LinearAllocator&                      MemPool) const noexcept
    {
        ReserveResourceLayout(CreateInfo.PSODesc.ResourceLayout, MemPool);
    }


    template <typename ShaderImplType, typename TShaderStages>
    void ExtractShaders(const GraphicsPipelineStateCreateInfo& CreateInfo,
                        TShaderStages&                         ShaderStages)
    {
        VERIFY(m_NumShaderStages == 0, "The number of shader stages is not zero! ExtractShaders must only be called once.");

        ShaderStages.clear();
        auto AddShaderStage = [&](IShader* pShader) {
            if (pShader != nullptr)
            {
                auto ShaderType = pShader->GetDesc().ShaderType;
                ShaderStages.emplace_back(ShaderType, ValidatedCast<ShaderImplType>(pShader));
                VERIFY(m_ShaderStageTypes[m_NumShaderStages] == SHADER_TYPE_UNKNOWN, "This shader stage is already initialized.");
                m_ShaderStageTypes[m_NumShaderStages++] = ShaderType;
            }
        };

        switch (CreateInfo.PSODesc.PipelineType)
        {
            case PIPELINE_TYPE_GRAPHICS:
            {
                AddShaderStage(CreateInfo.pVS);
                AddShaderStage(CreateInfo.pHS);
                AddShaderStage(CreateInfo.pDS);
                AddShaderStage(CreateInfo.pGS);
                AddShaderStage(CreateInfo.pPS);
                VERIFY(CreateInfo.pVS != nullptr, "Vertex shader must not be null");
                break;
            }

            case PIPELINE_TYPE_MESH:
            {
                AddShaderStage(CreateInfo.pAS);
                AddShaderStage(CreateInfo.pMS);
                AddShaderStage(CreateInfo.pPS);
                VERIFY(CreateInfo.pMS != nullptr, "Mesh shader must not be null");
                break;
            }

            default:
                UNEXPECTED("unknown pipeline type");
        }

        VERIFY_EXPR(!ShaderStages.empty() && ShaderStages.size() == m_NumShaderStages);
    }

    template <typename ShaderImplType, typename TShaderStages>
    void ExtractShaders(const ComputePipelineStateCreateInfo& CreateInfo,
                        TShaderStages&                        ShaderStages)
    {
        VERIFY(m_NumShaderStages == 0, "The number of shader stages is not zero! ExtractShaders must only be called once.");

        ShaderStages.clear();

        VERIFY_EXPR(CreateInfo.PSODesc.PipelineType == PIPELINE_TYPE_COMPUTE);
        VERIFY_EXPR(CreateInfo.pCS != nullptr);
        VERIFY_EXPR(CreateInfo.pCS->GetDesc().ShaderType == SHADER_TYPE_COMPUTE);

        ShaderStages.emplace_back(SHADER_TYPE_COMPUTE, ValidatedCast<ShaderImplType>(CreateInfo.pCS));
        m_ShaderStageTypes[m_NumShaderStages++] = SHADER_TYPE_COMPUTE;

        VERIFY_EXPR(!ShaderStages.empty() && ShaderStages.size() == m_NumShaderStages);
    }


    void InitializePipelineDesc(const GraphicsPipelineStateCreateInfo& CreateInfo,
                                LinearAllocator&                       MemPool)
    {
        this->m_pGraphicsPipelineDesc = MemPool.Copy(CreateInfo.GraphicsPipeline);

        auto& GraphicsPipeline = *this->m_pGraphicsPipelineDesc;
        CorrectGraphicsPipelineDesc(GraphicsPipeline);

        CopyResourceLayout(CreateInfo.PSODesc.ResourceLayout, this->m_Desc.ResourceLayout, MemPool);

        m_pRenderPass = GraphicsPipeline.pRenderPass;
        if (m_pRenderPass)
        {
            const auto& RPDesc = m_pRenderPass->GetDesc();
            VERIFY_EXPR(GraphicsPipeline.SubpassIndex < RPDesc.SubpassCount);
            const auto& Subpass = RPDesc.pSubpasses[GraphicsPipeline.SubpassIndex];

            GraphicsPipeline.NumRenderTargets = static_cast<Uint8>(Subpass.RenderTargetAttachmentCount);
            for (Uint32 rt = 0; rt < Subpass.RenderTargetAttachmentCount; ++rt)
            {
                const auto& RTAttachmentRef = Subpass.pRenderTargetAttachments[rt];
                if (RTAttachmentRef.AttachmentIndex != ATTACHMENT_UNUSED)
                {
                    VERIFY_EXPR(RTAttachmentRef.AttachmentIndex < RPDesc.AttachmentCount);
                    GraphicsPipeline.RTVFormats[rt] = RPDesc.pAttachments[RTAttachmentRef.AttachmentIndex].Format;
                }
            }

            if (Subpass.pDepthStencilAttachment != nullptr)
            {
                const auto& DSAttachmentRef = *Subpass.pDepthStencilAttachment;
                if (DSAttachmentRef.AttachmentIndex != ATTACHMENT_UNUSED)
                {
                    VERIFY_EXPR(DSAttachmentRef.AttachmentIndex < RPDesc.AttachmentCount);
                    GraphicsPipeline.DSVFormat = RPDesc.pAttachments[DSAttachmentRef.AttachmentIndex].Format;
                }
            }
        }

        const auto&    InputLayout     = GraphicsPipeline.InputLayout;
        LayoutElement* pLayoutElements = MemPool.Allocate<LayoutElement>(InputLayout.NumElements);
        for (size_t Elem = 0; Elem < InputLayout.NumElements; ++Elem)
        {
            const auto& SrcElem   = InputLayout.LayoutElements[Elem];
            pLayoutElements[Elem] = SrcElem;
            VERIFY_EXPR(SrcElem.HLSLSemantic != nullptr);
            pLayoutElements[Elem].HLSLSemantic = MemPool.CopyString(SrcElem.HLSLSemantic);
        }
        GraphicsPipeline.InputLayout.LayoutElements = pLayoutElements;


        // Correct description and compute offsets and tight strides
        std::array<Uint32, MAX_BUFFER_SLOTS> Strides, TightStrides = {};
        // Set all strides to an invalid value because an application may want to use 0 stride
        for (auto& Stride : Strides)
            Stride = LAYOUT_ELEMENT_AUTO_STRIDE;

        for (Uint32 i = 0; i < InputLayout.NumElements; ++i)
        {
            auto& LayoutElem = pLayoutElements[i];

            if (LayoutElem.ValueType == VT_FLOAT32 || LayoutElem.ValueType == VT_FLOAT16)
                LayoutElem.IsNormalized = false; // Floating point values cannot be normalized

            auto BuffSlot = LayoutElem.BufferSlot;
            if (BuffSlot >= Strides.size())
            {
                UNEXPECTED("Buffer slot (", BuffSlot, ") exceeds the maximum allowed value (", Strides.size() - 1, ")");
                continue;
            }
            VERIFY_EXPR(BuffSlot < m_BufferSlotsUsed);

            auto& CurrAutoStride = TightStrides[BuffSlot];
            // If offset is not explicitly specified, use current auto stride value
            if (LayoutElem.RelativeOffset == LAYOUT_ELEMENT_AUTO_OFFSET)
            {
                LayoutElem.RelativeOffset = CurrAutoStride;
            }

            // If stride is explicitly specified, use it for the current buffer slot
            if (LayoutElem.Stride != LAYOUT_ELEMENT_AUTO_STRIDE)
            {
                // Verify that the value is consistent with the previously specified stride, if any
                if (Strides[BuffSlot] != LAYOUT_ELEMENT_AUTO_STRIDE && Strides[BuffSlot] != LayoutElem.Stride)
                {
                    LOG_ERROR_MESSAGE("Inconsistent strides are specified for buffer slot ", BuffSlot,
                                      ". Input element at index ", LayoutElem.InputIndex, " explicitly specifies stride ",
                                      LayoutElem.Stride, ", while current value is ", Strides[BuffSlot],
                                      ". Specify consistent strides or use LAYOUT_ELEMENT_AUTO_STRIDE to allow "
                                      "the engine compute strides automatically.");
                }
                Strides[BuffSlot] = LayoutElem.Stride;
            }

            CurrAutoStride = std::max(CurrAutoStride, LayoutElem.RelativeOffset + LayoutElem.NumComponents * GetValueSize(LayoutElem.ValueType));
        }

        for (Uint32 i = 0; i < InputLayout.NumElements; ++i)
        {
            auto& LayoutElem = pLayoutElements[i];

            auto BuffSlot = LayoutElem.BufferSlot;
            // If no input elements explicitly specified stride for this buffer slot, use automatic stride
            if (Strides[BuffSlot] == LAYOUT_ELEMENT_AUTO_STRIDE)
            {
                Strides[BuffSlot] = TightStrides[BuffSlot];
            }
            else
            {
                if (Strides[BuffSlot] < TightStrides[BuffSlot])
                {
                    LOG_ERROR_MESSAGE("Stride ", Strides[BuffSlot], " explicitly specified for slot ", BuffSlot,
                                      " is smaller than the minimum stride ", TightStrides[BuffSlot],
                                      " required to accomodate all input elements.");
                }
            }
            if (LayoutElem.Stride == LAYOUT_ELEMENT_AUTO_STRIDE)
                LayoutElem.Stride = Strides[BuffSlot];
        }

        m_pStrides = MemPool.Allocate<Uint32>(m_BufferSlotsUsed);

        // Set strides for all unused slots to 0
        for (Uint32 i = 0; i < m_BufferSlotsUsed; ++i)
        {
            auto Stride   = Strides[i];
            m_pStrides[i] = Stride != LAYOUT_ELEMENT_AUTO_STRIDE ? Stride : 0;
        }
    }

    void InitializePipelineDesc(const ComputePipelineStateCreateInfo& CreateInfo,
                                LinearAllocator&                      MemPool)
    {
        CopyResourceLayout(CreateInfo.PSODesc.ResourceLayout, this->m_Desc.ResourceLayout, MemPool);
    }

private:
    static void ReserveResourceLayout(const PipelineResourceLayoutDesc& SrcLayout, LinearAllocator& MemPool) noexcept
    {
        if (SrcLayout.Variables != nullptr)
        {
            MemPool.AddSpace<ShaderResourceVariableDesc>(SrcLayout.NumVariables);
            for (Uint32 i = 0; i < SrcLayout.NumVariables; ++i)
            {
                VERIFY(SrcLayout.Variables[i].Name != nullptr, "Variable name can't be null");
                MemPool.AddSpaceForString(SrcLayout.Variables[i].Name);
            }
        }

        if (SrcLayout.ImmutableSamplers != nullptr)
        {
            MemPool.AddSpace<ImmutableSamplerDesc>(SrcLayout.NumImmutableSamplers);
            for (Uint32 i = 0; i < SrcLayout.NumImmutableSamplers; ++i)
            {
                VERIFY(SrcLayout.ImmutableSamplers[i].SamplerOrTextureName != nullptr, "Immutable sampler or texture name can't be null");
                MemPool.AddSpaceForString(SrcLayout.ImmutableSamplers[i].SamplerOrTextureName);
            }
        }
    }

    static void CopyResourceLayout(const PipelineResourceLayoutDesc& SrcLayout, PipelineResourceLayoutDesc& DstLayout, LinearAllocator& MemPool)
    {
        if (SrcLayout.Variables != nullptr)
        {
            auto* Variables     = MemPool.Allocate<ShaderResourceVariableDesc>(SrcLayout.NumVariables);
            DstLayout.Variables = Variables;
            for (Uint32 i = 0; i < SrcLayout.NumVariables; ++i)
            {
                const auto& SrcVar = SrcLayout.Variables[i];
                Variables[i]       = SrcVar;
                Variables[i].Name  = MemPool.CopyString(SrcVar.Name);
            }
        }

        if (SrcLayout.ImmutableSamplers != nullptr)
        {
            auto* ImmutableSamplers     = MemPool.Allocate<ImmutableSamplerDesc>(SrcLayout.NumImmutableSamplers);
            DstLayout.ImmutableSamplers = ImmutableSamplers;
            for (Uint32 i = 0; i < SrcLayout.NumImmutableSamplers; ++i)
            {
                const auto& SrcSmplr = SrcLayout.ImmutableSamplers[i];
#ifdef DILIGENT_DEVELOPMENT
                {
                    const auto& BorderColor = SrcSmplr.Desc.BorderColor;
                    if (!((BorderColor[0] == 0 && BorderColor[1] == 0 && BorderColor[2] == 0 && BorderColor[3] == 0) ||
                          (BorderColor[0] == 0 && BorderColor[1] == 0 && BorderColor[2] == 0 && BorderColor[3] == 1) ||
                          (BorderColor[0] == 1 && BorderColor[1] == 1 && BorderColor[2] == 1 && BorderColor[3] == 1)))
                    {
                        LOG_WARNING_MESSAGE("Immutable sampler for variable \"", SrcSmplr.SamplerOrTextureName, "\" specifies border color (",
                                            BorderColor[0], ", ", BorderColor[1], ", ", BorderColor[2], ", ", BorderColor[3],
                                            "). D3D12 static samplers only allow transparent black (0,0,0,0), opaque black (0,0,0,1) or opaque white (1,1,1,1) as border colors");
                    }
                }
#endif

                ImmutableSamplers[i]                      = SrcSmplr;
                ImmutableSamplers[i].SamplerOrTextureName = MemPool.CopyString(SrcSmplr.SamplerOrTextureName);
            }
        }
    }

protected:
    size_t m_ShaderResourceLayoutHash = 0; ///< Hash computed from the shader resource layout

    Uint32* m_pStrides        = nullptr;
    Uint8   m_BufferSlotsUsed = 0;

    Uint8 m_NumShaderStages = 0; ///< Number of shader stages in this PSO

    /// Array of shader types for every shader stage used by this PSO
    std::array<SHADER_TYPE, MAX_SHADERS_IN_PIPELINE> m_ShaderStageTypes = {};

    RefCntAutoPtr<IRenderPass> m_pRenderPass; ///< Strong reference to the render pass object

    GraphicsPipelineDesc* m_pGraphicsPipelineDesc = nullptr;
};

} // namespace Diligent
