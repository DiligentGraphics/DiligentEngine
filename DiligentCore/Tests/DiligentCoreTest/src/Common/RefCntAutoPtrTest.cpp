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

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <algorithm>

#include "DefaultRawMemoryAllocator.hpp"
#include "RefCntAutoPtr.hpp"
#include "RefCountedObjectImpl.hpp"
#include "ThreadSignal.hpp"

#include "gtest/gtest.h"

using namespace Diligent;

template <typename Type>
Type* MakeNewObj()
{
    return MakeNewRCObj<Type>{}();
}

namespace
{

class Object : public Diligent::RefCountedObject<Diligent::IObject>
{
public:
    static void DILIGENT_CALL_TYPE Create(Object** ppObj)
    {
        *ppObj = MakeNewObj<Object>();
        (*ppObj)->AddRef();
    }

    virtual void DILIGENT_CALL_TYPE QueryInterface(const Diligent::INTERFACE_ID& IID, Diligent::IObject** ppInterface)
    {
        *ppInterface = nullptr;
        if (IID == Diligent::IID_Unknown)
        {
            *ppInterface = this;
            (*ppInterface)->AddRef();
        }
    }

    Object(Diligent::IReferenceCounters* pRefCounters) :
        RefCountedObject<Diligent::IObject>{pRefCounters},
        m_Value(0)
    {
    }

