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

#include <sstream>
#include <iomanip>

namespace Diligent
{

template <typename SSType>
void FormatStrSS(SSType& ss)
{
}

template <typename SSType, typename ArgType>
void FormatStrSS(SSType& ss, const ArgType& Arg)
{
    ss << Arg;
}

template <typename SSType, typename FirstArgType, typename... RestArgsType>
void FormatStrSS(SSType& ss, const FirstArgType& FirstArg, const RestArgsType&... RestArgs)
{
    FormatStrSS(ss, FirstArg);
    FormatStrSS(ss, RestArgs...); // recursive call using pack expansion syntax
}

template <typename... RestArgsType>
std::string FormatString(const RestArgsType&... Args)
{
    std::stringstream ss;
    FormatStrSS(ss, Args...);
    return ss.str();
}

template <typename Type>
struct MemorySizeFormatter
{
    MemorySizeFormatter(Type _size, int _precision, Type _ref_size) :
        size{_size},
        precision{_precision},
        ref_size{_ref_size}
    {}
    Type size      = 0;
    int  precision = 0;
    Type ref_size  = 0;
};

template <typename Type>
MemorySizeFormatter<Type> FormatMemorySize(Type _size, int _precision = 0, Type _ref_size = 0)
{
    return MemorySizeFormatter<Type>{_size, _precision, _ref_size};
}

template <typename SSType, typename Type>
void FormatStrSS(SSType& ss, const MemorySizeFormatter<Type>& Arg)
{
    auto ref_size = Arg.ref_size != 0 ? Arg.ref_size : Arg.size;
    if (ref_size >= (1 << 30))
    {
        ss << std::fixed << std::setprecision(Arg.precision) << static_cast<double>(Arg.size) / double{1 << 30} << " GB";
    }
    else if (ref_size >= (1 << 20))
    {
        ss << std::fixed << std::setprecision(Arg.precision) << static_cast<double>(Arg.size) / double{1 << 20} << " MB";
    }
    else if (ref_size >= (1 << 10))
    {
        ss << std::fixed << std::setprecision(Arg.precision) << static_cast<double>(Arg.size) / double{1 << 10} << " KB";
    }
    else
    {
        ss << Arg.size << (((Arg.size & 0x01) == 0x01) ? " Byte" : " Bytes");
    }
}

} // namespace Diligent
