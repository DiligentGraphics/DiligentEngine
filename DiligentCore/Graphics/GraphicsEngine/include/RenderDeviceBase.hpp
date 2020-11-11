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
/// Implementation of the Diligent::RenderDeviceBase template class and related structures

#include "RenderDevice.h"
#include "DeviceObjectBase.hpp"
#include "Defines.h"
#include "ResourceMappingImpl.hpp"
#include "StateObjectsRegistry.hpp"
#include "HashUtils.hpp"
#include "ObjectBase.hpp"
#include "DeviceContext.h"
#include "SwapChain.h"
#include "GraphicsAccessories.hpp"
#include "FixedBlockMemoryAllocator.hpp"
#include "EngineMemory.h"
#include "STDAllocator.hpp"

namespace std
{
/// Hash function specialization for Diligent::SamplerDesc structure.
template <>
struct hash<Diligent::SamplerDesc>
{
    size_t operator()(const Diligent::SamplerDesc& SamDesc) const
    {
        // Sampler name is ignored in comparison operator
        // and should not be hashed
        return Diligent::ComputeHash( // SamDesc.Name,
            static_cast<int>(SamDesc.MinFilter),
            static_cast<int>(SamDesc.MagFilter),
            static_cast<int>(SamDesc.MipFilter),
            static_cast<int>(SamDesc.AddressU),
            static_cast<int>(SamDesc.AddressV),
            static_cast<int>(SamDesc.AddressW),
            SamDesc.MipLODBias,
            SamDesc.MaxAnisotropy,
            static_cast<int>(SamDesc.ComparisonFunc),
            SamDesc.BorderColor[0],
            SamDesc.BorderColor[1],
            SamDesc.BorderColor[2],
            SamDesc.BorderColor[3],
            SamDesc.MinLOD, SamDesc.MaxLOD);
    }
};

/// Hash function specialization for Diligent::StencilOpDesc structure.
template <>
struct hash<Diligent::StencilOpDesc>
{
    size_t operator()(const Diligent::StencilOpDesc& StOpDesc) const
    {
        return Diligent::ComputeHash(static_cast<int>(StOpDesc.StencilFailOp),
                                     static_cast<int>(StOpDesc.StencilDepthFailOp),
                                     static_cast<int>(StOpDesc.StencilPassOp),
                                     static_cast<int>(StOpDesc.StencilFunc));
    }
};

/// Hash function specialization for Diligent::DepthStencilStateDesc structure.
template <>
struct hash<Diligent::DepthStencilStateDesc>
{
    size_t operator()(const Diligent::DepthStencilStateDesc& DepthStencilDesc) const
    {
        return Diligent::ComputeHash(DepthStencilDesc.DepthEnable,
                                     DepthStencilDesc.DepthWriteEnable,
                                     static_cast<int>(DepthStencilDesc.DepthFunc),
                                     DepthStencilDesc.StencilEnable,
                                     DepthStencilDesc.StencilReadMask,
                                     DepthStencilDesc.StencilWriteMask,
                                     DepthStencilDesc.FrontFace,
                                     DepthStencilDesc.BackFace);
    }
};

/// Hash function specialization for Diligent::RasterizerStateDesc structure.
template <>
struct hash<Diligent::RasterizerStateDesc>
{
    size_t operator()(const Diligent::RasterizerStateDesc& RasterizerDesc) const
    {
        return Diligent::ComputeHash(static_cast<int>(RasterizerDesc.FillMode),
                                     static_cast<int>(RasterizerDesc.CullMode),
                                     RasterizerDesc.FrontCounterClockwise,
                                     RasterizerDesc.DepthBias,
                                     RasterizerDesc.DepthBiasClamp,
                                     RasterizerDesc.SlopeScaledDepthBias,
                                     RasterizerDesc.DepthClipEnable,
                                     RasterizerDesc.ScissorEnable,
                                     RasterizerDesc.AntialiasedLineEnable);
    }
};

/// Hash function specialization for Diligent::BlendStateDesc structure.
template <>
struct hash<Diligent::BlendStateDesc>
{
    size_t operator()(const Diligent::BlendStateDesc& BSDesc) const
    {
        std::size_t Seed = 0;
        for (size_t i = 0; i < Diligent::MAX_RENDER_TARGETS; ++i)
        {
            const auto& rt = BSDesc.RenderTargets[i];
            Diligent::HashCombine(Seed,
                                  rt.BlendEnable,
                                  static_cast<int>(rt.SrcBlend),
                                  static_cast<int>(rt.DestBlend),
                                  static_cast<int>(rt.BlendOp),
                                  static_cast<int>(rt.SrcBlendAlpha),
                                  static_cast<int>(rt.DestBlendAlpha),
                                  static_cast<int>(rt.BlendOpAlpha),
                                  rt.RenderTargetWriteMask);
        }
        Diligent::HashCombine(Seed,
                              BSDesc.AlphaToCoverageEnable,
                              BSDesc.IndependentBlendEnable);
        return Seed;
    }
};


/// Hash function specialization for Diligent::TextureViewDesc structure.
template <>
struct hash<Diligent::TextureViewDesc>
{
    size_t operator()(const Diligent::TextureViewDesc& TexViewDesc) const
    {
        std::size_t Seed = 0;
        Diligent::HashCombine(Seed,
                              static_cast<Diligent::Int32>(TexViewDesc.ViewType),
                              static_cast<Diligent::Int32>(TexViewDesc.TextureDim),
                              static_cast<Diligent::Int32>(TexViewDesc.Format),
                              TexViewDesc.MostDetailedMip,
                              TexViewDesc.NumMipLevels,
                              TexViewDesc.FirstArraySlice,
                              TexViewDesc.NumArraySlices,
                              static_cast<Diligent::Uint32>(TexViewDesc.AccessFlags),
                              static_cast<Diligent::Uint32>(TexViewDesc.Flags));
        return Seed;
    }
};
} // namespace std