    ~Object() {}
    std::atomic_int m_Value;
};


class DerivedObject : public Object
{
public:
    DerivedObject(Diligent::IReferenceCounters* pRefCounters) :
        Object{pRefCounters},
        m_Value2{1}
    {}
    int m_Value2;
};

using SmartPtr = Diligent::RefCntAutoPtr<Object>;
using WeakPtr  = Diligent::RefCntWeakPtr<Object>;

TEST(Common_RefCntAutoPtr, Constructors)
{
    {
        SmartPtr SP0;
        SmartPtr SP1(nullptr);
        auto*    pRawPtr = MakeNewObj<Object>();
        SmartPtr SP2(pRawPtr);
        SmartPtr SP2_1(pRawPtr);
        EXPECT_EQ(SP2, SP2_1);

        SmartPtr SP3(SP0);
        SmartPtr SP4(SP2);
        SmartPtr SP5(std::move(SP3));
        EXPECT_TRUE(!SP3);
        SmartPtr SP6(std::move(SP4));
        EXPECT_TRUE(!SP4);

        RefCntAutoPtr<DerivedObject> DerivedSP(MakeNewObj<DerivedObject>());

        SmartPtr SP7(DerivedSP);
        SmartPtr SP8(std::move(DerivedSP));
        EXPECT_EQ(SP7, SP8);
        EXPECT_TRUE(!DerivedSP);
    }
}

TEST(Common_RefCntAutoPtr, AttachDetach)
{
    {
        auto* pRawPtr = MakeNewObj<Object>();

        SmartPtr SP0;
        SP0.Attach(nullptr);
        EXPECT_TRUE(!SP0);
        SP0.Attach(pRawPtr);
        EXPECT_TRUE(SP0);
        pRawPtr->AddRef();
    }

    {
        auto* pRawPtr = MakeNewObj<Object>();

        SmartPtr SP0;
        pRawPtr->AddRef();
        SP0.Attach(pRawPtr);
        EXPECT_TRUE(SP0);
        SP0.Attach(nullptr);
        EXPECT_TRUE(!SP0);
    }

    {
        auto* pRawPtr = MakeNewObj<Object>();

        SmartPtr SP0(MakeNewObj<Object>());
        SP0.Attach(pRawPtr);
        EXPECT_TRUE(SP0);
        pRawPtr->AddRef();
    }

    {
        SmartPtr SP0(MakeNewObj<Object>());
        EXPECT_TRUE(SP0);

        auto* pRawPtr = MakeNewObj<Object>();
        pRawPtr->AddRef();
        SP0.Attach(pRawPtr);
        auto* pRawPtr2 = SP0.Detach();
        pRawPtr2->Release();

        auto* pRawPtr3 = SmartPtr().Detach();
        EXPECT_TRUE(pRawPtr3 == nullptr);
        auto* pRawPtr4 = SmartPtr(MakeNewObj<Object>()).Detach();
        EXPECT_TRUE(pRawPtr4 != nullptr);
        pRawPtr4->Release();
    }
}

TEST(Common_RefCntAutoPtr, OperatorEqual)
{
    {
        SmartPtr SP0;
        auto     pRawPtr1 = MakeNewObj<Object>();
        SmartPtr SP1(pRawPtr1);
        SmartPtr SP2(pRawPtr1);
        SP0 = SP0;
        SP0 = std::move(SP0);
        SP0 = nullptr;
        EXPECT_EQ(SP0, nullptr);

        SP1 = pRawPtr1;
        SP1 = SP1;
        SP1 = std::move(SP1);
        EXPECT_EQ(SP1, pRawPtr1);

        SP1 = SP2;
        SP1 = std::move(SP2);
        EXPECT_EQ(SP1, pRawPtr1);

        auto     pRawPtr2 = MakeNewObj<Object>();
        SmartPtr SP3(pRawPtr2);

        SP0 = pRawPtr2;
        SmartPtr SP4;
        SP4 = SP3;
        SmartPtr SP5;
        SP5 = std::move(SP4);
        EXPECT_TRUE(!SP4);

        SP1 = pRawPtr2;
        SP1 = nullptr;
        SP1 = std::move(SP5);
        EXPECT_TRUE(!SP5);

        RefCntAutoPtr<DerivedObject> DerivedSP(MakeNewObj<DerivedObject>());
        SP1 = DerivedSP;
        SP2 = std::move(DerivedSP);
        EXPECT_TRUE(!DerivedSP);
    }
}

TEST(Common_RefCntAutoPtr, LogicalOperators)
{
    {
        auto     pRawPtr1 = MakeNewObj<Object>();
        auto     pRawPtr2 = MakeNewObj<Object>();
        SmartPtr SP0, SP1(pRawPtr1), SP2(pRawPtr1), SP3(pRawPtr2);
        EXPECT_TRUE(!SP0);
        bool b1 = SP0.operator bool();
        EXPECT_TRUE(!b1);


        EXPECT_TRUE(!(!SP1));
        EXPECT_TRUE(SP1);
        EXPECT_TRUE(SP0 != SP1);
        EXPECT_TRUE(SP0 == SP0);
        EXPECT_TRUE(SP1 == SP1);
        EXPECT_TRUE(SP1 == SP2);
        EXPECT_TRUE(SP1 != SP3);
        EXPECT_TRUE(SP0 < SP3);
        EXPECT_TRUE((SP1 < SP3) == (pRawPtr1 < pRawPtr2));
    }
}

TEST(Common_RefCntAutoPtr, OperatorAmpersand)
{
    {
        SmartPtr SP0, SP1(MakeNewObj<Object>()), SP2, SP3, SP4(MakeNewObj<Object>());
        auto*    pRawPtr = MakeNewObj<Object>();
        pRawPtr->AddRef();

        *static_cast<Object**>(&SP0) = pRawPtr;
        SP0.Detach();
        *&SP2 = pRawPtr;
        SP2.Detach();

        Object::Create(&SP3);

        Object::Create(&SP1);
        *static_cast<Object**>(&SP4) = pRawPtr;

        {
            SmartPtr SP5(MakeNewObj<Object>());
            auto     pDblPtr = &SP5;
            *pDblPtr         = MakeNewObj<Object>();
            (*pDblPtr)->AddRef();
            auto pDblPtr2 = &SP5;
            Object::Create(pDblPtr2);
        }

        SmartPtr SP6(MakeNewObj<Object>());
        // This will not work:
        // Object **pDblPtr3 = &SP6;
        // *pDblPtr3 = new Object;
    }
}

TEST(Common_RefCntWeakPtr, Constructors)
{
    {
        SmartPtr SP0, SP1(MakeNewObj<Object>());
        WeakPtr  WP0;
        WeakPtr  WP1(WP0);
        WeakPtr  WP2(SP0);
        WeakPtr  WP3(SP1);
        WeakPtr  WP4(WP3);
        WeakPtr  WP5(std::move(WP0));
        WeakPtr  WP6(std::move(WP4));

        auto* pRawPtr = MakeNewObj<Object>();
        pRawPtr->AddRef();
        WeakPtr WP7(pRawPtr);
        pRawPtr->Release();
    }
}


TEST(Common_RefCntWeakPtr, OperatorEqual)
{
    {
        auto*    pRawPtr = MakeNewObj<Object>();
        SmartPtr SP0, SP1(pRawPtr);
        WeakPtr  WP0, WP1(SP1), WP2(SP1);
        WP0 = WP0;
        WP0 = std::move(WP0);
        WP1 = WP1;
        WP1 = std::move(WP1);
        WP1 = WP2;
        WP1 = std::move(WP2);
        WP1 = pRawPtr;
        WP0 = pRawPtr;
        WP0.Release();
        WP0 = WP2;

        WP1 = WP0;
        WP0 = SP1;
        WP2 = std::move(WP1);
    }
}

TEST(Common_RefCntWeakPtr, Lock)
{
    {
        SmartPtr SP0, SP1(MakeNewObj<Object>());
        WeakPtr  WP0, WP1(SP0), WP2(SP1), WP3(SP1);
        assert(WP0 == WP1);
        assert(WP0 != WP2);
        assert(WP2 == WP3);
        SP1.Release();
        assert(WP2 == WP3);
    }

    // Test Lock()
    {
        SmartPtr SP0, SP1(MakeNewObj<Object>());
        WeakPtr  WP0, WP1(SP0), WP2(SP1);
        WeakPtr  WP3(WP2);
        auto     L1 = WP0.Lock();
        assert(!L1);
        L1 = WP1.Lock();
        assert(!L1);
        L1 = WP2.Lock();
        assert(L1);
        L1 = WP3.Lock();
        assert(L1);
        auto pRawPtr = SP1.Detach();
        L1.Release();

        L1 = WP3.Lock();
        assert(L1);
        L1.Release();

        pRawPtr->Release();

        L1 = WP3.Lock();
        assert(!L1);
    }
}

TEST(Common_RefCntAutoPtr, Misc)
{
    {
        class OwnerTest : public RefCountedObject<IObject>
        {
        public:
            OwnerTest(IReferenceCounters* pRefCounters) :
                RefCountedObject<IObject>(pRefCounters),
                Obj(NEW_RC_OBJ(DefaultRawMemoryAllocator::GetAllocator(), "Test object", Object, this)())
            {
            }

            virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) {}
            ~OwnerTest()
            {
                Obj->~Object();
                DefaultRawMemoryAllocator::GetAllocator().Free(Obj);
            }

        private:
            Object* Obj;
        };

        OwnerTest* pOwnerObject = MakeNewObj<OwnerTest>();
        pOwnerObject->AddRef();
        pOwnerObject->Release();
    }

