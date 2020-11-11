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
/// Implementation of the Diligent::StateObjectsRegistry template class

#include "DeviceObject.h"
#include <unordered_map>
#include "STDAllocator.hpp"

namespace Diligent
{
/// Template class implementing state object registry

/// \tparam ResourceDescType - type of the resource description. The type must have
///                            operator== and a hash function defined.
///
/// \remarks
/// The following strategies do not work:
/// - To store raw pointers and remove the object from the registry in the
///   object's destructor. In this case other thread may obtain a reference
///   to the object while it is being deleted. This scenario is possible if one thread
///   has just entered the destructor, but other is executing Find() and entered
///   the protection section.
/// - Strong pointers will cause circular references and result in memory leaks.
/// \remarks
/// Only weak pointers provide thread-safe solution. The object is either atomically destroyed,
/// so that no other thread can obtain a reference to it through weak pointers. Or it is atomically
/// locked, so that strong reference is obtained. In this case no other thread can destroy the object,
/// because there is at least one strong reference now. Note however that removing the object from the
/// registry in the object's destructor may cause a deadlock at the point where Find() locks the weak pointer:
/// if other thread has started dtor, the object will be locked by Diligent::RefCountedObject::Release().
/// If after that this thread locks the registry first, it will be waiting for the object to unlock in
/// Diligent::RefCntWeakPtr::Lock(), while the dtor thread will be waiting for the registry to unlock.
template <typename ResourceDescType>
class StateObjectsRegistry
{
public:
    /// Number of outstanding deleted objects to purge the registry.
    static constexpr int DeletedObjectsToPurge = 32;

    StateObjectsRegistry(IMemoryAllocator& RawAllocator, const Char* RegistryName) :
        m_DescToObjHashMap(STD_ALLOCATOR_RAW_MEM(HashMapElem, RawAllocator, "Allocator for unordered_map<ResourceDescType, RefCntWeakPtr<IDeviceObject> >")),
        m_RegistryName{RegistryName}
    {}

    ~StateObjectsRegistry()
    {
        // Object registry is part of the device, and every device
        // object holds a strong reference to the device. So device
        // is destroyed after all device objects are destroyed, and there
        // may only be expired references in the registry. After we
        // purge it, the registry must be empty.
        Purge();
        VERIFY(m_DescToObjHashMap.empty(), "DescToObjHashMap is not empty");
    }

    /// Adds a new object to the registry

    /// \param [in] ObjectDesc - object description.
    /// \param [in] pObject - pointer to the object.
    ///
    /// Besides adding a new object, the function also checks the number of
    /// outstanding deleted objects and calls Purge() if the number has reached
    /// the threshold value DeletedObjectsToPurge. Creating a state object is
    /// assumed to be an expensive operation and should be performed during
    /// the initialization. Occasional purge operations should not add significant
    /// cost to it.
    void Add(const ResourceDescType& ObjectDesc, IDeviceObject* pObject)
    {
        ThreadingTools::LockHelper Lock(m_LockFlag);

        // If the number of outstanding deleted objects reached the threshold value,
        // purge the registry. Since we have exclusive access now, it is safe
        // to do.
        if (m_NumDeletedObjects >= DeletedObjectsToPurge)
        {
            Purge();
            m_NumDeletedObjects = 0;
        }

        // Try to construct the new element in place
        auto Elems = m_DescToObjHashMap.emplace(std::make_pair(ObjectDesc, Diligent::RefCntWeakPtr<IDeviceObject>(pObject)));
        // It is theorertically possible that the same object can be found
        // in the registry. This might happen if two threads try to create
        // the same object at the same time. They both will not find the
        // object and then will create and try to add it.
        //
        // If the object already exists, we replace the existing reference.
        // This is safer as there might be scenarios where existing reference
        // might be expired. For instance, two threads try to create the same
        // object which is not in the registry. The first thread creates
        // the object, adds it to the registry and then releases it. After that
        // the second thread creates the same object and tries to add it to
        // the registry. It will find an existing expired reference to the
        // object.
        if (!Elems.second)
        {
            VERIFY(Elems.first->first == ObjectDesc, "Incorrect object description");
            LOG_WARNING_MESSAGE("Object named '", Elems.first->first.Name,
                                "' with the same description already exists in the registry."
                                "Replacing with the new object named '",
                                ObjectDesc.Name ? ObjectDesc.Name : "", "'.");
            Elems.first->second = pObject;
        }
    }

    /// Finds the object in the registry
    void Find(const ResourceDescType& Desc, IDeviceObject** ppObject)
    {
        VERIFY(*ppObject == nullptr, "Overwriting reference to existing object may cause memory leaks");
        *ppObject = nullptr;
        ThreadingTools::LockHelper Lock(m_LockFlag);

        auto It = m_DescToObjHashMap.find(Desc);
        if (It != m_DescToObjHashMap.end())
        {
            // Try to obtain strong reference to the object.
            // This is an atomic operation and we either get
            // a new strong reference or object has been destroyed
            // and we get null.
            auto pObject = It->second.Lock();
            if (pObject)
            {
                *ppObject = pObject.Detach();
                //LOG_INFO_MESSAGE( "Equivalent of the requested state object named \"", Desc.Name ? Desc.Name : "", "\" found in the ", m_RegistryName, " registry. Reusing existing object.");
            }
            else
            {
                // Expired object found: remove it from the map
                m_DescToObjHashMap.erase(It);
                Atomics::AtomicDecrement(m_NumDeletedObjects);
            }
        }
    }

    /// Purges outstanding deleted objects from the registry
    void Purge()
    {
        Uint32 NumPurgedObjects = 0;
        auto   It               = m_DescToObjHashMap.begin();
        while (It != m_DescToObjHashMap.end())
        {
            auto NextIt = It;
            ++NextIt;
            // Note that IsValid() is not a thread-safe function in the sense that it
            // can give false positive results. The only thread-safe way to check if the
            // object is alive is to lock the weak pointer, but that requires thread
            // synchronization. We will immediately unlock the pointer anyway, so we
            // want to detect 100% expired pointers. IsValid() does provide that information
            // because once a weak pointer becomes invalid, it will be invalid
            // until it is destroyed. It is not a problem if we miss an expired weak
            // pointer as it will definitiely be removed next time.
            if (!It->second.IsValid())
            {
                m_DescToObjHashMap.erase(It);
                ++NumPurgedObjects;
            }

            It = NextIt;
        }
        LOG_INFO_MESSAGE("Purged ", NumPurgedObjects, " deleted objects from the ", m_RegistryName, " registry");
    }

    /// Increments the number of outstanding deleted objects.
    /// When this number reaches DeletedObjectsToPurge, Purge() will
    /// be called.
    void ReportDeletedObject()
    {
        Atomics::AtomicIncrement(m_NumDeletedObjects);
    }

private:
    /// Lock flag to protect the m_DescToObjHashMap
    ThreadingTools::LockFlag m_LockFlag;

    /// Nmber of outstanding deleted objects that have not been purged
    Atomics::AtomicLong m_NumDeletedObjects;

    /// Hash map that stores weak pointers to the referenced objects
    typedef std::pair<const ResourceDescType, RefCntWeakPtr<IDeviceObject>>                                                                                           HashMapElem;
    std::unordered_map<ResourceDescType, RefCntWeakPtr<IDeviceObject>, std::hash<ResourceDescType>, std::equal_to<ResourceDescType>, STDAllocatorRawMem<HashMapElem>> m_DescToObjHashMap;

    /// Registry name used for debug output
    const String m_RegistryName;
};
} // namespace Diligent