namespace Diligent
{

/// Base implementation of a render device

/// \tparam BaseInterface - base interface that this class will inheret.
/// \warning
/// Render device must *NOT* hold strong references to any
/// object it creates to avoid circular dependencies.
/// Device context, swap chain and all object the device creates
/// keep strong reference to the device.
/// Device only holds weak reference to the immediate context.
template <typename BaseInterface>
class RenderDeviceBase : public ObjectBase<BaseInterface>
{
public:
    using TObjectBase = ObjectBase<BaseInterface>;

    /// Describes the sizes of device objects
    struct DeviceObjectSizes
    {
        /// Size of the texture object (TextureD3D12Impl, TextureVkImpl, etc.), in bytes
        const size_t TextureObjSize;

        /// Size of the texture view object (TextureViewD3D12Impl, TextureViewVkImpl, etc.), in bytes
        const size_t TexViewObjSize;

        /// Size of the buffer object (BufferD3D12Impl, BufferVkImpl, etc.), in bytes
        const size_t BufferObjSize;

        /// Size of the buffer view object (BufferViewD3D12Impl, BufferViewVkImpl, etc.), in bytes
        const size_t BuffViewObjSize;

        /// Size of the shader object (ShaderD3D12Impl, ShaderVkImpl, etc.), in bytes
        const size_t ShaderObjSize;

        /// Size of the sampler object (SamplerD3D12Impl, SamplerVkImpl, etc.), in bytes
        const size_t SamplerObjSize;

        /// Size of the pipeline state object (PipelineStateD3D12Impl, PipelineStateVkImpl, etc.), in bytes
        const size_t PSOSize;

        /// Size of the shader resource binding object (ShaderResourceBindingD3D12Impl, ShaderResourceBindingVkImpl, etc.), in bytes
        const size_t SRBSize;

        /// Size of the fence object (FenceD3D12Impl, FenceVkImpl, etc.), in bytes
        const size_t FenceSize;

        /// Size of the query object (QueryD3D12Impl, QueryVkImpl, etc.), in bytes
        const size_t QuerySize;

        /// Size of the render pass object (RenderPassD3D12Impl, RenderPassVkImpl, etc.), in bytes
        const size_t RenderPassObjSize;

        /// Size of the framebuffer object (FramebufferD3D12Impl, FramebufferVkImpl, etc.), in bytes
        const size_t FramebufferObjSize;
    };

