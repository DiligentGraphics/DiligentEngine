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
/// Implementation of the template base class for reference counting objects

#include "../../Primitives/interface/Object.h"
#include "../../Primitives/interface/MemoryAllocator.h"
#include "../../Platforms/interface/Atomics.hpp"
#include "../../Platforms/Basic/interface/DebugUtilities.hpp"
#include "LockHelper.hpp"
#include "ValidatedCast.hpp"

namespace Diligent
{

// This class controls the lifetime of a refcounted object
class RefCountersImpl final : public IReferenceCounters
{
public:
    inline virtual ReferenceCounterValueType AddStrongRef() override final
    {
        VERIFY(m_ObjectState == ObjectState::Alive, "Attempting to increment strong reference counter for a destroyed or not itialized object!");
        VERIFY(m_ObjectWrapperBuffer[0] != 0 && m_ObjectWrapperBuffer[1] != 0, "Object wrapper is not initialized");
        return Atomics::AtomicIncrement(m_lNumStrongReferences);
    }

    template <class TPreObjectDestroy>
    inline ReferenceCounterValueType ReleaseStrongRef(TPreObjectDestroy PreObjectDestroy)
    {
        VERIFY(m_ObjectState == ObjectState::Alive, "Attempting to decrement strong reference counter for an object that is not alive");
        VERIFY(m_ObjectWrapperBuffer[0] != 0 && m_ObjectWrapperBuffer[1] != 0, "Object wrapper is not initialized");

        // Decrement strong reference counter without acquiring the lock.
        auto RefCount = Atomics::AtomicDecrement(m_lNumStrongReferences);
        VERIFY(RefCount >= 0, "Inconsistent call to ReleaseStrongRef()");
        if (RefCount == 0)
        {
            PreObjectDestroy();
            TryDestroyObject();
        }

        return RefCount;
    }

    inline virtual ReferenceCounterValueType ReleaseStrongRef() override final
    {
        return ReleaseStrongRef([]() {});
    }

    inline virtual ReferenceCounterValueType AddWeakRef() override final
    {
        return Atomics::AtomicIncrement(m_lNumWeakReferences);
    }

    inline virtual ReferenceCounterValueType ReleaseWeakRef() override final
    {
        // The method must be serialized!
        ThreadingTools::LockHelper Lock(m_LockFlag);
        // It is essentially important to check the number of weak references
        // while holding the lock. Otherwise reference counters object
        // may be destroyed twice if ReleaseStrongRef() is executed by other
        // thread.
        auto NumWeakReferences = Atomics::AtomicDecrement(m_lNumWeakReferences);
        VERIFY(NumWeakReferences >= 0, "Inconsistent call to ReleaseWeakRef()");

        // There are two special case when we must not destroy the ref counters object even
        // when NumWeakReferences == 0 && m_lNumStrongReferences == 0 :
        //
        //             This thread             |    Another thread - ReleaseStrongRef()
        //                                     |
        // 1. Lock the object                  |
        //                                     |
        // 2. Decrement m_lNumWeakReferences,  |   1. Decrement m_lNumStrongReferences,
        //    m_lNumWeakReferences==0          |      RefCount == 0
        //                                     |
        //                                     |   2. Start waiting for the lock to destroy
        //                                     |      the object, m_ObjectState != ObjectState::Destroyed
        // 3. Do not destroy reference         |
        //    counters, unlock                 |
        //                                     |   3. Acquire the lock,
        //                                     |      destroy the object,
        //                                     |      read m_lNumWeakReferences==0
        //                                     |      destroy the reference counters
        //

        // If an exception is thrown during the object construction and there is a weak pointer to the object itself,
        // we may get to this point, but should not destroy the reference counters, because it will be destroyed by MakeNewRCObj
        // Consider this example:
        //
        //   A ==sp==> B ---wp---> A
        //
        //   MakeNewRCObj::operator()
        //    try
        //    {
        //     A.ctor()
        //       B.ctor()
        //        wp.ctor m_lNumWeakReferences==1
        //        throw
        //        wp.dtor m_lNumWeakReferences==0, destroy this
        //    }
        //    catch(...)
        //    {
        //       Destory ref counters second time
        //    }
        //
        if (NumWeakReferences == 0 && /*m_lNumStrongReferences == 0 &&*/ m_ObjectState == ObjectState::Destroyed)
        {
            VERIFY_EXPR(m_lNumStrongReferences == 0);
            VERIFY(m_ObjectWrapperBuffer[0] == 0 && m_ObjectWrapperBuffer[1] == 0, "Object wrapper must be null");
            // m_ObjectState is set to ObjectState::Destroyed under the lock. If the state is not Destroyed,
            // ReleaseStrongRef() will take care of it.
            // Access to Object wrapper and decrementing m_lNumWeakReferences is atomic. Since we acquired the lock,
            // no other thread can access either of them.
            // Access to m_lNumStrongReferences is NOT PROTECTED by lock.

            // There are no more references to the ref counters object and the object itself
            // is already destroyed.
            // We can safely unlock it and destroy.
            // If we do not unlock it, this->m_LockFlag will expire,
            // which will cause Lock.~LockHelper() to crash.
            Lock.Unlock();
            SelfDestroy();
        }
        return NumWeakReferences;
    }

