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

#include "../../Primitives/interface/Object.h"
#include "../../Platforms/interface/Atomics.hpp"
#include "ValidatedCast.hpp"
#include "RefCountedObjectImpl.hpp"


namespace Diligent
{


template <typename T>
class RefCntWeakPtr;

// The main advantage of RefCntAutoPtr over the std::shared_ptr is that you can
// attach the same raw pointer to different smart pointers.
//
// For instance, the following code will crash since p will be released twice:
//
// auto *p = new char;
// std::shared_ptr<char> pTmp1(p);
// std::shared_ptr<char> pTmp2(p);
// ...

// This code, in contrast, works perfectly fine:
//
// ObjectBase *pRawPtr(new ObjectBase);
// RefCntAutoPtr<ObjectBase> pSmartPtr1(pRawPtr);
// RefCntAutoPtr<ObjectBase> pSmartPtr2(pRawPtr);
// ...

// Other advantage is that weak pointers remain valid until the
// object is alive, even if all smart pointers were destroyed:
//
// RefCntWeakPtr<ObjectBase> pWeakPtr(pSmartPtr1);
// pSmartPtr1.Release();
// auto pSmartPtr3 = pWeakPtr.Lock();
// ..

// Weak pointers can also be attached directly to the object:
// RefCntWeakPtr<ObjectBase> pWeakPtr(pRawPtr);
//

/// Template class that implements reference counting
template <typename T>
class RefCntAutoPtr
{
public:
    RefCntAutoPtr() noexcept {}

    explicit RefCntAutoPtr(T* pObj) noexcept :
        m_pObject{pObj}
    {
        if (m_pObject)
            m_pObject->AddRef();
    }

    RefCntAutoPtr(IObject* pObj, const INTERFACE_ID& IID) noexcept :
        m_pObject{nullptr}
    {
        if (pObj)
            pObj->QueryInterface(IID, reinterpret_cast<IObject**>(&m_pObject));
    }

    // Copy constructor must not be template!
    RefCntAutoPtr(const RefCntAutoPtr& AutoPtr) noexcept :
        m_pObject{AutoPtr.m_pObject}
    {
        if (m_pObject)
            m_pObject->AddRef();
    }

    template <typename DerivedType, typename = typename std::enable_if<std::is_base_of<T, DerivedType>::value>::type>
    RefCntAutoPtr(const RefCntAutoPtr<DerivedType>& AutoPtr) noexcept :
        RefCntAutoPtr<T>{AutoPtr.m_pObject}
    {
    }

    // Non-template move constructor
    RefCntAutoPtr(RefCntAutoPtr&& AutoPtr) noexcept :
        m_pObject{std::move(AutoPtr.m_pObject)}
    {
        //Make sure original pointer has no references to the object
        AutoPtr.m_pObject = nullptr;
    }

    template <typename DerivedType, typename = typename std::enable_if<std::is_base_of<T, DerivedType>::value>::type>
    RefCntAutoPtr(RefCntAutoPtr<DerivedType>&& AutoPtr) noexcept :
        m_pObject{std::move(AutoPtr.m_pObject)}
    {
        //Make sure original pointer has no references to the object
        AutoPtr.m_pObject = nullptr;
    }

    ~RefCntAutoPtr()
    {
        Release();
    }

    void swap(RefCntAutoPtr& AutoPtr) noexcept
    {
        std::swap(m_pObject, AutoPtr.m_pObject);
    }

    void Attach(T* pObj) noexcept
    {
        Release();
        m_pObject = pObj;
    }

    T* Detach() noexcept
    {
        T* pObj   = m_pObject;
        m_pObject = nullptr;
        return pObj;
    }

    void Release() noexcept
    {
        if (m_pObject)
        {
            m_pObject->Release();
            m_pObject = nullptr;
        }
    }

    RefCntAutoPtr& operator=(T* pObj) noexcept
    {
        if (m_pObject != pObj)
        {
            if (m_pObject)
                m_pObject->Release();
            m_pObject = pObj;
            if (m_pObject)
                m_pObject->AddRef();
        }
        return *this;
    }

    RefCntAutoPtr& operator=(const RefCntAutoPtr& AutoPtr) noexcept
    {
        return *this = AutoPtr.m_pObject;
    }

    template <typename DerivedType, typename = typename std::enable_if<std::is_base_of<T, DerivedType>::value>::type>
    RefCntAutoPtr& operator=(const RefCntAutoPtr<DerivedType>& AutoPtr) noexcept
    {
        return *this = static_cast<T*>(AutoPtr.m_pObject);
    }

    RefCntAutoPtr& operator=(RefCntAutoPtr&& AutoPtr) noexcept
    {
        if (m_pObject != AutoPtr.m_pObject)
            Attach(AutoPtr.Detach());

        return *this;
    }

