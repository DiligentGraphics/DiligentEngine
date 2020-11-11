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
/// Declaration of Diligent::QueryD3D11Impl class

#include <memory>

#include "QueryD3D11.h"
#include "QueryBase.hpp"
#include "RenderDeviceD3D11Impl.hpp"
#include "DisjointQueryPool.hpp"

namespace Diligent
{

class FixedBlockMemoryAllocator;

/// Query implementation in Direct3D11 backend.
class QueryD3D11Impl final : public QueryBase<IQueryD3D11, RenderDeviceD3D11Impl>
{
public:
    using TQueryBase = QueryBase<IQueryD3D11, RenderDeviceD3D11Impl>;

    QueryD3D11Impl(IReferenceCounters*    pRefCounters,
                   RenderDeviceD3D11Impl* pDevice,
                   const QueryDesc&       Desc);
    ~QueryD3D11Impl();

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_QueryD3D11, TQueryBase);

    /// Implementation of IQuery::GetData().
    virtual bool DILIGENT_CALL_TYPE GetData(void* pData, Uint32 DataSize, bool AutoInvalidate) override final;

    /// Implementation of IQuery::Invalidate().
    virtual void DILIGENT_CALL_TYPE Invalidate() override final
    {
        m_DisjointQuery.reset();
        TQueryBase::Invalidate();
    }

    /// Implementation of IQueryD3D11::GetD3D11Query().
    virtual ID3D11Query* DILIGENT_CALL_TYPE GetD3D11Query(Uint32 QueryId) override final
    {
        VERIFY_EXPR(QueryId == 0 || m_Desc.Type == QUERY_TYPE_DURATION && QueryId == 1);
        return m_pd3d11Query[QueryId];
    }

    void SetDisjointQuery(std::shared_ptr<DisjointQueryPool::DisjointQueryWrapper> DisjointQuery)
    {
        m_DisjointQuery = DisjointQuery;
    }

private:
    CComPtr<ID3D11Query> m_pd3d11Query[2];

    std::shared_ptr<DisjointQueryPool::DisjointQueryWrapper> m_DisjointQuery;
};

} // namespace Diligent