    inline virtual void GetObject(struct IObject** ppObject) override final
    {
        if (m_ObjectState != ObjectState::Alive)
            return; // Early exit

        // It is essential to INCREMENT REF COUNTER while HOLDING THE LOCK to make sure that
        // StrongRefCnt > 1 guarantees that the object is alive.

        // If other thread started deleting the object in ReleaseStrongRef(), then m_lNumStrongReferences==0
        // We must make sure only one thread is allowed to increment the counter to guarantee that if StrongRefCnt > 1,
        // there is at least one real strong reference left. Otherwise the following scenario may occur:
        //
        //                                      m_lNumStrongReferences == 1
        //
        //    Thread 1 - ReleaseStrongRef()    |     Thread 2 - GetObject()        |     Thread 3 - GetObject()
        //                                     |                                   |
        //  - Decrement m_lNumStrongReferences | -Increment m_lNumStrongReferences | -Increment m_lNumStrongReferences
        //  - Read RefCount == 0               | -Read StrongRefCnt==1             | -Read StrongRefCnt==2
        //    Destroy the object               |                                   | -Return reference to the soon
        //                                     |                                   |  to expire object
        //
        ThreadingTools::LockHelper Lock(m_LockFlag);

        auto StrongRefCnt = Atomics::AtomicIncrement(m_lNumStrongReferences);

        // Checking if m_ObjectState == ObjectState::Alive only is not reliable:
        //
        //           This thread                    |          Another thread
        //                                          |
        //   1. Acquire the lock                    |
        //                                          |    1. Decrement m_lNumStrongReferences
        //   2. Increment m_lNumStrongReferences    |    2. Test RefCount==0
        //   3. Read StrongRefCnt == 1              |    3. Start destroying the object
        //      m_ObjectState == ObjectState::Alive |
        //   4. DO NOT return the reference to      |    4. Wait for the lock, m_ObjectState == ObjectState::Alive
        //      the object                          |
        //   5. Decrement m_lNumStrongReferences    |
        //                                          |    5. Destroy the object

        if (m_ObjectState == ObjectState::Alive && StrongRefCnt > 1)
        {
            VERIFY(m_ObjectWrapperBuffer[0] != 0 && m_ObjectWrapperBuffer[1] != 0, "Object wrapper is not initialized");
            // QueryInterface() must not lock the object, or a deadlock happens.
            // The only other two methods that lock the object are ReleaseStrongRef()
            // and ReleaseWeakRef(), which are never called by QueryInterface()
            auto* pWrapper = reinterpret_cast<ObjectWrapperBase*>(m_ObjectWrapperBuffer);
            pWrapper->QueryInterface(IID_Unknown, ppObject);
        }
        Atomics::AtomicDecrement(m_lNumStrongReferences);
    }

