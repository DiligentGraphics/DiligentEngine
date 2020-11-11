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

#include <stdexcept>
#include <string>
#include <iostream>

#include "DebugOutput.h"
#include "FormatString.hpp"

namespace Diligent
{

template <bool>
void ThrowIf(std::string&&)
{
}

template <>
inline void ThrowIf<true>(std::string&& msg)
{
    throw std::runtime_error(std::move(msg));
}

template <bool bThrowException, typename... ArgsType>
void LogError(bool IsFatal, const char* Function, const char* FullFilePath, int Line, const ArgsType&... Args)
{
    std::string FileName(FullFilePath);

    auto LastSlashPos = FileName.find_last_of("/\\");
    if (LastSlashPos != std::string::npos)
        FileName.erase(0, LastSlashPos + 1);
    auto Msg = FormatString(Args...);
    if (DebugMessageCallback != nullptr)
    {
        DebugMessageCallback(IsFatal ? DEBUG_MESSAGE_SEVERITY_FATAL_ERROR : DEBUG_MESSAGE_SEVERITY_ERROR, Msg.c_str(), Function, FileName.c_str(), Line);
    }
    else
    {
        // No callback set - output to cerr
        std::cerr << "Diligent Engine: " << (IsFatal ? "Fatal Error" : "Error") << " in " << Function << "() (" << FileName << ", " << Line << "): " << Msg << '\n';
    }
    ThrowIf<bThrowException>(std::move(Msg));
}

} // namespace Diligent



#define LOG_ERROR(...)                                                                                 \
    do                                                                                                 \
    {                                                                                                  \
        Diligent::LogError<false>(/*IsFatal=*/false, __FUNCTION__, __FILE__, __LINE__, ##__VA_ARGS__); \
    } while (false)


#define LOG_FATAL_ERROR(...)                                                                          \
    do                                                                                                \
    {                                                                                                 \
        Diligent::LogError<false>(/*IsFatal=*/true, __FUNCTION__, __FILE__, __LINE__, ##__VA_ARGS__); \
    } while (false)

#define LOG_ERROR_ONCE(...)             \
    do                                  \
    {                                   \
        static bool IsFirstTime = true; \
        if (IsFirstTime)                \
        {                               \
            LOG_ERROR(##__VA_ARGS__);   \
            IsFirstTime = false;        \
        }                               \
    } while (false)


#define LOG_ERROR_AND_THROW(...)                                                                      \
    do                                                                                                \
    {                                                                                                 \
        Diligent::LogError<true>(/*IsFatal=*/false, __FUNCTION__, __FILE__, __LINE__, ##__VA_ARGS__); \
    } while (false)

#define LOG_FATAL_ERROR_AND_THROW(...)                                                               \
    do                                                                                               \
    {                                                                                                \
        Diligent::LogError<true>(/*IsFatal=*/true, __FUNCTION__, __FILE__, __LINE__, ##__VA_ARGS__); \
    } while (false)


#define LOG_DEBUG_MESSAGE(Severity, ...)                                                                                            \
    do                                                                                                                              \
    {                                                                                                                               \
        auto _msg = Diligent::FormatString(__VA_ARGS__);                                                                            \
        if (Diligent::DebugMessageCallback != nullptr) Diligent::DebugMessageCallback(Severity, _msg.c_str(), nullptr, nullptr, 0); \
    } while (false)

#define LOG_FATAL_ERROR_MESSAGE(...) LOG_DEBUG_MESSAGE(Diligent::DEBUG_MESSAGE_SEVERITY_FATAL_ERROR, ##__VA_ARGS__)
#define LOG_ERROR_MESSAGE(...)       LOG_DEBUG_MESSAGE(Diligent::DEBUG_MESSAGE_SEVERITY_ERROR, ##__VA_ARGS__)
#define LOG_WARNING_MESSAGE(...)     LOG_DEBUG_MESSAGE(Diligent::DEBUG_MESSAGE_SEVERITY_WARNING, ##__VA_ARGS__)
#define LOG_INFO_MESSAGE(...)        LOG_DEBUG_MESSAGE(Diligent::DEBUG_MESSAGE_SEVERITY_INFO, ##__VA_ARGS__)


#define LOG_DEBUG_MESSAGE_ONCE(Severity, ...)           \
    do                                                  \
    {                                                   \
        static bool IsFirstTime = true;                 \
        if (IsFirstTime)                                \
        {                                               \
            LOG_DEBUG_MESSAGE(Severity, ##__VA_ARGS__); \
            IsFirstTime = false;                        \
        }                                               \
    } while (false)

#define LOG_FATAL_ERROR_MESSAGE_ONCE(...) LOG_DEBUG_MESSAGE_ONCE(Diligent::DEBUG_MESSAGE_SEVERITY_FATAL_ERROR, ##__VA_ARGS__)
#define LOG_ERROR_MESSAGE_ONCE(...)       LOG_DEBUG_MESSAGE_ONCE(Diligent::DEBUG_MESSAGE_SEVERITY_ERROR, ##__VA_ARGS__)
#define LOG_WARNING_MESSAGE_ONCE(...)     LOG_DEBUG_MESSAGE_ONCE(Diligent::DEBUG_MESSAGE_SEVERITY_WARNING, ##__VA_ARGS__)
#define LOG_INFO_MESSAGE_ONCE(...)        LOG_DEBUG_MESSAGE_ONCE(Diligent::DEBUG_MESSAGE_SEVERITY_INFO, ##__VA_ARGS__)


#define CHECK(Expr, Severity, ...)                      \
    do                                                  \
    {                                                   \
        if (!(Expr))                                    \
        {                                               \
            LOG_DEBUG_MESSAGE(Severity, ##__VA_ARGS__); \
        }                                               \
    } while (false)

#define CHECK_FATAL_ERR(Expr, ...) CHECK(Expr, Diligent::DEBUG_MESSAGE_SEVERITY_FATAL_ERROR, ##__VA_ARGS__)
#define CHECK_ERR(Expr, ...)       CHECK(Expr, Diligent::DEBUG_MESSAGE_SEVERITY_ERROR, ##__VA_ARGS__)
#define CHECK_WARN(Expr, ...)      CHECK(Expr, Diligent::DEBUG_MESSAGE_SEVERITY_WARNING, ##__VA_ARGS__)
#define CHECK_INFO(Expr, ...)      CHECK(Expr, Diligent::DEBUG_MESSAGE_SEVERITY_INFO, ##__VA_ARGS__)

#define CHECK_THROW(Expr, ...)                  \
    do                                          \
    {                                           \
        if (!(Expr))                            \
        {                                       \
            LOG_ERROR_AND_THROW(##__VA_ARGS__); \
        }                                       \
    } while (false)
