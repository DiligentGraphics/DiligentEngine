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
/// Declaration of Diligent::ShaderResourceLayoutD3D11 class

#include "ShaderResources.hpp"
#include "ShaderBase.hpp"
#include "ShaderResourceCacheD3D11.hpp"
#include "EngineD3D11Defines.h"
#include "STDAllocator.hpp"
#include "ShaderVariableD3DBase.hpp"
#include "ShaderResourcesD3D11.hpp"

namespace Diligent
{

/// Diligent::ShaderResourceLayoutD3D11 class
/// http://diligentgraphics.com/diligent-engine/architecture/d3d11/shader-resource-layout/
// sizeof(ShaderResourceLayoutD3D11) == 64 (x64)
class ShaderResourceLayoutD3D11
{
public:
    ShaderResourceLayoutD3D11(IObject&                  Owner,
                              ShaderResourceCacheD3D11& ResourceCache) noexcept :
        m_Owner{Owner},
        m_ResourceCache{ResourceCache}
    {
    }

    void Initialize(std::shared_ptr<const ShaderResourcesD3D11> pSrcResources,
                    const PipelineResourceLayoutDesc&           ResourceLayout,
                    const SHADER_RESOURCE_VARIABLE_TYPE*        VarTypes,
                    Uint32                                      NumVarTypes,
                    IMemoryAllocator&                           ResCacheDataAllocator,
                    IMemoryAllocator&                           ResLayoutDataAllocator);
    ~ShaderResourceLayoutD3D11();

    // clang-format off
    // No copies, only moves are allowed
    ShaderResourceLayoutD3D11             (const ShaderResourceLayoutD3D11&)  = delete;
    ShaderResourceLayoutD3D11& operator = (const ShaderResourceLayoutD3D11&)  = delete;
    ShaderResourceLayoutD3D11             (      ShaderResourceLayoutD3D11&&) = default;
    ShaderResourceLayoutD3D11& operator = (      ShaderResourceLayoutD3D11&&) = delete;
    // clang-format on

    static size_t GetRequiredMemorySize(const ShaderResourcesD3D11&          SrcResources,
                                        const PipelineResourceLayoutDesc&    ResourceLayout,
                                        const SHADER_RESOURCE_VARIABLE_TYPE* AllowedVarTypes,
                                        Uint32                               NumAllowedTypes) noexcept;

    void CopyResources(ShaderResourceCacheD3D11& DstCache) const;

    using ShaderVariableD3D11Base = ShaderVariableD3DBase<ShaderResourceLayoutD3D11>;

    struct ConstBuffBindInfo final : ShaderVariableD3D11Base
    {
        ConstBuffBindInfo(const D3DShaderResourceAttribs& ResourceAttribs,
                          ShaderResourceLayoutD3D11&      ParentResLayout,
                          SHADER_RESOURCE_VARIABLE_TYPE   VariableType) :
            ShaderVariableD3D11Base{ParentResLayout, ResourceAttribs, VariableType}
        {}
        // Non-virtual function
        __forceinline void BindResource(IDeviceObject* pObject, Uint32 ArrayIndex);

        virtual void DILIGENT_CALL_TYPE Set(IDeviceObject* pObject) override final { BindResource(pObject, 0); }

        virtual void DILIGENT_CALL_TYPE SetArray(IDeviceObject* const* ppObjects,
                                                 Uint32                FirstElement,
                                                 Uint32                NumElements) override final
        {
            VerifyAndCorrectSetArrayArguments(m_Attribs.Name, m_Attribs.BindCount, FirstElement, NumElements);
            for (Uint32 elem = 0; elem < NumElements; ++elem)
                BindResource(ppObjects[elem], FirstElement + elem);
        }

        virtual bool DILIGENT_CALL_TYPE IsBound(Uint32 ArrayIndex) const override final
        {
            VERIFY_EXPR(ArrayIndex < m_Attribs.BindCount);
            return m_ParentResLayout.m_ResourceCache.IsCBBound(m_Attribs.BindPoint + ArrayIndex);
        }
    };

