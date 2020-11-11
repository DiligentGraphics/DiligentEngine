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
/// Implementation of the Diligent::TextureBase template class

#include <memory>

#include "Texture.h"
#include "GraphicsTypes.h"
#include "DeviceObjectBase.hpp"
#include "GraphicsAccessories.hpp"
#include "STDAllocator.hpp"
#include "FormatString.hpp"

namespace Diligent
{

struct CopyTextureAttribs;

void ValidateTextureDesc(const TextureDesc& TexDesc);
void ValidateUpdateTextureParams(const TextureDesc& TexDesc, Uint32 MipLevel, Uint32 Slice, const Box& DstBox, const TextureSubResData& SubresData);
void ValidateCopyTextureParams(const CopyTextureAttribs& CopyAttribs);
void ValidateMapTextureParams(const TextureDesc& TexDesc,
                              Uint32             MipLevel,
                              Uint32             ArraySlice,
                              MAP_TYPE           MapType,
                              Uint32             MapFlags,
                              const Box*         pMapRegion);

/// Base implementation of the ITexture interface

/// \tparam BaseInterface - base interface that this class will inheret
///                         (Diligent::ITextureD3D11, Diligent::ITextureD3D12,
///                          Diligent::ITextureGL or Diligent::ITextureVk).
/// \tparam TRenderDeviceImpl - type of the render device implementation
///                             (Diligent::RenderDeviceD3D11Impl, Diligent::RenderDeviceD3D12Impl,
///                              Diligent::RenderDeviceGLImpl, or Diligent::RenderDeviceVkImpl)
/// \tparam TTextureViewImpl - type of the texture view implementation
///                            (Diligent::TextureViewD3D11Impl, Diligent::TextureViewD3D12Impl,
///                             Diligent::TextureViewGLImpl or Diligent::TextureViewVkImpl).
/// \tparam TTexViewObjAllocator - type of the allocator that is used to allocate memory for the texture view object instances
template <class BaseInterface, class TRenderDeviceImpl, class TTextureViewImpl, class TTexViewObjAllocator>
class TextureBase : public DeviceObjectBase<BaseInterface, TRenderDeviceImpl, TextureDesc>
{
public:
    using TDeviceObjectBase = DeviceObjectBase<BaseInterface, TRenderDeviceImpl, TextureDesc>;

    /// \param pRefCounters - reference counters object that controls the lifetime of this texture.
    /// \param TexViewObjAllocator - allocator that is used to allocate memory for the instances of the texture view object.
    ///                              This parameter is only used for debug purposes.
    /// \param pDevice - pointer to the device
    /// \param Desc - texture description
    /// \param bIsDeviceInternal - flag indicating if the texture is an internal device object and
    ///							   must not keep a strong reference to the device
    TextureBase(IReferenceCounters*   pRefCounters,
                TTexViewObjAllocator& TexViewObjAllocator,
                TRenderDeviceImpl*    pDevice,
                const TextureDesc&    Desc,
                bool                  bIsDeviceInternal = false) :
        TDeviceObjectBase(pRefCounters, pDevice, Desc, bIsDeviceInternal),
#ifdef DILIGENT_DEBUG
        m_dbgTexViewObjAllocator(TexViewObjAllocator),
#endif
        m_pDefaultSRV(nullptr, STDDeleter<TTextureViewImpl, TTexViewObjAllocator>(TexViewObjAllocator)),
        m_pDefaultRTV(nullptr, STDDeleter<TTextureViewImpl, TTexViewObjAllocator>(TexViewObjAllocator)),
        m_pDefaultDSV(nullptr, STDDeleter<TTextureViewImpl, TTexViewObjAllocator>(TexViewObjAllocator)),
        m_pDefaultUAV(nullptr, STDDeleter<TTextureViewImpl, TTexViewObjAllocator>(TexViewObjAllocator))
    {
        if (this->m_Desc.MipLevels == 0)
        {
            // Compute the number of levels in the full mipmap chain
            if (this->m_Desc.Type == RESOURCE_DIM_TEX_1D ||
                this->m_Desc.Type == RESOURCE_DIM_TEX_1D_ARRAY)
            {
                this->m_Desc.MipLevels = ComputeMipLevelsCount(this->m_Desc.Width);
            }
            else if (this->m_Desc.Type == RESOURCE_DIM_TEX_2D ||
                     this->m_Desc.Type == RESOURCE_DIM_TEX_2D_ARRAY ||
                     this->m_Desc.Type == RESOURCE_DIM_TEX_CUBE ||
                     this->m_Desc.Type == RESOURCE_DIM_TEX_CUBE_ARRAY)
            {
                this->m_Desc.MipLevels = ComputeMipLevelsCount(this->m_Desc.Width, this->m_Desc.Height);
            }
            else if (this->m_Desc.Type == RESOURCE_DIM_TEX_3D)
            {
                this->m_Desc.MipLevels = ComputeMipLevelsCount(this->m_Desc.Width, this->m_Desc.Height, this->m_Desc.Depth);
            }
            else
            {
                UNEXPECTED("Unknown texture type");
            }
        }

        Uint64 DeviceQueuesMask = pDevice->GetCommandQueueMask();
        DEV_CHECK_ERR((this->m_Desc.CommandQueueMask & DeviceQueuesMask) != 0,
                      "No bits in the command queue mask (0x", std::hex, this->m_Desc.CommandQueueMask,
                      ") correspond to one of ", pDevice->GetCommandQueueCount(), " available device command queues");
        this->m_Desc.CommandQueueMask &= DeviceQueuesMask;

        if ((this->m_Desc.BindFlags & BIND_INPUT_ATTACHMENT) != 0)
            this->m_Desc.BindFlags |= BIND_SHADER_RESOURCE;

        // Validate correctness of texture description
        ValidateTextureDesc(this->m_Desc);
    }

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_Texture, TDeviceObjectBase)