    inline virtual ReferenceCounterValueType GetNumStrongRefs() const override final
    {
        return m_lNumStrongReferences;
    }

    inline virtual ReferenceCounterValueType GetNumWeakRefs() const override final
    {
        return m_lNumWeakReferences;
    }

private:
    template <typename AllocatorType, typename ObjectType>
    friend class MakeNewRCObj;

    RefCountersImpl() noexcept
    {
        m_lNumStrongReferences = 0;
        m_lNumWeakReferences   = 0;
#ifdef DILIGENT_DEBUG
        memset(m_ObjectWrapperBuffer, 0, sizeof(m_ObjectWrapperBuffer));
#endif
    }

    class ObjectWrapperBase
    {
    public:
        virtual void DestroyObject()                                                = 0;
        virtual void QueryInterface(const INTERFACE_ID& iid, IObject** ppInterface) = 0;
    };

    template <typename ObjectType, typename AllocatorType>
    class ObjectWrapper : public ObjectWrapperBase
    {
    public:
        ObjectWrapper(ObjectType* pObject, AllocatorType* pAllocator) noexcept :
            m_pObject{pObject},
            m_pAllocator{pAllocator}
        {}
        virtual void DestroyObject() override final
        {
            if (m_pAllocator)
            {
                m_pObject->~ObjectType();
                m_pAllocator->Free(m_pObject);
            }
            else
            {
                delete m_pObject;
            }
        }
        virtual void QueryInterface(const INTERFACE_ID& iid, IObject** ppInterface) override final
        {
            return m_pObject->QueryInterface(iid, ppInterface);
        }

    private:
        // It is crucially important that the type of the pointer
        // is ObjectType and not IObject, since the latter
        // does not have virtual dtor.
        ObjectType* const    m_pObject;
        AllocatorType* const m_pAllocator;
    };

    template <typename ObjectType, typename AllocatorType>
    void Attach(ObjectType* pObject, AllocatorType* pAllocator)
    {
        VERIFY(m_ObjectState == ObjectState::NotInitialized, "Object has already been attached");
        static_assert(sizeof(ObjectWrapper<ObjectType, AllocatorType>) == sizeof(m_ObjectWrapperBuffer), "Unexpected object wrapper size");
        new (m_ObjectWrapperBuffer) ObjectWrapper<ObjectType, AllocatorType>(pObject, pAllocator);
        m_ObjectState = ObjectState::Alive;
    }

