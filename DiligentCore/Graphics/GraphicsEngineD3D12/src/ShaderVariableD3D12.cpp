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

#include "ShaderVariableD3D12.hpp"
#include "ShaderResourceVariableBase.hpp"

namespace Diligent
{

size_t ShaderVariableManagerD3D12::GetRequiredMemorySize(const ShaderResourceLayoutD3D12&     Layout,
                                                         const SHADER_RESOURCE_VARIABLE_TYPE* AllowedVarTypes,
                                                         Uint32                               NumAllowedTypes,
                                                         Uint32&                              NumVariables)
{
    NumVariables           = 0;
    Uint32 AllowedTypeBits = GetAllowedTypeBits(AllowedVarTypes, NumAllowedTypes);
    for (SHADER_RESOURCE_VARIABLE_TYPE VarType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC; VarType < SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES; VarType = static_cast<SHADER_RESOURCE_VARIABLE_TYPE>(VarType + 1))
    {
        if (IsAllowedType(VarType, AllowedTypeBits))
        {
            NumVariables += Layout.GetCbvSrvUavCount(VarType);
            if (Layout.IsUsingSeparateSamplers())
                NumVariables += Layout.GetSamplerCount(VarType);
        }
    }

    return NumVariables * sizeof(ShaderVariableD3D12Impl);
}

// Creates shader variable for every resource from SrcLayout whose type is one AllowedVarTypes
void ShaderVariableManagerD3D12::Initialize(const ShaderResourceLayoutD3D12&     SrcLayout,
                                            IMemoryAllocator&                    Allocator,
                                            const SHADER_RESOURCE_VARIABLE_TYPE* AllowedVarTypes,
                                            Uint32                               NumAllowedTypes)
{
#ifdef DILIGENT_DEBUG
    m_pDbgAllocator = &Allocator;
#endif

    const Uint32 AllowedTypeBits = GetAllowedTypeBits(AllowedVarTypes, NumAllowedTypes);
    VERIFY_EXPR(m_NumVariables == 0);
    auto MemSize = GetRequiredMemorySize(SrcLayout, AllowedVarTypes, NumAllowedTypes, m_NumVariables);

    if (m_NumVariables == 0)
        return;

    auto* pRawMem = ALLOCATE_RAW(Allocator, "Raw memory buffer for shader variables", MemSize);
    m_pVariables  = reinterpret_cast<ShaderVariableD3D12Impl*>(pRawMem);

    Uint32 VarInd = 0;
    for (SHADER_RESOURCE_VARIABLE_TYPE VarType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC; VarType < SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES; VarType = static_cast<SHADER_RESOURCE_VARIABLE_TYPE>(VarType + 1))
    {
        if (!IsAllowedType(VarType, AllowedTypeBits))
            continue;

        Uint32 NumResources = SrcLayout.GetCbvSrvUavCount(VarType);
        for (Uint32 r = 0; r < NumResources; ++r)
        {
            const auto& SrcRes = SrcLayout.GetSrvCbvUav(VarType, r);
            ::new (m_pVariables + VarInd) ShaderVariableD3D12Impl{*this, SrcRes};
            ++VarInd;
        }

        if (SrcLayout.IsUsingSeparateSamplers())
        {
            Uint32 NumSamplers = SrcLayout.GetSamplerCount(VarType);
            for (Uint32 r = 0; r < NumSamplers; ++r)
            {
                const auto& SrcSampler = SrcLayout.GetSampler(VarType, r);
                ::new (m_pVariables + VarInd) ShaderVariableD3D12Impl{*this, SrcSampler};
                ++VarInd;
            }
        }
    }
    VERIFY_EXPR(VarInd == m_NumVariables);
}

ShaderVariableManagerD3D12::~ShaderVariableManagerD3D12()
{
    VERIFY(m_pVariables == nullptr, "Destroy() has not been called");
}

void ShaderVariableManagerD3D12::Destroy(IMemoryAllocator& Allocator)
{
    if (m_pVariables != nullptr)
    {
        VERIFY(m_pDbgAllocator == &Allocator, "Incosistent alloctor");

        for (Uint32 v = 0; v < m_NumVariables; ++v)
            m_pVariables[v].~ShaderVariableD3D12Impl();
        Allocator.Free(m_pVariables);
        m_pVariables = nullptr;
    }
}

ShaderVariableD3D12Impl* ShaderVariableManagerD3D12::GetVariable(const Char* Name)
{
    ShaderVariableD3D12Impl* pVar = nullptr;
    for (Uint32 v = 0; v < m_NumVariables; ++v)
    {
        auto& Var = m_pVariables[v];
        if (strcmp(Var.m_Resource.Attribs.Name, Name) == 0)
        {
            pVar = &Var;
            break;
        }
    }
    return pVar;
}


ShaderVariableD3D12Impl* ShaderVariableManagerD3D12::GetVariable(Uint32 Index)
{
    if (Index >= m_NumVariables)
    {
        LOG_ERROR("Index ", Index, " is out of range");
        return nullptr;
    }

    return m_pVariables + Index;
}

Uint32 ShaderVariableManagerD3D12::GetVariableIndex(const ShaderVariableD3D12Impl& Variable)
{
    if (m_pVariables == nullptr)
    {
        LOG_ERROR("This shader variable manager has no variables");
        return static_cast<Uint32>(-1);
    }

    auto Offset = reinterpret_cast<const Uint8*>(&Variable) - reinterpret_cast<Uint8*>(m_pVariables);
    VERIFY(Offset % sizeof(ShaderVariableD3D12Impl) == 0, "Offset is not multiple of ShaderVariableD3D12Impl class size");
    auto Index = static_cast<Uint32>(Offset / sizeof(ShaderVariableD3D12Impl));
    if (Index < m_NumVariables)
        return Index;
    else
    {
        LOG_ERROR("Failed to get variable index. The variable ", &Variable, " does not belong to this shader variable manager");
        return static_cast<Uint32>(-1);
    }
}

void ShaderVariableManagerD3D12::BindResources(IResourceMapping* pResourceMapping, Uint32 Flags)
{
    DEV_CHECK_ERR(pResourceMapping != nullptr, "Failed to bind resources: resource mapping is null");

    if ((Flags & BIND_SHADER_RESOURCES_UPDATE_ALL) == 0)
        Flags |= BIND_SHADER_RESOURCES_UPDATE_ALL;

    for (Uint32 v = 0; v < m_NumVariables; ++v)
    {
        auto&       Var = m_pVariables[v];
        const auto& Res = Var.m_Resource;

        if ((Flags & (1 << Res.GetVariableType())) == 0)
            continue;

        for (Uint32 ArrInd = 0; ArrInd < Res.Attribs.BindCount; ++ArrInd)
        {
            if ((Flags & BIND_SHADER_RESOURCES_KEEP_EXISTING) && Res.IsBound(ArrInd, m_ResourceCache))
                continue;

            RefCntAutoPtr<IDeviceObject> pObj;
            VERIFY_EXPR(pResourceMapping != nullptr);
            pResourceMapping->GetResource(Res.Attribs.Name, &pObj, ArrInd);
            if (pObj)
            {
                //  Call non-virtual function
                Res.BindResource(pObj, ArrInd, m_ResourceCache);
            }
            else
            {
                if ((Flags & BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED) && !Res.IsBound(ArrInd, m_ResourceCache))
                    LOG_ERROR_MESSAGE("Unable to bind resource to shader variable '", Res.Attribs.GetPrintName(ArrInd), "': resource is not found in the resource mapping");
            }
        }
    }
}

} // namespace Diligent
