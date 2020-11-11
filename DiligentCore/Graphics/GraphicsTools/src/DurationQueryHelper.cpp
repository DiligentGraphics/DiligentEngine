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
#include "DurationQueryHelper.hpp"

namespace Diligent
{

DurationQueryHelper::DurationQuery::DurationQuery(IRenderDevice* pDevice)
{
    QueryDesc queryDesc{QUERY_TYPE_TIMESTAMP};

    queryDesc.Name = "Duration start timestamp query";
    pDevice->CreateQuery(queryDesc, &StartTimestamp);
    VERIFY(StartTimestamp, "Failed to create start query");

    queryDesc.Name = "Duration end timestamp query";
    pDevice->CreateQuery(queryDesc, &EndTimestamp);
    VERIFY(EndTimestamp, "Failed to create end query");
}

DurationQueryHelper::DurationQueryHelper(IRenderDevice* pDevice,
                                         Uint32         NumQueriesToReserve,
                                         Uint32         ExpectedQueryLimit) :
    m_pDevice{pDevice},
    m_ExpectedQueryLimit{ExpectedQueryLimit}
{
    m_AvailableQueries.reserve(NumQueriesToReserve);
    for (Uint32 i = 0; i < NumQueriesToReserve; ++i)
    {
        m_AvailableQueries.emplace_back(pDevice);
    }
}

void DurationQueryHelper::Begin(IDeviceContext* pCtx)
{
    if (m_AvailableQueries.empty())
    {
        m_AvailableQueries.emplace_back(m_pDevice);
    }

    auto DurationQuery = std::move(m_AvailableQueries.back());
    m_AvailableQueries.pop_back();

    pCtx->EndQuery(DurationQuery.StartTimestamp);
    m_PendingQueries.push_front(std::move(DurationQuery));
}

bool DurationQueryHelper::End(IDeviceContext* pCtx, double& Duration)
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

    pCtx->EndQuery(m_PendingQueries.front().EndTimestamp);

    auto& LastQuery = m_PendingQueries.back();

    QueryDataTimestamp StartTimestampData;

    // Do not invalidate the query until we also get end timestamp
    auto DataAvailable = LastQuery.StartTimestamp->GetData(&StartTimestampData, sizeof(StartTimestampData), false);
    if (DataAvailable)
    {
        QueryDataTimestamp EndTimestampData;
        DataAvailable = LastQuery.EndTimestamp->GetData(&EndTimestampData, sizeof(EndTimestampData));
        if (DataAvailable)
        {
            LastQuery.StartTimestamp->Invalidate();
            Duration =
                static_cast<double>(EndTimestampData.Counter) / static_cast<double>(EndTimestampData.Frequency) -
                static_cast<double>(StartTimestampData.Counter) / static_cast<double>(StartTimestampData.Frequency);
        }

        m_AvailableQueries.emplace_back(std::move(LastQuery));
        m_PendingQueries.pop_back();
    }

    return DataAvailable;
}

} // namespace Diligent
