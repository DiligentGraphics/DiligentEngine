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

#pragma once

#include "RenderDevice.h"
#include "RefCntAutoPtr.h"
#include "RefCountedObjectImpl.h"
#include <mutex>
#include <condition_variable>
#include <atomic>

#include "UnitTestBase.h"

class SmartPointerTest : public UnitTestBase
{
public:    
    class Object : public Diligent::RefCountedObject<Diligent::IObject>
    {
    public:
        virtual void QueryInterface( const Diligent::INTERFACE_ID &IID, Diligent::IObject **ppInterface )
        {
            *ppInterface = nullptr;
            if(IID==Diligent::IID_Unknown)
            {
                *ppInterface = this;
                (*ppInterface)->AddRef();
            }
        }
    
        Object( Diligent::IReferenceCounters *pRefCounters) :
            RefCountedObject<Diligent::IObject>(pRefCounters),
            m_Value(0)
        {
        }

        ~Object(){}
        int m_Value;
    };

    typedef Diligent::RefCntAutoPtr<Object> SmartPtr;
    typedef Diligent::RefCntWeakPtr<Object> WeakPtr;

    void CreateObject(Object **ppObj);
    SmartPointerTest();
    ~SmartPointerTest();

    std::vector<std::thread> m_Threads;
    static void WorkerThreadFunc(SmartPointerTest *This, size_t ThreadNum);
    Object *m_pSharedObject = nullptr;
    void StartConcurrencyTest();
    void RunConcurrencyTest();
    void WaitForThreadStart(int VarId);
    void StartThreadsAndWait(int VarId, size_t NumThreads);
    std::atomic<int> m_NumThreadsCompleted;
    bool m_bStopThreads = false;
    int m_NumTestsPerformed = 0;
#ifdef _DEBUG
    static const int NumThreadInterations = 20;
#else
    static const int NumThreadInterations = 50;
#endif
    std::mutex m_Mtx;
    std::condition_variable m_CondVar;
    bool m_bThreadStart[2] = {false, false};
};