    /// Implementaiton of ITexture::CreateView(); calls CreateViewInternal() virtual function that
    /// creates texture view for the specific engine implementation.
    virtual void DILIGENT_CALL_TYPE CreateView(const struct TextureViewDesc& ViewDesc, ITextureView** ppView) override
    {
        DEV_CHECK_ERR(ViewDesc.ViewType != TEXTURE_VIEW_UNDEFINED, "Texture view type is not specified");
        if (ViewDesc.ViewType == TEXTURE_VIEW_SHADER_RESOURCE)
            DEV_CHECK_ERR(this->m_Desc.BindFlags & BIND_SHADER_RESOURCE, "Attempting to create SRV for texture '", this->m_Desc.Name, "' that was not created with BIND_SHADER_RESOURCE flag");
        else if (ViewDesc.ViewType == TEXTURE_VIEW_UNORDERED_ACCESS)
            DEV_CHECK_ERR(this->m_Desc.BindFlags & BIND_UNORDERED_ACCESS, "Attempting to create UAV for texture '", this->m_Desc.Name, "' that was not created with BIND_UNORDERED_ACCESS flag");
        else if (ViewDesc.ViewType == TEXTURE_VIEW_RENDER_TARGET)
            DEV_CHECK_ERR(this->m_Desc.BindFlags & BIND_RENDER_TARGET, "Attempting to create RTV for texture '", this->m_Desc.Name, "' that was not created with BIND_RENDER_TARGET flag");
        else if (ViewDesc.ViewType == TEXTURE_VIEW_DEPTH_STENCIL)
            DEV_CHECK_ERR(this->m_Desc.BindFlags & BIND_DEPTH_STENCIL, "Attempting to create DSV for texture '", this->m_Desc.Name, "' that was not created with BIND_DEPTH_STENCIL flag");
        else
            UNEXPECTED("Unexpected texture view type");

        CreateViewInternal(ViewDesc, ppView, false);
    }

    /// Creates default texture views.

    ///
    /// - Creates default shader resource view addressing the entire texture if Diligent::BIND_SHADER_RESOURCE flag is set.
    /// - Creates default render target view addressing the most detailed mip level if Diligent::BIND_RENDER_TARGET flag is set.
    /// - Creates default depth-stencil view addressing the most detailed mip level if Diligent::BIND_DEPTH_STENCIL flag is set.
    /// - Creates default unordered access view addressing the entire texture if Diligent::BIND_UNORDERED_ACCESS flag is set.
    ///
    /// The function calls CreateViewInternal().
    void CreateDefaultViews();

    virtual void DILIGENT_CALL_TYPE SetState(RESOURCE_STATE State) override final
    {
        this->m_State = State;
    }

    virtual RESOURCE_STATE DILIGENT_CALL_TYPE GetState() const override final
    {
        return this->m_State;
    }

