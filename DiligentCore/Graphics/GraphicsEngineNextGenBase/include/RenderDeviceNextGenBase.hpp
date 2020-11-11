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

#include <vector>
#include <mutex>

#include "EngineFactory.h"
#include "Atomics.hpp"
#include "BasicTypes.h"
#include "ReferenceCounters.h"
#include "MemoryAllocator.h"
#include "RefCntAutoPtr.hpp"
#include "PlatformMisc.hpp"
#include "ResourceReleaseQueue.hpp"
#include "EngineMemory.h"

namespace Diligent
{

/// Base implementation of the render device for next-generation backends.

template <class TBase, typename CommandQueueType>
class RenderDeviceNextGenBase : public TBase
{
public:
    using typename TBase::DeviceObjectSizes;

    RenderDeviceNextGenBase(IReferenceCounters*      pRefCounters,
                            IMemoryAllocator&        RawMemAllocator,
                            IEngineFactory*          pEngineFactory,
                            size_t                   CmdQueueCount,
                            CommandQueueType**       Queues,
                            Uint32                   NumDeferredContexts,
                            const DeviceObjectSizes& ObjectSizes) :
        TBase{pRefCounters, RawMemAllocator, pEngineFactory, NumDeferredContexts, ObjectSizes},
        m_CmdQueueCount{CmdQueueCount}
    {
        m_CommandQueues = ALLOCATE(this->m_RawMemAllocator, "Raw memory for the device command/release queues", CommandQueue, m_CmdQueueCount);
        for (size_t q = 0; q < m_CmdQueueCount; ++q)
            new (m_CommandQueues + q) CommandQueue(RefCntAutoPtr<CommandQueueType>(Queues[q]), this->m_RawMemAllocator);
    }

    ~RenderDeviceNextGenBase()
    {
        DestroyCommandQueues();
    }

    // The following basic requirement guarantees correctness of resource deallocation:
    //
    //        A resource is never released before the last draw command referencing it is submitted to the command queue
    //

    //
    // CPU
    //                       Last Reference
    //                        of resource X
    //                             |
    //                             |     Submit Cmd       Submit Cmd            Submit Cmd
    //                             |      List N           List N+1              List N+2
    //                             V         |                |                     |
    //    NextFenceValue       |   *  N      |      N+1       |          N+2        |
    //
    //
    //    CompletedFenceValue       |     N-3      |      N-2      |        N-1        |        N       |
    //                              .              .               .                   .                .
    // -----------------------------.--------------.---------------.-------------------.----------------.-------------
    //                              .              .               .                   .                .
    //
    // GPU                          | Cmd List N-2 | Cmd List N-1  |    Cmd List N     |   Cmd List N+1 |
    //                                                                                 |
    //                                                                                 |
    //                                                                          Resource X can
    //                                                                           be released
    template <typename ObjectType, typename = typename std::enable_if<std::is_object<ObjectType>::value>::type>
    void SafeReleaseDeviceObject(ObjectType&& Object, Uint64 QueueMask)
    {
        VERIFY(m_CommandQueues != nullptr, "Command queues have been destroyed. Are you releasing an object from the render device destructor?");

        QueueMask &= GetCommandQueueMask();

        VERIFY(QueueMask != 0, "At least one bit should be set in the command queue mask");
        if (QueueMask == 0)
            return;

        Atomics::Long NumReferences = PlatformMisc::CountOneBits(QueueMask);
        auto          Wrapper       = DynamicStaleResourceWrapper::Create(std::move(Object), NumReferences);

        while (QueueMask != 0)
        {
            auto QueueIndex = PlatformMisc::GetLSB(QueueMask);
            VERIFY_EXPR(QueueIndex < m_CmdQueueCount);

            auto& Queue = m_CommandQueues[QueueIndex];
            // Do not use std::move on wrapper!!!
            Queue.ReleaseQueue.SafeReleaseResource(Wrapper, Queue.NextCmdBufferNumber);
            QueueMask &= ~(Uint64{1} << Uint64{QueueIndex});
            --NumReferences;
        }
        VERIFY_EXPR(NumReferences == 0);

        Wrapper.GiveUpOwnership();
    }

