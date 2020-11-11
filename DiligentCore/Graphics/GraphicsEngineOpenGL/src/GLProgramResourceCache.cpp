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
#include "GLProgramResourceCache.hpp"

namespace Diligent
{

size_t GLProgramResourceCache::GetRequriedMemorySize(Uint32 UBCount, Uint32 SamplerCount, Uint32 ImageCount, Uint32 SSBOCount)
{
    // clang-format off
    auto MemSize = 
                sizeof(CachedUB)           * UBCount + 
                sizeof(CachedResourceView) * SamplerCount + 
                sizeof(CachedResourceView) * ImageCount + 
                sizeof(CachedSSBO)         * SSBOCount;
    // clang-format on
    return MemSize;
}

void GLProgramResourceCache::Initialize(Uint32 UBCount, Uint32 SamplerCount, Uint32 ImageCount, Uint32 SSBOCount, IMemoryAllocator& MemAllocator)
{
    // clang-format off
    m_SmplrsOffset    = static_cast<Uint16>(m_UBsOffset    + sizeof(CachedUB)           * UBCount);
    m_ImgsOffset      = static_cast<Uint16>(m_SmplrsOffset + sizeof(CachedResourceView) * SamplerCount);
    m_SSBOsOffset     = static_cast<Uint16>(m_ImgsOffset   + sizeof(CachedResourceView) * ImageCount);
    m_MemoryEndOffset = static_cast<Uint16>(m_SSBOsOffset  + sizeof(CachedSSBO)         * SSBOCount);

    VERIFY_EXPR(GetUBCount()      == static_cast<Uint32>(UBCount));
    VERIFY_EXPR(GetSamplerCount() == static_cast<Uint32>(SamplerCount));
    VERIFY_EXPR(GetImageCount()   == static_cast<Uint32>(ImageCount));
    VERIFY_EXPR(GetSSBOCount()    == static_cast<Uint32>(SSBOCount));
    // clang-format on

    VERIFY_EXPR(m_pResourceData == nullptr);
    size_t BufferSize = m_MemoryEndOffset;

    VERIFY_EXPR(BufferSize == GetRequriedMemorySize(UBCount, SamplerCount, ImageCount, SSBOCount));

#ifdef DILIGENT_DEBUG
    m_pdbgMemoryAllocator = &MemAllocator;
#endif
    if (BufferSize > 0)
    {
        m_pResourceData = ALLOCATE(MemAllocator, "Shader resource cache data buffer", Uint8, BufferSize);
        memset(m_pResourceData, 0, BufferSize);
    }

    // Explicitly construct all objects
    for (Uint32 cb = 0; cb < UBCount; ++cb)
        new (&GetUB(cb)) CachedUB;

    for (Uint32 s = 0; s < SamplerCount; ++s)
        new (&GetSampler(s)) CachedResourceView;

    for (Uint32 i = 0; i < ImageCount; ++i)
        new (&GetImage(i)) CachedResourceView;

    for (Uint32 s = 0; s < SSBOCount; ++s)
        new (&GetSSBO(s)) CachedSSBO;
}

GLProgramResourceCache::~GLProgramResourceCache()
{
    VERIFY(!IsInitialized(), "Shader resource cache memory must be released with GLProgramResourceCache::Destroy()");
}

void GLProgramResourceCache::Destroy(IMemoryAllocator& MemAllocator)
{
    if (!IsInitialized())
        return;

    VERIFY(m_pdbgMemoryAllocator == &MemAllocator, "The allocator does not match the one used to create resources");

    for (Uint32 cb = 0; cb < GetUBCount(); ++cb)
        GetUB(cb).~CachedUB();

    for (Uint32 s = 0; s < GetSamplerCount(); ++s)
        GetSampler(s).~CachedResourceView();

    for (Uint32 i = 0; i < GetImageCount(); ++i)
        GetImage(i).~CachedResourceView();

    for (Uint32 s = 0; s < GetSSBOCount(); ++s)
        GetSSBO(s).~CachedSSBO();

    if (m_pResourceData != nullptr)
        MemAllocator.Free(m_pResourceData);

    m_pResourceData   = nullptr;
    m_SmplrsOffset    = InvalidResourceOffset;
    m_ImgsOffset      = InvalidResourceOffset;
    m_SSBOsOffset     = InvalidResourceOffset;
    m_MemoryEndOffset = InvalidResourceOffset;
}

} // namespace Diligent
