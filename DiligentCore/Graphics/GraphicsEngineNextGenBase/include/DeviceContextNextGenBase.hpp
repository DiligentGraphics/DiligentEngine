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

#include "Atomics.hpp"
#include "BasicTypes.h"
#include "ReferenceCounters.h"
#include "RefCntAutoPtr.hpp"
#include "DeviceContextBase.hpp"
#include "RefCntAutoPtr.hpp"

namespace Diligent
{

/// Base implementation of the device context for next-generation backends.

template <typename BaseInterface, typename ImplementationTraits>
class DeviceContextNextGenBase : public DeviceContextBase<BaseInterface, ImplementationTraits>
{
public:
    using TBase             = DeviceContextBase<BaseInterface, ImplementationTraits>;
    using DeviceImplType    = typename ImplementationTraits::DeviceType;
    using ICommandQueueType = typename ImplementationTraits::ICommandQueueType;

    DeviceContextNextGenBase(IReferenceCounters* pRefCounters,
                             DeviceImplType*     pRenderDevice,
                             Uint32              ContextId,
                             Uint32              CommandQueueId,
                             Uint32              NumCommandsToFlush,
                             bool                bIsDeferred) :
        // clang-format off
        TBase{pRefCounters, pRenderDevice, bIsDeferred},
        m_ContextId                   {ContextId         },
        m_CommandQueueId              {CommandQueueId    },
        m_NumCommandsToFlush          {NumCommandsToFlush},
        m_ContextFrameNumber          {0},
        m_SubmittedBuffersCmdQueueMask{bIsDeferred ? 0 : Uint64{1} << Uint64{CommandQueueId}}
    // clang-format on
    {
    }

    ~DeviceContextNextGenBase()
    {
    }

    virtual ICommandQueueType* DILIGENT_CALL_TYPE LockCommandQueue() override final
    {
        if (this->m_bIsDeferred)
        {
            LOG_WARNING_MESSAGE("Deferred contexts have no associated command queues");
            return nullptr;
        }
        return this->m_pDevice->LockCommandQueue(m_CommandQueueId);
    }

    virtual void DILIGENT_CALL_TYPE UnlockCommandQueue() override final
    {
        if (this->m_bIsDeferred)
        {
            LOG_WARNING_MESSAGE("Deferred contexts have no associated command queues");
            return;
        }
        this->m_pDevice->UnlockCommandQueue(m_CommandQueueId);
    }

    Uint32 GetCommandQueueId() const
    {
        return m_CommandQueueId;
    }

protected:
    // Should be called at the end of FinishFrame()
    void EndFrame()
    {
        if (this->m_bIsDeferred)
        {
            // For deferred context, reset submitted cmd queue mask
            m_SubmittedBuffersCmdQueueMask = 0;
        }
        else
        {
            this->m_pDevice->FlushStaleResources(m_CommandQueueId);
        }
        Atomics::AtomicIncrement(m_ContextFrameNumber);
    }

    const Uint32         m_ContextId;
    const Uint32         m_CommandQueueId;
    const Uint32         m_NumCommandsToFlush;
    Atomics::AtomicInt64 m_ContextFrameNumber;

    // This mask indicates which command queues command buffers from this context were submitted to.
    // For immediate context, this will always be 1 << m_CommandQueueId.
    // For deferred contexts, this will accumulate bits of the queues to which command buffers
    // were submitted to before FinishFrame() was called. This mask is used to release resources
    // allocated by the context during the frame when FinishFrame() is called.
    Uint64 m_SubmittedBuffersCmdQueueMask = 0;
};

} // namespace Diligent