    bool IsInKnownState() const
    {
        return this->m_State != RESOURCE_STATE_UNKNOWN;
    }

    bool CheckState(RESOURCE_STATE State) const
    {
        VERIFY((State & (State - 1)) == 0, "Single state is expected");
        VERIFY(IsInKnownState(), "Texture state is unknown");
        return (this->m_State & State) == State;
    }

    bool CheckAnyState(RESOURCE_STATE States) const
    {
        VERIFY(IsInKnownState(), "Texture state is unknown");
        return (this->m_State & States) != 0;
    }

    /// Implementation of ITexture::GetDefaultView().
    virtual ITextureView* DILIGENT_CALL_TYPE GetDefaultView(TEXTURE_VIEW_TYPE ViewType) override
    {
        switch (ViewType)
        {
            // clang-format off
            case TEXTURE_VIEW_SHADER_RESOURCE:  return m_pDefaultSRV.get();
            case TEXTURE_VIEW_RENDER_TARGET:    return m_pDefaultRTV.get();
            case TEXTURE_VIEW_DEPTH_STENCIL:    return m_pDefaultDSV.get();
            case TEXTURE_VIEW_UNORDERED_ACCESS: return m_pDefaultUAV.get();
            // clang-format on
            default: UNEXPECTED("Unknown view type"); return nullptr;
        }
    }

protected:
    /// Pure virtual function that creates texture view for the specific engine implementation.
    virtual void CreateViewInternal(const struct TextureViewDesc& ViewDesc, ITextureView** ppView, bool bIsDefaultView) = 0;

#ifdef DILIGENT_DEBUG
    TTexViewObjAllocator& m_dbgTexViewObjAllocator;
#endif
    // WARNING! We cannot use ITextureView here, because ITextureView has no virtual dtor!
    /// Default SRV addressing the entire texture
    std::unique_ptr<TTextureViewImpl, STDDeleter<TTextureViewImpl, TTexViewObjAllocator>> m_pDefaultSRV;
    /// Default RTV addressing the most detailed mip level
    std::unique_ptr<TTextureViewImpl, STDDeleter<TTextureViewImpl, TTexViewObjAllocator>> m_pDefaultRTV;
    /// Default DSV addressing the most detailed mip level
    std::unique_ptr<TTextureViewImpl, STDDeleter<TTextureViewImpl, TTexViewObjAllocator>> m_pDefaultDSV;
    /// Default UAV addressing the entire texture
    std::unique_ptr<TTextureViewImpl, STDDeleter<TTextureViewImpl, TTexViewObjAllocator>> m_pDefaultUAV;

    void CorrectTextureViewDesc(struct TextureViewDesc& ViewDesc);

    RESOURCE_STATE m_State = RESOURCE_STATE_UNKNOWN;
};


