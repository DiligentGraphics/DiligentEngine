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
#include "ScopedQueryHelper.hpp"

namespace Diligent
{

ScopedQueryHelper::ScopedQueryHelper(IRenderDevice*   pDevice,
                                     const QueryDesc& queryDesc,
                                     Uint32           NumQueriesToReserve,
                                     Uint32           ExpectedQueryLimit) :
    m_pDevice{pDevice},
    m_QueryDesc{queryDesc},
    m_ExpectedQueryLimit{ExpectedQueryLimit}
{
    VERIFY(queryDesc.Type != QUERY_TYPE_TIMESTAMP, "Scoped query type is expected");
    m_AvailableQueries.resize(NumQueriesToReserve);
    for (Uint32 i = 0; i < NumQueriesToReserve; ++i)
    {
        m_pDevice->CreateQuery(queryDesc, &m_AvailableQueries[i]);
        VERIFY(m_AvailableQueries[i], "Failed to create query");
    }
}

void ScopedQueryHelper::Begin(IDeviceContext* pCtx)
{
    if (m_AvailableQueries.empty())
    {
        m_AvailableQueries.resize(1);
        m_pDevice->CreateQuery(m_QueryDesc, &m_AvailableQueries.front());
    }

    auto pQuery = std::move(m_AvailableQueries.back());
    m_AvailableQueries.pop_back();

    pCtx->BeginQuery(pQuery);
    m_PendingQueries.push_front(std::move(pQuery));
}

bool ScopedQueryHelper::End(IDeviceContext* pCtx, void* pData, Uint32 DataSize)
{
    if (m_PendingQueries.empty())
    {
        LOG_ERROR_MESSAGE("There are no pending queries, which likely indicates incosistent Begin()/End() calls");
        return false;
    }

    if (m_PendingQueries.size() > m_ExpectedQueryLimit)
    {
        LOG_WARNING_MESSAGE("There are ", m_PendingQueries.size(), " pending queries which exceeds the specified expected limit (", m_ExpectedQueryLimit, ")");
    }

    pCtx->EndQuery(m_PendingQueries.front());

    auto& pLastQuery    = m_PendingQueries.back();
    auto  DataAvailable = pLastQuery->GetData(pData, DataSize);
    if (DataAvailable)
    {
        m_AvailableQueries.emplace_back(std::move(pLastQuery));
        m_PendingQueries.pop_back();
    }

    return DataAvailable;
}

} // namespace Diligent
