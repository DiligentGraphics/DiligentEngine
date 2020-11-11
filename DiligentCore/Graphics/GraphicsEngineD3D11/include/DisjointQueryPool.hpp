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
/// Declaration of Diligent::DisjointQueryPool class

#include <memory>

namespace Diligent
{

class DisjointQueryPool final
{
public:
    struct DisjointQueryWrapper
    {
        DisjointQueryPool&   Pool;
        CComPtr<ID3D11Query> pd3d11Query;
        bool                 IsEnded = false;

        DisjointQueryWrapper(DisjointQueryPool&     _Pool,
                             CComPtr<ID3D11Query>&& _pd3d11Query) :
            Pool(_Pool),
            pd3d11Query(std::move(_pd3d11Query))
        {
        }

        ~DisjointQueryWrapper()
        {
            Pool.m_AvailableQueries.emplace_back(std::move(pd3d11Query));
        }

        DisjointQueryWrapper(DisjointQueryWrapper&&) = default;

        DisjointQueryWrapper(const DisjointQueryWrapper&) = delete;
        DisjointQueryWrapper& operator=(const DisjointQueryWrapper&) = delete;
        DisjointQueryWrapper& operator=(DisjointQueryWrapper&&) = delete;
    };

    std::shared_ptr<DisjointQueryWrapper> GetDisjointQuery(ID3D11Device* pd3d11Device)
    {
        CComPtr<ID3D11Query> pd3d11Query;
        if (!m_AvailableQueries.empty())
        {
            pd3d11Query = std::move(m_AvailableQueries.back());
            m_AvailableQueries.pop_back();
        }
        else
        {
            pd3d11Query = CreateQuery(pd3d11Device);
        }
        return std::shared_ptr<DisjointQueryWrapper>{new DisjointQueryWrapper{*this, std::move(pd3d11Query)}};
    }

    ~DisjointQueryPool()
    {
        LOG_INFO_MESSAGE("Disjoint query pool: created ", m_NumQueriesCreated, (m_NumQueriesCreated == 1 ? " query" : " queries"));
    }

private:
    CComPtr<ID3D11Query> CreateQuery(ID3D11Device* pd3d11Device)
    {
        D3D11_QUERY_DESC QueryDesc = {D3D11_QUERY_TIMESTAMP_DISJOINT, 0};

        CComPtr<ID3D11Query> pd3d11Query;
        pd3d11Device->CreateQuery(&QueryDesc, &pd3d11Query);
        DEV_CHECK_ERR(pd3d11Query, "Failed to create D3D11 disjoint query");
        ++m_NumQueriesCreated;
        return pd3d11Query;
    }
    std::vector<CComPtr<ID3D11Query>> m_AvailableQueries;

    Uint32 m_NumQueriesCreated = 0;
};

} // namespace Diligent