template <class BaseInterface, class TRenderDeviceImpl, class TTextureViewImpl, class TTexViewObjAllocator>
void TextureBase<BaseInterface, TRenderDeviceImpl, TTextureViewImpl, TTexViewObjAllocator>::CorrectTextureViewDesc(struct TextureViewDesc& ViewDesc)
{
#define TEX_VIEW_VALIDATION_ERROR(...) LOG_ERROR_AND_THROW("\n                 Failed to create texture view '", (ViewDesc.Name ? ViewDesc.Name : ""), "' for texture '", this->m_Desc.Name, "': ", ##__VA_ARGS__)

    if (!(ViewDesc.ViewType > TEXTURE_VIEW_UNDEFINED && ViewDesc.ViewType < TEXTURE_VIEW_NUM_VIEWS))
        TEX_VIEW_VALIDATION_ERROR("Texture view type is not specified");

    if (ViewDesc.MostDetailedMip >= this->m_Desc.MipLevels)
        TEX_VIEW_VALIDATION_ERROR("Most detailed mip (", ViewDesc.MostDetailedMip, ") is out of range. The texture has only ", this->m_Desc.MipLevels, " mip ", (this->m_Desc.MipLevels > 1 ? "levels." : "level."));

    if (ViewDesc.NumMipLevels != REMAINING_MIP_LEVELS && ViewDesc.MostDetailedMip + ViewDesc.NumMipLevels > this->m_Desc.MipLevels)
        TEX_VIEW_VALIDATION_ERROR("Most detailed mip (", ViewDesc.MostDetailedMip, ") and number of mip levels in the view (", ViewDesc.NumMipLevels, ") is out of range. The texture has only ", this->m_Desc.MipLevels, " mip ", (this->m_Desc.MipLevels > 1 ? "levels." : "level."));

    if (ViewDesc.Format == TEX_FORMAT_UNKNOWN)
        ViewDesc.Format = GetDefaultTextureViewFormat(this->m_Desc.Format, ViewDesc.ViewType, this->m_Desc.BindFlags);

    if (ViewDesc.TextureDim == RESOURCE_DIM_UNDEFINED)
    {
        if (this->m_Desc.Type == RESOURCE_DIM_TEX_CUBE || this->m_Desc.Type == RESOURCE_DIM_TEX_CUBE_ARRAY)
        {
            switch (ViewDesc.ViewType)
            {
                case TEXTURE_VIEW_SHADER_RESOURCE:
                    ViewDesc.TextureDim = this->m_Desc.Type;
                    break;

                case TEXTURE_VIEW_RENDER_TARGET:
                case TEXTURE_VIEW_DEPTH_STENCIL:
                case TEXTURE_VIEW_UNORDERED_ACCESS:
                    ViewDesc.TextureDim = RESOURCE_DIM_TEX_2D_ARRAY;
                    break;

                default: UNEXPECTED("Unexpected view type");
            }
        }
        else
        {
            ViewDesc.TextureDim = this->m_Desc.Type;
        }
    }

    switch (this->m_Desc.Type)
    {
        case RESOURCE_DIM_TEX_1D:
            if (ViewDesc.TextureDim != RESOURCE_DIM_TEX_1D)
            {
                TEX_VIEW_VALIDATION_ERROR("Incorrect texture type for Texture 1D view: only Texture 1D is allowed");
            }
            break;

        case RESOURCE_DIM_TEX_1D_ARRAY:
            if (ViewDesc.TextureDim != RESOURCE_DIM_TEX_1D_ARRAY &&
                ViewDesc.TextureDim != RESOURCE_DIM_TEX_1D)
            {
                TEX_VIEW_VALIDATION_ERROR("Incorrect view type for Texture 1D Array: only Texture 1D or Texture 1D Array are allowed");
            }
            break;

        case RESOURCE_DIM_TEX_2D:
            if (ViewDesc.TextureDim != RESOURCE_DIM_TEX_2D_ARRAY &&
                ViewDesc.TextureDim != RESOURCE_DIM_TEX_2D)
            {
                TEX_VIEW_VALIDATION_ERROR("Incorrect texture type for Texture 2D view: only Texture 2D or Texture 2D Array are allowed");
            }
            break;

        case RESOURCE_DIM_TEX_2D_ARRAY:
            if (ViewDesc.TextureDim != RESOURCE_DIM_TEX_2D_ARRAY &&
                ViewDesc.TextureDim != RESOURCE_DIM_TEX_2D)
            {
                TEX_VIEW_VALIDATION_ERROR("Incorrect texture type for Texture 2D Array view: only Texture 2D or Texture 2D Array are allowed");
            }
            break;

        case RESOURCE_DIM_TEX_3D:
            if (ViewDesc.TextureDim != RESOURCE_DIM_TEX_3D)
            {
                TEX_VIEW_VALIDATION_ERROR("Incorrect texture type for Texture 3D view: only Texture 3D is allowed");
            }
            break;

        case RESOURCE_DIM_TEX_CUBE:
            if (ViewDesc.ViewType == TEXTURE_VIEW_SHADER_RESOURCE)
            {
                if (ViewDesc.TextureDim != RESOURCE_DIM_TEX_2D &&
                    ViewDesc.TextureDim != RESOURCE_DIM_TEX_2D_ARRAY &&
                    ViewDesc.TextureDim != RESOURCE_DIM_TEX_CUBE)
                {
                    TEX_VIEW_VALIDATION_ERROR("Incorrect texture type for Texture cube SRV: Texture 2D, Texture 2D array or Texture Cube is allowed");
                }
            }
            else
            {
                if (ViewDesc.TextureDim != RESOURCE_DIM_TEX_2D &&
                    ViewDesc.TextureDim != RESOURCE_DIM_TEX_2D_ARRAY)
                {
                    TEX_VIEW_VALIDATION_ERROR("Incorrect texture type for Texture cube non-shader resource view: Texture 2D or Texture 2D array is allowed");
                }
            }
            break;

        case RESOURCE_DIM_TEX_CUBE_ARRAY:
            if (ViewDesc.ViewType == TEXTURE_VIEW_SHADER_RESOURCE)
            {
                if (ViewDesc.TextureDim != RESOURCE_DIM_TEX_2D &&
                    ViewDesc.TextureDim != RESOURCE_DIM_TEX_2D_ARRAY &&
                    ViewDesc.TextureDim != RESOURCE_DIM_TEX_CUBE &&
                    ViewDesc.TextureDim != RESOURCE_DIM_TEX_CUBE_ARRAY)
                {
                    TEX_VIEW_VALIDATION_ERROR("Incorrect texture type for Texture cube array SRV: Texture 2D, Texture 2D array, Texture Cube or Texture Cube Array is allowed");
                }
            }
            else
            {
                if (ViewDesc.TextureDim != RESOURCE_DIM_TEX_2D &&
                    ViewDesc.TextureDim != RESOURCE_DIM_TEX_2D_ARRAY)
                {
                    TEX_VIEW_VALIDATION_ERROR("Incorrect texture type for Texture cube array non-shader resource view: Texture 2D or Texture 2D array is allowed");
                }
            }
            break;

        default:
            UNEXPECTED("Unexpected texture type");
            break;
    }

    if (ViewDesc.TextureDim == RESOURCE_DIM_TEX_CUBE)
    {
        if (ViewDesc.ViewType != TEXTURE_VIEW_SHADER_RESOURCE)
            TEX_VIEW_VALIDATION_ERROR("Unexpected view type: SRV is expected");
        if (ViewDesc.NumArraySlices != 6 && ViewDesc.NumArraySlices != 0 && ViewDesc.NumArraySlices != REMAINING_ARRAY_SLICES)
            TEX_VIEW_VALIDATION_ERROR("Texture cube SRV is expected to have 6 array slices, while ", ViewDesc.NumArraySlices, " is provided");
        if (ViewDesc.FirstArraySlice != 0)
            TEX_VIEW_VALIDATION_ERROR("First slice (", ViewDesc.FirstArraySlice, ") must be 0 for non-array texture cube SRV");
    }
    if (ViewDesc.TextureDim == RESOURCE_DIM_TEX_CUBE_ARRAY)
    {
        if (ViewDesc.ViewType != TEXTURE_VIEW_SHADER_RESOURCE)
            TEX_VIEW_VALIDATION_ERROR("Unexpected view type: SRV is expected");
        if (ViewDesc.NumArraySlices != REMAINING_ARRAY_SLICES && (ViewDesc.NumArraySlices % 6) != 0)
            TEX_VIEW_VALIDATION_ERROR("Number of slices in texture cube array SRV is expected to be multiple of 6. ", ViewDesc.NumArraySlices, " slices is provided.");
    }

    if (ViewDesc.TextureDim == RESOURCE_DIM_TEX_1D ||
        ViewDesc.TextureDim == RESOURCE_DIM_TEX_2D)
    {
        if (ViewDesc.FirstArraySlice != 0)
            TEX_VIEW_VALIDATION_ERROR("First slice (", ViewDesc.FirstArraySlice, ") must be 0 for non-array texture 1D/2D views");

        if (ViewDesc.NumArraySlices != REMAINING_ARRAY_SLICES && ViewDesc.NumArraySlices > 1)
            TEX_VIEW_VALIDATION_ERROR("Number of slices in the view (", ViewDesc.NumArraySlices, ") must be 1 (or 0) for non-array texture 1D/2D views");
    }
    else if (ViewDesc.TextureDim == RESOURCE_DIM_TEX_1D_ARRAY ||
             ViewDesc.TextureDim == RESOURCE_DIM_TEX_2D_ARRAY ||
             ViewDesc.TextureDim == RESOURCE_DIM_TEX_CUBE ||
             ViewDesc.TextureDim == RESOURCE_DIM_TEX_CUBE_ARRAY)
    {
        if (ViewDesc.FirstArraySlice >= this->m_Desc.ArraySize)
            TEX_VIEW_VALIDATION_ERROR("First array slice (", ViewDesc.FirstArraySlice, ") exceeds the number of slices in the texture array (", this->m_Desc.ArraySize, ")");

        if (ViewDesc.NumArraySlices != REMAINING_ARRAY_SLICES && ViewDesc.FirstArraySlice + ViewDesc.NumArraySlices > this->m_Desc.ArraySize)
            TEX_VIEW_VALIDATION_ERROR("First slice (", ViewDesc.FirstArraySlice, ") and number of slices in the view (", ViewDesc.NumArraySlices, ") specify more slices than target texture has (", this->m_Desc.ArraySize, ")");
    }
    else if (ViewDesc.TextureDim == RESOURCE_DIM_TEX_3D)
    {
        auto MipDepth = this->m_Desc.Depth >> ViewDesc.MostDetailedMip;
        if (ViewDesc.FirstDepthSlice + ViewDesc.NumDepthSlices > MipDepth)
            TEX_VIEW_VALIDATION_ERROR("First slice (", ViewDesc.FirstDepthSlice, ") and number of slices in the view (", ViewDesc.NumDepthSlices, ") specify more slices than target 3D texture mip level has (", MipDepth, ")");
    }
    else
    {
        UNEXPECTED("Unexpected texture dimension");
    }

    if (GetTextureFormatAttribs(ViewDesc.Format).IsTypeless)
    {
        TEX_VIEW_VALIDATION_ERROR("Texture view format (", GetTextureFormatAttribs(ViewDesc.Format).Name, ") cannot be typeless");
    }

    if ((ViewDesc.Flags & TEXTURE_VIEW_FLAG_ALLOW_MIP_MAP_GENERATION) != 0)
    {
        if ((this->m_Desc.MiscFlags & MISC_TEXTURE_FLAG_GENERATE_MIPS) == 0)
            TEX_VIEW_VALIDATION_ERROR("TEXTURE_VIEW_FLAG_ALLOW_MIP_MAP_GENERATION flag can only set if the texture was created with MISC_TEXTURE_FLAG_GENERATE_MIPS flag");

        if (ViewDesc.ViewType != TEXTURE_VIEW_SHADER_RESOURCE)
            TEX_VIEW_VALIDATION_ERROR("TEXTURE_VIEW_FLAG_ALLOW_MIP_MAP_GENERATION flag can only be used with TEXTURE_VIEW_SHADER_RESOURCE view type");
    }

#undef TEX_VIEW_VALIDATION_ERROR

    if (ViewDesc.NumMipLevels == 0 || ViewDesc.NumMipLevels == REMAINING_MIP_LEVELS)
    {
        if (ViewDesc.ViewType == TEXTURE_VIEW_SHADER_RESOURCE)
            ViewDesc.NumMipLevels = this->m_Desc.MipLevels - ViewDesc.MostDetailedMip;
        else
            ViewDesc.NumMipLevels = 1;
    }

    if (ViewDesc.NumArraySlices == 0 || ViewDesc.NumArraySlices == REMAINING_ARRAY_SLICES)
    {
        if (ViewDesc.TextureDim == RESOURCE_DIM_TEX_1D_ARRAY ||
            ViewDesc.TextureDim == RESOURCE_DIM_TEX_2D_ARRAY ||
            ViewDesc.TextureDim == RESOURCE_DIM_TEX_CUBE ||
            ViewDesc.TextureDim == RESOURCE_DIM_TEX_CUBE_ARRAY)
            ViewDesc.NumArraySlices = this->m_Desc.ArraySize - ViewDesc.FirstArraySlice;
        else if (ViewDesc.TextureDim == RESOURCE_DIM_TEX_3D)
        {
            auto MipDepth           = this->m_Desc.Depth >> ViewDesc.MostDetailedMip;
            ViewDesc.NumDepthSlices = MipDepth - ViewDesc.FirstDepthSlice;
        }
        else
            ViewDesc.NumArraySlices = 1;
    }

    if ((ViewDesc.ViewType == TEXTURE_VIEW_RENDER_TARGET) &&
        (ViewDesc.Format == TEX_FORMAT_R8_SNORM || ViewDesc.Format == TEX_FORMAT_RG8_SNORM || ViewDesc.Format == TEX_FORMAT_RGBA8_SNORM ||
         ViewDesc.Format == TEX_FORMAT_R16_SNORM || ViewDesc.Format == TEX_FORMAT_RG16_SNORM || ViewDesc.Format == TEX_FORMAT_RGBA16_SNORM))
    {
        const auto* FmtName = GetTextureFormatAttribs(ViewDesc.Format).Name;
        LOG_WARNING_MESSAGE(FmtName, " render target view is created.\n"
                                     "There might be an issue in OpenGL driver on NVidia hardware: when rendering to SNORM textures, all negative values are clamped to zero.\n"
                                     "Use UNORM format instead.");
    }
}