    void TryDestroyObject()
    {
        // Since RefCount==0, there are no more strong references and the only place
        // where strong ref counter can be incremented is from GetObject().

        // If several threads were allowed to get to this point, there would
        // be serious risk that <this> had already been destroyed and m_LockFlag expired.
        // Consider the following scenario:
        //                                      |
        //             This thread              |             Another thread
        //                                      |
        //                      m_lNumStrongReferences == 1
        //                      m_lNumWeakReferences == 1
        //                                      |
        // 1. Decrement m_lNumStrongReferences  |
        //    Read RefCount==0, no lock acquired|
        //                                      |   1. Run GetObject()
        //                                      |      - acquire the lock
        //                                      |      - increment m_lNumStrongReferences
        //                                      |      - release the lock
        //                                      |
        //                                      |   2. Run ReleaseWeakRef()
        //                                      |      - decrement m_lNumWeakReferences
        //                                      |
        //                                      |   3. Run ReleaseStrongRef()
        //                                      |      - decrement m_lNumStrongReferences
        //                                      |      - read RefCount==0
        //
        //         Both threads will get to this point. The first one will destroy <this>
        //         The second one will read expired m_LockFlag

        //  IT IS CRUCIALLY IMPORTANT TO ASSURE THAT ONLY ONE THREAD WILL EVER
        //  EXECUTE THIS CODE

        // The sloution is to atomically increment strong ref counter in GetObject().
        // There are two possible scenarios depending on who first increments the counter:


        //                                                     Scenario I
        //
        //             This thread              |      Another thread - GetObject()         |   One more thread - GetObject()
        //                                      |                                           |
        //                       m_lNumStrongReferences == 1                                |
        //                                      |                                           |
        //                                      |   1. Acquire the lock                     |
        // 1. Decrement m_lNumStrongReferences  |                                           |   1. Wait for the lock
        // 2. Read RefCount==0                  |   2. Increment m_lNumStrongReferences     |
        // 3. Start destroying the object       |   3. Read StrongRefCnt == 1               |
        // 4. Wait for the lock                 |   4. DO NOT return the reference          |
        //                                      |      to the object                        |
        //                                      |   5. Decrement m_lNumStrongReferences     |
        // _  _  _  _  _  _  _  _  _  _  _  _  _|   6. Release the lock _  _  _  _  _  _  _ |_  _  _  _  _  _  _  _  _  _  _  _  _  _
        //                                      |                                           |   2. Acquire the lock
        //                                      |                                           |   3. Increment m_lNumStrongReferences
        //                                      |                                           |   4. Read StrongRefCnt == 1
        //                                      |                                           |   5. DO NOT return the reference
        //                                      |                                           |      to the object
        //                                      |                                           |   6. Decrement m_lNumStrongReferences
        //  _  _  _  _  _  _  _  _  _  _  _  _  | _  _  _  _  _  _  _  _  _  _  _  _  _  _  | _ 7. Release the lock _  _  _  _  _  _
        // 5. Acquire the lock                  |                                           |
        //   - m_lNumStrongReferences==0        |                                           |
        // 6. DESTROY the object                |                                           |
        //                                      |                                           |

        //  GetObject() MUST BE SERIALIZED for this to work properly!


        //                                   Scenario II
        //
        //             This thread              |      Another thread - GetObject()
        //                                      |
        //                       m_lNumStrongReferences == 1
        //                                      |
        //                                      |   1. Acquire the lock
        //                                      |   2. Increment m_lNumStrongReferences
        // 1. Decrement m_lNumStrongReferences  |
        // 2. Read RefCount>0                   |
        // 3. DO NOT destroy the object         |   3. Read StrongRefCnt > 1 (while m_lNumStrongReferences == 1)
        //                                      |   4. Return the reference to the object
        //                                      |       - Increment m_lNumStrongReferences
        //                                      |   5. Decrement m_lNumStrongReferences

#ifdef DILIGENT_DEBUG
        {
            Atomics::Long NumStrongRefs = m_lNumStrongReferences;
            VERIFY(NumStrongRefs == 0 || NumStrongRefs == 1, "Num strong references (", NumStrongRefs, ") is expected to be 0 or 1");
        }
#endif

        // Acquire the lock.
        ThreadingTools::LockHelper Lock(m_LockFlag);

        // GetObject() first acquires the lock, and only then increments and
        // decrements the ref counter. If it reads 1 after incremeting the counter,
        // it does not return the reference to the object and decrements the counter.
        // If we acquired the lock, GetObject() will not start until we are done
        VERIFY_EXPR(m_lNumStrongReferences == 0 && m_ObjectState == ObjectState::Alive);

        // Extra caution
        if (m_lNumStrongReferences == 0 && m_ObjectState == ObjectState::Alive)
        {
            VERIFY(m_ObjectWrapperBuffer[0] != 0 && m_ObjectWrapperBuffer[1] != 0, "Object wrapper is not initialized");
            // We cannot destroy the object while reference counters are locked as this will
            // cause a deadlock in cases like this:
            //
            //    A ==sp==> B ---wp---> A
            //
            //    RefCounters_A.Lock();
            //    delete A{
            //      A.~dtor(){
            //          B.~dtor(){
            //              wpA.ReleaseWeakRef(){
            //                  RefCounters_A.Lock(); // Deadlock
            //

            // So we copy the object wrapper and destroy the object after unlocking the
            // reference counters
            size_t ObjectWrapperBufferCopy[ObjectWrapperBufferSize];
            for (size_t i = 0; i < ObjectWrapperBufferSize; ++i)
                ObjectWrapperBufferCopy[i] = m_ObjectWrapperBuffer[i];
#ifdef DILIGENT_DEBUG
            memset(m_ObjectWrapperBuffer, 0, sizeof(m_ObjectWrapperBuffer));
#endif
            auto* pWrapper = reinterpret_cast<ObjectWrapperBase*>(ObjectWrapperBufferCopy);

            // In a multithreaded environment, reference counters object may
            // be destroyed at any time while m_pObject->~dtor() is running.
            // NOTE: m_pObject may not be the only object referencing m_pRefCounters.
            //       All objects that are owned by m_pObject will point to the same
            //       reference counters object.

            // Note that this is the only place where m_ObjectState is
            // modified after the ref counters object has been created
            m_ObjectState = ObjectState::Destroyed;
            // The object is now detached from the reference counters and it is if
            // it was destroyed since no one can obtain access to it.


            // It is essentially important to check the number of weak references
            // while the object is locked. Otherwise reference counters object
            // may be destroyed twice if ReleaseWeakRef() is executed by other thread:
            //
            //             This thread             |    Another thread - ReleaseWeakRef()
            //                                     |
            // 1. Decrement m_lNumStrongReferences,|
            //    m_lNumStrongReferences==0,       |
            //    acquire the lock, destroy        |
            //    the obj, release the lock        |
            //    m_lNumWeakReferences == 1        |
            //                                     |   1. Aacquire the lock,
            //                                     |      decrement m_lNumWeakReferences,
            //                                     |      m_lNumWeakReferences == 0,
            //                                     |      m_ObjectState == ObjectState::Destroyed
            //                                     |
            // 2. Read m_lNumWeakReferences == 0   |
            // 3. Destroy the ref counters obj     |   2. Destroy the ref counters obj
            //
            bool bDestroyThis = m_lNumWeakReferences == 0;
            // ReleaseWeakRef() decrements m_lNumWeakReferences, and checks it for
            // zero only after acquiring the lock. So if m_lNumWeakReferences==0, no
            // weak reference-related code may be running


            // We must explicitly unlock the object now to avoid deadlocks. Also,
            // if this is deleted, this->m_LockFlag will expire, which will cause
            // Lock.~LockHelper() to crash
            Lock.Unlock();

            // Destroy referenced object
            pWrapper->DestroyObject();

            // Note that <this> may be destroyed here already,
            // see comments in ~ControlledObjectType()
            if (bDestroyThis)
                SelfDestroy();
        }
    }

