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
/// Declaration of Diligent::QueryVkImpl class

#include <array>

#include "QueryVk.h"
#include "QueryBase.hpp"
#include "RenderDeviceVkImpl.hpp"
#include "QueryManagerVk.hpp"

namespace Diligent
{

class FixedBlockMemoryAllocator;

/// Query implementation in Vulkan backend.
class QueryVkImpl final : public QueryBase<IQueryVk, RenderDeviceVkImpl>
{
public:
    using TQueryBase = QueryBase<IQueryVk, RenderDeviceVkImpl>;

    QueryVkImpl(IReferenceCounters* pRefCounters,
                RenderDeviceVkImpl* pRendeDeviceVkImpl,
                const QueryDesc&    Desc,
                bool                IsDeviceInternal = false);
    ~QueryVkImpl();

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_QueryVk, TQueryBase);

    /// Implementation of IQuery::GetData().
    virtual bool DILIGENT_CALL_TYPE GetData(void* pData, Uint32 DataSize, bool AutoInvalidate) override final;

    /// Implementation of IQuery::Invalidate().
    virtual void DILIGENT_CALL_TYPE Invalidate() override final;

    Uint32 GetQueryPoolIndex(Uint32 QueryId) const
    {
        VERIFY_EXPR(QueryId == 0 || m_Desc.Type == QUERY_TYPE_DURATION && QueryId == 1);
        return m_QueryPoolIndex[QueryId];
    }

    bool OnEndQuery(IDeviceContext* pContext);
    bool OnBeginQuery(IDeviceContext* pContext);

private:
    bool AllocateQueries();
    void DiscardQueries();

    std::array<Uint32, 2> m_QueryPoolIndex = {QueryManagerVk::InvalidIndex, QueryManagerVk::InvalidIndex};

    Uint64 m_QueryEndFenceValue = 0;
};

} // namespace Diligent