    /// \param pRefCounters        - reference counters object that controls the lifetime of this render device
    /// \param RawMemAllocator     - allocator that will be used to allocate memory for all device objects (including render device itself)
    /// \param pEngineFactory      - engine factory that was used to create this device
    /// \param NumDeferredContexts - number of deferred device contexts
    /// \param ObjectSizes         - device object sizes
    ///
    /// \remarks Render device uses fixed block allocators (see FixedBlockMemoryAllocator) to allocate memory for
    ///          device objects. The object sizes provided to constructor are used to initialize the allocators.
    RenderDeviceBase(IReferenceCounters*      pRefCounters,
                     IMemoryAllocator&        RawMemAllocator,
                     IEngineFactory*          pEngineFactory,
                     Uint32                   NumDeferredContexts,
                     const DeviceObjectSizes& ObjectSizes) :
        // clang-format off
        TObjectBase             {pRefCounters},
        m_pEngineFactory        {pEngineFactory},
        m_SamplersRegistry      {RawMemAllocator, "sampler"},
        m_TextureFormatsInfo    (TEX_FORMAT_NUM_FORMATS, TextureFormatInfoExt(), STD_ALLOCATOR_RAW_MEM(TextureFormatInfoExt, RawMemAllocator, "Allocator for vector<TextureFormatInfoExt>")),
        m_TexFmtInfoInitFlags   (TEX_FORMAT_NUM_FORMATS, false, STD_ALLOCATOR_RAW_MEM(bool, RawMemAllocator, "Allocator for vector<bool>")),
        m_wpDeferredContexts    (NumDeferredContexts, RefCntWeakPtr<IDeviceContext>(), STD_ALLOCATOR_RAW_MEM(RefCntWeakPtr<IDeviceContext>, RawMemAllocator, "Allocator for vector< RefCntWeakPtr<IDeviceContext> >")),
        m_RawMemAllocator       {RawMemAllocator},
        m_TexObjAllocator       {RawMemAllocator, ObjectSizes.TextureObjSize,     64  },
        m_TexViewObjAllocator   {RawMemAllocator, ObjectSizes.TexViewObjSize,     64  },
        m_BufObjAllocator       {RawMemAllocator, ObjectSizes.BufferObjSize,      128 },
        m_BuffViewObjAllocator  {RawMemAllocator, ObjectSizes.BuffViewObjSize,    128 },
        m_ShaderObjAllocator    {RawMemAllocator, ObjectSizes.ShaderObjSize,      32  },
        m_SamplerObjAllocator   {RawMemAllocator, ObjectSizes.SamplerObjSize,     32  },
        m_PSOAllocator          {RawMemAllocator, ObjectSizes.PSOSize,            128 },
        m_SRBAllocator          {RawMemAllocator, ObjectSizes.SRBSize,            1024},
        m_ResMappingAllocator   {RawMemAllocator, sizeof(ResourceMappingImpl),    16  },
        m_FenceAllocator        {RawMemAllocator, ObjectSizes.FenceSize,          16  },
        m_QueryAllocator        {RawMemAllocator, ObjectSizes.QuerySize,          16  },
        m_RenderPassAllocator   {RawMemAllocator, ObjectSizes.RenderPassObjSize,  16  },
        m_FramebufferAllocator  {RawMemAllocator, ObjectSizes.FramebufferObjSize, 16  }
    // clang-format on
    {
        // Initialize texture format info
        for (Uint32 Fmt = TEX_FORMAT_UNKNOWN; Fmt < TEX_FORMAT_NUM_FORMATS; ++Fmt)
            static_cast<TextureFormatAttribs&>(m_TextureFormatsInfo[Fmt]) = GetTextureFormatAttribs(static_cast<TEXTURE_FORMAT>(Fmt));

        // https://msdn.microsoft.com/en-us/library/windows/desktop/ff471325(v=vs.85).aspx
        TEXTURE_FORMAT FilterableFormats[] =
            {
                TEX_FORMAT_RGBA32_FLOAT, // OpenGL ES3.1 does not require this format to be filterable
                TEX_FORMAT_RGBA16_FLOAT,
                TEX_FORMAT_RGBA16_UNORM,
                TEX_FORMAT_RGBA16_SNORM,
                TEX_FORMAT_RG32_FLOAT, // OpenGL ES3.1 does not require this format to be filterable
                TEX_FORMAT_R32_FLOAT_X8X24_TYPELESS,
                //TEX_FORMAT_R10G10B10A2_UNORM,
                TEX_FORMAT_R11G11B10_FLOAT,
                TEX_FORMAT_RGBA8_UNORM,
                TEX_FORMAT_RGBA8_UNORM_SRGB,
                TEX_FORMAT_RGBA8_SNORM,
                TEX_FORMAT_RG16_FLOAT,
                TEX_FORMAT_RG16_UNORM,
                TEX_FORMAT_RG16_SNORM,
                TEX_FORMAT_R32_FLOAT, // OpenGL ES3.1 does not require this format to be filterable
                TEX_FORMAT_R24_UNORM_X8_TYPELESS,
                TEX_FORMAT_RG8_UNORM,
                TEX_FORMAT_RG8_SNORM,
                TEX_FORMAT_R16_FLOAT,
                TEX_FORMAT_R16_UNORM,
                TEX_FORMAT_R16_SNORM,
                TEX_FORMAT_R8_UNORM,
                TEX_FORMAT_R8_SNORM,
                TEX_FORMAT_A8_UNORM,
                TEX_FORMAT_RGB9E5_SHAREDEXP,
                TEX_FORMAT_RG8_B8G8_UNORM,
                TEX_FORMAT_G8R8_G8B8_UNORM,
                TEX_FORMAT_BC1_UNORM,
                TEX_FORMAT_BC1_UNORM_SRGB,
                TEX_FORMAT_BC2_UNORM,
                TEX_FORMAT_BC2_UNORM_SRGB,
                TEX_FORMAT_BC3_UNORM,
                TEX_FORMAT_BC3_UNORM_SRGB,
                TEX_FORMAT_BC4_UNORM,
                TEX_FORMAT_BC4_SNORM,
                TEX_FORMAT_BC5_UNORM,
                TEX_FORMAT_BC5_SNORM,
                TEX_FORMAT_B5G6R5_UNORM};
        for (Uint32 fmt = 0; fmt < _countof(FilterableFormats); ++fmt)
            m_TextureFormatsInfo[FilterableFormats[fmt]].Filterable = true;
    }

