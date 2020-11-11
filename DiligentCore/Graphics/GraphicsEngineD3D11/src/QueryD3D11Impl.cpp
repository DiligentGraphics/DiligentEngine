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

#include "QueryD3D11Impl.hpp"
#include "DeviceContextD3D11Impl.hpp"

namespace Diligent
{

QueryD3D11Impl::QueryD3D11Impl(IReferenceCounters*    pRefCounters,
                               RenderDeviceD3D11Impl* pDevice,
                               const QueryDesc&       Desc) :
    TQueryBase{pRefCounters, pDevice, Desc}
{
    D3D11_QUERY_DESC d3d11QueryDesc = {};
    switch (Desc.Type)
    {
        case QUERY_TYPE_UNDEFINED:
            LOG_ERROR_AND_THROW("Query type is undefined");

        case QUERY_TYPE_OCCLUSION:
            d3d11QueryDesc.Query = D3D11_QUERY_OCCLUSION;
            break;

        case QUERY_TYPE_BINARY_OCCLUSION:
            d3d11QueryDesc.Query = D3D11_QUERY_OCCLUSION_PREDICATE;
            break;

        case QUERY_TYPE_TIMESTAMP:
        case QUERY_TYPE_DURATION:
            d3d11QueryDesc.Query = D3D11_QUERY_TIMESTAMP;
            break;

        case QUERY_TYPE_PIPELINE_STATISTICS:
            d3d11QueryDesc.Query = D3D11_QUERY_PIPELINE_STATISTICS;
            break;

        default:
            UNEXPECTED("Unexpected query type");
    }
    auto* pd3d11Device = pDevice->GetD3D11Device();

    for (Uint32 i = 0; i < (Desc.Type == QUERY_TYPE_DURATION ? Uint32{2} : Uint32{1}); ++i)
    {
        auto hr = pd3d11Device->CreateQuery(&d3d11QueryDesc, &m_pd3d11Query[i]);
        CHECK_D3D_RESULT_THROW(hr, "Failed to create D3D11 query object");
        VERIFY_EXPR(m_pd3d11Query[i] != nullptr);
    }
}

QueryD3D11Impl::~QueryD3D11Impl()
{
}

bool QueryD3D11Impl::GetData(void* pData, Uint32 DataSize, bool AutoInvalidate)
{
    if (!TQueryBase::CheckQueryDataPtr(pData, DataSize))
        return false;

    auto* pCtxD3D11Impl = m_pContext.RawPtr<DeviceContextD3D11Impl>();
    VERIFY_EXPR(!pCtxD3D11Impl->IsDeferred());

    auto* pd3d11Ctx = pCtxD3D11Impl->GetD3D11DeviceContext();

    bool DataReady = false;
    switch (m_Desc.Type)
    {
        case QUERY_TYPE_OCCLUSION:
        {
            UINT64 NumSamples;
            DataReady = pd3d11Ctx->GetData(m_pd3d11Query[0], &NumSamples, sizeof(NumSamples), 0) == S_OK;
            if (DataReady && pData != nullptr)
            {
                auto& QueryData      = *reinterpret_cast<QueryDataOcclusion*>(pData);
                QueryData.NumSamples = NumSamples;
            }
        }
        break;

        case QUERY_TYPE_BINARY_OCCLUSION:
        {
            BOOL AnySamplePassed;
            DataReady = pd3d11Ctx->GetData(m_pd3d11Query[0], &AnySamplePassed, sizeof(AnySamplePassed), 0) == S_OK;
            if (DataReady && pData != nullptr)
            {
                auto& QueryData           = *reinterpret_cast<QueryDataBinaryOcclusion*>(pData);
                QueryData.AnySamplePassed = AnySamplePassed != FALSE;
            }
        }
        break;

        case QUERY_TYPE_TIMESTAMP:
        {
            // Timestamp query is only useful if two timestamp queries are done in the middle of a D3D11_QUERY_TIMESTAMP_DISJOINT query.
            // Timestamp disjoint query is begun by the device context when the first timestamp query is begun and ended
            // by FinishFrame. Thus timestamp queries will only become available after FinishFrame.

            VERIFY_EXPR(m_DisjointQuery);

            DataReady = m_DisjointQuery->IsEnded;
            if (DataReady)
            {
                UINT64 Counter = 0;
                DataReady      = pd3d11Ctx->GetData(m_pd3d11Query[0], &Counter, sizeof(Counter), 0) == S_OK;

                // Note: DataReady is a return value, so we query the counter first, and then check pData for null.
                if (DataReady && pData != nullptr)
                {
                    D3D11_QUERY_DATA_TIMESTAMP_DISJOINT DisjointQueryData;
                    DataReady = pd3d11Ctx->GetData(m_DisjointQuery->pd3d11Query, &DisjointQueryData, sizeof(DisjointQueryData), 0) == S_OK;

                    if (DataReady)
                    {
                        auto& QueryData   = *reinterpret_cast<QueryDataTimestamp*>(pData);
                        QueryData.Counter = Counter;
                        // The timestamp returned by ID3D11DeviceContext::GetData for a timestamp query is only reliable if Disjoint is FALSE.
                        QueryData.Frequency = DisjointQueryData.Disjoint ? 0 : DisjointQueryData.Frequency;
                    }
                }
            }
        }
        break;

        case QUERY_TYPE_PIPELINE_STATISTICS:
        {
            D3D11_QUERY_DATA_PIPELINE_STATISTICS d3d11QueryData;
            DataReady = pd3d11Ctx->GetData(m_pd3d11Query[0], &d3d11QueryData, sizeof(d3d11QueryData), 0) == S_OK;
            if (DataReady && pData != nullptr)
            {
                auto& QueryData = *reinterpret_cast<QueryDataPipelineStatistics*>(pData);

                QueryData.InputVertices       = d3d11QueryData.IAVertices;
                QueryData.InputPrimitives     = d3d11QueryData.IAPrimitives;
                QueryData.GSPrimitives        = d3d11QueryData.GSPrimitives;
                QueryData.ClippingInvocations = d3d11QueryData.CInvocations;
                QueryData.ClippingPrimitives  = d3d11QueryData.CPrimitives;
                QueryData.VSInvocations       = d3d11QueryData.VSInvocations;
                QueryData.GSInvocations       = d3d11QueryData.GSInvocations;
                QueryData.PSInvocations       = d3d11QueryData.PSInvocations;
                QueryData.HSInvocations       = d3d11QueryData.HSInvocations;
                QueryData.DSInvocations       = d3d11QueryData.DSInvocations;
                QueryData.CSInvocations       = d3d11QueryData.CSInvocations;
            }
        }
        break;

        case QUERY_TYPE_DURATION:
        {
            // Timestamp query is only useful if two timestamp queries are done in the middle of a D3D11_QUERY_TIMESTAMP_DISJOINT query.
            // Timestamp disjoint query is begun by the device context when the first timestamp query is begun and ended
            // by FinishFrame. Thus timestamp queries will only become available after FinishFrame.

            VERIFY_EXPR(m_DisjointQuery);

            DataReady = m_DisjointQuery->IsEnded;
            if (DataReady)
            {
                UINT64 StartCounter = 0;
                UINT64 EndCounter   = 0;

                DataReady = pd3d11Ctx->GetData(m_pd3d11Query[0], &StartCounter, sizeof(StartCounter), 0) == S_OK;
                if (DataReady)
                {
                    DataReady = pd3d11Ctx->GetData(m_pd3d11Query[1], &EndCounter, sizeof(EndCounter), 0) == S_OK;

                    // Note: DataReady is a return value, so we query the counters first, and then check pData for null.
                    if (DataReady && pData != nullptr)
                    {
                        D3D11_QUERY_DATA_TIMESTAMP_DISJOINT DisjointQueryData;
                        DataReady = pd3d11Ctx->GetData(m_DisjointQuery->pd3d11Query, &DisjointQueryData, sizeof(DisjointQueryData), 0) == S_OK;

                        if (DataReady)
                        {
                            auto& QueryData = *reinterpret_cast<QueryDataDuration*>(pData);
                            VERIFY_EXPR(EndCounter >= StartCounter);
                            QueryData.Duration = EndCounter - StartCounter;
                            // The timestamp returned by ID3D11DeviceContext::GetData for a timestamp query is only reliable if Disjoint is FALSE.
                            QueryData.Frequency = DisjointQueryData.Disjoint ? 0 : DisjointQueryData.Frequency;
                        }
                    }
                }
            }
        }
        break;

        default:
            UNEXPECTED("Unexpected query type");
    }

    if (DataReady && pData != nullptr && AutoInvalidate)
    {
        Invalidate();
    }

    return DataReady;
}

} // namespace Diligent
