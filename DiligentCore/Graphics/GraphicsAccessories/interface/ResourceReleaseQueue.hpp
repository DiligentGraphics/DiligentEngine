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

// Helper class that handles free memory block management to accommodate variable-size allocation requests
// See http://diligentgraphics.com/diligent-engine/architecture/d3d12/variable-size-memory-allocations-manager/

#pragma once

/// \file
/// Implementation of Diligent::ResourceReleaseQueue class

#include <mutex>
#include <deque>

#include "../../../Primitives/interface/MemoryAllocator.h"
#include "../../../Common/interface/STDAllocator.hpp"
#include "../../../Platforms/interface/Atomics.hpp"
#include "../../../Platforms/Basic/interface/DebugUtilities.hpp"

namespace Diligent
{

/// Helper class that wraps stale resources of different types
class DynamicStaleResourceWrapper final
{
public:
    //   ___________________________                                  ___________________________
    //  |DynamicStaleResourceWrapper|                                |DynamicStaleResourceWrapper|
    //  |                           |                                |                           |
    //  |   m_pStaleResource        |                                |   m_pStaleResource        |
    //  |__________|________________|                                |__________|________________|
    //             |                                                            |
    //             |                                                            |
    //             |                                                            |
    //   __________V_______________________________________           __________V___________________________________
    //  |SpecificSharedStaleResource<VulkanBufferWrapper>  |         |SpecificStaleResource<VulkanMemoryAllocation> |
    //  |                                                  |         |                                              |
    //  |  VulkanBufferWrapper m_SpecificResource;         |         |  VulkanMemoryAllocation m_SpecificResource;  |
    //  |  AtomicLong          m_RefCounter                |         |______________________________________________|
    //  |__________________________________________________|
    //

    template <typename ResourceType, typename = typename std::enable_if<std::is_object<ResourceType>::value>::type>
    static DynamicStaleResourceWrapper Create(ResourceType&& Resource, Atomics::Long NumReferences)
    {
        VERIFY_EXPR(NumReferences >= 1);

        class SpecificStaleResource final : public StaleResourceBase
        {
        public:
            SpecificStaleResource(ResourceType&& SpecificResource) :
                m_SpecificResource(std::move(SpecificResource))
            {}

            // clang-format off
            SpecificStaleResource             (const SpecificStaleResource&) = delete;
            SpecificStaleResource             (SpecificStaleResource&&)      = delete;
            SpecificStaleResource& operator = (const SpecificStaleResource&) = delete;
            SpecificStaleResource& operator = (SpecificStaleResource&&)      = delete;
            // clang-format on

            virtual void Release() override final
            {
                delete this;
            }

        private:
            ResourceType m_SpecificResource;
        };

        class SpecificSharedStaleResource final : public StaleResourceBase
        {
        public:
            SpecificSharedStaleResource(ResourceType&& SpecificResource, Atomics::Long NumReferences) :
                m_SpecificResource(std::move(SpecificResource))
            {
                m_RefCounter = NumReferences;
            }

            // clang-format off
            SpecificSharedStaleResource             (const SpecificSharedStaleResource&) = delete;
            SpecificSharedStaleResource             (SpecificSharedStaleResource&&)      = delete;
            SpecificSharedStaleResource& operator = (const SpecificSharedStaleResource&) = delete;
            SpecificSharedStaleResource& operator = (SpecificSharedStaleResource&&)      = delete;
            // clang-format on

            virtual void Release() override final
            {
                if (Atomics::AtomicDecrement(m_RefCounter) == 0)
                {
                    delete this;
                }
            }

        private:
            ResourceType        m_SpecificResource;
            Atomics::AtomicLong m_RefCounter;
        };

        return DynamicStaleResourceWrapper{
            NumReferences == 1 ?
                static_cast<StaleResourceBase*>(new SpecificStaleResource{std::move(Resource)}) :
                static_cast<StaleResourceBase*>(new SpecificSharedStaleResource{std::move(Resource), NumReferences})};
    }

    DynamicStaleResourceWrapper(DynamicStaleResourceWrapper&& rhs) noexcept :
        m_pStaleResource(std::move(rhs.m_pStaleResource))
    {
        rhs.m_pStaleResource = nullptr;
    }

    DynamicStaleResourceWrapper(const DynamicStaleResourceWrapper& rhs) noexcept :
        m_pStaleResource{rhs.m_pStaleResource}
    {
    }

    // clang-format off
    DynamicStaleResourceWrapper& operator = (const DynamicStaleResourceWrapper&)  = delete;
    DynamicStaleResourceWrapper& operator = (      DynamicStaleResourceWrapper&&) = delete;
    // clang-format on

    void GiveUpOwnership()
    {
        m_pStaleResource = nullptr;
    }

    ~DynamicStaleResourceWrapper()
    {
        if (m_pStaleResource != nullptr)
            m_pStaleResource->Release();
    }

private:
    class StaleResourceBase
    {
    public:
        virtual ~StaleResourceBase() = 0;
        virtual void Release()       = 0;
    };

