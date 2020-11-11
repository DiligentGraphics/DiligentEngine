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

#include "BufferGLImpl.hpp"
#include "TextureBaseGL.hpp"
#include "SamplerGLImpl.hpp"

namespace Diligent
{

/// The class implements a cache that holds resources bound to a specific GL program
// All resources are stored in the continuous memory using the following layout:
//
//   |        Cached UBs        |     Cached Samplers     |       Cached Images      | Cached Storage Blocks     |
//   |----------------------------------------------------|--------------------------|---------------------------|
//   |  0 | 1 | ... | UBCount-1 | 0 | 1 | ...| SmpCount-1 | 0 | 1 | ... | ImgCount-1 | 0 | 1 |  ... | SBOCount-1 |
//    -----------------------------------------------------------------------------------------------------------
//
class GLProgramResourceCache
{
public:
    GLProgramResourceCache() noexcept
    {}

    ~GLProgramResourceCache();

    // clang-format off
    GLProgramResourceCache             (const GLProgramResourceCache&) = delete;
    GLProgramResourceCache& operator = (const GLProgramResourceCache&) = delete;
    GLProgramResourceCache             (GLProgramResourceCache&&)      = delete;
    GLProgramResourceCache& operator = (GLProgramResourceCache&&)      = delete;
    // clang-format on

    /// Describes a resource bound to a uniform buffer or a shader storage block slot
    struct CachedUB
    {
        /// Strong reference to the buffer
        RefCntAutoPtr<BufferGLImpl> pBuffer;
    };

    /// Describes a resource bound to a sampler or an image slot
    struct CachedResourceView
    {
        /// We keep strong reference to the view instead of the reference
        /// to the texture or buffer because this is more efficient from
        /// performance point of view: this avoids one pair of
        /// AddStrongRef()/ReleaseStrongRef(). The view holds a strong reference
        /// to the texture or the buffer, so it makes no difference.
        RefCntAutoPtr<IDeviceObject> pView;

        TextureBaseGL* pTexture = nullptr;
        union
        {
            BufferGLImpl*  pBuffer = nullptr; // When pTexture == nullptr
            SamplerGLImpl* pSampler;          // When pTexture != nullptr
        };
        CachedResourceView() noexcept {}

        void Set(RefCntAutoPtr<TextureViewGLImpl>&& pTexView, bool SetSampler)
        {
            // Do not null out pSampler as it could've been initialized by PipelineStateGLImpl::InitializeSRBResourceCache!
            // pSampler = nullptr;

            // Avoid unnecessary virtual function calls
            pTexture = pTexView ? ValidatedCast<TextureBaseGL>(pTexView->TextureViewGLImpl::GetTexture()) : nullptr;
            if (pTexView && SetSampler)
            {
                pSampler = ValidatedCast<SamplerGLImpl>(pTexView->GetSampler());
            }

            pView.Attach(pTexView.Detach());
        }

        void Set(RefCntAutoPtr<BufferViewGLImpl>&& pBufView)
        {
            pTexture = nullptr;
            // Avoid unnecessary virtual function calls
            pBuffer = pBufView ? ValidatedCast<BufferGLImpl>(pBufView->BufferViewGLImpl::GetBuffer()) : nullptr;
            pView.Attach(pBufView.Detach());
        }
    };

    struct CachedSSBO
    {
        /// Strong reference to the buffer
        RefCntAutoPtr<BufferViewGLImpl> pBufferView;
    };


    static size_t GetRequriedMemorySize(Uint32 UBCount, Uint32 SamplerCount, Uint32 ImageCount, Uint32 SSBOCount);

    void Initialize(Uint32 UBCount, Uint32 SamplerCount, Uint32 ImageCount, Uint32 SSBOCount, IMemoryAllocator& MemAllocator);
    void Destroy(IMemoryAllocator& MemAllocator);

    void SetUniformBuffer(Uint32 Binding, RefCntAutoPtr<BufferGLImpl>&& pBuff)
    {
        GetUB(Binding).pBuffer = std::move(pBuff);
    }

    void SetTexSampler(Uint32 Binding, RefCntAutoPtr<TextureViewGLImpl>&& pTexView, bool SetSampler)
    {
        GetSampler(Binding).Set(std::move(pTexView), SetSampler);
    }

    void SetImmutableSampler(Uint32 Binding, ISampler* pImtblSampler)
    {
        GetSampler(Binding).pSampler = ValidatedCast<SamplerGLImpl>(pImtblSampler);
    }

    void CopySampler(Uint32 Binding, const CachedResourceView& SrcSam)
    {
        GetSampler(Binding) = SrcSam;
    }

    void SetBufSampler(Uint32 Binding, RefCntAutoPtr<BufferViewGLImpl>&& pBuffView)
    {
        GetSampler(Binding).Set(std::move(pBuffView));
    }

    void SetTexImage(Uint32 Binding, RefCntAutoPtr<TextureViewGLImpl>&& pTexView)
    {
        GetImage(Binding).Set(std::move(pTexView), false);
    }