    {
        class SelfRefTest : public RefCountedObject<IObject>
        {
        public:
            SelfRefTest(IReferenceCounters* pRefCounters) :
                RefCountedObject<IObject>(pRefCounters),
                wpSelf(this)
            {}

            virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) {}

        private:
            RefCntWeakPtr<SelfRefTest> wpSelf;
        };

        SelfRefTest* pSelfRefTest = MakeNewObj<SelfRefTest>();
        pSelfRefTest->AddRef();
        pSelfRefTest->Release();
    }

    {
        class ExceptionTest1 : public RefCountedObject<IObject>
        {
        public:
            ExceptionTest1(IReferenceCounters* pRefCounters) :
                RefCountedObject<IObject>(pRefCounters),
                wpSelf(this)
            {
                throw std::runtime_error("test exception");
            }

            virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) {}

        private:
            RefCntWeakPtr<ExceptionTest1> wpSelf;
        };

        try
        {
            auto* pExceptionTest = MakeNewObj<ExceptionTest1>();
            (void)pExceptionTest;
        }
        catch (std::runtime_error&)
        {
        }
    }

    {
        class ExceptionTest2 : public RefCountedObject<IObject>
        {
        public:
            ExceptionTest2(IReferenceCounters* pRefCounters) :
                RefCountedObject<IObject>(pRefCounters),
                wpSelf(this)
            {
                throw std::runtime_error("test exception");
            }

            virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) {}

        private:
            RefCntWeakPtr<ExceptionTest2> wpSelf;
        };

        try
        {
            auto* pExceptionTest = NEW_RC_OBJ(DefaultRawMemoryAllocator::GetAllocator(), "Test object", ExceptionTest2)();
            (void)pExceptionTest;
        }
        catch (std::runtime_error&)
        {
        }
    }

    {
        class ExceptionTest3 : public RefCountedObject<IObject>
        {
        public:
            ExceptionTest3(IReferenceCounters* pRefCounters) :
                RefCountedObject<IObject>(pRefCounters),
                m_Member(*this)
            {
            }

            class Subclass
            {
            public:
                Subclass(ExceptionTest3& parent) :
                    wpSelf(&parent)
                {
                    throw std::runtime_error("test exception");
                }

            private:
                RefCntWeakPtr<ExceptionTest3> wpSelf;
            };
            virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) {}

        private:
            Subclass m_Member;
        };

        try
        {
            auto* pExceptionTest = NEW_RC_OBJ(DefaultRawMemoryAllocator::GetAllocator(), "Test object", ExceptionTest3)();
            (void)pExceptionTest;
        }
        catch (std::runtime_error&)
        {
        }
    }

    {
        class OwnerObject : public RefCountedObject<IObject>
        {
        public:
            OwnerObject(IReferenceCounters* pRefCounters) :
                RefCountedObject<IObject>(pRefCounters)
            {}

            void CreateMember()
            {
                try
                {
                    m_pMember = NEW_RC_OBJ(DefaultRawMemoryAllocator::GetAllocator(), "Test object", ExceptionTest4, this)(*this);
                }
                catch (...)
                {
                }
            }
            virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) {}

            class ExceptionTest4 : public RefCountedObject<IObject>
            {
            public:
                ExceptionTest4(IReferenceCounters* pRefCounters, OwnerObject& owner) :
                    RefCountedObject<IObject>(pRefCounters),
                    m_Member(owner, *this)
                {
                }

                class Subclass
                {
                public:
                    Subclass(OwnerObject& owner, ExceptionTest4& parent) :
                        wpParent(&parent),
                        wpOwner(&owner)
                    {
                        throw std::runtime_error("test exception");
                    }

                private:
                    RefCntWeakPtr<ExceptionTest4> wpParent;
                    RefCntWeakPtr<OwnerObject>    wpOwner;
                };
                virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) {}

            private:
                Subclass m_Member;
            };

            RefCntAutoPtr<ExceptionTest4> m_pMember;
        };

        RefCntAutoPtr<OwnerObject> pOwner(NEW_RC_OBJ(DefaultRawMemoryAllocator::GetAllocator(), "Test object", OwnerObject)());
        pOwner->CreateMember();
    }


    {
        class OwnerObject : public RefCountedObject<IObject>
        {
        public:
            OwnerObject(IReferenceCounters* pRefCounters) :
                RefCountedObject<IObject>(pRefCounters)
            {
                m_pMember = NEW_RC_OBJ(DefaultRawMemoryAllocator::GetAllocator(), "Test object", ExceptionTest4, this)(*this);
            }

            virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) {}

            class ExceptionTest4 : public RefCountedObject<IObject>
            {
            public:
                ExceptionTest4(IReferenceCounters* pRefCounters, OwnerObject& owner) :
                    RefCountedObject<IObject>(pRefCounters),
                    m_Member(owner, *this)
                {
                }

                class Subclass
                {
                public:
                    Subclass(OwnerObject& owner, ExceptionTest4& parent) :
                        wpParent(&parent),
                        wpOwner(&owner)
                    {
                        throw std::runtime_error("test exception");
                    }

                private:
                    RefCntWeakPtr<ExceptionTest4> wpParent;
                    RefCntWeakPtr<OwnerObject>    wpOwner;
                };
                virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) {}

            private:
                Subclass m_Member;
            };

            RefCntAutoPtr<ExceptionTest4> m_pMember;
        };

        try
        {
            RefCntAutoPtr<OwnerObject> pOwner(NEW_RC_OBJ(DefaultRawMemoryAllocator::GetAllocator(), "Test object", OwnerObject)());
        }
        catch (...)
        {
        }
    }

    {
        class TestObject : public RefCountedObject<IObject>
        {
        public:
            TestObject(IReferenceCounters* pRefCounters) :
                RefCountedObject<IObject>(pRefCounters)
            {
            }

            virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override final {}

            inline virtual Atomics::Long DILIGENT_CALL_TYPE Release() override final
            {
                return RefCountedObject<IObject>::Release(
                    [&]() //
                    {
                        ppWeakPtr->Release();
                    } //
                );
            }
            RefCntWeakPtr<TestObject>* ppWeakPtr = nullptr;
        };

        RefCntAutoPtr<TestObject> pObj(NEW_RC_OBJ(DefaultRawMemoryAllocator::GetAllocator(), "Test object", TestObject)());
        RefCntWeakPtr<TestObject> pWeakPtr(pObj);

        pObj->ppWeakPtr = &pWeakPtr;
        pObj.Release();
    }
}