    struct TexSRVBindInfo final : ShaderVariableD3D11Base
    {
        TexSRVBindInfo(const D3DShaderResourceAttribs& _TextureAttribs,
                       Uint32                          _SamplerIndex,
                       ShaderResourceLayoutD3D11&      ParentResLayout,
                       SHADER_RESOURCE_VARIABLE_TYPE   VariableType) :
            ShaderVariableD3D11Base{ParentResLayout, _TextureAttribs, VariableType},
            SamplerIndex{_SamplerIndex}
        {}

        // Non-virtual function
        __forceinline void BindResource(IDeviceObject* pObject, Uint32 ArrayIndex);

        virtual void DILIGENT_CALL_TYPE Set(IDeviceObject* pObject) override final { BindResource(pObject, 0); }

        virtual void DILIGENT_CALL_TYPE SetArray(IDeviceObject* const* ppObjects,
                                                 Uint32                FirstElement,
                                                 Uint32                NumElements) override final
        {
            VerifyAndCorrectSetArrayArguments(m_Attribs.Name, m_Attribs.BindCount, FirstElement, NumElements);
            for (Uint32 elem = 0; elem < NumElements; ++elem)
                BindResource(ppObjects[elem], FirstElement + elem);
        }

        virtual bool DILIGENT_CALL_TYPE IsBound(Uint32 ArrayIndex) const override final
        {
            VERIFY_EXPR(ArrayIndex < m_Attribs.BindCount);
            return m_ParentResLayout.m_ResourceCache.IsSRVBound(m_Attribs.BindPoint + ArrayIndex, true);
        }


        bool ValidSamplerAssigned() const { return SamplerIndex != InvalidSamplerIndex; }

        static constexpr Uint32 InvalidSamplerIndex = static_cast<Uint32>(-1);
        const Uint32            SamplerIndex;
    };

    struct TexUAVBindInfo final : ShaderVariableD3D11Base
    {
        TexUAVBindInfo(const D3DShaderResourceAttribs& ResourceAttribs,
                       ShaderResourceLayoutD3D11&      ParentResLayout,
                       SHADER_RESOURCE_VARIABLE_TYPE   VariableType) :
            ShaderVariableD3D11Base(ParentResLayout, ResourceAttribs, VariableType)
        {}

        // Provide non-virtual function
        __forceinline void BindResource(IDeviceObject* pObject, Uint32 ArrayIndex);

        virtual void DILIGENT_CALL_TYPE Set(IDeviceObject* pObject) override final { BindResource(pObject, 0); }

        virtual void DILIGENT_CALL_TYPE SetArray(IDeviceObject* const* ppObjects,
                                                 Uint32                FirstElement,
                                                 Uint32                NumElements) override final
        {
            VerifyAndCorrectSetArrayArguments(m_Attribs.Name, m_Attribs.BindCount, FirstElement, NumElements);
            for (Uint32 elem = 0; elem < NumElements; ++elem)
                BindResource(ppObjects[elem], FirstElement + elem);
        }

        __forceinline virtual bool DILIGENT_CALL_TYPE IsBound(Uint32 ArrayIndex) const override final
        {
            VERIFY_EXPR(ArrayIndex < m_Attribs.BindCount);
            return m_ParentResLayout.m_ResourceCache.IsUAVBound(m_Attribs.BindPoint + ArrayIndex, true);
        }
    };

    struct BuffUAVBindInfo final : ShaderVariableD3D11Base
    {
        BuffUAVBindInfo(const D3DShaderResourceAttribs& ResourceAttribs,
                        ShaderResourceLayoutD3D11&      ParentResLayout,
                        SHADER_RESOURCE_VARIABLE_TYPE   VariableType) :
            ShaderVariableD3D11Base{ParentResLayout, ResourceAttribs, VariableType}
        {}

        // Non-virtual function
        __forceinline void BindResource(IDeviceObject* pObject, Uint32 ArrayIndex);

        virtual void DILIGENT_CALL_TYPE Set(IDeviceObject* pObject) override final { BindResource(pObject, 0); }

        virtual void DILIGENT_CALL_TYPE SetArray(IDeviceObject* const* ppObjects,
                                                 Uint32                FirstElement,
                                                 Uint32                NumElements) override final
        {
            VerifyAndCorrectSetArrayArguments(m_Attribs.Name, m_Attribs.BindCount, FirstElement, NumElements);
            for (Uint32 elem = 0; elem < NumElements; ++elem)
                BindResource(ppObjects[elem], FirstElement + elem);
        }

