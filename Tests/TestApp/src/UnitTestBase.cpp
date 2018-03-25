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

#include <iomanip>
#include <ios>

#include "pch.h"
#include "UnitTestBase.h"
#include "Errors.h"

using namespace Diligent;

int UnitTestBase::m_TotalTests = 0;
size_t UnitTestBase::m_MaxNameLen = 0;

UnitTestBase::UnitTestBase(const char *Name) :
    m_TestName(Name)
{
    m_MaxNameLen = std::max(m_MaxNameLen, m_TestName.length());
    m_TestNum = ++m_TotalTests;
    LOG_INFO_MESSAGE("Created test ", m_TestNum, ": ", Name);
}

UnitTestBase::~UnitTestBase()
{
    const char* TestResltStr[] =
    {
        "UNKNOWN",
        "SKIPPED",
        "FAILED",
        "SUCCEEDED"
    };
    LOG_INFO_MESSAGE("Test ", std::setw(2), m_TestNum, "/", m_TotalTests, " - ", std::setw(m_MaxNameLen), std::left, m_TestName, " : ",
                     TestResltStr[static_cast<int>(m_TestResult)], ". ", m_TestResultInfo);
}