    void SetBufImage(Uint32 Binding, RefCntAutoPtr<BufferViewGLImpl>&& pBuffView)
    {
        GetImage(Binding).Set(std::move(pBuffView));
    }

    void CopyImage(Uint32 Binding, const CachedResourceView& SrcImg)
    {
        GetImage(Binding) = SrcImg;
    }

    void SetSSBO(Uint32 Binding, RefCntAutoPtr<BufferViewGLImpl>&& pBuffView)
    {
        GetSSBO(Binding).pBufferView = std::move(pBuffView);
    }

    bool IsUBBound(Uint32 Binding) const
    {
        if (Binding >= GetUBCount())
            return false;

        const auto& UB = GetConstUB(Binding);
        return UB.pBuffer;
    }

    bool IsSamplerBound(Uint32 Binding, bool dbgIsTextureView) const
    {
        if (Binding >= GetSamplerCount())
            return false;

        const auto& Sampler = GetConstSampler(Binding);
        VERIFY_EXPR(dbgIsTextureView || Sampler.pTexture == nullptr);
        return Sampler.pView;
    }

    bool IsImageBound(Uint32 Binding, bool dbgIsTextureView) const
    {
        if (Binding >= GetImageCount())
            return false;

        const auto& Image = GetConstImage(Binding);
        VERIFY_EXPR(dbgIsTextureView || Image.pTexture == nullptr);
        return Image.pView;
    }

    bool IsSSBOBound(Uint32 Binding) const
    {
        if (Binding >= GetSSBOCount())
            return false;

        const auto& SSBO = GetConstSSBO(Binding);
        return SSBO.pBufferView;
    }

    // clang-format off
    Uint32 GetUBCount()      const { return (m_SmplrsOffset    - m_UBsOffset)    / sizeof(CachedUB);            }
    Uint32 GetSamplerCount() const { return (m_ImgsOffset      - m_SmplrsOffset) / sizeof(CachedResourceView);  }
    Uint32 GetImageCount()   const { return (m_SSBOsOffset     - m_ImgsOffset)   / sizeof(CachedResourceView);  }
    Uint32 GetSSBOCount()    const { return (m_MemoryEndOffset - m_SSBOsOffset)  / sizeof(CachedSSBO);          }
    // clang-format on

    const CachedUB& GetConstUB(Uint32 Binding) const
    {
        VERIFY(Binding < GetUBCount(), "Uniform buffer binding (", Binding, ") is out of range");
        return reinterpret_cast<CachedUB*>(m_pResourceData + m_UBsOffset)[Binding];
    }

    const CachedResourceView& GetConstSampler(Uint32 Binding) const
    {
        VERIFY(Binding < GetSamplerCount(), "Sampler binding (", Binding, ") is out of range");
        return reinterpret_cast<CachedResourceView*>(m_pResourceData + m_SmplrsOffset)[Binding];
    }

    const CachedResourceView& GetConstImage(Uint32 Binding) const
    {
        VERIFY(Binding < GetImageCount(), "Image buffer binding (", Binding, ") is out of range");
        return reinterpret_cast<CachedResourceView*>(m_pResourceData + m_ImgsOffset)[Binding];
    }

    const CachedSSBO& GetConstSSBO(Uint32 Binding) const
    {
        VERIFY(Binding < GetSSBOCount(), "Shader storage block binding (", Binding, ") is out of range");
        return reinterpret_cast<CachedSSBO*>(m_pResourceData + m_SSBOsOffset)[Binding];
    }

    bool IsInitialized() const
    {
        return m_MemoryEndOffset != InvalidResourceOffset;
    }

private:
    CachedUB& GetUB(Uint32 Binding)
    {
        return const_cast<CachedUB&>(const_cast<const GLProgramResourceCache*>(this)->GetConstUB(Binding));
    }

    CachedResourceView& GetSampler(Uint32 Binding)
    {
        return const_cast<CachedResourceView&>(const_cast<const GLProgramResourceCache*>(this)->GetConstSampler(Binding));
    }

    CachedResourceView& GetImage(Uint32 Binding)
    {
        return const_cast<CachedResourceView&>(const_cast<const GLProgramResourceCache*>(this)->GetConstImage(Binding));
    }

    CachedSSBO& GetSSBO(Uint32 Binding)
    {
        return const_cast<CachedSSBO&>(const_cast<const GLProgramResourceCache*>(this)->GetConstSSBO(Binding));
    }

    static constexpr const Uint16 InvalidResourceOffset = 0xFFFF;
    static constexpr const Uint16 m_UBsOffset           = 0;

    Uint16 m_SmplrsOffset    = InvalidResourceOffset;
    Uint16 m_ImgsOffset      = InvalidResourceOffset;
    Uint16 m_SSBOsOffset     = InvalidResourceOffset;
    Uint16 m_MemoryEndOffset = InvalidResourceOffset;

    Uint8* m_pResourceData = nullptr;

#ifdef DILIGENT_DEBUG
    IMemoryAllocator* m_pdbgMemoryAllocator = nullptr;
#endif
};

} // namespace Diligent