template <class BaseInterface, class TRenderDeviceImpl, class TTextureViewImpl, class TTexViewObjAllocator>
void TextureBase<BaseInterface, TRenderDeviceImpl, TTextureViewImpl, TTexViewObjAllocator>::CreateDefaultViews()
{
    const auto& TexFmtAttribs = GetTextureFormatAttribs(this->m_Desc.Format);
    if (TexFmtAttribs.ComponentType == COMPONENT_TYPE_UNDEFINED)
    {
        // Cannot create default view for TYPELESS formats
        return;
    }

    if (this->m_Desc.BindFlags & BIND_SHADER_RESOURCE)
    {
        TextureViewDesc ViewDesc;
        ViewDesc.ViewType = TEXTURE_VIEW_SHADER_RESOURCE;
        auto ViewName     = FormatString("Default SRV of texture '", this->m_Desc.Name, "'");
        ViewDesc.Name     = ViewName.c_str();

        ITextureView* pSRV = nullptr;
        if ((this->m_Desc.MiscFlags & MISC_TEXTURE_FLAG_GENERATE_MIPS) != 0)
            ViewDesc.Flags |= TEXTURE_VIEW_FLAG_ALLOW_MIP_MAP_GENERATION;
        CreateViewInternal(ViewDesc, &pSRV, true);
        m_pDefaultSRV.reset(static_cast<TTextureViewImpl*>(pSRV));
        VERIFY(m_pDefaultSRV->GetDesc().ViewType == TEXTURE_VIEW_SHADER_RESOURCE, "Unexpected view type");
    }

    if (this->m_Desc.BindFlags & BIND_RENDER_TARGET)
    {
        TextureViewDesc ViewDesc;
        ViewDesc.ViewType = TEXTURE_VIEW_RENDER_TARGET;
        auto ViewName     = FormatString("Default RTV of texture '", this->m_Desc.Name, "'");
        ViewDesc.Name     = ViewName.c_str();

        ITextureView* pRTV = nullptr;
        CreateViewInternal(ViewDesc, &pRTV, true);
        m_pDefaultRTV.reset(static_cast<TTextureViewImpl*>(pRTV));
        VERIFY(m_pDefaultRTV->GetDesc().ViewType == TEXTURE_VIEW_RENDER_TARGET, "Unexpected view type");
    }

    if (this->m_Desc.BindFlags & BIND_DEPTH_STENCIL)
    {
        TextureViewDesc ViewDesc;
        ViewDesc.ViewType = TEXTURE_VIEW_DEPTH_STENCIL;
        auto ViewName     = FormatString("Default DSV of texture '", this->m_Desc.Name, "'");
        ViewDesc.Name     = ViewName.c_str();

        ITextureView* pDSV = nullptr;
        CreateViewInternal(ViewDesc, &pDSV, true);
        m_pDefaultDSV.reset(static_cast<TTextureViewImpl*>(pDSV));
        VERIFY(m_pDefaultDSV->GetDesc().ViewType == TEXTURE_VIEW_DEPTH_STENCIL, "Unexpected view type");
    }

    if (this->m_Desc.BindFlags & BIND_UNORDERED_ACCESS)
    {
        TextureViewDesc ViewDesc;
        ViewDesc.ViewType = TEXTURE_VIEW_UNORDERED_ACCESS;
        auto ViewName     = FormatString("Default UAV of texture '", this->m_Desc.Name, "'");
        ViewDesc.Name     = ViewName.c_str();

        ViewDesc.AccessFlags = UAV_ACCESS_FLAG_READ_WRITE;
        ITextureView* pUAV   = nullptr;
        CreateViewInternal(ViewDesc, &pUAV, true);
        m_pDefaultUAV.reset(static_cast<TTextureViewImpl*>(pUAV));
        VERIFY(m_pDefaultUAV->GetDesc().ViewType == TEXTURE_VIEW_UNORDERED_ACCESS, "Unexpected view type");
    }
}

} // namespace Diligent