    template <typename DerivedType, typename = typename std::enable_if<std::is_base_of<T, DerivedType>::value>::type>
    RefCntAutoPtr& operator=(RefCntAutoPtr<DerivedType>&& AutoPtr) noexcept
    {
        if (m_pObject != AutoPtr.m_pObject)
            Attach(AutoPtr.Detach());

        return *this;
    }

    // All the access functions do not require locking reference counters pointer because if it is valid,
    // the smart pointer holds strong reference to the object and it thus cannot be released by
    // ohter thread
    bool operator!() const noexcept { return m_pObject == nullptr; }
    operator bool() const noexcept { return m_pObject != nullptr; }
    bool operator==(const RefCntAutoPtr& Ptr) const noexcept { return m_pObject == Ptr.m_pObject; }
    bool operator!=(const RefCntAutoPtr& Ptr) const noexcept { return m_pObject != Ptr.m_pObject; }
    bool operator<(const RefCntAutoPtr& Ptr) const noexcept { return static_cast<const T*>(*this) < static_cast<const T*>(Ptr); }

    T&       operator*() noexcept { return *m_pObject; }
    const T& operator*() const noexcept { return *m_pObject; }

    T*       RawPtr() noexcept { return m_pObject; }
    const T* RawPtr() const noexcept { return m_pObject; }

    template <typename DstType>
    DstType* RawPtr() noexcept { return ValidatedCast<DstType>(m_pObject); }
    template <typename DstType>
    DstType* RawPtr() const noexcept { return ValidatedCast<DstType>(m_pObject); }

    operator T*() noexcept { return RawPtr(); }
    operator const T*() const noexcept { return RawPtr(); }

    T*       operator->() noexcept { return m_pObject; }
    const T* operator->() const noexcept { return m_pObject; }

    template <typename InterfaceType>
    RefCntAutoPtr<InterfaceType> Cast(const INTERFACE_ID& IID) const
    {
        return RefCntAutoPtr<InterfaceType>{m_pObject, IID};
    }

private:
    // Note that the DoublePtrHelper is a private class, and can be created only by RefCntWeakPtr
    // Thus if no special effort is made, the lifetime of the instances of this class cannot be
    // longer than the lifetime of the creating object
    class DoublePtrHelper
    {
    public:
        DoublePtrHelper(RefCntAutoPtr& AutoPtr) noexcept :
            NewRawPtr{static_cast<T*>(AutoPtr)},
            m_pAutoPtr{std::addressof(AutoPtr)}
        {
        }

        DoublePtrHelper(DoublePtrHelper&& Helper) noexcept :
            NewRawPtr{Helper.NewRawPtr},
            m_pAutoPtr{Helper.m_pAutoPtr}
        {
            Helper.m_pAutoPtr = nullptr;
            Helper.NewRawPtr  = nullptr;
        }

        ~DoublePtrHelper()
        {
            if (m_pAutoPtr && static_cast<T*>(*m_pAutoPtr) != NewRawPtr)
            {
                m_pAutoPtr->Attach(NewRawPtr);
            }
        }

        T*&      operator*() noexcept { return NewRawPtr; }
        const T* operator*() const noexcept { return NewRawPtr; }

        // clang-format off
        operator T**() noexcept { return &NewRawPtr; }
        operator const T**() const noexcept { return &NewRawPtr; }
        // clang-format on

    private:
        T*             NewRawPtr;
        RefCntAutoPtr* m_pAutoPtr;

        // clang-format off
        DoublePtrHelper             (const DoublePtrHelper&) = delete;
        DoublePtrHelper& operator = (const DoublePtrHelper&) = delete;
        DoublePtrHelper& operator = (DoublePtrHelper&&)      = delete;
        // clang-format on
    };

public:
    DoublePtrHelper operator&()
    {
        return DoublePtrHelper(*this);
    }

    const DoublePtrHelper operator&() const
    {
        return DoublePtrHelper(*this);
    }

    T**       GetRawDblPtr() noexcept { return &m_pObject; }
    const T** GetRawDblPtr() const noexcept { return &m_pObject; }

    template <typename DstType, typename = typename std::enable_if<std::is_convertible<T*, DstType*>::value>::type>
    DstType** GetRawDblPtr() noexcept { return reinterpret_cast<DstType**>(&m_pObject); }
    template <typename DstType, typename = typename std::enable_if<std::is_convertible<T*, DstType*>::value>::type>
    DstType** GetRawDblPtr() const noexcept { return reinterpret_cast<DstType**>(&m_pObject); }

private:
    template <typename OtherType>
    friend class RefCntAutoPtr;

