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
/// Implementation of Diligent::QueryBase template class

#include "Query.h"
#include "DeviceObjectBase.hpp"
#include "GraphicsTypes.h"
#include "RefCntAutoPtr.hpp"

namespace Diligent
{

/// Template class implementing base functionality for a Query object

/// \tparam BaseInterface - base interface that this class will inheret
///                         (Diligent::IQueryD3D11, Diligent::IQueryD3D12,
///                          Diligent::IQueryGL or Diligent::IQueryVk).
/// \tparam RenderDeviceImplType - type of the render device implementation
template <class BaseInterface, class RenderDeviceImplType>
class QueryBase : public DeviceObjectBase<BaseInterface, RenderDeviceImplType, QueryDesc>
{
public:
    enum class QueryState
    {
        Inactive,
        Querying,
        Ended
    };

    using TDeviceObjectBase = DeviceObjectBase<BaseInterface, RenderDeviceImplType, QueryDesc>;

    /// \param pRefCounters      - reference counters object that controls the lifetime of this command list.
    /// \param pDevice           - pointer to the device.
    /// \param Desc              - Query description
    /// \param bIsDeviceInternal - flag indicating if the Query is an internal device object and
    ///							   must not keep a strong reference to the device.
    QueryBase(IReferenceCounters*   pRefCounters,
              RenderDeviceImplType* pDevice,
              const QueryDesc&      Desc,
              bool                  bIsDeviceInternal = false) :
        TDeviceObjectBase{pRefCounters, pDevice, Desc, bIsDeviceInternal}
    {
        const auto& deviceFeatures = pDevice->GetDeviceCaps().Features;
        static_assert(QUERY_TYPE_NUM_TYPES == 6, "Not all QUERY_TYPE enum values are handled below");
        switch (Desc.Type)
        {
            case QUERY_TYPE_OCCLUSION:
                if (!deviceFeatures.OcclusionQueries)
                    LOG_ERROR_AND_THROW("Occlusion queries are not supported by this device");
                break;

            case QUERY_TYPE_BINARY_OCCLUSION:
                if (!deviceFeatures.BinaryOcclusionQueries)
                    LOG_ERROR_AND_THROW("Binary occlusion queries are not supported by this device");
                break;

            case QUERY_TYPE_TIMESTAMP:
                if (!deviceFeatures.TimestampQueries)
                    LOG_ERROR_AND_THROW("Timestamp queries are not supported by this device");
                break;

            case QUERY_TYPE_PIPELINE_STATISTICS:
                if (!deviceFeatures.PipelineStatisticsQueries)
                    LOG_ERROR_AND_THROW("Pipeline statistics queries are not supported by this device");
                break;

            case QUERY_TYPE_DURATION:
                if (!deviceFeatures.DurationQueries)
                    LOG_ERROR_AND_THROW("Duration queries are not supported by this device");
                break;

            default:
                UNEXPECTED("Unexpected device type");
        }
    }

    ~QueryBase()
    {
        if (m_State == QueryState::Querying)
        {
            LOG_ERROR_MESSAGE("Destroying query '", this->m_Desc.Name,
                              "' that is in querying state. End the query before releasing it.");
        }
    }

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_Query, TDeviceObjectBase)

    virtual void DILIGENT_CALL_TYPE Invalidate() override
    {
        m_State = QueryState::Inactive;
    }

    bool OnBeginQuery(struct IDeviceContext* pContext)
    {
        if (this->m_Desc.Type == QUERY_TYPE_TIMESTAMP)
        {
            LOG_ERROR_MESSAGE("BeginQuery cannot be called on timestamp query '", this->m_Desc.Name,
                              "'. Call EndQuery to set the timestamp.");
            return false;
        }

        if (m_State == QueryState::Querying)
        {
            LOG_ERROR_MESSAGE("Attempting to begin query '", this->m_Desc.Name,
                              "' twice. A query must be ended before it can be begun again.");
            return false;
        }

        m_pContext = pContext;
        m_State    = QueryState::Querying;
        return true;
    }

