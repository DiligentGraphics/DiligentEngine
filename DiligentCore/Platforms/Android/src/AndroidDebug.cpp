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

#include "AndroidDebug.hpp"
#include "FormatString.hpp"
#include <android/log.h>
#include <csignal>

using namespace Diligent;

void AndroidDebug::AssertionFailed(const Char* Message, const char* Function, const char* File, int Line)
{
    auto AssertionFailedMessage = FormatAssertionFailedMessage(Message, Function, File, Line);
    OutputDebugMessage(DEBUG_MESSAGE_SEVERITY_ERROR, AssertionFailedMessage.c_str(), nullptr, nullptr, 0);

    raise(SIGTRAP);
};


void AndroidDebug::OutputDebugMessage(DEBUG_MESSAGE_SEVERITY Severity, const Char* Message, const char* Function, const char* File, int Line)
{
    auto msg = FormatDebugMessage(Severity, Message, Function, File, Line);

    static const android_LogPriority Priorities[] = {ANDROID_LOG_INFO, ANDROID_LOG_WARN, ANDROID_LOG_ERROR, ANDROID_LOG_FATAL};
    __android_log_print(Priorities[static_cast<int>(Severity)], "Diligent Engine", "%s", msg.c_str());
}

void DebugAssertionFailed(const Char* Message, const char* Function, const char* File, int Line)
{
    AndroidDebug::AssertionFailed(Message, Function, File, Line);
}

namespace Diligent
{

DebugMessageCallbackType DebugMessageCallback = AndroidDebug::OutputDebugMessage;

}
