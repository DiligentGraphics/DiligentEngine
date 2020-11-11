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
/// Implementation of the Diligent::ShaderBase template class

#include <vector>

#include "Atomics.hpp"
#include "ShaderResourceVariable.h"
#include "PipelineState.h"
#include "StringTools.hpp"
#include "GraphicsAccessories.hpp"

namespace Diligent
{

template <typename TNameCompare>
SHADER_RESOURCE_VARIABLE_TYPE GetShaderVariableType(SHADER_TYPE                       ShaderStage,
                                                    SHADER_RESOURCE_VARIABLE_TYPE     DefaultVariableType,
                                                    const ShaderResourceVariableDesc* Variables,
                                                    Uint32                            NumVars,
                                                    TNameCompare                      NameCompare)
{
    for (Uint32 v = 0; v < NumVars; ++v)
    {
        const auto& CurrVarDesc = Variables[v];
        if (((CurrVarDesc.ShaderStages & ShaderStage) != 0) && NameCompare(CurrVarDesc.Name))
        {
            return CurrVarDesc.Type;
        }
    }
    return DefaultVariableType;
}

inline SHADER_RESOURCE_VARIABLE_TYPE GetShaderVariableType(SHADER_TYPE                       ShaderStage,
                                                           const Char*                       Name,
                                                           SHADER_RESOURCE_VARIABLE_TYPE     DefaultVariableType,
                                                           const ShaderResourceVariableDesc* Variables,
                                                           Uint32                            NumVars)
{
    return GetShaderVariableType(ShaderStage, DefaultVariableType, Variables, NumVars,
                                 [&](const char* VarName) //
                                 {
                                     return strcmp(VarName, Name) == 0;
                                 });
}

inline SHADER_RESOURCE_VARIABLE_TYPE GetShaderVariableType(SHADER_TYPE                       ShaderStage,
                                                           const Char*                       Name,
                                                           const PipelineResourceLayoutDesc& LayoutDesc)
{
    return GetShaderVariableType(ShaderStage, Name, LayoutDesc.DefaultVariableType, LayoutDesc.Variables, LayoutDesc.NumVariables);
}

inline SHADER_RESOURCE_VARIABLE_TYPE GetShaderVariableType(SHADER_TYPE                       ShaderStage,
                                                           const String&                     Name,
                                                           SHADER_RESOURCE_VARIABLE_TYPE     DefaultVariableType,
                                                           const ShaderResourceVariableDesc* Variables,
                                                           Uint32                            NumVars)
{
    return GetShaderVariableType(ShaderStage, DefaultVariableType, Variables, NumVars,
                                 [&](const char* VarName) //
                                 {
                                     return Name.compare(VarName) == 0;
                                 });
}

inline SHADER_RESOURCE_VARIABLE_TYPE GetShaderVariableType(SHADER_TYPE                       ShaderStage,
                                                           const String&                     Name,
                                                           const PipelineResourceLayoutDesc& LayoutDesc)
{
    return GetShaderVariableType(ShaderStage, Name, LayoutDesc.DefaultVariableType, LayoutDesc.Variables, LayoutDesc.NumVariables);
}

inline bool IsAllowedType(SHADER_RESOURCE_VARIABLE_TYPE VarType, Uint32 AllowedTypeBits) noexcept
{
    return ((1 << VarType) & AllowedTypeBits) != 0;
}

inline Uint32 GetAllowedTypeBits(const SHADER_RESOURCE_VARIABLE_TYPE* AllowedVarTypes, Uint32 NumAllowedTypes) noexcept
{
    if (AllowedVarTypes == nullptr)
        return 0xFFFFFFFF;

    Uint32 AllowedTypeBits = 0;
    for (Uint32 i = 0; i < NumAllowedTypes; ++i)
        AllowedTypeBits |= 1 << AllowedVarTypes[i];
    return AllowedTypeBits;
}

inline Int32 FindImmutableSampler(const ImmutableSamplerDesc* ImtblSamplers,
                                  Uint32                      NumImtblSamplers,
                                  SHADER_TYPE                 ShaderType,
                                  const char*                 ResourceName,
                                  const char*                 SamplerSuffix)
{
    for (Uint32 s = 0; s < NumImtblSamplers; ++s)
    {
        const auto& StSam = ImtblSamplers[s];
        if (((StSam.ShaderStages & ShaderType) != 0) && StreqSuff(ResourceName, StSam.SamplerOrTextureName, SamplerSuffix))
            return s;
    }

    return -1;
}




template <typename ResourceAttribsType,
          typename BufferImplType>
bool VerifyConstantBufferBinding(const ResourceAttribsType&    Attribs,
                                 SHADER_RESOURCE_VARIABLE_TYPE VarType,
                                 Uint32                        ArrayIndex,
                                 const IDeviceObject*          pBuffer,
                                 const BufferImplType*         pBufferImpl,
                                 const IDeviceObject*          pCachedBuffer,
                                 const char*                   ShaderName = nullptr)
{
    if (pBuffer != nullptr && pBufferImpl == nullptr)
    {
        std::stringstream ss;
        ss << "Failed to bind resource '" << pBuffer->GetDesc().Name << "' to variable '" << Attribs.GetPrintName(ArrayIndex) << '\'';
        if (ShaderName != nullptr)
        {
            ss << " in shader '" << ShaderName << '\'';
        }
        ss << ". Invalid resource type: buffer is expected.";
        LOG_ERROR_MESSAGE(ss.str());
        return false;
    }

    bool BindingOK = true;
    if (pBufferImpl != nullptr && (pBufferImpl->GetDesc().BindFlags & BIND_UNIFORM_BUFFER) == 0)
    {
        std::stringstream ss;
        ss << "Error binding buffer '" << pBufferImpl->GetDesc().Name << "' to variable '" << Attribs.GetPrintName(ArrayIndex) << '\'';
        if (ShaderName != nullptr)
        {
            ss << " in shader '" << ShaderName << '\'';
        }
        ss << ". The buffer was not created with BIND_UNIFORM_BUFFER flag.";
        LOG_ERROR_MESSAGE(ss.str());
        BindingOK = false;
    }

    if (VarType != SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC && pCachedBuffer != nullptr && pCachedBuffer != pBufferImpl)
    {
        auto VarTypeStr = GetShaderVariableTypeLiteralName(VarType);

        std::stringstream ss;
        ss << "Non-null constant (uniform) buffer '" << pCachedBuffer->GetDesc().Name << "' is already bound to " << VarTypeStr
           << " shader variable '" << Attribs.GetPrintName(ArrayIndex) << '\'';
        if (ShaderName != nullptr)
        {
            ss << " in shader '" << ShaderName << '\'';
        }
        ss << ". Attempting to bind ";
        if (pBufferImpl)
        {
            ss << "another resource ('" << pBufferImpl->GetDesc().Name << "')";
        }
        else
        {
            ss << "null";
        }
        ss << " is an error and may cause unpredicted behavior. Use another shader resource binding instance or label the variable as dynamic.";
        LOG_ERROR_MESSAGE(ss.str());

        BindingOK = false;
    }

    return BindingOK;
}

template <typename TViewTypeEnum>
const char* GetResourceTypeName();

template <>
inline const char* GetResourceTypeName<TEXTURE_VIEW_TYPE>()
{
    return "texture view";
}

template <>
inline const char* GetResourceTypeName<BUFFER_VIEW_TYPE>()
{
    return "buffer view";
}

inline RESOURCE_DIMENSION GetResourceViewDimension(const ITextureView* pTexView)
{
    VERIFY_EXPR(pTexView != nullptr);
    return pTexView->GetDesc().TextureDim;
}

inline RESOURCE_DIMENSION GetResourceViewDimension(const IBufferView* /*pBuffView*/)
{
    return RESOURCE_DIM_BUFFER;
}

inline Uint32 GetResourceSampleCount(const ITextureView* pTexView)
{
    VERIFY_EXPR(pTexView != nullptr);
    return const_cast<ITextureView*>(pTexView)->GetTexture()->GetDesc().SampleCount;
}

inline Uint32 GetResourceSampleCount(const IBufferView* /*pBuffView*/)
{
    return 0;
}

template <typename ResourceAttribsType,
          typename ResourceViewImplType,
          typename ViewTypeEnumType>
bool VerifyResourceViewBinding(const ResourceAttribsType&              Attribs,
                               SHADER_RESOURCE_VARIABLE_TYPE           VarType,
                               Uint32                                  ArrayIndex,
                               const IDeviceObject*                    pView,
                               const ResourceViewImplType*             pViewImpl,
                               std::initializer_list<ViewTypeEnumType> ExpectedViewTypes,
                               const IDeviceObject*                    pCachedView,
                               const char*                             ShaderName = nullptr)
{
    const char* ExpectedResourceType = GetResourceTypeName<ViewTypeEnumType>();

    if (pView && !pViewImpl)
    {
        std::stringstream ss;
        ss << "Failed to bind resource '" << pView->GetDesc().Name << "' to variable '" << Attribs.GetPrintName(ArrayIndex) << '\'';
        if (ShaderName != nullptr)
        {
            ss << " in shader '" << ShaderName << '\'';
        }
        ss << ". Invalid resource type: " << ExpectedResourceType << " is expected.";
        LOG_ERROR_MESSAGE(ss.str());
        return false;
    }

    bool BindingOK = true;
    if (pViewImpl)
    {
        auto ViewType           = pViewImpl->GetDesc().ViewType;
        bool IsExpectedViewType = false;
        for (auto ExpectedViewType : ExpectedViewTypes)
        {
            if (ExpectedViewType == ViewType)
                IsExpectedViewType = true;
        }

        if (!IsExpectedViewType)
        {
            std::stringstream ss;
            ss << "Error binding " << ExpectedResourceType << " '" << pViewImpl->GetDesc().Name << "' to variable '"
               << Attribs.GetPrintName(ArrayIndex) << '\'';
            if (ShaderName != nullptr)
            {
                ss << " in shader '" << ShaderName << '\'';
            }
            ss << ". Incorrect view type: ";
            bool IsFirstViewType = true;
            for (auto ExpectedViewType : ExpectedViewTypes)
            {
                if (!IsFirstViewType)
                {
                    ss << " or ";
                }
                ss << GetViewTypeLiteralName(ExpectedViewType);
                IsFirstViewType = false;
            }
            ss << " is expected, " << GetViewTypeLiteralName(ViewType) << " is provided.";
            LOG_ERROR_MESSAGE(ss.str());

            BindingOK = false;
        }

        const auto ExpectedResourceDim = Attribs.GetResourceDimension();
        if (ExpectedResourceDim != RESOURCE_DIM_UNDEFINED)
        {
            auto ResourceDim = GetResourceViewDimension(pViewImpl);
            if (ResourceDim != ExpectedResourceDim)
            {
                LOG_ERROR_MESSAGE("Error binding ", ExpectedResourceType, " '", pViewImpl->GetDesc().Name, "' to variable '",
                                  Attribs.GetPrintName(ArrayIndex), "': incorrect resource dimension: ",
                                  GetResourceDimString(ExpectedResourceDim), " is expected, but the actual dimension is ",
                                  GetResourceDimString(ResourceDim));

                BindingOK = false;
            }

            if (ResourceDim == RESOURCE_DIM_TEX_2D || ResourceDim == RESOURCE_DIM_TEX_2D_ARRAY)
            {
                auto SampleCount = GetResourceSampleCount(pViewImpl);
                auto IsMS        = Attribs.IsMultisample();
                if (IsMS && SampleCount == 1)
                {
                    LOG_ERROR_MESSAGE("Error binding ", ExpectedResourceType, " '", pViewImpl->GetDesc().Name, "' to variable '",
                                      Attribs.GetPrintName(ArrayIndex), "': multisample texture is expected.");
                    BindingOK = false;
                }
                else if (!IsMS && SampleCount > 1)
                {
                    LOG_ERROR_MESSAGE("Error binding ", ExpectedResourceType, " '", pViewImpl->GetDesc().Name, "' to variable '",
                                      Attribs.GetPrintName(ArrayIndex), "': single-sample texture is expected.");
                    BindingOK = false;
                }
            }
        }
    }

    if (VarType != SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC && pCachedView != nullptr && pCachedView != pViewImpl)
    {
        const auto* VarTypeStr = GetShaderVariableTypeLiteralName(VarType);

        std::stringstream ss;
        ss << "Non-null resource '" << pCachedView->GetDesc().Name << "' is already bound to " << VarTypeStr
           << " shader variable '" << Attribs.GetPrintName(ArrayIndex) << '\'';
        if (ShaderName != nullptr)
        {
            ss << " in shader '" << ShaderName << '\'';
        }
        ss << ". Attempting to bind ";
        if (pViewImpl)
        {
            ss << "another resource ('" << pViewImpl->GetDesc().Name << "')";
        }
        else
        {
            ss << "null";
        }
        ss << " is an error and may cause unpredicted behavior. Use another shader resource binding instance or label the variable as dynamic.";
        LOG_ERROR_MESSAGE(ss.str());

        BindingOK = false;
    }

    return BindingOK;
}

inline void VerifyAndCorrectSetArrayArguments(const char* Name, Uint32 ArraySize, Uint32& FirstElement, Uint32& NumElements)
{
    if (FirstElement >= ArraySize)
    {
        LOG_ERROR_MESSAGE("SetArray arguments are invalid for '", Name, "' variable: FirstElement (", FirstElement, ") is out of allowed range 0 .. ", ArraySize - 1);
        FirstElement = ArraySize - 1;
        NumElements  = 0;
    }

    if (FirstElement + NumElements > ArraySize)
    {
        LOG_ERROR_MESSAGE("SetArray arguments are invalid for '", Name, "' variable: specified element range (", FirstElement, " .. ",
                          FirstElement + NumElements - 1, ") is out of array bounds 0 .. ", ArraySize - 1);
        NumElements = ArraySize - FirstElement;
    }
}

struct DefaultShaderVariableIDComparator
{
    bool operator()(const INTERFACE_ID& IID) const
    {
        return IID == IID_ShaderResourceVariable || IID == IID_Unknown;
    }
};

/// Base implementation of a shader variable
template <typename ResourceLayoutType,
          typename ResourceVariableBaseInterface = IShaderResourceVariable,
          typename VariableIDComparator          = DefaultShaderVariableIDComparator>
struct ShaderVariableBase : public ResourceVariableBaseInterface
{
    ShaderVariableBase(ResourceLayoutType& ParentResLayout) :
        m_ParentResLayout{ParentResLayout}
    {
    }

    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override final
    {
        if (ppInterface == nullptr)
            return;

        *ppInterface = nullptr;
        if (VariableIDComparator{}(IID))
        {
            *ppInterface = this;
            (*ppInterface)->AddRef();
        }
    }

    virtual Atomics::Long DILIGENT_CALL_TYPE AddRef() override final
    {
        return m_ParentResLayout.GetOwner().AddRef();
    }

    virtual Atomics::Long DILIGENT_CALL_TYPE Release() override final
    {
        return m_ParentResLayout.GetOwner().Release();
    }

    virtual IReferenceCounters* DILIGENT_CALL_TYPE GetReferenceCounters() const override final
    {
        return m_ParentResLayout.GetOwner().GetReferenceCounters();
    }

protected:
    ResourceLayoutType& m_ParentResLayout;
};

} // namespace Diligent
