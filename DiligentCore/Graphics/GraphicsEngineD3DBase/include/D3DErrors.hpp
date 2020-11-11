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

#include <string.h>

#include "Errors.hpp"

/// \file
/// Declaration of Diligent::ComErrorDesc class

namespace Diligent
{

/// Helper class that provides description of a COM error
class ComErrorDesc
{
public:
    ComErrorDesc(HRESULT hr)
    {
        auto NumCharsWritten = FormatMessageA(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            hr,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            m_Msg,
            _countof(m_Msg),
            NULL);

        if (NumCharsWritten == 0)
        {
            strcpy_s(m_Msg, _countof(m_Msg), "Unknown error");
        }
        else
        {
            auto nLen = strlen(m_Msg);
            if (nLen > 1 && m_Msg[nLen - 1] == '\n')
            {
                m_Msg[nLen - 1] = 0;
                if (m_Msg[nLen - 2] == '\r')
                {
                    m_Msg[nLen - 2] = 0;
                }
            }
        }
    }

    const char* Get() { return m_Msg; }

private:
    char m_Msg[4096];
};

} // namespace Diligent


#define CHECK_D3D_RESULT_THROW(Expr, Message)                                \
    do                                                                       \
    {                                                                        \
        HRESULT _hr_ = Expr;                                                 \
        if (FAILED(_hr_))                                                    \
        {                                                                    \
            ComErrorDesc ErrDesc(_hr_);                                      \
            LOG_ERROR_AND_THROW(Message, "\nHRESULT Desc: ", ErrDesc.Get()); \
        }                                                                    \
    } while (false)

#define CHECK_D3D_RESULT_THROW_EX(Expr, ...)                             \
    do                                                                   \
    {                                                                    \
        HRESULT _hr_ = Expr;                                             \
        if (FAILED(_hr_))                                                \
        {                                                                \
            auto         msg = Diligent::FormatString(__VA_ARGS__);      \
            ComErrorDesc ErrDesc(_hr_);                                  \
            LOG_ERROR_AND_THROW(msg, "\nHRESULT Desc: ", ErrDesc.Get()); \
        }                                                                \
    } while (false)

#define LOG_D3D_ERROR(Expr, Message)                                       \
    do                                                                     \
    {                                                                      \
        HRESULT _hr_ = Expr;                                               \
        if (FAILED(_hr_))                                                  \
        {                                                                  \
            ComErrorDesc ErrDesc(_hr_);                                    \
            LOG_ERROR_MESSAGE(Message, "\nHRESULT Desc: ", ErrDesc.Get()); \
        }                                                                  \
    } while (false)