        virtual bool DILIGENT_CALL_TYPE IsBound(Uint32 ArrayIndex) const override final
        {
            VERIFY_EXPR(ArrayIndex < m_Attribs.BindCount);
            return m_ParentResLayout.m_ResourceCache.IsUAVBound(m_Attribs.BindPoint + ArrayIndex, false);
        }
    };

    struct BuffSRVBindInfo final : ShaderVariableD3D11Base
    {
        BuffSRVBindInfo(const D3DShaderResourceAttribs& ResourceAttribs,
                        ShaderResourceLayoutD3D11&      ParentResLayout,
                        SHADER_RESOURCE_VARIABLE_TYPE   VariableType) :
            ShaderVariableD3D11Base{ParentResLayout, ResourceAttribs, VariableType}
        {}

        // Non-virtual function
        __forceinline void BindResource(IDeviceObject* pObject, Uint32 ArrayIndex);

        virtual void DILIGENT_CALL_TYPE Set(IDeviceObject* pObject) override final { BindResource(pObject, 0); }

        virtual void DILIGENT_CALL_TYPE SetArray(IDeviceObject* const* ppObjects,
                                                 Uint32                FirstElement,
                                                 Uint32                NumElements) override final
        {
            VerifyAndCorrectSetArrayArguments(m_Attribs.Name, m_Attribs.BindCount, FirstElement, NumElements);
            for (Uint32 elem = 0; elem < NumElements; ++elem)
                BindResource(ppObjects[elem], FirstElement + elem);
        }

        virtual bool DILIGENT_CALL_TYPE IsBound(Uint32 ArrayIndex) const override final
        {
            VERIFY_EXPR(ArrayIndex < m_Attribs.BindCount);
            return m_ParentResLayout.m_ResourceCache.IsSRVBound(m_Attribs.BindPoint + ArrayIndex, false);
        }
    };

    struct SamplerBindInfo final : ShaderVariableD3D11Base
    {
        SamplerBindInfo(const D3DShaderResourceAttribs& ResourceAttribs,
                        ShaderResourceLayoutD3D11&      ParentResLayout,
                        SHADER_RESOURCE_VARIABLE_TYPE   VariableType) :
            ShaderVariableD3D11Base{ParentResLayout, ResourceAttribs, VariableType}
        {}

        // Non-virtual function
        __forceinline void BindResource(IDeviceObject* pObject, Uint32 ArrayIndex);

        virtual void DILIGENT_CALL_TYPE Set(IDeviceObject* pObject) override final { BindResource(pObject, 0); }

        virtual void DILIGENT_CALL_TYPE SetArray(IDeviceObject* const* ppObjects,
                                                 Uint32                FirstElement,
                                                 Uint32                NumElements) override final
        {
            VerifyAndCorrectSetArrayArguments(m_Attribs.Name, m_Attribs.BindCount, FirstElement, NumElements);
            for (Uint32 elem = 0; elem < NumElements; ++elem)
                BindResource(ppObjects[elem], FirstElement + elem);
        }

        virtual bool DILIGENT_CALL_TYPE IsBound(Uint32 ArrayIndex) const override final
        {
            VERIFY_EXPR(ArrayIndex < m_Attribs.BindCount);
            return m_ParentResLayout.m_ResourceCache.IsSamplerBound(m_Attribs.BindPoint + ArrayIndex);
        }
    };

    // dbgResourceCache is only used for sanity check and as a remainder that the resource cache must be alive
    // while Layout is alive
    void BindResources(IResourceMapping* pResourceMapping, Uint32 Flags, const ShaderResourceCacheD3D11& dbgResourceCache);

#ifdef DILIGENT_DEVELOPMENT
    bool dvpVerifyBindings() const;
#endif

    IShaderResourceVariable*  GetShaderVariable(const Char* Name);
    IShaderResourceVariable*  GetShaderVariable(Uint32 Index);
    __forceinline SHADER_TYPE GetShaderType() const { return m_pResources->GetShaderType(); }

    IObject& GetOwner() { return m_Owner; }

