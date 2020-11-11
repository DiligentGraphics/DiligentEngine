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

#include <functional>
#include <vector>
#include <string>

#include "../../GraphicsEngine/interface/RenderDevice.h"
#include "../../GraphicsEngine/interface/DeviceContext.h"
#include "../../GraphicsEngine/interface/Buffer.h"
#include "../../../Common/interface/RefCntAutoPtr.hpp"
#include "MapHelper.hpp"

namespace Diligent
{

struct StreamingBufferCreateInfo
{
    IRenderDevice*                pDevice = nullptr;
    BufferDesc                    BuffDesc;
    std::function<void(IBuffer*)> OnBufferResizeCallback = nullptr;
    Uint32                        NumContexts            = 1;
    bool                          AllowPersistentMapping = false;
};

class StreamingBuffer
{
public:
    StreamingBuffer() noexcept
    {}

    explicit StreamingBuffer(const StreamingBufferCreateInfo& CI) :
        m_UsePersistentMap{CI.AllowPersistentMapping && (CI.pDevice->GetDeviceCaps().IsVulkanDevice() || CI.pDevice->GetDeviceCaps().DevType == RENDER_DEVICE_TYPE_D3D12)},
        m_BufferSize{CI.BuffDesc.uiSizeInBytes},
        m_OnBufferResizeCallback{CI.OnBufferResizeCallback},
        m_MapInfo(CI.NumContexts)
    {
        VERIFY_EXPR(CI.pDevice != nullptr);
        VERIFY_EXPR(CI.BuffDesc.Usage == USAGE_DYNAMIC);
        CI.pDevice->CreateBuffer(CI.BuffDesc, nullptr, &m_pBuffer);
        VERIFY_EXPR(m_pBuffer);
        if (m_OnBufferResizeCallback)
            m_OnBufferResizeCallback(m_pBuffer);
    }

    StreamingBuffer(const StreamingBuffer&) = delete;
    StreamingBuffer& operator=(const StreamingBuffer&) = delete;

    StreamingBuffer(StreamingBuffer&&) = default;
    StreamingBuffer& operator=(StreamingBuffer&&) = default;

    ~StreamingBuffer()
    {
        for (const auto& mapInfo : m_MapInfo)
        {
            VERIFY(!mapInfo.m_MappedData, "Destroying streaming buffer that is still mapped");
        }
    }

    // Returns offset of the allocated region
    Uint32 Map(IDeviceContext* pCtx, IRenderDevice* pDevice, Uint32 Size, size_t CtxNum = 0)
    {
        VERIFY_EXPR(Size > 0);

        auto& MapInfo = m_MapInfo[CtxNum];
        // Check if there is enough space in the buffer
        if (MapInfo.m_CurrOffset + Size > m_BufferSize)
        {
            // Unmap the buffer
            Flush(CtxNum);
            VERIFY_EXPR(MapInfo.m_CurrOffset == 0);

            if (Size > m_BufferSize)
            {
                while (m_BufferSize < Size)
                    m_BufferSize *= 2;

                auto BuffDesc          = m_pBuffer->GetDesc();
                BuffDesc.uiSizeInBytes = m_BufferSize;
                // BuffDesc.Name becomes invalid after old buffer is released
                std::string Name = BuffDesc.Name;
                BuffDesc.Name    = Name.c_str();

                m_pBuffer.Release();
                pDevice->CreateBuffer(BuffDesc, nullptr, &m_pBuffer);
                if (m_OnBufferResizeCallback)
                    m_OnBufferResizeCallback(m_pBuffer);

                LOG_INFO_MESSAGE("Extended streaming buffer '", BuffDesc.Name, "' to ", m_BufferSize, " bytes");
            }
        }

        if (!m_UsePersistentMap)
        {
            VERIFY(MapInfo.m_MappedData == nullptr, "Streaming buffer must be unmapped before it can be mapped next time when persistent mapping is not used");
        }

        if (MapInfo.m_MappedData == nullptr)
        {
            // If current offset is zero, we are mapping the buffer for the first time after it has been Reseted. Use MAP_FLAG_DISCARD flag.
            // Otherwise use MAP_FLAG_NO_OVERWRITE flag.
            MapInfo.m_MappedData.Map(pCtx, m_pBuffer, MAP_WRITE, MapInfo.m_CurrOffset == 0 ? MAP_FLAG_DISCARD : MAP_FLAG_NO_OVERWRITE);
            VERIFY_EXPR(MapInfo.m_MappedData);
        }

        auto Offset = MapInfo.m_CurrOffset;
        // Update offset
        MapInfo.m_CurrOffset += Size;
        return Offset;
    }

    Uint32 Update(IDeviceContext* pCtx, IRenderDevice* pDevice, const void* pData, Uint32 Size, size_t CtxNum = 0)
    {
        VERIFY_EXPR(pData != nullptr);
        auto  Offset      = Map(pCtx, pDevice, Size, CtxNum);
        auto* pCPUAddress = reinterpret_cast<Uint8*>(GetMappedCPUAddress(CtxNum)) + Offset;
        memcpy(pCPUAddress, pData, Size);
        Unmap(CtxNum);

        return Offset;
    }

    void Unmap(size_t CtxNum = 0)
    {
        if (!m_UsePersistentMap)
        {
            m_MapInfo[CtxNum].m_MappedData.Unmap();
        }
    }

    void Flush(size_t CtxNum = 0)
    {
        m_MapInfo[CtxNum].m_MappedData.Unmap();
        m_MapInfo[CtxNum].m_CurrOffset = 0;
    }

    void Reset()
    {
        for (Uint32 ctx = 0; ctx < m_MapInfo.size(); ++ctx)
            Flush(ctx);
    }

    IBuffer* GetBuffer() const { return m_pBuffer.RawPtr<IBuffer>(); }

    void* GetMappedCPUAddress(size_t CtxNum = 0)
    {
        return m_MapInfo[CtxNum].m_MappedData;
    }

private:
    bool m_UsePersistentMap = false;

    Uint32 m_BufferSize = 0;

    RefCntAutoPtr<IBuffer> m_pBuffer;

    std::function<void(IBuffer*)> m_OnBufferResizeCallback;

    struct MapInfo
    {
        MapHelper<Uint8> m_MappedData;
        Uint32           m_CurrOffset = 0;
    };
    // We need to keep track of mapped data for every context
    std::vector<MapInfo> m_MapInfo;
};

} // namespace Diligent
