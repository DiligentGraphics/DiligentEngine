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

#include <mutex>
#include <condition_variable>
#include <atomic>

#include "../../Platforms/Basic/interface/DebugUtilities.hpp"

namespace ThreadingTools
{

class Signal
{
public:
    Signal()
    {
        m_SignaledValue    = 0;
        m_NumThreadsAwaken = 0;
    }

    // http://en.cppreference.com/w/cpp/thread/condition_variable
    void Trigger(bool NotifyAll = false, int SignalValue = 1)
    {
        VERIFY(SignalValue != 0, "Signal value must not be zero");

        //  The thread that intends to modify the variable has to
        //  * acquire a std::mutex (typically via std::lock_guard)
        //  * perform the modification while the lock is held
        //  * execute notify_one or notify_all on the std::condition_variable (the lock does not need to be held for notification)
        {
            // std::condition_variable works only with std::unique_lock<std::mutex>
            std::lock_guard<std::mutex> Lock{m_Mutex};
            VERIFY(SignalValue != 0, "Signal value must not be 0");
            VERIFY(m_SignaledValue == 0 && m_NumThreadsAwaken == 0, "Not all threads have been awaken since the signal was triggered last time, or the signal has not been reset");
            m_SignaledValue = SignalValue;
        }
        // Unlocking is done before notifying, to avoid waking up the waiting
        // thread only to block again (see notify_one for details)
        if (NotifyAll)
            m_CondVar.notify_all();
        else
            m_CondVar.notify_one();
    }

    // WARNING!
    // If multiple threads are waiting for a signal in an infinite loop,
    // autoresetting the signal does not guarantee that one thread cannot
    // go through the loop twice. In this case, every thread must wait for its
    // own auto-reset signal or the threads must be blocked by another signal

    int Wait(bool AutoReset = false, int NumThreadsWaiting = 0)
    {
        //  Any thread that intends to wait on std::condition_variable has to
        //  * acquire a std::unique_lock<std::mutex>, on the SAME MUTEX as used to protect the shared variable
        //  * execute wait, wait_for, or wait_until. The wait operations atomically release the mutex
        //    and suspend the execution of the thread.
        //  * When the condition variable is notified, a timeout expires, or a spurious wakeup occurs,
        //    the thread is awakened, and the mutex is atomically reacquired:
        //    - The thread should then check the condition and resume waiting if the wake up was spurious.
        std::unique_lock<std::mutex> Lock(m_Mutex);
        // It is safe to check m_SignaledValue since we are holding
        // the mutex
        if (m_SignaledValue == 0)
        {
            m_CondVar.wait(Lock, [&] { return m_SignaledValue != 0; });
        }
        int SignaledValue = m_SignaledValue;
        // Count the number of threads awaken while holding the mutex
        ++m_NumThreadsAwaken;
        if (AutoReset)
        {
            VERIFY(NumThreadsWaiting > 0, "Number of waiting threads must not be 0 when auto resetting the signal");
            // Reset the signal while holding the mutex. If Trigger() is executed by another
            // thread, it will wait until we release the mutex
            if (m_NumThreadsAwaken == NumThreadsWaiting)
            {
                m_SignaledValue    = 0;
                m_NumThreadsAwaken = 0;
            }
        }
        return SignaledValue;
    }

    void Reset()
    {
        std::lock_guard<std::mutex> Lock{m_Mutex};
        m_SignaledValue    = 0;
        m_NumThreadsAwaken = 0;
    }

    bool IsTriggered() const { return m_SignaledValue != 0; }

private:
    std::mutex              m_Mutex;
    std::condition_variable m_CondVar;
    std::atomic_int         m_SignaledValue;
    std::atomic_int         m_NumThreadsAwaken;

    Signal(const Signal&) = delete;
    Signal& operator=(const Signal&) = delete;
};

} // namespace ThreadingTools