    Uint32 GetVariableIndex(const ShaderVariableD3D11Base& Variable) const;
    Uint32 GetTotalResourceCount() const
    {
        auto ResourceCount = GetNumCBs() + GetNumTexSRVs() + GetNumTexUAVs() + GetNumBufUAVs() + GetNumBufSRVs();
        // Do not expose sampler variables when using combined texture samplers
        if (!m_pResources->IsUsingCombinedTextureSamplers())
            ResourceCount += GetNumSamplers();
        return ResourceCount;
    }

    // clang-format off
    Uint32 GetNumCBs()      const { return (m_TexSRVsOffset  - 0               ) / sizeof(ConstBuffBindInfo);}
    Uint32 GetNumTexSRVs()  const { return (m_TexUAVsOffset  - m_TexSRVsOffset ) / sizeof(TexSRVBindInfo);   }
    Uint32 GetNumTexUAVs()  const { return (m_BuffSRVsOffset - m_TexUAVsOffset ) / sizeof(TexUAVBindInfo) ;  }
    Uint32 GetNumBufSRVs()  const { return (m_BuffUAVsOffset - m_BuffSRVsOffset) / sizeof(BuffSRVBindInfo);  }
    Uint32 GetNumBufUAVs()  const { return (m_SamplerOffset  - m_BuffUAVsOffset) / sizeof(BuffUAVBindInfo);  }
    Uint32 GetNumSamplers() const { return (m_MemorySize     - m_SamplerOffset ) / sizeof(SamplerBindInfo);  }

    template<typename ResourceType> Uint32 GetNumResources()const;
    template<> Uint32 GetNumResources<ConstBuffBindInfo>() const { return GetNumCBs();      }
    template<> Uint32 GetNumResources<TexSRVBindInfo>   () const { return GetNumTexSRVs();  }
    template<> Uint32 GetNumResources<TexUAVBindInfo>   () const { return GetNumTexUAVs();  }
    template<> Uint32 GetNumResources<BuffSRVBindInfo>  () const { return GetNumBufSRVs();  }
    template<> Uint32 GetNumResources<BuffUAVBindInfo>  () const { return GetNumBufUAVs();  }
    template<> Uint32 GetNumResources<SamplerBindInfo>  () const { return GetNumSamplers(); }
    // clang-format on

    const Char* GetShaderName() const
    {
        return m_pResources->GetShaderName();
    }

private:
    // clang-format off

/* 0 */ IObject&                                       m_Owner;
/* 8 */ std::shared_ptr<const ShaderResourcesD3D11>    m_pResources;

       // No need to use shared pointer, as the resource cache is either part of the same
       // ShaderD3D11Impl object, or ShaderResourceBindingD3D11Impl object
/*24*/ ShaderResourceCacheD3D11&                      m_ResourceCache;

/*32*/ std::unique_ptr<void, STDDeleterRawMem<void> > m_ResourceBuffer;
    
       // Offsets in bytes
       using OffsetType = Uint16;
/*48*/ OffsetType m_TexSRVsOffset  = 0;
/*50*/ OffsetType m_TexUAVsOffset  = 0;
/*52*/ OffsetType m_BuffSRVsOffset = 0;
/*54*/ OffsetType m_BuffUAVsOffset = 0;
/*56*/ OffsetType m_SamplerOffset  = 0;
/*58*/ OffsetType m_MemorySize     = 0;
/*60 - 64*/    
/*64*/ // End of data


    template<typename ResourceType> OffsetType GetResourceOffset()const;
    template<> OffsetType GetResourceOffset<ConstBuffBindInfo>() const { return 0;                }
    template<> OffsetType GetResourceOffset<TexSRVBindInfo>   () const { return m_TexSRVsOffset;  }
    template<> OffsetType GetResourceOffset<TexUAVBindInfo>   () const { return m_TexUAVsOffset;  }
    template<> OffsetType GetResourceOffset<BuffSRVBindInfo>  () const { return m_BuffSRVsOffset; }
    template<> OffsetType GetResourceOffset<BuffUAVBindInfo>  () const { return m_BuffUAVsOffset; }
    template<> OffsetType GetResourceOffset<SamplerBindInfo>  () const { return m_SamplerOffset;  }

    // clang-format on

