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
/// Implementation of the Diligent::DeviceObjectBase template class

#include <cstring>
#include <cstdio>
#include "RefCntAutoPtr.hpp"
#include "ObjectBase.hpp"
#include "UniqueIdentifier.hpp"
#include "EngineMemory.h"

namespace Diligent
{

/// Template class implementing base functionality for a device object
template <class BaseInterface, typename RenderDeviceImplType, typename ObjectDescType>
class DeviceObjectBase : public ObjectBase<BaseInterface>
{
public:
    typedef ObjectBase<BaseInterface> TBase;

    /// \param pRefCounters - reference counters object that controls the lifetime of this device object
    /// \param pDevice - pointer to the render device.
    /// \param ObjDesc - object description.
    /// \param bIsDeviceInternal - flag indicating if the object is an internal device object
    ///							   and must not keep a strong reference to the device.
    DeviceObjectBase(IReferenceCounters*   pRefCounters,
                     RenderDeviceImplType* pDevice,
                     const ObjectDescType& ObjDesc,
                     bool                  bIsDeviceInternal = false) :
        // clang-format off
        TBase              {pRefCounters},
        m_pDevice          {pDevice          },
        m_Desc             {ObjDesc          },
        m_bIsDeviceInternal{bIsDeviceInternal}
    //clang-format on
    {
        // Do not keep strong reference to the device if the object is an internal device object
        if (!m_bIsDeviceInternal)
        {
            m_pDevice->AddRef();
        }

        if (ObjDesc.Name != nullptr)
        {
            auto  size     = strlen(ObjDesc.Name) + 1;
            auto* NameCopy = ALLOCATE(GetStringAllocator(), "Object name copy", char, size);
            memcpy(NameCopy, ObjDesc.Name, size);
            m_Desc.Name = NameCopy;
        }
        else
        {
            size_t size       = 16 + 2 + 1; // 0x12345678
            auto*  AddressStr = ALLOCATE(GetStringAllocator(), "Object address string", char, size);
            snprintf(AddressStr, size, "0x%llX", static_cast<unsigned long long>(reinterpret_cast<size_t>(this)));
            m_Desc.Name = AddressStr;
        }

        //                        !!!WARNING!!!
        // We cannot add resource to the hash table from here, because the object
        // has not been completely created yet and the reference counters object
        // is not initialized!
        //m_pDevice->AddResourceToHash( this ); - ERROR!
    }

    // clang-format off
    DeviceObjectBase             (const DeviceObjectBase& ) = delete;
    DeviceObjectBase             (      DeviceObjectBase&&) = delete;
    DeviceObjectBase& operator = (const DeviceObjectBase& ) = delete;
    DeviceObjectBase& operator = (      DeviceObjectBase&&) = delete;
    // clang-format on

    virtual ~DeviceObjectBase()
    {
        FREE(GetStringAllocator(), const_cast<Char*>(m_Desc.Name));

        if (!m_bIsDeviceInternal)
        {
            m_pDevice->Release();
        }
    }

    inline virtual Atomics::Long DILIGENT_CALL_TYPE Release() override final
    {
        // Render device owns allocators for all types of device objects,
        // so it must be destroyed after all device objects are released.
        // Consider the following scenario: an object A owns the last strong
        // reference to the device:
        //
        // 1. A::~A() completes
        // 2. A::~DeviceObjectBase() completes
        // 3. A::m_pDevice is released
        //       Render device is destroyed, all allocators are invalid
        // 4. RefCountersImpl::ObjectWrapperBase::DestroyObject() calls
        //    m_pAllocator->Free(m_pObject) - crash!

        RefCntAutoPtr<RenderDeviceImplType> pDevice;
        return TBase::Release(
            [&]() //
            {
                // We must keep the device alive while the object is being destroyed
                // Note that internal device objects do not keep strong reference to the device
                if (!m_bIsDeviceInternal)
                {
                    pDevice = m_pDevice;
                }
            });
    }

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_DeviceObject, TBase)

    virtual const ObjectDescType& DILIGENT_CALL_TYPE GetDesc() const override final
    {
        return m_Desc;
    }

    /// Returns unique identifier
    virtual Int32 DILIGENT_CALL_TYPE GetUniqueID() const override final
    {
        /// \note
        /// This unique ID is used to unambiguously identify device object for
        /// tracking purposes.
        /// Niether GL handle nor pointer could be safely used for this purpose
        /// as both GL reuses released handles and the OS reuses released pointers
        return m_UniqueID.GetID();
    }

    static bool IsSameObject(DeviceObjectBase* pObj1, DeviceObjectBase* pObj2)
    {
        UniqueIdentifier Id1 = (pObj1 != nullptr) ? pObj1->GetUniqueID() : 0;
        UniqueIdentifier Id2 = (pObj2 != nullptr) ? pObj2->GetUniqueID() : 0;
        return Id1 == Id2;
    }

    RenderDeviceImplType* GetDevice() const { return m_pDevice; }

protected:
    /// Pointer to the device
    RenderDeviceImplType* const m_pDevice;

    /// Object description
    ObjectDescType m_Desc;

    // Template argument is only used to separate counters for
    // different groups of objects
    UniqueIdHelper<BaseInterface> m_UniqueID;
    const bool                    m_bIsDeviceInternal;
};

} // namespace Diligent