    void SelfDestroy()
    {
        delete this;
    }

    ~RefCountersImpl()
    {
        VERIFY(m_lNumStrongReferences == 0 && m_lNumWeakReferences == 0,
               "There exist outstanding references to the object being destroyed");
    }

    // No copies/moves
    // clang-format off
    RefCountersImpl             (const RefCountersImpl&) = delete;
    RefCountersImpl             (RefCountersImpl&&)      = delete;
    RefCountersImpl& operator = (const RefCountersImpl&) = delete;
    RefCountersImpl& operator = (RefCountersImpl&&)      = delete;
    // clang-format on

    struct IObjectStub : public IObject
    {
        virtual ~IObjectStub() = 0;
    };
    // MSVC starting with 19.25.28610.4 fails to compile sizeof(ObjectWrapper<IObject, IMemoryAllocator>) because
    // IObject does not have virtual destructor. The compiler is technically right, so we use IObjectStub,
    // which does have virtual destructor.
    static constexpr size_t ObjectWrapperBufferSize = sizeof(ObjectWrapper<IObjectStub, IMemoryAllocator>) / sizeof(size_t);

    size_t                   m_ObjectWrapperBuffer[ObjectWrapperBufferSize];
    Atomics::AtomicLong      m_lNumStrongReferences;
    Atomics::AtomicLong      m_lNumWeakReferences;
    ThreadingTools::LockFlag m_LockFlag;
    enum class ObjectState : Int32
    {
        NotInitialized,
        Alive,
        Destroyed
    };
    volatile ObjectState m_ObjectState = ObjectState::NotInitialized;
};


/// Base class for all reference counting objects
template <typename Base>
class RefCountedObject : public Base
{
public:
    template <typename... BaseCtorArgTypes>
    RefCountedObject(IReferenceCounters* pRefCounters, BaseCtorArgTypes&&... BaseCtorArgs) noexcept :
        // clang-format off
        Base          {std::forward<BaseCtorArgTypes>(BaseCtorArgs)...},
        m_pRefCounters{ValidatedCast<RefCountersImpl>(pRefCounters)   }
    // clang-format on
    {
        // If object is allocated on stack, ref counters will be null
        //VERIFY(pRefCounters != nullptr, "Reference counters must not be null")
    }