    template <typename ResourceType>
    ResourceType& GetResource(Uint32 ResIndex)
    {
        VERIFY(ResIndex < GetNumResources<ResourceType>(), "Resource index (", ResIndex, ") exceeds max allowed value (", GetNumResources<ResourceType>() - 1, ")");
        auto Offset = GetResourceOffset<ResourceType>();
        return reinterpret_cast<ResourceType*>(reinterpret_cast<Uint8*>(m_ResourceBuffer.get()) + Offset)[ResIndex];
    }

    template <typename ResourceType>
    const ResourceType& GetConstResource(Uint32 ResIndex) const
    {
        VERIFY(ResIndex < GetNumResources<ResourceType>(), "Resource index (", ResIndex, ") exceeds max allowed value (", GetNumResources<ResourceType>() - 1, ")");
        auto Offset = GetResourceOffset<ResourceType>();
        return reinterpret_cast<const ResourceType*>(reinterpret_cast<const Uint8*>(m_ResourceBuffer.get()) + Offset)[ResIndex];
    }

    template <typename ResourceType>
    IShaderResourceVariable* GetResourceByName(const Char* Name);

    template <typename THandleCB,
              typename THandleTexSRV,
              typename THandleTexUAV,
              typename THandleBufSRV,
              typename THandleBufUAV,
              typename THandleSampler>
    void HandleResources(THandleCB      HandleCB,
                         THandleTexSRV  HandleTexSRV,
                         THandleTexUAV  HandleTexUAV,
                         THandleBufSRV  HandleBufSRV,
                         THandleBufUAV  HandleBufUAV,
                         THandleSampler HandleSampler)
    {
        for (Uint32 cb = 0; cb < GetNumResources<ConstBuffBindInfo>(); ++cb)
            HandleCB(GetResource<ConstBuffBindInfo>(cb));

        for (Uint32 t = 0; t < GetNumResources<TexSRVBindInfo>(); ++t)
            HandleTexSRV(GetResource<TexSRVBindInfo>(t));

        for (Uint32 u = 0; u < GetNumResources<TexUAVBindInfo>(); ++u)
            HandleTexUAV(GetResource<TexUAVBindInfo>(u));

        for (Uint32 s = 0; s < GetNumResources<BuffSRVBindInfo>(); ++s)
            HandleBufSRV(GetResource<BuffSRVBindInfo>(s));

        for (Uint32 u = 0; u < GetNumResources<BuffUAVBindInfo>(); ++u)
            HandleBufUAV(GetResource<BuffUAVBindInfo>(u));

        for (Uint32 s = 0; s < GetNumResources<SamplerBindInfo>(); ++s)
            HandleSampler(GetResource<SamplerBindInfo>(s));
    }

    template <typename THandleCB,
              typename THandleTexSRV,
              typename THandleTexUAV,
              typename THandleBufSRV,
              typename THandleBufUAV,
              typename THandleSampler>
    void HandleConstResources(THandleCB      HandleCB,
                              THandleTexSRV  HandleTexSRV,
                              THandleTexUAV  HandleTexUAV,
                              THandleBufSRV  HandleBufSRV,
                              THandleBufUAV  HandleBufUAV,
                              THandleSampler HandleSampler) const
    {
        for (Uint32 cb = 0; cb < GetNumResources<ConstBuffBindInfo>(); ++cb)
            HandleCB(GetConstResource<ConstBuffBindInfo>(cb));

        for (Uint32 t = 0; t < GetNumResources<TexSRVBindInfo>(); ++t)
            HandleTexSRV(GetConstResource<TexSRVBindInfo>(t));

        for (Uint32 u = 0; u < GetNumResources<TexUAVBindInfo>(); ++u)
            HandleTexUAV(GetConstResource<TexUAVBindInfo>(u));

        for (Uint32 s = 0; s < GetNumResources<BuffSRVBindInfo>(); ++s)
            HandleBufSRV(GetConstResource<BuffSRVBindInfo>(s));

        for (Uint32 u = 0; u < GetNumResources<BuffUAVBindInfo>(); ++u)
            HandleBufUAV(GetConstResource<BuffUAVBindInfo>(u));

        for (Uint32 s = 0; s < GetNumResources<SamplerBindInfo>(); ++s)
            HandleSampler(GetConstResource<SamplerBindInfo>(s));
    }

    friend class ShaderVariableIndexLocator;
    friend class ShaderVariableLocator;
};

} // namespace Diligent
