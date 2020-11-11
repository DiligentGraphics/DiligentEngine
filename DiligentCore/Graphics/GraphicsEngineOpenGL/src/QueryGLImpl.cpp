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

#include "QueryGLImpl.hpp"
#include "EngineMemory.h"

namespace Diligent
{

QueryGLImpl::QueryGLImpl(IReferenceCounters* pRefCounters,
                         RenderDeviceGLImpl* pDevice,
                         const QueryDesc&    Desc) :
    // clang-format off
    TQueryBase
    {
        pRefCounters,
        pDevice,
        Desc
    },
    m_GlQuery{true}
// clang-format on
{
}

QueryGLImpl::~QueryGLImpl()
{
}

bool QueryGLImpl::GetData(void* pData, Uint32 DataSize, bool AutoInvalidate)
{
    if (!TQueryBase::CheckQueryDataPtr(pData, DataSize))
        return false;

    GLuint ResultAvailable = GL_FALSE;

    switch (m_Desc.Type)
    {
#if GL_SAMPLES_PASSED
        case QUERY_TYPE_OCCLUSION:
#endif

        case QUERY_TYPE_BINARY_OCCLUSION:

#if GL_PRIMITIVES_GENERATED
        case QUERY_TYPE_PIPELINE_STATISTICS:
#endif

        case QUERY_TYPE_DURATION:
        case QUERY_TYPE_TIMESTAMP:
            glGetQueryObjectuiv(m_GlQuery, GL_QUERY_RESULT_AVAILABLE, &ResultAvailable);
            CHECK_GL_ERROR("Failed to get query result");
            break;

        default:
            return false;
    }

    if (ResultAvailable && pData != nullptr)
    {
        switch (m_Desc.Type)
        {
            case QUERY_TYPE_OCCLUSION:
            {
                auto& QueryData = *reinterpret_cast<QueryDataOcclusion*>(pData);

                GLuint SamplesPassed = 0;
                glGetQueryObjectuiv(m_GlQuery, GL_QUERY_RESULT, &SamplesPassed);
                CHECK_GL_ERROR("Failed to get query result");
                QueryData.NumSamples = SamplesPassed;
            }
            break;

            case QUERY_TYPE_BINARY_OCCLUSION:
            {
                auto& QueryData = *reinterpret_cast<QueryDataBinaryOcclusion*>(pData);

                GLuint AnySamplePassed = 0;
                glGetQueryObjectuiv(m_GlQuery, GL_QUERY_RESULT, &AnySamplePassed);
                CHECK_GL_ERROR("Failed to get query result");
                QueryData.AnySamplePassed = AnySamplePassed != 0;
            }
            break;

            case QUERY_TYPE_PIPELINE_STATISTICS:
            {
                auto& QueryData = *reinterpret_cast<QueryDataPipelineStatistics*>(pData);

                GLuint PrimitivesGenerated = 0;
                glGetQueryObjectuiv(m_GlQuery, GL_QUERY_RESULT, &PrimitivesGenerated);
                CHECK_GL_ERROR("Failed to get query result");
                QueryData.ClippingInvocations = PrimitivesGenerated;
            }
            break;

            case QUERY_TYPE_TIMESTAMP:
            case QUERY_TYPE_DURATION:
            {
                if (glGetQueryObjectui64v != nullptr)
                {
                    GLuint64 Counter = 0;
                    glGetQueryObjectui64v(m_GlQuery, GL_QUERY_RESULT, &Counter);
                    CHECK_GL_ERROR("Failed to get query result");
                    if (m_Desc.Type == QUERY_TYPE_TIMESTAMP)
                    {
                        auto& QueryData   = *reinterpret_cast<QueryDataTimestamp*>(pData);
                        QueryData.Counter = Counter;
                        // Counter is always measured in nanoseconds (10^-9 seconds)
                        QueryData.Frequency = 1000000000;
                    }
                    else
                    {
                        VERIFY_EXPR(m_Desc.Type == QUERY_TYPE_DURATION);
                        auto& QueryData    = *reinterpret_cast<QueryDataDuration*>(pData);
                        QueryData.Duration = Counter;
                        // Counter is always measured in nanoseconds (10^-9 seconds)
                        QueryData.Frequency = 1000000000;
                    }
                }
            }
            break;

            default:
                UNEXPECTED("Unexpected query type");
                return false;
        }

        if (AutoInvalidate)
        {
            Invalidate();
        }
    }

    return ResultAvailable != GL_FALSE;
}

} // namespace Diligent
