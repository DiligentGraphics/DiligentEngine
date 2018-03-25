/*     Copyright 2015-2018 Egor Yusov
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF ANY PROPRIETARY RIGHTS.
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

// EngineSandbox.cpp : Defines the entry point for the application.
//

#include "pch.h"
#include <thread>
#include "SmartPointerTest.h"
#include "Errors.h"
#include "DefaultRawMemoryAllocator.h"

using namespace Diligent;

template<typename Type>
Type* MakeNewObj()
{
    return MakeNewRCObj<Type>().operator()();
}

void SmartPointerTest::CreateObject(Object **ppObj)
{
    *ppObj = MakeNewObj<Object>();
    (*ppObj)->AddRef();
}

void SmartPointerTest::WaitForThreadStart(int VarId)
{
    std::unique_lock<std::mutex> lk(m_Mtx);
    m_CondVar.wait(lk, [&]{return m_bThreadStart[VarId] || m_bStopThreads;});
}

void SmartPointerTest::StartThreadsAndWait(int VarId, size_t NumThreads)
{
    {
        std::unique_lock<std::mutex> lk(m_Mtx);
        m_bThreadStart[VarId] = true;
        m_NumThreadsCompleted = 0;
    }
    m_CondVar.notify_all();

    while((size_t)m_NumThreadsCompleted < NumThreads)
        std::this_thread::yield();
    m_bThreadStart[VarId] = false;
    VERIFY_EXPR(m_NumThreadsCompleted == NumThreads);
}

void SmartPointerTest::WorkerThreadFunc(SmartPointerTest *This, size_t ThreadNum)
{
    while(!This->m_bStopThreads)
    {
        for(int i =0; i < NumThreadInterations; ++i)
        {
            // Wait until main() sends data
            This->WaitForThreadStart(0);
            if(This->m_bStopThreads)
            {
                This->m_NumThreadsCompleted++;
                return;
            }

            {
                auto *pObject = This->m_pSharedObject;
                for(int j=0; j < 100; ++j)
                {
                    //LOG_INFO_MESSAGE("t",std::this_thread::get_id(), ": AddRef" );
                    pObject->m_Value++;
                    pObject->AddRef();
                }
                This->m_NumThreadsCompleted++;

                This->WaitForThreadStart(1);
                for(int j=0; j < 100; ++j)
                {
                    //LOG_INFO_MESSAGE("t",std::this_thread::get_id(), ": Release" );
                    pObject->m_Value--;
                    pObject->Release();
                }
                This->m_NumThreadsCompleted++;
            }

            {
                This->WaitForThreadStart(0);
                auto *pObject = This->m_pSharedObject;
                auto *pRefCounters = pObject->GetReferenceCounters();
                if (ThreadNum % 3 == 0)
                {
                    pObject->m_Value++;
                    pObject->AddRef();
                }
                else
                    pRefCounters->AddWeakRef();
                This->m_NumThreadsCompleted++;

                This->WaitForThreadStart(1);
                if (ThreadNum % 3 == 0)
                {
                    pObject->m_Value--;
                    pObject->Release();
                }
                else
                    pRefCounters->ReleaseWeakRef();
                This->m_NumThreadsCompleted++;
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

                This->WaitForThreadStart(0);
                auto *pObject = This->m_pSharedObject;
                RefCntWeakPtr<Object> weakPtr(pObject);
                RefCntAutoPtr<Object> strongPtr, strongPtr2;
                if (ThreadNum < 2)
                { 
                    strongPtr = pObject;
                    strongPtr->m_Value++;
                }
                else
                    weakPtr = WeakPtr(pObject);
                This->m_NumThreadsCompleted++;

                This->WaitForThreadStart(1);
                if (ThreadNum == 0)
                {
                    strongPtr->m_Value--;
                    strongPtr.Release();
                }
                else
                {
                    strongPtr2 = weakPtr.Lock();
                    if(strongPtr2)
                        strongPtr2->m_Value++;
                    weakPtr.Release();
                }
                This->m_NumThreadsCompleted++;
            }


            {
                This->WaitForThreadStart(0);
                auto *pObject = This->m_pSharedObject;
                RefCntWeakPtr<Object> weakPtr;
                RefCntAutoPtr<Object> strongPtr;
                if (ThreadNum % 4 == 0)
                {
                    strongPtr = pObject;
                    strongPtr->m_Value++;
                }
                else
                    weakPtr = WeakPtr(pObject);
                This->m_NumThreadsCompleted++;

                This->WaitForThreadStart(1);
                if (ThreadNum % 4 == 0)
                {
                    strongPtr->m_Value--;
                    strongPtr.Release();
                }
                else
                {
                    auto Ptr = weakPtr.Lock();
                    if(Ptr)
                        Ptr->m_Value++;
                    Ptr.Release();
                }
                This->m_NumThreadsCompleted++;
            }
        }
    }
}

void SmartPointerTest::StartConcurrencyTest()
{
    auto numCores = std::thread::hardware_concurrency();
    m_Threads.resize(numCores);
    for(auto &t : m_Threads)
        t = std::thread(WorkerThreadFunc, this, &t-m_Threads.data());
}

SmartPointerTest::~SmartPointerTest()
{
    m_bStopThreads = true;
    StartThreadsAndWait(0, m_Threads.size());

    for(auto &t : m_Threads)
        t.join();

    auto NumIterations = NumThreadInterations;
    std::stringstream infoss;
    infoss << "Performed " << m_NumTestsPerformed << " concurrency tests with " << NumIterations << " iterations on " << m_Threads.size() << " threads";
    SetStatus(TestResult::Succeeded, infoss.str().c_str());
}

void SmartPointerTest::RunConcurrencyTest()
{
    for(int i=0;i<NumThreadInterations; ++i)
    {
        m_pSharedObject = MakeNewObj<Object>();
       
        StartThreadsAndWait(0, m_Threads.size());

        StartThreadsAndWait(1, m_Threads.size());

        m_pSharedObject = MakeNewObj<Object>();

        StartThreadsAndWait(0, m_Threads.size());

        StartThreadsAndWait(1, m_Threads.size());

        m_pSharedObject = MakeNewObj<Object>();

        StartThreadsAndWait(0, m_Threads.size());

        StartThreadsAndWait(1, m_Threads.size());

        m_pSharedObject = MakeNewObj<Object>();

        StartThreadsAndWait(0, m_Threads.size());

        StartThreadsAndWait(1, m_Threads.size());
    }
    ++m_NumTestsPerformed;
}


SmartPointerTest::SmartPointerTest() :
    UnitTestBase("Smart pointer test"),
    m_pSharedObject(nullptr),
    m_NumThreadsCompleted(0)
{
    // Test constructors of RefCntAutoPtr
    {
        SmartPtr SP0;
        SmartPtr SP1(nullptr);
        auto *pRawPtr = MakeNewObj<Object>();
        SmartPtr SP2(pRawPtr);
        SmartPtr SP2_1(pRawPtr);

        SmartPtr SP3(SP0);
        SmartPtr SP4(SP2);
        SmartPtr SP5(std::move(SP3));
        SmartPtr SP6(std::move(SP4));
    }

    // Test Attach/Detach
    {
        {
            SmartPtr SP0;
            auto *pRawPtr = MakeNewObj<Object>();
            SP0.Attach(nullptr);
            SP0.Attach(pRawPtr);
            pRawPtr->AddRef();
        }

        {
            SmartPtr SP0;
            auto *pRawPtr = MakeNewObj<Object>();
            pRawPtr->AddRef();
            SP0.Attach(pRawPtr);
            SP0.Attach(nullptr);
        }

        {
            SmartPtr SP0(MakeNewObj<Object>());
            auto *pRawPtr = MakeNewObj<Object>();
            SP0.Attach(pRawPtr);
            pRawPtr->AddRef();
        }

        {
            SmartPtr SP0(MakeNewObj<Object>());
            auto *pRawPtr = MakeNewObj<Object>();
            pRawPtr->AddRef();
            SP0.Attach(pRawPtr);
            auto *pRawPtr2 = SP0.Detach();
            pRawPtr2->Release();

            auto *pRawPtr3 = SmartPtr().Detach();
            (void)pRawPtr3;
            auto *pRawPtr4 = SmartPtr(MakeNewObj<Object>()).Detach();
            (void)pRawPtr4;
            pRawPtr4->Release();
        }
    }

    // Test operator = 
    {
        SmartPtr SP0;
        auto pRawPtr1 = MakeNewObj<Object>();
        SmartPtr SP1(pRawPtr1);
        SmartPtr SP2(pRawPtr1);
        SP0 = SP0;
        SP0 = std::move(SP0);
        SP0 = nullptr;
        assert(SP0 == nullptr);

        SP1 = pRawPtr1;
        SP1 = SP1;
        SP1 = std::move(SP1);
        assert(SP1 == pRawPtr1);

        SP1 = SP2;
        SP1 = std::move(SP2);
        assert(SP1 == pRawPtr1);

        auto pRawPtr2 = MakeNewObj<Object>();
        SmartPtr SP3(pRawPtr2);

        SP0 = pRawPtr2;
        SmartPtr SP4;
        SP4 = SP3;
        SmartPtr SP5;
        SP5 = std::move(SP4);

        SP1 = pRawPtr2;
        SP1 = nullptr;
        SP1 = std::move(SP5);
    }

    // Test logical operators
    {
        auto pRawPtr1 = MakeNewObj<Object>();
        auto pRawPtr2 = MakeNewObj<Object>();
        SmartPtr SP0, SP1(pRawPtr1), SP2(pRawPtr1), SP3(pRawPtr2);
        assert( !SP0 );
        bool b1 = SP0.operator bool();
        assert( !b1 );
        if(SP0)
            assert( false );

        assert( !(!SP1) );
        assert( SP1 );
        assert( SP0 != SP1 );
        assert( SP0 == SP0 );
        assert( SP1 == SP1 );
        assert( SP1 == SP2 );
        assert( SP1 != SP3 );
        assert( SP0 < SP3 );
        assert( (SP1 < SP3) == (pRawPtr1<pRawPtr2) );
    }

    // Test operator &
    {
        SmartPtr SP0, SP1(MakeNewObj<Object>()), SP2, SP3, SP4(MakeNewObj<Object>());
        auto *pRawPtr = MakeNewObj<Object>();
        pRawPtr->AddRef();

        *static_cast<Object**>(&SP0) = pRawPtr;
        SP0.Detach();
        *&SP2 = pRawPtr;
        SP2.Detach();

        CreateObject(&SP3);

        CreateObject(&SP1);
        *static_cast<Object**>(&SP4) = pRawPtr;
            
        {
            SmartPtr SP5(MakeNewObj<Object>());
            auto pDblPtr = &SP5;
            *pDblPtr = MakeNewObj<Object>();
            (*pDblPtr)->AddRef();
            auto pDblPtr2 = &SP5;
            CreateObject(pDblPtr2);
        }

        SmartPtr SP6(MakeNewObj<Object>());
        // This will not work:
        // Object **pDblPtr3 = &SP6;
        // *pDblPtr3 = new Object;
    }

    // Test constructors of RefCntWeakPtr
    {
        SmartPtr SP0, SP1(MakeNewObj<Object>());
        WeakPtr WP0;
        WeakPtr WP1(WP0);
        WeakPtr WP2(SP0);
        WeakPtr WP3(SP1);
        WeakPtr WP4(WP3);
        WeakPtr WP5(std::move(WP0));
        WeakPtr WP6(std::move(WP4));

        auto *pRawPtr = MakeNewObj<Object>();
        pRawPtr->AddRef();
        WeakPtr WP7(pRawPtr);
        pRawPtr->Release();
    }

    // Test operator = 
    {
        auto *pRawPtr = MakeNewObj<Object>();
        SmartPtr SP0, SP1(pRawPtr);
        WeakPtr WP0, WP1(SP1), WP2(SP1);
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

    // Test logical operators
    {
        SmartPtr SP0, SP1(MakeNewObj<Object>());
        WeakPtr WP0, WP1(SP0), WP2(SP1), WP3(SP1);
        assert( WP0 == WP1 );
        assert( WP0 != WP2 );
        assert( WP2 == WP3 );
        SP1.Release();
        assert( WP2 == WP3 );
    }

    // Test Lock()
    {
        SmartPtr SP0, SP1(MakeNewObj<Object>());
        WeakPtr WP0, WP1(SP0), WP2(SP1);
        WeakPtr WP3(WP2);
        auto L1 = WP0.Lock();
        assert( !L1 );
        L1 = WP1.Lock();
        assert( !L1 );
        L1 = WP2.Lock();
        assert( L1 );
        L1 = WP3.Lock();
        assert( L1 );
        auto pRawPtr = SP1.Detach();
        L1.Release();

        L1 = WP3.Lock();
        assert( L1 );
        L1.Release();

        pRawPtr->Release();

        L1 = WP3.Lock();
        assert( !L1 );
    }
    
    {
        class OwnerTest : public RefCountedObject<IObject>
        {
        public:
            OwnerTest(IReferenceCounters *pRefCounters) : 
                RefCountedObject<IObject>(pRefCounters),
                Obj( NEW_RC_OBJ( DefaultRawMemoryAllocator::GetAllocator(), "Test object", Object, this)())
            {
            }

            virtual void QueryInterface( const INTERFACE_ID &IID, IObject **ppInterface ){}
            ~OwnerTest()
            { 
                Obj->~Object();
                DefaultRawMemoryAllocator::GetAllocator().Free(Obj);
            }
        private:
            Object *Obj;
        };

        OwnerTest *pOwnerObject = MakeNewObj<OwnerTest>();
        pOwnerObject->AddRef();
        pOwnerObject->Release();
    }

    {
        class SelfRefTest : public RefCountedObject<IObject>
        {
        public:
            SelfRefTest(IReferenceCounters *pRefCounters) : 
                RefCountedObject<IObject>(pRefCounters),
                wpSelf( this )
            {}

            virtual void QueryInterface( const Diligent::INTERFACE_ID &IID, IObject **ppInterface ){}
        private:
            Diligent::RefCntWeakPtr<SelfRefTest> wpSelf;
        };

        SelfRefTest *pSelfRefTest = MakeNewObj<SelfRefTest>();
        pSelfRefTest->AddRef();
        pSelfRefTest->Release();
    }
    
    {
        class ExceptionTest1 : public RefCountedObject<IObject>
        {
        public:
            ExceptionTest1(IReferenceCounters *pRefCounters)  :
                RefCountedObject<IObject>(pRefCounters),
                wpSelf(this)
            {
                throw std::runtime_error("test exception");
            }

            virtual void QueryInterface( const Diligent::INTERFACE_ID &IID, IObject **ppInterface ){}
        private:
            Diligent::RefCntWeakPtr<ExceptionTest1> wpSelf;
        };

        try
        {
            auto *pExceptionTest = MakeNewObj<ExceptionTest1>();
            (void)pExceptionTest;
        }
        catch(std::runtime_error &)
        {

        }
    }

    {
        class ExceptionTest2 : public RefCountedObject<IObject>
        {
        public:
            ExceptionTest2(IReferenceCounters *pRefCounters)  :
                RefCountedObject<IObject>(pRefCounters),
                wpSelf(this)
            {
                throw std::runtime_error("test exception");
            }

            virtual void QueryInterface( const Diligent::INTERFACE_ID &IID, IObject **ppInterface ){}
        private:
            Diligent::RefCntWeakPtr<ExceptionTest2> wpSelf;
        };

        try
        {
            auto *pExceptionTest = NEW_RC_OBJ( DefaultRawMemoryAllocator::GetAllocator(), "Test object", ExceptionTest2)();
            (void)pExceptionTest;
        }
        catch(std::runtime_error &)
        {

        }
    }

    {
        class ExceptionTest3 : public RefCountedObject<IObject>
        {
        public:
            ExceptionTest3(IReferenceCounters *pRefCounters)  :
                RefCountedObject<IObject>(pRefCounters),
                m_Member(*this)
            {
            }

            class Subclass
            {
            public:
                Subclass(ExceptionTest3 &parent) :
                    wpSelf(&parent)
                {
                    throw std::runtime_error("test exception");
                }
            private:
                Diligent::RefCntWeakPtr<ExceptionTest3> wpSelf;
            };
            virtual void QueryInterface( const Diligent::INTERFACE_ID &IID, IObject **ppInterface ){}
        private:
            Subclass m_Member;
        };

        try
        {
            auto *pExceptionTest = NEW_RC_OBJ( DefaultRawMemoryAllocator::GetAllocator(), "Test object", ExceptionTest3)();
            (void)pExceptionTest;
        }
        catch(std::runtime_error &)
        {

        }
    }

    {
        class OwnerObject : public RefCountedObject<IObject>
        {
        public:
            OwnerObject(IReferenceCounters *pRefCounters) : 
                RefCountedObject<IObject>(pRefCounters)
            {}

            void CreateMember()
            {
                try
                {
                    m_pMember = NEW_RC_OBJ( DefaultRawMemoryAllocator::GetAllocator(), "Test object", ExceptionTest4, this)(*this);
                }
                catch (...)
                {

                }
            }
            virtual void QueryInterface( const Diligent::INTERFACE_ID &IID, IObject **ppInterface ){}

            class ExceptionTest4 : public RefCountedObject<IObject>
            {
            public:
                ExceptionTest4(IReferenceCounters *pRefCounters, OwnerObject &owner)  :
                    RefCountedObject<IObject>(pRefCounters),
                    m_Member(owner, *this)
                {
                }

                class Subclass
                {
                public:
                    Subclass(OwnerObject &owner, ExceptionTest4 &parent) :
                        wpParent(&parent),
                        wpOwner(&owner)
                    {
                        throw std::runtime_error("test exception");
                    }
                private:
                    Diligent::RefCntWeakPtr<ExceptionTest4> wpParent;
                    Diligent::RefCntWeakPtr<OwnerObject> wpOwner;
                };
                virtual void QueryInterface( const Diligent::INTERFACE_ID &IID, IObject **ppInterface ){}
            private:
                Subclass m_Member;
            };

            RefCntAutoPtr<ExceptionTest4> m_pMember;
        };

        RefCntAutoPtr<OwnerObject> pOwner( NEW_RC_OBJ( DefaultRawMemoryAllocator::GetAllocator(), "Test object", OwnerObject)() );
        pOwner->CreateMember();
    }

    
    {
        class OwnerObject : public RefCountedObject<IObject>
        {
        public:
            OwnerObject(IReferenceCounters *pRefCounters) : 
                RefCountedObject<IObject>(pRefCounters)
            {
                m_pMember = NEW_RC_OBJ( DefaultRawMemoryAllocator::GetAllocator(), "Test object", ExceptionTest4, this)(*this);
            }

            virtual void QueryInterface( const Diligent::INTERFACE_ID &IID, IObject **ppInterface ){}

            class ExceptionTest4 : public RefCountedObject<IObject>
            {
            public:
                ExceptionTest4(IReferenceCounters *pRefCounters, OwnerObject &owner)  :
                    RefCountedObject<IObject>(pRefCounters),
                    m_Member(owner, *this)
                {
                }

                class Subclass
                {
                public:
                    Subclass(OwnerObject &owner, ExceptionTest4 &parent) :
                        wpParent(&parent),
                        wpOwner(&owner)
                    {
                        throw std::runtime_error("test exception");
                    }
                private:
                    Diligent::RefCntWeakPtr<ExceptionTest4> wpParent;
                    Diligent::RefCntWeakPtr<OwnerObject> wpOwner;
                };
                virtual void QueryInterface( const Diligent::INTERFACE_ID &IID, IObject **ppInterface ){}
            private:
                Subclass m_Member;
            };

            RefCntAutoPtr<ExceptionTest4> m_pMember;
        };

        try
        {
            RefCntAutoPtr<OwnerObject> pOwner( NEW_RC_OBJ( DefaultRawMemoryAllocator::GetAllocator(), "Test object", OwnerObject)() );
        }
        catch (...)
        {

        }
    }

    StartConcurrencyTest();
    RunConcurrencyTest();
}

