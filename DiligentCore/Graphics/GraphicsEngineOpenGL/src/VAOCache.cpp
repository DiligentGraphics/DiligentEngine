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

#include "VAOCache.hpp"
#include "RenderDeviceGLImpl.hpp"
#include "GLObjectWrapper.hpp"
#include "BufferGLImpl.hpp"
#include "GLTypeConversions.hpp"
#include "GLContextState.hpp"
#include "PipelineStateGLImpl.hpp"

namespace Diligent
{

VAOCache::VAOCache() :
    m_EmptyVAO{true}
{
    m_Cache.max_load_factor(0.5f);
    m_PSOToKey.max_load_factor(0.5f);
    m_BuffToKey.max_load_factor(0.5f);
}

VAOCache::~VAOCache()
{
    VERIFY(m_Cache.empty(), "VAO cache is not empty. Are there any unreleased objects?");
    VERIFY(m_PSOToKey.empty(), "PSOToKey hash is not empty");
    VERIFY(m_BuffToKey.empty(), "BuffToKey hash is not empty");
}

void VAOCache::OnDestroyBuffer(IBuffer* pBuffer)
{
    ThreadingTools::LockHelper CacheLock(m_CacheLockFlag);

    auto EqualRange = m_BuffToKey.equal_range(pBuffer);
    for (auto It = EqualRange.first; It != EqualRange.second; ++It)
    {
        m_Cache.erase(It->second);
    }
    m_BuffToKey.erase(EqualRange.first, EqualRange.second);
}

void VAOCache::OnDestroyPSO(IPipelineState* pPSO)
{
    ThreadingTools::LockHelper CacheLock(m_CacheLockFlag);

    auto EqualRange = m_PSOToKey.equal_range(pPSO);
    for (auto It = EqualRange.first; It != EqualRange.second; ++It)
    {
        m_Cache.erase(It->second);
    }
    m_PSOToKey.erase(EqualRange.first, EqualRange.second);
}

const GLObjectWrappers::GLVertexArrayObj& VAOCache::GetVAO(IPipelineState*                pPSO,
                                                           IBuffer*                       pIndexBuffer,
                                                           VertexStreamInfo<BufferGLImpl> VertexStreams[],
                                                           Uint32                         NumVertexStreams,
                                                           GLContextState&                GLState)
{
    // Lock the cache
    ThreadingTools::LockHelper CacheLock{m_CacheLockFlag};

    BufferGLImpl* VertexBuffers[MAX_BUFFER_SLOTS];
    for (Uint32 s = 0; s < NumVertexStreams; ++s)
        VertexBuffers[s] = nullptr;

    // Get layout
    auto*                pPSOGL         = ValidatedCast<PipelineStateGLImpl>(pPSO);
    auto*                pIndexBufferGL = ValidatedCast<BufferGLImpl>(pIndexBuffer);
    const auto&          InputLayout    = pPSOGL->GetGraphicsPipelineDesc().InputLayout;
    const LayoutElement* LayoutElems    = InputLayout.LayoutElements;
    Uint32               NumElems       = InputLayout.NumElements;
    // Construct the key
    VAOCacheKey Key(pPSOGL->GetUniqueID(), pIndexBufferGL ? pIndexBufferGL->GetUniqueID() : 0);

    {
        auto LayoutIt = LayoutElems;
        for (size_t Elem = 0; Elem < NumElems; ++Elem, ++LayoutIt)
        {
            auto BuffSlot = LayoutIt->BufferSlot;
            if (BuffSlot >= NumVertexStreams)
            {
                UNEXPECTED("Input layout requires more buffers than bound to the pipeline");
                continue;
            }
            if (BuffSlot >= MAX_BUFFER_SLOTS)
            {
                VERIFY(BuffSlot >= MAX_BUFFER_SLOTS, "Incorrect input slot");
                continue;
            }
            auto MaxUsedSlot = std::max(Key.NumUsedSlots, BuffSlot + 1);
            for (Uint32 s = Key.NumUsedSlots; s < MaxUsedSlot; ++s)
                Key.Streams[s] = VAOCacheKey::StreamAttribs{};
            Key.NumUsedSlots = MaxUsedSlot;

            auto& CurrStream    = VertexStreams[BuffSlot];
            auto  Stride        = pPSOGL->GetBufferStride(BuffSlot);
            auto& pCurrBuf      = VertexBuffers[BuffSlot];
            auto& CurrStreamKey = Key.Streams[BuffSlot];
            if (pCurrBuf == nullptr)
            {
                pCurrBuf = CurrStream.pBuffer;
                VERIFY(pCurrBuf != nullptr, "No buffer bound to slot ", BuffSlot);

                ValidatedCast<BufferGLImpl>(pCurrBuf)->BufferMemoryBarrier(
                    GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT, // Vertex data sourced from buffer objects after the barrier
                                                        // will reflect data written by shaders prior to the barrier.
                                                        // The set of buffer objects affected by this bit is derived
                                                        // from the GL_VERTEX_ARRAY_BUFFER_BINDING bindings
                    GLState);

                CurrStreamKey.BufferUId = pCurrBuf ? pCurrBuf->GetUniqueID() : 0;
                CurrStreamKey.Stride    = Stride;
                CurrStreamKey.Offset    = CurrStream.Offset;
            }
            else
            {
                VERIFY(pCurrBuf == CurrStream.pBuffer, "Buffer no longer exists");
                VERIFY(CurrStreamKey.Stride == Stride, "Unexpected buffer stride");
                VERIFY(CurrStreamKey.Offset == CurrStream.Offset, "Unexpected buffer offset");
            }
        }
    }

    if (pIndexBuffer)
    {
        pIndexBufferGL->BufferMemoryBarrier(
            GL_ELEMENT_ARRAY_BARRIER_BIT, // Vertex array indices sourced from buffer objects after the barrier
                                          // will reflect data written by shaders prior to the barrier.
                                          // The buffer objects affected by this bit are derived from the
                                          // ELEMENT_ARRAY_BUFFER binding.
            GLState);
    }

    // Try to find VAO in the map
    auto It = m_Cache.find(Key);
    if (It != m_Cache.end())
    {
        return It->second;
    }
    else
    {
        // Create new VAO
        GLObjectWrappers::GLVertexArrayObj NewVAO(true);

        // Initialize VAO
        GLState.BindVAO(NewVAO);
        auto LayoutIt = LayoutElems;
        for (size_t Elem = 0; Elem < NumElems; ++Elem, ++LayoutIt)
        {
            auto BuffSlot = LayoutIt->BufferSlot;
            if (BuffSlot >= NumVertexStreams || BuffSlot >= MAX_BUFFER_SLOTS)
            {
                UNEXPECTED("Incorrect input buffer slot");
                continue;
            }
            // Get buffer through the strong reference. Note that we are not
            // using pointers stored in the key for safety
            auto& CurrStream = VertexStreams[BuffSlot];
            auto  Stride     = pPSOGL->GetBufferStride(BuffSlot);
            auto* pBuff      = VertexBuffers[BuffSlot];
            VERIFY(pBuff != nullptr, "Vertex buffer is null");
            const BufferGLImpl* pBufferOGL = static_cast<const BufferGLImpl*>(pBuff);

            constexpr bool ResetVAO = false;
            GLState.BindBuffer(GL_ARRAY_BUFFER, pBufferOGL->m_GlBuffer, ResetVAO);
            GLvoid* DataStartOffset = reinterpret_cast<GLvoid*>(static_cast<size_t>(CurrStream.Offset) + static_cast<size_t>(LayoutIt->RelativeOffset));
            auto    GlType          = TypeToGLType(LayoutIt->ValueType);
            if (!LayoutIt->IsNormalized &&
                (LayoutIt->ValueType == VT_INT8 ||
                 LayoutIt->ValueType == VT_INT16 ||
                 LayoutIt->ValueType == VT_INT32 ||
                 LayoutIt->ValueType == VT_UINT8 ||
                 LayoutIt->ValueType == VT_UINT16 ||
                 LayoutIt->ValueType == VT_UINT32))
                glVertexAttribIPointer(LayoutIt->InputIndex, LayoutIt->NumComponents, GlType, Stride, DataStartOffset);
            else
                glVertexAttribPointer(LayoutIt->InputIndex, LayoutIt->NumComponents, GlType, LayoutIt->IsNormalized, Stride, DataStartOffset);

            if (LayoutIt->Frequency == INPUT_ELEMENT_FREQUENCY_PER_INSTANCE)
            {
                // If divisor is zero, then the attribute acts like normal, being indexed by the array or index
                // buffer. If divisor is non-zero, then the current instance is divided by this divisor, and
                // the result of that is used to access the attribute array.
                glVertexAttribDivisor(LayoutIt->InputIndex, LayoutIt->InstanceDataStepRate);
            }
            glEnableVertexAttribArray(LayoutIt->InputIndex);
        }
        if (pIndexBuffer)
        {
            const BufferGLImpl* pIndBufferOGL = static_cast<const BufferGLImpl*>(pIndexBuffer);
            constexpr bool      ResetVAO      = false;
            GLState.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, pIndBufferOGL->m_GlBuffer, ResetVAO);
        }

        auto NewElems = m_Cache.emplace(std::make_pair(Key, std::move(NewVAO)));
        // New element must be actually inserted
        VERIFY(NewElems.second, "New element was not inserted into the cache");
        m_PSOToKey.insert(std::make_pair(pPSO, Key));
        for (Uint32 Slot = 0; Slot < Key.NumUsedSlots; ++Slot)
        {
            auto* pCurrBuff = VertexBuffers[Slot];
            if (pCurrBuff)
                m_BuffToKey.insert(std::make_pair(pCurrBuff, Key));
        }

        return NewElems.first->second;
    }
}

const GLObjectWrappers::GLVertexArrayObj& VAOCache::GetEmptyVAO()
{
    return m_EmptyVAO;
}

} // namespace Diligent
