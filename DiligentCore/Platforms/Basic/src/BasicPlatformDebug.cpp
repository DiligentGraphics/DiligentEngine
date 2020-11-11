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

#include "BasicPlatformDebug.hpp"
#include "FormatString.hpp"
#include "BasicFileSystem.hpp"
#include <iostream>

using namespace Diligent;

String BasicPlatformDebug::FormatAssertionFailedMessage(const char* Message,
                                                        const char* Function, // type of __FUNCTION__
                                                        const char* File,     // type of __FILE__
                                                        int         Line)
{
    String FileName;
    BasicFileSystem::SplitFilePath(File, nullptr, &FileName);
    return Diligent::FormatString("Debug assertion failed in ", Function, "(), file ", FileName, ", line ", Line, ":\n", Message);
}

String BasicPlatformDebug::FormatDebugMessage(DEBUG_MESSAGE_SEVERITY Severity,
                                              const Char*            Message,
                                              const char*            Function, // type of __FUNCTION__
                                              const char*            File,     // type of __FILE__
                                              int                    Line)
{
    std::stringstream msg_ss;

    static const Char* const strSeverities[] = {"Info", "Warning", "ERROR", "CRITICAL ERROR"};
    const auto*              MessageSevery   = strSeverities[static_cast<int>(Severity)];

    msg_ss << "Diligent Engine: " << MessageSevery;
    if (Function != nullptr || File != nullptr)
    {
        msg_ss << " in ";
        if (Function != nullptr)
        {
            msg_ss << Function << "()";
            if (File != nullptr)
                msg_ss << " (";
        }

        if (File != nullptr)
        {
            msg_ss << File << ", " << Line << ')';
        }
    }
    msg_ss << ": " << Message << '\n';

    return msg_ss.str();
}