class RefCntAutoPtrThreadingTest
{
public:
    ~RefCntAutoPtrThreadingTest();

    void StartConcurrencyTest();
    void RunConcurrencyTest();

    static void WorkerThreadFunc(RefCntAutoPtrThreadingTest* This, size_t ThreadNum);

    void StartWorkerThreadsAndWait(int SignalIdx);
    void WaitSiblingWorkerThreads(int SignalIdx);

    std::vector<std::thread> m_Threads;

    Object* m_pSharedObject = nullptr;
#ifdef DILIGENT_DEBUG
    static const int NumThreadInterations = 10000;
#else
    static const int NumThreadInterations = 50000;
#endif
    ThreadingTools::Signal m_WorkerThreadSignal[2];
    ThreadingTools::Signal m_MainThreadSignal;

    std::mutex      m_NumThreadsCompletedMtx;
    std::atomic_int m_NumThreadsCompleted[2];
    std::atomic_int m_NumThreadsReady;
};


RefCntAutoPtrThreadingTest::~RefCntAutoPtrThreadingTest()
{
    m_WorkerThreadSignal[0].Trigger(true, -1);

    for (auto& t : m_Threads)
        t.join();

    LOG_INFO_MESSAGE("Performed ", int{NumThreadInterations}, " iterations on ", m_Threads.size(), " threads");
}

