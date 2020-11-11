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

#include <string>
#include <sstream>
#include <locale>
#include <algorithm>
#include <cctype>
#include <string.h>

#include "../../Platforms/Basic/interface/DebugUtilities.hpp"

namespace Diligent
{

inline std::string NarrowString(const std::wstring& WideStr)
{
    std::string NarrowStr;

    const std::ctype<wchar_t>& ctfacet = std::use_facet<std::ctype<wchar_t>>(std::wstringstream().getloc());
    for (std::wstring::const_iterator CurrWChar = WideStr.begin();
         CurrWChar != WideStr.end();
         CurrWChar++)
        NarrowStr.push_back(ctfacet.narrow(*CurrWChar, 0));

    return NarrowStr;
}

inline std::string NarrowString(const wchar_t* WideStr)
{
    std::string NarrowStr;

    const std::ctype<wchar_t>& ctfacet = std::use_facet<std::ctype<wchar_t>>(std::wstringstream().getloc());
    for (auto CurrWChar = WideStr; *CurrWChar != 0; ++CurrWChar)
        NarrowStr.push_back(ctfacet.narrow(*CurrWChar, 0));

    return NarrowStr;
}

inline std::wstring WidenString(const char* Str)
{
    std::wstring WideStr;

    const std::ctype<wchar_t>& ctfacet = std::use_facet<std::ctype<wchar_t>>(std::wstringstream().getloc());
    for (auto CurrChar = Str; *CurrChar != 0; ++CurrChar)
        WideStr.push_back(ctfacet.widen(*CurrChar));

    return WideStr;
}

inline std::wstring WidenString(const std::string& Str)
{
    std::wstring WideStr;

    const std::ctype<wchar_t>& ctfacet = std::use_facet<std::ctype<wchar_t>>(std::wstringstream().getloc());
    for (std::string::const_iterator CurrChar = Str.begin();
         CurrChar != Str.end();
         CurrChar++)
        WideStr.push_back(ctfacet.widen(*CurrChar));

    return WideStr;
}

inline int StrCmpNoCase(const char* Str1, const char* Str2, size_t NumChars)
{
#if PLATFORM_ANDROID || PLATFORM_LINUX || PLATFORM_MACOS || PLATFORM_IOS
#    define _strnicmp strncasecmp
#endif

    return _strnicmp(Str1, Str2, NumChars);
}

inline int StrCmpNoCase(const char* Str1, const char* Str2)
{
#if PLATFORM_ANDROID || PLATFORM_LINUX || PLATFORM_MACOS || PLATFORM_IOS
#    define _stricmp strcasecmp
#endif

    return _stricmp(Str1, Str2);
}

// Returns true if RefStr == Str + Suff
// If Suff == nullptr or NoSuffixAllowed == true, also returns true if RefStr == Str
inline bool StreqSuff(const char* RefStr, const char* Str, const char* Suff, bool NoSuffixAllowed = false)
{
    VERIFY_EXPR(RefStr != nullptr && Str != nullptr);
    if (RefStr == nullptr)
        return false;

    const auto* r = RefStr;
    const auto* s = Str;
    // abc_def     abc
    // ^           ^
    // r           s
    for (; *r != 0 && *s != 0; ++r, ++s)
    {
        if (*r != *s)
        {
            // abc_def     abx
            //   ^           ^
            //   r           s
            return false;
        }
    }

    if (*s != 0)
    {
        // ab         abc
        //   ^          ^
        //   r          s
        VERIFY_EXPR(*r == 0);
        return false;
    }
    else
    {
        // abc_def     abc
        //    ^           ^
        //    r           s

        if (NoSuffixAllowed && *r == 0)
        {
            // abc         abc      _def
            //    ^           ^
            //    r           s
            return true;
        }

        if (Suff != nullptr)
        {
            // abc_def     abc       _def
            //    ^           ^      ^
            //    r           s      Suff
            return strcmp(r, Suff) == 0;
        }
        else
        {
            // abc         abc                abc_def         abc
            //    ^           ^       or         ^               ^
            //    r           s                  r               s
            return *r == 0;
        }
    }
}

inline void StrToLowerInPlace(std::string& str)
{
    std::transform(str.begin(), str.end(), str.begin(),
                   // http://en.cppreference.com/w/cpp/string/byte/tolower
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
}

inline std::string StrToLower(std::string str)
{
    StrToLowerInPlace(str);
    return str;
}

inline bool IsNum(char c)
{
    return c >= '0' && c <= '9';
}

/// Returns the number of chararcters at the beginning of the string that form a
/// floating point number.
inline size_t CountFloatNumberChars(const char* str)
{
    if (str == nullptr)
        return 0;

    const auto* num_end = str;
    const auto* c       = str;
    if (*c == 0)
        return 0;

    if (*c == '+' || *c == '-')
        ++c;

    if (*c == 0)
        return 0;

    if (*c == '0' && IsNum(*(c + 1)))
    {
        // 01 is invalid
        return c - str + 1;
    }

    while (IsNum(*c))
        num_end = ++c;

    if (*c == '.')
    {
        if (c != str && IsNum(c[-1]))
        {
            // . as well as +. or -. are not valid numbers, however 0., +0., and -0. are.
            num_end = c + 1;
        }

        ++c;
        while (IsNum(*c))
            num_end = ++c;

        if (*c == 'e' || *c == 'E')
        {
            if (c - str < 2 || !IsNum(c[-2]))
            {
                // .e as well as +.e are invalid
                return num_end - str;
            }
        }
    }
    else if (*c == 'e' || *c == 'E')
    {
        if (c - str < 1 || !IsNum(c[-1]))
        {
            // e as well as e+1 are invalid
            return num_end - str;
        }
    }

    if (*c == 'e' || *c == 'E')
    {
        ++c;
        if (*c != '+' && *c != '-')
            return num_end - str;

        ++c;
        while (IsNum(*c))
            num_end = ++c;
    }

    return num_end - str;
}

} // namespace Diligent
