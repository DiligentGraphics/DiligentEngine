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

#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

#include "UnitTestBase.h"

class MTResourceCreationTest : public UnitTestBase
{
public:
    MTResourceCreationTest(Diligent::IRenderDevice *pDevice, Diligent::IDeviceContext *pContext, Diligent::Uint32 NumThreads);
    void StartThreads();
    void StopThreads();

private:
    void ThreadWorkerFunc(bool bIsMasterThread);
    void WaitForThreadStart(int VarId);
    void StartThreads(int VarId);
    void WaitThreads(int VarId);

    Diligent::RefCntAutoPtr<Diligent::IRenderDevice> m_pDevice;
    Diligent::RefCntAutoPtr<Diligent::IDeviceContext> m_pContext;
    std::vector<std::thread> m_Threads;
    volatile bool m_bStopThreadsFlag = false;
    volatile bool m_bStopThreadsFlagInternal = false;

    std::mutex m_Mtx;
    std::condition_variable m_CondVar;
    bool m_bReleaseThread[2] = {false, false};
    std::atomic<int> m_NumThreadsCompleted;
    std::atomic<int> m_NumBuffersCreated;
    std::atomic<int> m_NumTexturesCreated;
    std::atomic<int> m_NumPSOCreated;
};