    T* m_pObject = nullptr;
};

/// Implementation of weak pointers
template <typename T>
class RefCntWeakPtr
{
public:
    explicit RefCntWeakPtr(T* pObj = nullptr) noexcept :
        m_pRefCounters{nullptr},
        m_pObject{pObj}
    {
        if (m_pObject)
        {
            m_pRefCounters = ValidatedCast<RefCountersImpl>(m_pObject->GetReferenceCounters());
            m_pRefCounters->AddWeakRef();
        }
    }

    ~RefCntWeakPtr()
    {
        Release();
    }

    RefCntWeakPtr(const RefCntWeakPtr& WeakPtr) noexcept :
        m_pRefCounters{WeakPtr.m_pRefCounters},
        m_pObject{WeakPtr.m_pObject}
    {
        if (m_pRefCounters)
            m_pRefCounters->AddWeakRef();
    }

    RefCntWeakPtr(RefCntWeakPtr&& WeakPtr) noexcept :
        m_pRefCounters{std::move(WeakPtr.m_pRefCounters)},
        m_pObject{std::move(WeakPtr.m_pObject)}
    {
        WeakPtr.m_pRefCounters = nullptr;
        WeakPtr.m_pObject      = nullptr;
    }

    explicit RefCntWeakPtr(RefCntAutoPtr<T>& AutoPtr) noexcept :
        m_pRefCounters{AutoPtr ? ValidatedCast<RefCountersImpl>(AutoPtr->GetReferenceCounters()) : nullptr},
        m_pObject{static_cast<T*>(AutoPtr)}
    {
        if (m_pRefCounters)
            m_pRefCounters->AddWeakRef();
    }

    RefCntWeakPtr& operator=(const RefCntWeakPtr& WeakPtr) noexcept
    {
        if (*this == WeakPtr)
            return *this;

        Release();
        m_pObject      = WeakPtr.m_pObject;
        m_pRefCounters = WeakPtr.m_pRefCounters;
        if (m_pRefCounters)
            m_pRefCounters->AddWeakRef();
        return *this;
    }

    RefCntWeakPtr& operator=(T* pObj) noexcept
    {
        return operator=(RefCntWeakPtr(pObj));
    }

    RefCntWeakPtr& operator=(RefCntWeakPtr&& WeakPtr) noexcept
    {
        if (*this == WeakPtr)
            return *this;

        Release();
        m_pObject              = std::move(WeakPtr.m_pObject);
        m_pRefCounters         = std::move(WeakPtr.m_pRefCounters);
        WeakPtr.m_pRefCounters = nullptr;
        WeakPtr.m_pObject      = nullptr;
        return *this;
    }

    RefCntWeakPtr& operator=(RefCntAutoPtr<T>& AutoPtr) noexcept
    {
        Release();
        m_pObject      = static_cast<T*>(AutoPtr);
        m_pRefCounters = m_pObject ? ValidatedCast<RefCountersImpl>(m_pObject->GetReferenceCounters()) : nullptr;
        if (m_pRefCounters)
            m_pRefCounters->AddWeakRef();
        return *this;
    }

    void Release() noexcept
    {
        if (m_pRefCounters)
            m_pRefCounters->ReleaseWeakRef();
        m_pRefCounters = nullptr;
        m_pObject      = nullptr;
    }

    /// \note This method may not be reliable in a multithreaded environment.
    ///       However, when false is returned, the strong pointer created from
    ///       this weak pointer will reliably be empty.
    bool IsValid() const noexcept
    {
        return m_pObject != nullptr && m_pRefCounters != nullptr && m_pRefCounters->GetNumStrongRefs() > 0;
    }

    /// Obtains a strong reference to the object
    RefCntAutoPtr<T> Lock()
    {
        RefCntAutoPtr<T> spObj;
        if (m_pRefCounters)
        {
            // Try to obtain pointer to the owner object.
            // spOwner is only used to keep the object
            // alive while obtaining strong reference from
            // the raw pointer m_pObject
            RefCntAutoPtr<IObject> spOwner;
            m_pRefCounters->GetObject(&spOwner);
            if (spOwner)
            {
                // If owner is alive, we can use our RAW pointer to
                // create strong reference
                spObj = m_pObject;
            }
            else
            {
                // Owner object has been destroyed. There is no reason
                // to keep this weak reference anymore
                Release();
            }
        }
        return spObj;
    }

    bool operator==(const RefCntWeakPtr& Ptr) const noexcept { return m_pRefCounters == Ptr.m_pRefCounters; }
    bool operator!=(const RefCntWeakPtr& Ptr) const noexcept { return m_pRefCounters != Ptr.m_pRefCounters; }

protected:
    RefCountersImpl* m_pRefCounters;
    // We need to store raw pointer to object itself,
    // because if the object is owned by another object,
    // m_pRefCounters->GetObject( &pObj ) will return
    // pointer to owner, which is not what we need.
    T* m_pObject;
};

} // namespace Diligent