    // Virtual destructor makes sure all derived classes can be destroyed
    // through the pointer to the base class
    virtual ~RefCountedObject()
    {
        // WARNING! m_pRefCounters may be expired in scenarios like this:
        //
        //    A ==sp==> B ---wp---> A
        //
        //    RefCounters_A.ReleaseStrongRef(){ // NumStrongRef == 0, NumWeakRef == 1
        //      bDestroyThis = (m_lNumWeakReferences == 0) == false;
        //      delete A{
        //        A.~dtor(){
        //            B.~dtor(){
        //                wpA.ReleaseWeakRef(){ // NumStrongRef == 0, NumWeakRef == 0, m_pObject==nullptr
        //                    delete RefCounters_A;
        //        ...
        //        VERIFY( m_pRefCounters->GetNumStrongRefs() == 0 // Access violation!

        // This also may happen if one thread is executing ReleaseStrongRef(), while
        // another one is simultaneously running ReleaseWeakRef().

        //VERIFY( m_pRefCounters->GetNumStrongRefs() == 0,
        //        "There remain strong references to the object being destroyed" );
    }

    inline virtual IReferenceCounters* DILIGENT_CALL_TYPE GetReferenceCounters() const override final
    {
        VERIFY_EXPR(m_pRefCounters != nullptr);
        return m_pRefCounters;
    }

    inline virtual ReferenceCounterValueType DILIGENT_CALL_TYPE AddRef() override final
    {
        VERIFY_EXPR(m_pRefCounters != nullptr);
        // Since type of m_pRefCounters is RefCountersImpl,
        // this call will not be virtual and should be inlined
        return m_pRefCounters->AddStrongRef();
    }

    inline virtual ReferenceCounterValueType DILIGENT_CALL_TYPE Release() override
    {
        VERIFY_EXPR(m_pRefCounters != nullptr);
        // Since type of m_pRefCounters is RefCountersImpl,
        // this call will not be virtual and should be inlined
        return m_pRefCounters->ReleaseStrongRef();
    }

    template <class TPreObjectDestroy>
    inline ReferenceCounterValueType Release(TPreObjectDestroy PreObjectDestroy)
    {
        VERIFY_EXPR(m_pRefCounters != nullptr);
        return m_pRefCounters->ReleaseStrongRef(PreObjectDestroy);
    }

protected:
    template <typename AllocatorType, typename ObjectType>
    friend class MakeNewRCObj;

    friend class RefCountersImpl;


    // Operator delete can only be called from MakeNewRCObj if an exception is thrown,
    // or from RefCountersImpl when object is destroyed
    // It needs to be protected (not private!) to allow generation of destructors in derived classes

    void operator delete(void* ptr)
    {
        delete[] reinterpret_cast<Uint8*>(ptr);
    }

    template <typename ObjectAllocatorType>
    void operator delete(void* ptr, ObjectAllocatorType& Allocator, const Char* dbgDescription, const char* dbgFileName, const Int32 dbgLineNumber)
    {
        return Allocator.Free(ptr);
    }

private:
    // Operator new is private, and can only be called by MakeNewRCObj

    void* operator new(size_t Size)
    {
        return new Uint8[Size];
    }