void RefCntAutoPtrThreadingTest::WaitSiblingWorkerThreads(int SignalIdx)
{
    auto NumThreads = static_cast<int>(m_Threads.size());
    if (++m_NumThreadsCompleted[SignalIdx] == NumThreads)
    {
        ASSERT_FALSE(m_WorkerThreadSignal[1 - SignalIdx].IsTriggered());
        m_MainThreadSignal.Trigger();
    }
    else
    {
        while (m_NumThreadsCompleted[SignalIdx] < NumThreads)
            std::this_thread::yield();
    }
}

void RefCntAutoPtrThreadingTest::StartWorkerThreadsAndWait(int SignalIdx)
{
    m_NumThreadsCompleted[SignalIdx] = 0;
    m_WorkerThreadSignal[SignalIdx].Trigger(true);

    m_MainThreadSignal.Wait(true, 1);
}

void RefCntAutoPtrThreadingTest::WorkerThreadFunc(RefCntAutoPtrThreadingTest* This, size_t ThreadNum)
{
    const int NumThreads = static_cast<int>(This->m_Threads.size());
    while (true)
    {
        for (int i = 0; i < NumThreadInterations; ++i)
        {
            // Wait until main() sends data
            auto SignaledValue = This->m_WorkerThreadSignal[0].Wait(true, NumThreads);
            if (SignaledValue < 0)
            {
                return;
            }

            {
                auto* pObject = This->m_pSharedObject;
                for (int j = 0; j < 100; ++j)
                {
                    //LOG_INFO_MESSAGE("t",std::this_thread::get_id(), ": AddRef" );
                    pObject->m_Value++;
                    pObject->AddRef();
                }
                This->WaitSiblingWorkerThreads(0);

                This->m_WorkerThreadSignal[1].Wait(true, NumThreads);
                for (int j = 0; j < 100; ++j)
                {
                    //LOG_INFO_MESSAGE("t",std::this_thread::get_id(), ": Release" );
                    pObject->m_Value--;
                    pObject->Release();
                }
                This->WaitSiblingWorkerThreads(1);
            }

            {
                This->m_WorkerThreadSignal[0].Wait(true, NumThreads);
                auto* pObject      = This->m_pSharedObject;
                auto* pRefCounters = pObject->GetReferenceCounters();
                if (ThreadNum % 3 == 0)
                {
                    pObject->m_Value++;
                    pObject->AddRef();
                }
                else
                    pRefCounters->AddWeakRef();
                This->WaitSiblingWorkerThreads(0);

                This->m_WorkerThreadSignal[1].Wait(true, NumThreads);
                if (ThreadNum % 3 == 0)
                {
                    pObject->m_Value--;
                    pObject->Release();
                }
                else
                    pRefCounters->ReleaseWeakRef();
                This->WaitSiblingWorkerThreads(1);
            }

            {
                // Test interferences of ReleaseStrongRef() and GetObject()

                // Goal: catch scenario when GetObject() runs between
                // AtomicDecrement() and acquiring the lock in ReleaseStrongRef():

                //                       m_lNumStrongReferences == 1
                //

                //                                   Scenario I
                //
                //             Thread 1                 |                  Thread 2             |            Thread 3
                //                                      |                                       |
                //                                      |                                       |
                //                                      |                                       |
                //                                      |   1. Acquire the lock                 |
                //                                      |   2. Increment m_lNumStrongReferences |
                // 1. Decrement m_lNumStrongReferences  |   3. Read StrongRefCnt > 1            |
                // 2. Test RefCount!=0                  |   4. Return the reference to object   |
                // 3. DO NOT destroy the object         |                                       |
                // 4. Wait for the lock                 |                                       |


                //                                   Scenario I
                //
                //             Thread 1                 |                  Thread 2             |            Thread 3
                //                                      |                                       |
                //                                      |                                       |
                // 1. Decrement m_lNumStrongReferences  |                                       |
                //                                      |   1. Acquire the lock                 |
                // 2. Test RefCount==0                  |   2. Increment m_lNumStrongReferences |
                // 3. Start destroying the object       |   3. Read StrongRefCnt == 1           |
                // 4. Wait for the lock                 |   4. DO NOT create the object         |
                //                                      |   5. Decrement m_lNumStrongReferences |
                //                                      |                                       | 1. Acquire the lock
                //                                      |                                       | 2. Increment m_lNumStrongReferences
                //                                      |                                       | 3. Read StrongRefCnt == 1
                //                                      |                                       | 4. DO NOT create the object
                //                                      |                                       | 5. Decrement m_lNumStrongReferences
                // 5. Acquire the lock                  |
                // 6. DESTROY the object                |

                This->m_WorkerThreadSignal[0].Wait(true, NumThreads);
                auto* pObject = This->m_pSharedObject;

                RefCntWeakPtr<Object> weakPtr(pObject);
                RefCntAutoPtr<Object> strongPtr, strongPtr2;
                if (ThreadNum < 2)
                {
                    strongPtr = pObject;
                    strongPtr->m_Value++;
                }
                else
                    weakPtr = WeakPtr(pObject);
                This->WaitSiblingWorkerThreads(0);

                This->m_WorkerThreadSignal[1].Wait(true, NumThreads);
                if (ThreadNum == 0)
                {
                    strongPtr->m_Value--;
                    strongPtr.Release();
                }
                else
                {
                    strongPtr2 = weakPtr.Lock();
                    if (strongPtr2)
                        strongPtr2->m_Value++;
                    weakPtr.Release();
                }
                This->WaitSiblingWorkerThreads(1);
            }


            {
                This->m_WorkerThreadSignal[0].Wait(true, NumThreads);
                auto* pObject = This->m_pSharedObject;

                RefCntWeakPtr<Object> weakPtr;
                RefCntAutoPtr<Object> strongPtr;
                if (ThreadNum % 4 == 0)
                {
                    strongPtr = pObject;
                    strongPtr->m_Value++;
                }
                else
                    weakPtr = WeakPtr(pObject);
                This->WaitSiblingWorkerThreads(0);

                This->m_WorkerThreadSignal[1].Wait(true, NumThreads);
                if (ThreadNum % 4 == 0)
                {
                    strongPtr->m_Value--;
                    strongPtr.Release();
                }
                else
                {
                    auto Ptr = weakPtr.Lock();
                    if (Ptr)
                        Ptr->m_Value++;
                    Ptr.Release();
                }
                This->WaitSiblingWorkerThreads(1);
            }
        }
    }
}

