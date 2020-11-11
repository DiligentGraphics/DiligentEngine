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

#include <cstring>
#include "GraphicsTypes.h"
#include "Buffer.h"
#include "InputLayout.h"
#include "LockHelper.hpp"
#include "HashUtils.hpp"
#include "DeviceContextBase.hpp"
#include "BaseInterfacesGL.h"

namespace Diligent
{

class VAOCache
{
public:
    VAOCache();
    ~VAOCache();

    // clang-format off
    VAOCache             (const VAOCache&)  = delete;
    VAOCache             (      VAOCache&&) = delete;
    VAOCache& operator = (const VAOCache&)  = delete;
    VAOCache& operator = (      VAOCache&&) = delete;
    // clang-format on

    const GLObjectWrappers::GLVertexArrayObj& GetVAO(IPipelineState*                      pPSO,
                                                     IBuffer*                             pIndexBuffer,
                                                     VertexStreamInfo<class BufferGLImpl> VertexStreams[],
                                                     Uint32                               NumVertexStreams,
                                                     class GLContextState&                GLContextState);
    const GLObjectWrappers::GLVertexArrayObj& GetEmptyVAO();

    void OnDestroyBuffer(IBuffer* pBuffer);
    void OnDestroyPSO(IPipelineState* pPSO);

private:
    // This structure is used as the key to find VAO
    struct VAOCacheKey
    {
        VAOCacheKey(UniqueIdentifier pso_id, UniqueIdentifier ib_id) :
            // clang-format off
            PSOUId         {pso_id},
            IndexBufferUId {ib_id },
            NumUsedSlots   {0     }
        // clang-format on
        {}

        // Note that using pointers is unsafe as they may (and will) be reused:
        // pBuffer->Release();
        // pDevice->CreateBuffer(&pBuffer); // Returns same pointer

        // VAO encapsulates both input layout and all bound buffers.
        // PSO uniqly defines the layout (attrib pointers, divisors, etc.),
        // so we do not need to add individual layout elements to the key.
        // The key needs to contain all bound buffers.
        const UniqueIdentifier PSOUId;
        const UniqueIdentifier IndexBufferUId;
        Uint32                 NumUsedSlots;
        struct StreamAttribs
        {
            UniqueIdentifier BufferUId;
            Uint32           Stride;
            Uint32           Offset;
        } Streams[MAX_BUFFER_SLOTS];

        mutable size_t Hash = 0;

        bool operator==(const VAOCacheKey& Key) const
        {
            return PSOUId == Key.PSOUId &&
                IndexBufferUId == Key.IndexBufferUId &&
                NumUsedSlots == Key.NumUsedSlots &&
                std::memcmp(Streams, Key.Streams, sizeof(StreamAttribs) * NumUsedSlots) == 0;
        }
    };

    struct VAOCacheKeyHashFunc
    {
        std::size_t operator()(const VAOCacheKey& Key) const
        {
            if (Key.Hash == 0)
            {
                std::size_t Seed = 0;
                HashCombine(Seed, Key.PSOUId, Key.IndexBufferUId, Key.NumUsedSlots);
                for (Uint32 slot = 0; slot < Key.NumUsedSlots; ++slot)
                {
                    auto& CurrStream = Key.Streams[slot];
                    HashCombine(Seed, CurrStream.BufferUId);
                    HashCombine(Seed, CurrStream.Offset);
                    HashCombine(Seed, CurrStream.Stride);
                }
                Key.Hash = Seed;
            }
            return Key.Hash;
        }
    };


    friend class RenderDeviceGLImpl;
    ThreadingTools::LockFlag                                                                 m_CacheLockFlag;
    std::unordered_map<VAOCacheKey, GLObjectWrappers::GLVertexArrayObj, VAOCacheKeyHashFunc> m_Cache;

    std::unordered_multimap<const IPipelineState*, VAOCacheKey> m_PSOToKey;
    std::unordered_multimap<const IBuffer*, VAOCacheKey>        m_BuffToKey;

    // Any draw command fails if no VAO is bound. We will use this empty
    // VAO for draw commands with null input layout, such as these that
    // only use VertexID as input.
    GLObjectWrappers::GLVertexArrayObj m_EmptyVAO;
};

} // namespace Diligent
