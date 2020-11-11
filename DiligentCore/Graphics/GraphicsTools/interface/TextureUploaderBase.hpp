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

#include <vector>

#include "TextureUploader.hpp"
#include "../../../Common/interface/ObjectBase.hpp"
#include "../../../Common/interface/HashUtils.hpp"
#include "../../../Common/interface/RefCntAutoPtr.hpp"

namespace std
{

template <>
struct hash<Diligent::UploadBufferDesc>
{
    size_t operator()(const Diligent::UploadBufferDesc& Desc) const
    {
        return Diligent::ComputeHash(Desc.Width, Desc.Height, Desc.Depth, static_cast<Diligent::Int32>(Desc.Format));
    }
};

} // namespace std

namespace Diligent
{
class UploadBufferBase : public ObjectBase<IUploadBuffer>
{
public:
    UploadBufferBase(IReferenceCounters* pRefCounters, const UploadBufferDesc& Desc) :
        // clang-format off
        ObjectBase<IUploadBuffer>{pRefCounters},
        m_Desc                   {Desc},
        m_MappedData             (m_Desc.ArraySize * m_Desc.MipLevels)
    // clang-format on
    {
    }

    virtual MappedTextureSubresource GetMappedData(Uint32 Mip, Uint32 Slice) override final
    {
        VERIFY_EXPR(Mip < m_Desc.MipLevels && Slice < m_Desc.ArraySize);
        return m_MappedData[m_Desc.MipLevels * Slice + Mip];
    }
    virtual const UploadBufferDesc& GetDesc() const override final { return m_Desc; }

    void SetMappedData(Uint32 Mip, Uint32 Slice, const MappedTextureSubresource& MappedData)
    {
        VERIFY_EXPR(Mip < m_Desc.MipLevels && Slice < m_Desc.ArraySize);
        m_MappedData[m_Desc.MipLevels * Slice + Mip] = MappedData;
    }

    bool IsMapped(Uint32 Mip, Uint32 Slice) const
    {
        VERIFY_EXPR(Mip < m_Desc.MipLevels && Slice < m_Desc.ArraySize);
        return m_MappedData[m_Desc.MipLevels * Slice + Mip].pData != nullptr;
    }

    void Reset()
    {
        for (auto& MappedData : m_MappedData)
            MappedData = MappedTextureSubresource{};
    }

protected:
    const UploadBufferDesc                m_Desc;
    std::vector<MappedTextureSubresource> m_MappedData;
};

class TextureUploaderBase : public ObjectBase<ITextureUploader>
{
public:
    TextureUploaderBase(IReferenceCounters* pRefCounters, IRenderDevice* pDevice, const TextureUploaderDesc Desc) :
        ObjectBase<ITextureUploader>{pRefCounters},
        m_pDevice{pDevice}
    {}

protected:
    RefCntAutoPtr<IRenderDevice> m_pDevice;
};

} // namespace Diligent