    DynamicStaleResourceWrapper(StaleResourceBase* pStaleResource) :
        m_pStaleResource(pStaleResource)
    {}

    StaleResourceBase* m_pStaleResource;
};

inline DynamicStaleResourceWrapper::StaleResourceBase::~StaleResourceBase()
{
}

/// Helper class that wraps stale resources of the same type
template <typename ResourceType>
class StaticStaleResourceWrapper
{
public:
    static StaticStaleResourceWrapper Create(ResourceType&& Resource, Atomics::Long NumReferences)
    {
        VERIFY(NumReferences == 1, "Number of references must be 1 for StaticStaleResourceWrapper");
        return StaticStaleResourceWrapper{std::move(Resource)};
    }

    StaticStaleResourceWrapper(StaticStaleResourceWrapper&& rhs) noexcept :
        m_StaleResource(std::move(rhs.m_StaleResource))
    {}

    StaticStaleResourceWrapper& operator=(StaticStaleResourceWrapper&& rhs) noexcept
    {
        m_StaleResource = std::move(rhs.m_StaleResource);
        return *this;
    }

    // clang-format off
    StaticStaleResourceWrapper             (const StaticStaleResourceWrapper&) = delete;
    StaticStaleResourceWrapper& operator = (const StaticStaleResourceWrapper&) = delete;
    // clang-format on

private:
    StaticStaleResourceWrapper(ResourceType&& StaleResource) :
        m_StaleResource{std::move(StaleResource)}
    {}

    ResourceType m_StaleResource;
};

/// Facilitates safe resource destruction in D3D12 and Vulkan

/// Resource destruction is a two-stage process:
/// * When resource is released, it is moved into the stale objects queue along with the next command list number
/// * When command list is submitted to the command queue, all stale objects associated with this
///   and earlier command lists are moved to the release queue, along with the fence value associated with
///   the command list
/// * Resources are removed and actually destroyed from the queue when fence is signaled and the queue is Purged
///
/// \tparam ResourceWrapperType -  Type of the resource wrapper used by the release queue.
template <typename ResourceWrapperType>
class ResourceReleaseQueue
{
public:
    // clang-format off
    ResourceReleaseQueue(IMemoryAllocator& Allocator) :
        m_ReleaseQueue  (STD_ALLOCATOR_RAW_MEM(ReleaseQueueElemType, Allocator, "Allocator for deque<ReleaseQueueElemType>")),
        m_StaleResources(STD_ALLOCATOR_RAW_MEM(ReleaseQueueElemType, Allocator, "Allocator for deque<ReleaseQueueElemType>"))
    {}
    // clang-format on

    ~ResourceReleaseQueue()
    {
        DEV_CHECK_ERR(m_StaleResources.empty(), "Not all stale objects were destroyed");
        DEV_CHECK_ERR(m_ReleaseQueue.empty(), "Release queue is not empty");
    }

    /// Creates a resource wrapper for the specific resource type
    /// \param [in] Resource      - Resource to be released
    /// \param [in] NumReferences - Number of references to the resource
    template <typename ResourceType, typename = typename std::enable_if<std::is_object<ResourceType>::value>::type>
    static ResourceWrapperType CreateWrapper(ResourceType&& Resource, Atomics::Long NumReferences)
    {
        return ResourceWrapperType::Create(std::move(Resource), NumReferences);
    }

    /// Moves a resource to the stale resources queue
    /// \param [in] Resource              - Resource to be released
    /// \param [in] NextCommandListNumber - Number of the command list that will be submitted to the queue next
    template <typename ResourceType, typename = typename std::enable_if<std::is_object<ResourceType>::value>::type>
    void SafeReleaseResource(ResourceType&& Resource, Uint64 NextCommandListNumber)
    {
        SafeReleaseResource(CreateWrapper(std::move(Resource), 1), NextCommandListNumber);
    }

    /// Moves a resource wrapper to the stale resources queue
    /// \param [in] Wrapper               - Resource wrapper containing the resource to be released
    /// \param [in] NextCommandListNumber - Number of the command list that will be submitted to the queue next
    void SafeReleaseResource(ResourceWrapperType&& Wrapper, Uint64 NextCommandListNumber)
    {
        std::lock_guard<std::mutex> LockGuard(m_StaleObjectsMutex);
        m_StaleResources.emplace_back(NextCommandListNumber, std::move(Wrapper));
    }

    /// Moves a copy of the resource wrapper to the stale resources queue
    /// \param [in] Wrapper               - Resource wrapper containing the resource to be released
    /// \param [in] NextCommandListNumber - Number of the command list that will be submitted to the queue next
    void SafeReleaseResource(const ResourceWrapperType& Wrapper, Uint64 NextCommandListNumber)
    {
        std::lock_guard<std::mutex> LockGuard(m_StaleObjectsMutex);
        m_StaleResources.emplace_back(NextCommandListNumber, Wrapper);
    }