    ~RenderDeviceBase()
    {
    }

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_RenderDevice, ObjectBase<BaseInterface>)

    // It is important to have final implementation of Release() method to avoid
    // virtual calls
    inline virtual ReferenceCounterValueType DILIGENT_CALL_TYPE Release() override final
    {
        return TObjectBase::Release();
    }

    /// Implementation of IRenderDevice::CreateResourceMapping().
    virtual void DILIGENT_CALL_TYPE CreateResourceMapping(const ResourceMappingDesc& MappingDesc, IResourceMapping** ppMapping) override final;

    /// Implementation of IRenderDevice::GetDeviceCaps().
    virtual const DeviceCaps& DILIGENT_CALL_TYPE GetDeviceCaps() const override final
    {
        return m_DeviceCaps;
    }

    /// Implementation of IRenderDevice::GetTextureFormatInfo().
    virtual const TextureFormatInfo& DILIGENT_CALL_TYPE GetTextureFormatInfo(TEXTURE_FORMAT TexFormat) override final
    {
        VERIFY(TexFormat >= TEX_FORMAT_UNKNOWN && TexFormat < TEX_FORMAT_NUM_FORMATS, "Texture format out of range");
        const auto& TexFmtInfo = m_TextureFormatsInfo[TexFormat];
        VERIFY(TexFmtInfo.Format == TexFormat, "Sanity check failed");
        return TexFmtInfo;
    }