    bool OnEndQuery(IDeviceContext* pContext)
    {
        if (this->m_Desc.Type != QUERY_TYPE_TIMESTAMP)
        {
            if (m_State != QueryState::Querying)
            {
                LOG_ERROR_MESSAGE("Attempting to end query '", this->m_Desc.Name, "' that has not been begun");
                return false;
            }
        }

        if (m_pContext == nullptr)
        {
            if (this->m_Desc.Type != QUERY_TYPE_TIMESTAMP)
            {
                LOG_ERROR_MESSAGE("Ending query '", this->m_Desc.Name, "' that has not been begun");
                return false;
            }

            m_pContext = pContext;
        }
        else if (m_pContext != pContext)
        {
            LOG_ERROR_MESSAGE("Query '", this->m_Desc.Name, "' has been begun by another context");
            return false;
        }

        m_State = QueryState::Ended;
        return true;
    }

    QueryState GetState() const
    {
        return m_State;
    }

    bool CheckQueryDataPtr(void* pData, Uint32 DataSize)
    {
        if (m_State != QueryState::Ended)
        {
            LOG_ERROR_MESSAGE("Attempting to get data of query '", this->m_Desc.Name, "' that has not been ended");
            return false;
        }

        if (pData != nullptr)
        {
            if (*reinterpret_cast<QUERY_TYPE*>(pData) != this->m_Desc.Type)
            {
                LOG_ERROR_MESSAGE("Incorrect query data structure type.");
                return false;
            }

            static_assert(QUERY_TYPE_NUM_TYPES == 6, "Not all QUERY_TYPE enum values are handled below");
            switch (this->m_Desc.Type)
            {
                case QUERY_TYPE_UNDEFINED:
                    UNEXPECTED("Undefined query type is unexpected");
                    return false;

                case QUERY_TYPE_OCCLUSION:
                    if (DataSize != sizeof(QueryDataOcclusion))
                    {
                        LOG_ERROR_MESSAGE("The size of query data (", DataSize, ") is incorrect: ", sizeof(QueryDataOcclusion), " (aka sizeof(QueryDataOcclusion)) is expected");
                        return false;
                    }
                    break;

                case QUERY_TYPE_BINARY_OCCLUSION:
                    if (DataSize != sizeof(QueryDataBinaryOcclusion))
                    {
                        LOG_ERROR_MESSAGE("The size of query data (", DataSize, ") is incorrect: ", sizeof(QueryDataBinaryOcclusion), " (aka sizeof(QueryDataBinaryOcclusion)) is expected");
                        return false;
                    }
                    break;

                case QUERY_TYPE_TIMESTAMP:
                    if (DataSize != sizeof(QueryDataTimestamp))
                    {
                        LOG_ERROR_MESSAGE("The size of query data (", DataSize, ") is incorrect: ", sizeof(QueryDataTimestamp), " (aka sizeof(QueryDataTimestamp)) is expected");
                        return false;
                    }
                    break;

                case QUERY_TYPE_PIPELINE_STATISTICS:
                    if (DataSize != sizeof(QueryDataPipelineStatistics))
                    {
                        LOG_ERROR_MESSAGE("The size of query data (", DataSize, ") is incorrect: ", sizeof(QueryDataPipelineStatistics), " (aka sizeof(QueryDataPipelineStatistics)) is expected");
                        return false;
                    }
                    break;

                case QUERY_TYPE_DURATION:
                    if (DataSize != sizeof(QueryDataDuration))
                    {
                        LOG_ERROR_MESSAGE("The size of query data (", DataSize, ") is incorrect: ", sizeof(QueryDataDuration), " (aka sizeof(QueryDataDuration)) is expected");
                        return false;
                    }
                    break;

                default:
                    UNEXPECTED("Unexpected query type");
                    return false;
            }
        }

        return true;
    }

protected:
    RefCntAutoPtr<IDeviceContext> m_pContext;

    QueryState m_State = QueryState::Inactive;
};

} // namespace Diligent