    /// Adds a resource directly to the release queue
    /// \param [in] Resource    - Resource to be released.
    /// \param [in] FenceValue  - Fence value indicating when the resource was used last time.
    template <typename ResourceType, typename = typename std::enable_if<std::is_object<ResourceType>::value>::type>
    void DiscardResource(ResourceType&& Resource, Uint64 FenceValue)
    {
        DiscardResource(CreateWrapper(std::move(Resource), 1), FenceValue);
    }

    /// Adds a resource wrapper directly to the release queue
    /// \param [in] Wrapper     - Resource wrapper containing the resource to be released.
    /// \param [in] FenceValue  - Fence value indicating when the resource was used last time.
    void DiscardResource(ResourceWrapperType&& Wrapper, Uint64 FenceValue)
    {
        std::lock_guard<std::mutex> ReleaseQueueLock(m_ReleaseQueueMutex);
        m_ReleaseQueue.emplace_back(FenceValue, std::move(Wrapper));
    }

    /// Adds a copy of the resource wrapper directly to the release queue
    /// \param [in] Wrapper     - Resource wrapper containing the resource to be released.
    /// \param [in] FenceValue  - Fence value indicating when the resource was used last time.
    void DiscardResource(const ResourceWrapperType& Wrapper, Uint64 FenceValue)
    {
        std::lock_guard<std::mutex> ReleaseQueueLock(m_ReleaseQueueMutex);
        m_ReleaseQueue.emplace_back(FenceValue, Wrapper);
    }

    /// Adds multiple resources directly to the release queue
    /// \param [in] FenceValue  - Fence value indicating when the resource was used last time.
    /// \param [in] Iterator    - Iterator that returns resources to be relased.
    template <typename ResourceType, typename IteratorType>
    void DiscardResources(Uint64 FenceValue, IteratorType Iterator)
    {
        std::lock_guard<std::mutex> ReleaseQueueLock(m_ReleaseQueueMutex);
        ResourceType                Resource;
        while (Iterator(Resource))
        {
            m_ReleaseQueue.emplace_back(FenceValue, CreateWrapper(std::move(Resource), 1));
        }
    }

    /// Moves stale objects to the release queue
    /// \param [in] SubmittedCmdBuffNumber - number of the last submitted command list.
    ///                                      All resources in the stale object list whose command list number is
    ///                                      less than or equal to this value are moved to the release queue.
    /// \param [in] FenceValue             - Fence value associated with the resources moved to the release queue.
    ///                                      A resource will be destroyed by Purge() method when completed fence value
    ///                                      is greater or equal to the fence value associated with the resource
    void DiscardStaleResources(Uint64 SubmittedCmdBuffNumber, Uint64 FenceValue)
    {
        // Only discard these stale objects that were released before CmdBuffNumber
        // was executed
        std::lock_guard<std::mutex> StaleObjectsLock(m_StaleObjectsMutex);
        std::lock_guard<std::mutex> ReleaseQueueLock(m_ReleaseQueueMutex);
        while (!m_StaleResources.empty())
        {
            auto& FirstStaleObj = m_StaleResources.front();
            if (FirstStaleObj.first <= SubmittedCmdBuffNumber)
            {
                m_ReleaseQueue.emplace_back(FenceValue, std::move(FirstStaleObj.second));
                m_StaleResources.pop_front();
            }
            else
                break;
        }
    }


    /// Removes all objects from the release queue whose fence value is
    /// less than or equal to CompletedFenceValue
    /// \param [in] CompletedFenceValue  -  Value of the fence that has been completed by the GPU
    void Purge(Uint64 CompletedFenceValue)
    {
        std::lock_guard<std::mutex> LockGuard(m_ReleaseQueueMutex);

        // Release all objects whose associated fence value is at most CompletedFenceValue
        // See http://diligentgraphics.com/diligent-engine/architecture/d3d12/managing-resource-lifetimes/
        while (!m_ReleaseQueue.empty())
        {
            auto& FirstObj = m_ReleaseQueue.front();
            if (FirstObj.first <= CompletedFenceValue)
                m_ReleaseQueue.pop_front();
            else
                break;
        }
    }

    /// Returns the number of stale resources
    size_t GetStaleResourceCount() const
    {
        return m_StaleResources.size();
    }

    /// Returns the number of resources pending release
    size_t GetPendingReleaseResourceCount() const
    {
        return m_ReleaseQueue.size();
    }

private:
    std::mutex m_ReleaseQueueMutex;
    using ReleaseQueueElemType = std::pair<Uint64, ResourceWrapperType>;
    std::deque<ReleaseQueueElemType, STDAllocatorRawMem<ReleaseQueueElemType>> m_ReleaseQueue;

    std::mutex                                                                 m_StaleObjectsMutex;
    std::deque<ReleaseQueueElemType, STDAllocatorRawMem<ReleaseQueueElemType>> m_StaleResources;
};

} // namespace Diligent