    /// Implementation of IRenderDevice::GetTextureFormatInfoExt().
    virtual const TextureFormatInfoExt& DILIGENT_CALL_TYPE GetTextureFormatInfoExt(TEXTURE_FORMAT TexFormat) override final
    {
        VERIFY(TexFormat >= TEX_FORMAT_UNKNOWN && TexFormat < TEX_FORMAT_NUM_FORMATS, "Texture format out of range");
        const auto& TexFmtInfo = m_TextureFormatsInfo[TexFormat];
        VERIFY(TexFmtInfo.Format == TexFormat, "Sanity check failed");
        if (!m_TexFmtInfoInitFlags[TexFormat])
        {
            if (TexFmtInfo.Supported)
                TestTextureFormat(TexFormat);
            m_TexFmtInfoInitFlags[TexFormat] = true;
        }
        return TexFmtInfo;
    }

    virtual IEngineFactory* DILIGENT_CALL_TYPE GetEngineFactory() const override final
    {
        return m_pEngineFactory.RawPtr<IEngineFactory>();
    }

    void OnCreateDeviceObject(IDeviceObject* pNewObject)
    {
    }

    StateObjectsRegistry<SamplerDesc>& GetSamplerRegistry() { return m_SamplersRegistry; }

    /// Set weak reference to the immediate context
    void SetImmediateContext(IDeviceContext* pImmediateContext)
    {
        VERIFY(m_wpImmediateContext.Lock() == nullptr, "Immediate context has already been set");
        m_wpImmediateContext = pImmediateContext;
    }

    /// Set weak reference to the deferred context
    void SetDeferredContext(size_t Ctx, IDeviceContext* pDeferredCtx)
    {
        VERIFY(m_wpDeferredContexts[Ctx].Lock() == nullptr, "Deferred context has already been set");
        m_wpDeferredContexts[Ctx] = pDeferredCtx;
    }

    /// Returns number of deferred contexts
    size_t GetNumDeferredContexts() const
    {
        return m_wpDeferredContexts.size();
    }

    RefCntAutoPtr<IDeviceContext> GetImmediateContext() { return m_wpImmediateContext.Lock(); }
    RefCntAutoPtr<IDeviceContext> GetDeferredContext(size_t Ctx) { return m_wpDeferredContexts[Ctx].Lock(); }

    FixedBlockMemoryAllocator& GetTexViewObjAllocator() { return m_TexViewObjAllocator; }
    FixedBlockMemoryAllocator& GetBuffViewObjAllocator() { return m_BuffViewObjAllocator; }
    FixedBlockMemoryAllocator& GetSRBAllocator() { return m_SRBAllocator; }

protected:
    virtual void TestTextureFormat(TEXTURE_FORMAT TexFormat) = 0;

    /// Helper template function to facilitate device object creation
    template <typename TObjectType, typename TObjectDescType, typename TObjectConstructor>
    void CreateDeviceObject(const Char* ObjectTypeName, const TObjectDescType& Desc, TObjectType** ppObject, TObjectConstructor ConstructObject);

    RefCntAutoPtr<IEngineFactory> m_pEngineFactory;

    DeviceCaps m_DeviceCaps;

    // All state object registries hold raw pointers.
    // This is safe because every object unregisters itself
    // when it is deleted.
    StateObjectsRegistry<SamplerDesc>                                           m_SamplersRegistry; ///< Sampler state registry
    std::vector<TextureFormatInfoExt, STDAllocatorRawMem<TextureFormatInfoExt>> m_TextureFormatsInfo;
    std::vector<bool, STDAllocatorRawMem<bool>>                                 m_TexFmtInfoInitFlags;

    /// Weak reference to the immediate context. Immediate context holds strong reference
    /// to the device, so we must use weak reference to avoid circular dependencies.
    RefCntWeakPtr<IDeviceContext> m_wpImmediateContext;

    /// Weak references to deferred contexts.
    std::vector<RefCntWeakPtr<IDeviceContext>, STDAllocatorRawMem<RefCntWeakPtr<IDeviceContext>>> m_wpDeferredContexts;

