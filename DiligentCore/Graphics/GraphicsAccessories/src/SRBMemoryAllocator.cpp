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

#include "SRBMemoryAllocator.hpp"

namespace Diligent
{

SRBMemoryAllocator::~SRBMemoryAllocator()
{
    if (m_DataAllocators != nullptr)
    {
        auto TotalAllocatorCount = m_ShaderVariableDataAllocatorCount + m_ResourceCacheDataAllocatorCount;
        for (Uint32 s = 0; s < TotalAllocatorCount; ++s)
        {
            m_DataAllocators[s].~FixedBlockMemoryAllocator();
        }
        m_RawMemAllocator.Free(m_DataAllocators);
    }
}

void SRBMemoryAllocator::Initialize(Uint32              SRBAllocationGranularity,
                                    Uint32              ShaderVariableDataAllocatorCount,
                                    const size_t* const ShaderVariableDataSizes,
                                    Uint32              ResourceCacheDataAllocatorCount,
                                    const size_t* const ResourceCacheDataSizes)
{
    VERIFY_EXPR(SRBAllocationGranularity > 1);
    VERIFY(m_DataAllocators == nullptr && m_ShaderVariableDataAllocatorCount == 0 && m_ResourceCacheDataAllocatorCount == 0, "Allocator is already initialized");

    m_ShaderVariableDataAllocatorCount = ShaderVariableDataAllocatorCount;
    m_ResourceCacheDataAllocatorCount  = ResourceCacheDataAllocatorCount;
    auto TotalAllocatorCount           = m_ShaderVariableDataAllocatorCount + m_ResourceCacheDataAllocatorCount;

    if (TotalAllocatorCount == 0)
        return;

    auto* pAllocatorsRawMem = m_RawMemAllocator.Allocate(
        sizeof(FixedBlockMemoryAllocator) * TotalAllocatorCount,
        "Raw memory for SRBMemoryAllocator::m_ShaderVariableDataAllocators",
        __FILE__, __LINE__);
    m_DataAllocators = reinterpret_cast<FixedBlockMemoryAllocator*>(pAllocatorsRawMem);

    for (Uint32 s = 0; s < TotalAllocatorCount; ++s)
    {
        auto size = s < ShaderVariableDataAllocatorCount ? ShaderVariableDataSizes[s] : ResourceCacheDataSizes[s - ShaderVariableDataAllocatorCount];
        new (m_DataAllocators + s) FixedBlockMemoryAllocator(GetRawAllocator(), size, SRBAllocationGranularity);
    }
}

} // namespace Diligent
