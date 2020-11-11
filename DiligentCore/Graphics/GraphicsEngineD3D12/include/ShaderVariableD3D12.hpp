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
/// Declaration of Diligent::ShaderVariableManagerD3D12 and Diligent::ShaderVariableD3D12Impl classes

//
//  * ShaderVariableManagerD3D12 keeps list of variables of specific types
//  * Every ShaderVariableD3D12Impl references D3D12Resource from ShaderResourceLayoutD3D12
//  * ShaderVariableManagerD3D12 keeps pointer to ShaderResourceCacheD3D12
//  * ShaderVariableManagerD3D12 is used by PipelineStateD3D12Impl to manage static resources and by
//    ShaderResourceBindingD3D12Impl to manage mutable and dynamic resources
//
//          _____________________________                   ________________________________________________________________________________
//         |                             |                 |                              |                               |                 |
//    .----|  ShaderVariableManagerD3D12 |---------------->|  ShaderVariableD3D12Impl[0]  |   ShaderVariableD3D12Impl[1]  |     ...         |
//    |    |_____________________________|                 |______________________________|_______________________________|_________________|
//    |                |                                                    \                          |
//    |                |                                                    Ref                       Ref
//    |                |                                                      \                        |
//    |     ___________V_______________                  ______________________V_______________________V_____________________________
//    |    |                           |   unique_ptr   |                  |                  |             |                        |
//    |    | ShaderResourceLayoutD3D12 |--------------->| D3D12Resource[0] | D3D12Resource[1] |     ...     | D3D12Resource[s+m+d-1] |
//    |    |___________________________|                |__________________|__________________|_____________|________________________|
//    |                                                        |                                                            |
//    |                                                        |                                                            |
//    |                                                        | (RootTable, Offset)                                       / (RootTable, Offset)
//    |                                                         \                                                         /
//    |     __________________________                   ________V_______________________________________________________V_______
//    |    |                          |                 |                                                                        |
//    '--->| ShaderResourceCacheD3D12 |---------------->|                                   Resources                            |
//         |__________________________|                 |________________________________________________________________________|
//
//   Memory buffer is allocated through the allocator provided by the pipeline state. If allocation granularity > 1, fixed block
//   memory allocator is used. This ensures that all resources from different shader resource bindings reside in
//   continuous memory. If allocation granularity == 1, raw allocator is used.

#include <memory>

#include "ShaderResourceVariableD3D.h"
#include "ShaderResourceLayoutD3D12.hpp"
#include "ShaderResourceVariableBase.hpp"

namespace Diligent
{

class ShaderVariableD3D12Impl;

// sizeof(ShaderVariableManagerD3D12) == 32 (x64, msvc, Release)
class ShaderVariableManagerD3D12
{
public:
    ShaderVariableManagerD3D12(IObject&                  Owner,
                               ShaderResourceCacheD3D12& ResourceCache) noexcept :
        m_Owner{Owner},
        m_ResourceCache{ResourceCache}
    {}

    void Initialize(const ShaderResourceLayoutD3D12&     Layout,
                    IMemoryAllocator&                    Allocator,
                    const SHADER_RESOURCE_VARIABLE_TYPE* AllowedVarTypes,
                    Uint32                               NumAllowedTypes);
    ~ShaderVariableManagerD3D12();

    void Destroy(IMemoryAllocator& Allocator);

    ShaderVariableD3D12Impl* GetVariable(const Char* Name);
    ShaderVariableD3D12Impl* GetVariable(Uint32 Index);

    void BindResources(IResourceMapping* pResourceMapping, Uint32 Flags);

    static size_t GetRequiredMemorySize(const ShaderResourceLayoutD3D12&     Layout,
                                        const SHADER_RESOURCE_VARIABLE_TYPE* AllowedVarTypes,
                                        Uint32                               NumAllowedTypes,
                                        Uint32&                              NumVariables);

    Uint32 GetVariableCount() const { return m_NumVariables; }

private:
    friend ShaderVariableD3D12Impl;

    Uint32 GetVariableIndex(const ShaderVariableD3D12Impl& Variable);

    // clang-format off

    IObject&                         m_Owner;

    // Variable mgr is owned by either Pipeline state object (in which case m_ResourceCache references
    // static resource cache owned by the same PSO object), or by SRB object (in which case 
    // m_ResourceCache references the cache in the SRB). Thus the cache and the resource layout
    // (which the variables reference) are guaranteed to be alive while the manager is alive.
    ShaderResourceCacheD3D12&        m_ResourceCache;