    size_t GetCommandQueueCount() const
    {
        return m_CmdQueueCount;
    }

    Uint64 GetCommandQueueMask() const
    {
        return (m_CmdQueueCount < 64) ? ((Uint64{1} << Uint64{m_CmdQueueCount}) - 1) : ~Uint64{0};
    }

    void PurgeReleaseQueues(bool ForceRelease = false)
    {
        for (Uint32 q = 0; q < m_CmdQueueCount; ++q)
            PurgeReleaseQueue(q, ForceRelease);
    }

    void PurgeReleaseQueue(Uint32 QueueIndex, bool ForceRelease = false)
    {
        VERIFY_EXPR(QueueIndex < m_CmdQueueCount);
        auto& Queue               = m_CommandQueues[QueueIndex];
        auto  CompletedFenceValue = ForceRelease ? std::numeric_limits<Uint64>::max() : Queue.CmdQueue->GetCompletedFenceValue();
        Queue.ReleaseQueue.Purge(CompletedFenceValue);
    }

    void IdleCommandQueue(size_t QueueIdx, bool ReleaseResources)
    {
        VERIFY_EXPR(QueueIdx < m_CmdQueueCount);
        auto& Queue = m_CommandQueues[QueueIdx];

        Uint64 CmdBufferNumber = 0;
        Uint64 FenceValue      = 0;
        {
            std::lock_guard<std::mutex> Lock{Queue.Mtx};

            if (ReleaseResources)
            {
                // Increment command buffer number before idling the queue.
                // This will make sure that any resource released while this function
                // is running will be associated with the next command buffer submission.
                CmdBufferNumber = static_cast<Uint64>(Queue.NextCmdBufferNumber);
                Atomics::AtomicIncrement(Queue.NextCmdBufferNumber);
            }

            FenceValue = Queue.CmdQueue->WaitForIdle();
        }

        if (ReleaseResources)
        {
            Queue.ReleaseQueue.DiscardStaleResources(CmdBufferNumber, FenceValue);
            Queue.ReleaseQueue.Purge(Queue.CmdQueue->GetCompletedFenceValue());
        }
    }

    void IdleAllCommandQueues(bool ReleaseResources)
    {
        for (size_t q = 0; q < m_CmdQueueCount; ++q)
            IdleCommandQueue(q, ReleaseResources);
    }

    struct SubmittedCommandBufferInfo
    {
        Uint64 CmdBufferNumber = 0;
        Uint64 FenceValue      = 0;
    };
    template <typename SubmitDataType>
    SubmittedCommandBufferInfo SubmitCommandBuffer(Uint32 QueueIndex, SubmitDataType& SubmitData, bool DiscardStaleResources)
    {
        SubmittedCommandBufferInfo CmdBuffInfo;
        VERIFY_EXPR(QueueIndex < m_CmdQueueCount);
        auto& Queue = m_CommandQueues[QueueIndex];

        {
            std::lock_guard<std::mutex> Lock{Queue.Mtx};

            // Increment command buffer number before submitting the cmd buffer.
            // This will make sure that any resource released while this function
            // is running will be associated with the next command buffer.
            CmdBuffInfo.CmdBufferNumber = static_cast<Uint64>(Queue.NextCmdBufferNumber);
            Atomics::AtomicIncrement(Queue.NextCmdBufferNumber);

            CmdBuffInfo.FenceValue = Queue.CmdQueue->Submit(SubmitData);
        }

        if (DiscardStaleResources)
        {
            // The following basic requirement guarantees correctness of resource deallocation:
            //
            //        A resource is never released before the last draw command referencing it is submitted for execution
            //

            // Move stale objects into the release queue.
            // Note that objects are moved from stale list to release queue based on the cmd buffer number,
            // not fence value. This makes sure that basic requirement is met even when the fence value is
            // not incremented while executing the command buffer (as is the case with Unity command queue).

            // As long as resources used by deferred contexts are not released before the command list
            // is executed through immediate context, this stategy always works.
            Queue.ReleaseQueue.DiscardStaleResources(CmdBuffInfo.CmdBufferNumber, CmdBuffInfo.FenceValue);
        }

        return CmdBuffInfo;
    }