void RefCntAutoPtrThreadingTest::StartConcurrencyTest()
{
    auto numCores = std::thread::hardware_concurrency();
    m_Threads.resize(std::max(numCores, 4u));
    for (auto& t : m_Threads)
        t = std::thread(WorkerThreadFunc, this, &t - m_Threads.data());
}

void RefCntAutoPtrThreadingTest::RunConcurrencyTest()
{
    for (int i = 0; i < NumThreadInterations; ++i)
    {
        m_pSharedObject = MakeNewObj<Object>();

        StartWorkerThreadsAndWait(0);

        StartWorkerThreadsAndWait(1);

        m_pSharedObject = MakeNewObj<Object>();

        StartWorkerThreadsAndWait(0);

        StartWorkerThreadsAndWait(1);

        m_pSharedObject = MakeNewObj<Object>();

        StartWorkerThreadsAndWait(0);

        StartWorkerThreadsAndWait(1);

        m_pSharedObject = MakeNewObj<Object>();

        StartWorkerThreadsAndWait(0);

        StartWorkerThreadsAndWait(1);
    }
}

TEST(Common_RefCntAutoPtr, Threading)
{
    RefCntAutoPtrThreadingTest ThreadingTest;
    ThreadingTest.StartConcurrencyTest();
    ThreadingTest.RunConcurrencyTest();
}

} // namespace
