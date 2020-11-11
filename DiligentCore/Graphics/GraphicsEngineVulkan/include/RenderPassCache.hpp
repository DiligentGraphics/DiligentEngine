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
/// Declaration of Diligent::RenderPassCache class

#include <unordered_map>
#include <mutex>
#include "GraphicsTypes.h"
#include "Constants.h"
#include "HashUtils.hpp"
#include "VulkanUtilities/VulkanObjectWrappers.hpp"
#include "RefCntAutoPtr.hpp"

namespace Diligent
{

class RenderDeviceVkImpl;
class RenderPassVkImpl;

class RenderPassCache
{
public:
    RenderPassCache(RenderDeviceVkImpl& DeviceVk) noexcept;

    // clang-format off
    RenderPassCache             (const RenderPassCache&) = delete;
    RenderPassCache             (RenderPassCache&&)      = delete;
    RenderPassCache& operator = (const RenderPassCache&) = delete;
    RenderPassCache& operator = (RenderPassCache&&)      = delete;
    // clang-format on

    ~RenderPassCache();

    // This structure is used as the key to find framebuffer
    struct RenderPassCacheKey
    {
        // clang-format off
        RenderPassCacheKey() : 
            NumRenderTargets{0},
            SampleCount     {0},
            DSVFormat       {TEX_FORMAT_UNKNOWN}
        {}
        // clang-format on

        RenderPassCacheKey(Uint32               _NumRenderTargets,
                           Uint32               _SampleCount,
                           const TEXTURE_FORMAT _RTVFormats[],
                           TEXTURE_FORMAT       _DSVFormat) :
            // clang-format off
            NumRenderTargets{static_cast<decltype(NumRenderTargets)>(_NumRenderTargets)},
            SampleCount     {static_cast<decltype(SampleCount)>     (_SampleCount)     },
            DSVFormat       {_DSVFormat                                                }
        // clang-format on
        {
            VERIFY_EXPR(_NumRenderTargets <= std::numeric_limits<decltype(NumRenderTargets)>::max());
            VERIFY_EXPR(_SampleCount <= std::numeric_limits<decltype(SampleCount)>::max());
            for (Uint32 rt = 0; rt < NumRenderTargets; ++rt)
                RTVFormats[rt] = _RTVFormats[rt];
        }
        // Default memeber initialization is intentionally omitted
        Uint8          NumRenderTargets;
        Uint8          SampleCount;
        TEXTURE_FORMAT DSVFormat;
        TEXTURE_FORMAT RTVFormats[MAX_RENDER_TARGETS];

        bool operator==(const RenderPassCacheKey& rhs) const
        {
            // clang-format off
            if (GetHash()        != rhs.GetHash()        ||
                NumRenderTargets != rhs.NumRenderTargets ||
                SampleCount      != rhs.SampleCount      ||
                DSVFormat        != rhs.DSVFormat)
            {
                return false;
            }
            // clang-format on

            for (Uint32 rt = 0; rt < NumRenderTargets; ++rt)
                if (RTVFormats[rt] != rhs.RTVFormats[rt])
                    return false;

            return true;
        }

        size_t GetHash() const
        {
            if (Hash == 0)
            {
                Hash = ComputeHash(NumRenderTargets, SampleCount, DSVFormat);
                for (Uint32 rt = 0; rt < NumRenderTargets; ++rt)
                    HashCombine(Hash, RTVFormats[rt]);
            }
            return Hash;
        }

    private:
        mutable size_t Hash = 0;
    };

    RenderPassVkImpl* GetRenderPass(const RenderPassCacheKey& Key);

    void Destroy();

private:
    struct RenderPassCacheKeyHash
    {
        std::size_t operator()(const RenderPassCacheKey& Key) const
        {
            return Key.GetHash();
        }
    };

    RenderDeviceVkImpl& m_DeviceVkImpl;

    std::mutex                                                                                      m_Mutex;
    std::unordered_map<RenderPassCacheKey, RefCntAutoPtr<RenderPassVkImpl>, RenderPassCacheKeyHash> m_Cache;
};

} // namespace Diligent