    // Memory is allocated through the allocator provided by the pipeline state. If allocation granularity > 1, fixed block
    // memory allocator is used. This ensures that all resources from different shader resource bindings reside in
    // continuous memory. If allocation granularity == 1, raw allocator is used.
    ShaderVariableD3D12Impl*         m_pVariables     = nullptr;
    Uint32                           m_NumVariables = 0;

#ifdef DILIGENT_DEBUG
    IMemoryAllocator*                m_pDbgAllocator = nullptr;
#endif
    // clang-format on
};

// sizeof(ShaderVariableD3D12Impl) == 24 (x64)
class ShaderVariableD3D12Impl final : public IShaderResourceVariableD3D
{
public:
    ShaderVariableD3D12Impl(ShaderVariableManagerD3D12&                     ParentManager,
                            const ShaderResourceLayoutD3D12::D3D12Resource& Resource) :
        m_ParentManager{ParentManager},
        m_Resource{Resource}
    {}

    // clang-format off
    ShaderVariableD3D12Impl            (const ShaderVariableD3D12Impl&) = delete;
    ShaderVariableD3D12Impl            (ShaderVariableD3D12Impl&&)      = delete;
    ShaderVariableD3D12Impl& operator= (const ShaderVariableD3D12Impl&) = delete;
    ShaderVariableD3D12Impl& operator= (ShaderVariableD3D12Impl&&)      = delete;
    // clang-format on

    virtual IReferenceCounters* DILIGENT_CALL_TYPE GetReferenceCounters() const override final
    {
        return m_ParentManager.m_Owner.GetReferenceCounters();
    }

    virtual Atomics::Long DILIGENT_CALL_TYPE AddRef() override final
    {
        return m_ParentManager.m_Owner.AddRef();
    }

    virtual Atomics::Long DILIGENT_CALL_TYPE Release() override final
    {
        return m_ParentManager.m_Owner.Release();
    }

    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override final
    {
        if (ppInterface == nullptr)
            return;

        *ppInterface = nullptr;
        if (IID == IID_ShaderResourceVariableD3D || IID == IID_ShaderResourceVariable || IID == IID_Unknown)
        {
            *ppInterface = this;
            (*ppInterface)->AddRef();
        }
    }

    virtual SHADER_RESOURCE_VARIABLE_TYPE DILIGENT_CALL_TYPE GetType() const override final
    {
        return m_Resource.GetVariableType();
    }

    virtual void DILIGENT_CALL_TYPE Set(IDeviceObject* pObject) override final
    {
        m_Resource.BindResource(pObject, 0, m_ParentManager.m_ResourceCache);
    }

    virtual void DILIGENT_CALL_TYPE SetArray(IDeviceObject* const* ppObjects, Uint32 FirstElement, Uint32 NumElements) override final
    {
        VerifyAndCorrectSetArrayArguments(m_Resource.Attribs.Name, m_Resource.Attribs.BindCount, FirstElement, NumElements);
        for (Uint32 Elem = 0; Elem < NumElements; ++Elem)
            m_Resource.BindResource(ppObjects[Elem], FirstElement + Elem, m_ParentManager.m_ResourceCache);
    }

    virtual void DILIGENT_CALL_TYPE GetResourceDesc(ShaderResourceDesc& ResourceDesc) const override final
    {
        ResourceDesc = GetHLSLResourceDesc();
    }

    virtual HLSLShaderResourceDesc DILIGENT_CALL_TYPE GetHLSLResourceDesc() const override final
    {
        return m_Resource.Attribs.GetHLSLResourceDesc();
    }

    virtual Uint32 DILIGENT_CALL_TYPE GetIndex() const override final
    {
        return m_ParentManager.GetVariableIndex(*this);
    }

    virtual bool DILIGENT_CALL_TYPE IsBound(Uint32 ArrayIndex) const override final
    {
        return m_Resource.IsBound(ArrayIndex, m_ParentManager.m_ResourceCache);
    }

    const ShaderResourceLayoutD3D12::D3D12Resource& GetResource() const
    {
        return m_Resource;
    }

private:
    friend ShaderVariableManagerD3D12;

    ShaderVariableManagerD3D12&                     m_ParentManager;
    const ShaderResourceLayoutD3D12::D3D12Resource& m_Resource;
};

} // namespace Diligent
