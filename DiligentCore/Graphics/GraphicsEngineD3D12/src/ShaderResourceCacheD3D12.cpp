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

#include "ShaderResourceCacheD3D12.hpp"
#include "BufferD3D12Impl.hpp"

namespace Diligent
{

size_t ShaderResourceCacheD3D12::GetRequiredMemorySize(Uint32 NumTables,
                                                       Uint32 TableSizes[])
{
    size_t MemorySize = NumTables * sizeof(RootTable);
    for (Uint32 t = 0; t < NumTables; ++t)
        MemorySize += TableSizes[t] * sizeof(Resource);
    return MemorySize;
}

// http://diligentgraphics.com/diligent-engine/architecture/d3d12/shader-resource-cache#Cache-Structure
void ShaderResourceCacheD3D12::Initialize(IMemoryAllocator& MemAllocator, Uint32 NumTables, Uint32 TableSizes[])
{
    // Memory layout:
    //                                         __________________________________________________________
    //  m_pMemory                             |             m_pResources, m_NumResources                 |
    //  |                                     |                                                          |
    //  V                                     |                                                          V
    //  |  RootTable[0]  |   ....    |  RootTable[Nrt-1]  |  Res[0]  |  ... |  Res[n-1]  |    ....     | Res[0]  |  ... |  Res[m-1]  |
    //       |                                                A
    //       |                                                |
    //       |________________________________________________|
    //                    m_pResources, m_NumResources
    //

    VERIFY(m_pAllocator == nullptr && m_pMemory == nullptr, "Cache already initialized");
    m_pAllocator          = &MemAllocator;
    m_NumTables           = NumTables;
    Uint32 TotalResources = 0;
    for (Uint32 t = 0; t < NumTables; ++t)
        TotalResources += TableSizes[t];
    auto MemorySize = NumTables * sizeof(RootTable) + TotalResources * sizeof(Resource);
    VERIFY_EXPR(MemorySize == GetRequiredMemorySize(NumTables, TableSizes));
    if (MemorySize > 0)
    {
        m_pMemory         = ALLOCATE_RAW(*m_pAllocator, "Memory for shader resource cache data", MemorySize);
        auto* pTables     = reinterpret_cast<RootTable*>(m_pMemory);
        auto* pCurrResPtr = reinterpret_cast<Resource*>(pTables + m_NumTables);
        for (Uint32 res = 0; res < TotalResources; ++res)
            new (pCurrResPtr + res) Resource();

        for (Uint32 t = 0; t < NumTables; ++t)
        {
            new (&GetRootTable(t)) RootTable(TableSizes[t], TableSizes[t] > 0 ? pCurrResPtr : nullptr);
            pCurrResPtr += TableSizes[t];
        }
        VERIFY_EXPR((char*)pCurrResPtr == (char*)m_pMemory + MemorySize);
    }
}

ShaderResourceCacheD3D12::~ShaderResourceCacheD3D12()
{
    if (m_pMemory)
    {
        Uint32 TotalResources = 0;
        for (Uint32 t = 0; t < m_NumTables; ++t)
            TotalResources += GetRootTable(t).GetSize();
        auto* pResources = reinterpret_cast<Resource*>(reinterpret_cast<RootTable*>(m_pMemory) + m_NumTables);
        for (Uint32 res = 0; res < TotalResources; ++res)
            pResources[res].~Resource();
        for (Uint32 t = 0; t < m_NumTables; ++t)
            GetRootTable(t).~RootTable();

        m_pAllocator->Free(m_pMemory);
    }
}

#ifdef DILIGENT_DEBUG
void ShaderResourceCacheD3D12::DbgVerifyBoundDynamicCBsCounter() const
{
    Uint32 NumDynamicCBsBound = 0;
    for (Uint32 t = 0; t < m_NumTables; ++t)
    {
        const auto& RT = GetRootTable(t);
        for (Uint32 res = 0; res < RT.GetSize(); ++res)
        {
            const auto& Res = RT.GetResource(res);
            if (Res.Type == CachedResourceType::CBV && Res.pObject && Res.pObject.RawPtr<const BufferD3D12Impl>()->GetDesc().Usage == USAGE_DYNAMIC)
                ++NumDynamicCBsBound;
        }
    }
    VERIFY(NumDynamicCBsBound == m_NumDynamicCBsBound, "The number of dynamic CBs bound is invalid");
}
#endif

} // namespace Diligent