    ResourceReleaseQueue<DynamicStaleResourceWrapper>& GetReleaseQueue(Uint32 QueueIndex)
    {
        VERIFY_EXPR(QueueIndex < m_CmdQueueCount);
        return m_CommandQueues[QueueIndex].ReleaseQueue;
    }

    const CommandQueueType& DILIGENT_CALL_TYPE GetCommandQueue(Uint32 QueueIndex) const
    {
        VERIFY_EXPR(QueueIndex < m_CmdQueueCount);
        return *m_CommandQueues[QueueIndex].CmdQueue;
    }

    virtual Uint64 DILIGENT_CALL_TYPE GetCompletedFenceValue(Uint32 QueueIndex) override final
    {
        return m_CommandQueues[QueueIndex].CmdQueue->GetCompletedFenceValue();
    }

    virtual Uint64 DILIGENT_CALL_TYPE GetNextFenceValue(Uint32 QueueIndex) override final
    {
        return m_CommandQueues[QueueIndex].CmdQueue->GetNextFenceValue();
    }

    virtual Bool DILIGENT_CALL_TYPE IsFenceSignaled(Uint32 QueueIndex, Uint64 FenceValue) override final
    {
        return FenceValue <= GetCompletedFenceValue(QueueIndex);
    }

    template <typename TAction>
    void LockCmdQueueAndRun(Uint32 QueueIndex, TAction Action)
    {
        VERIFY_EXPR(QueueIndex < m_CmdQueueCount);
        auto&                       Queue = m_CommandQueues[QueueIndex];
        std::lock_guard<std::mutex> Lock{Queue.Mtx};
        Action(Queue.CmdQueue);
    }

    CommandQueueType* LockCommandQueue(Uint32 QueueIndex)
    {
        VERIFY_EXPR(QueueIndex < m_CmdQueueCount);
        auto& Queue = m_CommandQueues[QueueIndex];
        Queue.Mtx.lock();
        return Queue.CmdQueue;
    }

    void UnlockCommandQueue(Uint32 QueueIndex)
    {
        VERIFY_EXPR(QueueIndex < m_CmdQueueCount);
        auto& Queue = m_CommandQueues[QueueIndex];
        Queue.Mtx.unlock();
    }

protected:
    void DestroyCommandQueues()
    {
        if (m_CommandQueues != nullptr)
        {
            for (size_t q = 0; q < m_CmdQueueCount; ++q)
            {
                auto& Queue = m_CommandQueues[q];
                DEV_CHECK_ERR(Queue.ReleaseQueue.GetStaleResourceCount() == 0, "All stale resources must be released before destroying a command queue");
                DEV_CHECK_ERR(Queue.ReleaseQueue.GetPendingReleaseResourceCount() == 0, "All resources must be released before destroying a command queue");
                Queue.~CommandQueue();
            }
            this->m_RawMemAllocator.Free(m_CommandQueues);
            m_CommandQueues = nullptr;
        }
    }

    struct CommandQueue
    {
        CommandQueue(RefCntAutoPtr<CommandQueueType> _CmdQueue, IMemoryAllocator& Allocator) noexcept :
            CmdQueue{std::move(_CmdQueue)},
            ReleaseQueue{Allocator}
        {
            NextCmdBufferNumber = 0;
        }

        // clang-format off
        CommandQueue             (const CommandQueue&)  = delete;
        CommandQueue             (      CommandQueue&&) = delete;
        CommandQueue& operator = (const CommandQueue&)  = delete;
        CommandQueue& operator = (      CommandQueue&&) = delete;
        // clang-format on

        std::mutex                                        Mtx;
        Atomics::AtomicInt64                              NextCmdBufferNumber;
        RefCntAutoPtr<CommandQueueType>                   CmdQueue;
        ResourceReleaseQueue<DynamicStaleResourceWrapper> ReleaseQueue;
    };
    const size_t  m_CmdQueueCount = 0;
    CommandQueue* m_CommandQueues = nullptr;
};

} // namespace Diligent