    IMemoryAllocator&         m_RawMemAllocator;      ///< Raw memory allocator
    FixedBlockMemoryAllocator m_TexObjAllocator;      ///< Allocator for texture objects
    FixedBlockMemoryAllocator m_TexViewObjAllocator;  ///< Allocator for texture view objects
    FixedBlockMemoryAllocator m_BufObjAllocator;      ///< Allocator for buffer objects
    FixedBlockMemoryAllocator m_BuffViewObjAllocator; ///< Allocator for buffer view objects
    FixedBlockMemoryAllocator m_ShaderObjAllocator;   ///< Allocator for shader objects
    FixedBlockMemoryAllocator m_SamplerObjAllocator;  ///< Allocator for sampler objects
    FixedBlockMemoryAllocator m_PSOAllocator;         ///< Allocator for pipeline state objects
    FixedBlockMemoryAllocator m_SRBAllocator;         ///< Allocator for shader resource binding objects
    FixedBlockMemoryAllocator m_ResMappingAllocator;  ///< Allocator for resource mapping objects
    FixedBlockMemoryAllocator m_FenceAllocator;       ///< Allocator for fence objects
    FixedBlockMemoryAllocator m_QueryAllocator;       ///< Allocator for query objects
    FixedBlockMemoryAllocator m_RenderPassAllocator;  ///< Allocator for render pass objects
    FixedBlockMemoryAllocator m_FramebufferAllocator; ///< Allocator for framebuffer objects
};


template <typename BaseInterface>
void RenderDeviceBase<BaseInterface>::CreateResourceMapping(const ResourceMappingDesc& MappingDesc, IResourceMapping** ppMapping)
{
    VERIFY(ppMapping != nullptr, "Null pointer provided");
    if (ppMapping == nullptr)
        return;
    VERIFY(*ppMapping == nullptr, "Overwriting reference to existing object may cause memory leaks");

    auto* pResourceMapping(NEW_RC_OBJ(m_ResMappingAllocator, "ResourceMappingImpl instance", ResourceMappingImpl)(GetRawAllocator()));
    pResourceMapping->QueryInterface(IID_ResourceMapping, reinterpret_cast<IObject**>(ppMapping));
    if (MappingDesc.pEntries)
    {
        for (auto* pEntry = MappingDesc.pEntries; pEntry->Name && pEntry->pObject; ++pEntry)
        {
            (*ppMapping)->AddResourceArray(pEntry->Name, pEntry->ArrayIndex, &pEntry->pObject, 1, true);
        }
    }
}


/// \tparam TObjectType - type of the object being created (IBuffer, ITexture, etc.)
/// \tparam TObjectDescType - type of the object description structure (BufferDesc, TextureDesc, etc.)
/// \tparam TObjectConstructor - type of the function that constructs the object
/// \param ObjectTypeName - string name of the object type ("buffer", "texture", etc.)
/// \param Desc - object description
/// \param ppObject - memory address where the pointer to the created object will be stored
/// \param ConstructObject - function that constructs the object
template <typename BaseInterface>
template <typename TObjectType, typename TObjectDescType, typename TObjectConstructor>
void RenderDeviceBase<BaseInterface>::CreateDeviceObject(const Char* ObjectTypeName, const TObjectDescType& Desc, TObjectType** ppObject, TObjectConstructor ConstructObject)
{
    VERIFY(ppObject != nullptr, "Null pointer provided");
    if (!ppObject)
        return;

    VERIFY(*ppObject == nullptr, "Overwriting reference to existing object may cause memory leaks");
    // Do not release *ppObject here!
    // Should this happen, RefCntAutoPtr<> will take care of this!
    //if( *ppObject )
    //{
    //    (*ppObject)->Release();
    //    *ppObject = nullptr;
    //}

    *ppObject = nullptr;

    try
    {
        ConstructObject();
    }
    catch (const std::runtime_error&)
    {
        VERIFY(*ppObject == nullptr, "Object was created despite error");
        if (*ppObject)
        {
            (*ppObject)->Release();
            *ppObject = nullptr;
        }
        auto ObjectDescString = GetObjectDescString(Desc);
        if (ObjectDescString.length())
        {
            LOG_ERROR("Failed to create ", ObjectTypeName, " object '", (Desc.Name ? Desc.Name : ""), "'\n", ObjectDescString);
        }
        else
        {
            LOG_ERROR("Failed to create ", ObjectTypeName, " object '", (Desc.Name ? Desc.Name : ""), "'");
        }
    }
}

} // namespace Diligent