    template <typename ObjectAllocatorType>
    void* operator new(size_t Size, ObjectAllocatorType& Allocator, const Char* dbgDescription, const char* dbgFileName, const Int32 dbgLineNumber)
    {
        return Allocator.Allocate(Size, dbgDescription, dbgFileName, dbgLineNumber);
    }


    // Note that the type of the reference counters is RefCountersImpl,
    // not IReferenceCounters. This avoids virtual calls from
    // AddRef() and Release() methods
    RefCountersImpl* const m_pRefCounters;
};


template <typename ObjectType, typename AllocatorType = IMemoryAllocator>
class MakeNewRCObj
{
public:
    MakeNewRCObj(AllocatorType& Allocator, const Char* Description, const char* FileName, const Int32 LineNumber, IObject* pOwner = nullptr) noexcept :
        // clang-format off
        m_pAllocator{&Allocator},
        m_pOwner{pOwner}
#ifdef DILIGENT_DEVELOPMENT
      , m_dvpDescription{Description}
      , m_dvpFileName   {FileName   }
      , m_dvpLineNumber {LineNumber }
    // clang-format on
#endif
    {
    }

    MakeNewRCObj(IObject* pOwner = nullptr) noexcept :
        // clang-format off
        m_pAllocator    {nullptr},
        m_pOwner        {pOwner }
#ifdef DILIGENT_DEVELOPMENT
      , m_dvpDescription{nullptr}
      , m_dvpFileName   {nullptr}
      , m_dvpLineNumber {0      }
#endif
    // clang-format on
    {}

    // clang-format off
    MakeNewRCObj           (const MakeNewRCObj&) = delete;
    MakeNewRCObj           (MakeNewRCObj&&)      = delete;
    MakeNewRCObj& operator=(const MakeNewRCObj&) = delete;
    MakeNewRCObj& operator=(MakeNewRCObj&&)      = delete;
    // clang-format on

    template <typename... CtorArgTypes>
    ObjectType* operator()(CtorArgTypes&&... CtorArgs)
    {
        RefCountersImpl*    pNewRefCounters = nullptr;
        IReferenceCounters* pRefCounters    = nullptr;
        if (m_pOwner != nullptr)
            pRefCounters = m_pOwner->GetReferenceCounters();
        else
        {
            // Constructor of RefCountersImpl class is private and only accessible
            // by methods of MakeNewRCObj
            pNewRefCounters = new RefCountersImpl();
            pRefCounters    = pNewRefCounters;
        }
        ObjectType* pObj = nullptr;
        try
        {
#ifndef DILIGENT_DEVELOPMENT
            static constexpr const char* m_dvpDescription = "<Unavailable in release build>";
            static constexpr const char* m_dvpFileName    = "<Unavailable in release build>";
            static constexpr Int32       m_dvpLineNumber  = -1;
#endif
            // Operators new and delete of RefCountedObject are private and only accessible
            // by methods of MakeNewRCObj
            if (m_pAllocator)
                pObj = new (*m_pAllocator, m_dvpDescription, m_dvpFileName, m_dvpLineNumber) ObjectType(pRefCounters, std::forward<CtorArgTypes>(CtorArgs)...);
            else
                pObj = new ObjectType(pRefCounters, std::forward<CtorArgTypes>(CtorArgs)...);
            if (pNewRefCounters != nullptr)
                pNewRefCounters->Attach<ObjectType, AllocatorType>(pObj, m_pAllocator);
        }
        catch (...)
        {
            if (pNewRefCounters != nullptr)
                pNewRefCounters->SelfDestroy();
            throw;
        }
        return pObj;
    }

private:
    AllocatorType* const m_pAllocator;
    IObject* const       m_pOwner;

#ifdef DILIGENT_DEVELOPMENT
    const Char* const m_dvpDescription;
    const char* const m_dvpFileName;
    Int32 const       m_dvpLineNumber;
#endif
};

#define NEW_RC_OBJ(Allocator, Desc, Type, ...) MakeNewRCObj<Type, typename std::remove_reference<decltype(Allocator)>::type>(Allocator, Desc, __FILE__, __LINE__, ##__VA_ARGS__)

} // namespace Diligent
