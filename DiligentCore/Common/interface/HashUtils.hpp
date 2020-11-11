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

#include <functional>
#include <memory>
#include <cstring>

#include "../../Primitives/interface/Errors.hpp"
#include "../../Platforms/Basic/interface/DebugUtilities.hpp"

#define LOG_HASH_CONFLICTS 1

namespace Diligent
{

// http://www.boost.org/doc/libs/1_35_0/doc/html/hash/combine.html
template <typename T>
void HashCombine(std::size_t& Seed, const T& Val)
{
    Seed ^= std::hash<T>()(Val) + 0x9e3779b9 + (Seed << 6) + (Seed >> 2);
}

template <typename FirstArgType, typename... RestArgsType>
void HashCombine(std::size_t& Seed, const FirstArgType& FirstArg, const RestArgsType&... RestArgs)
{
    HashCombine(Seed, FirstArg);
    HashCombine(Seed, RestArgs...); // recursive call using pack expansion syntax
}

template <typename... ArgsType>
std::size_t ComputeHash(const ArgsType&... Args)
{
    std::size_t Seed = 0;
    HashCombine(Seed, Args...);
    return Seed;
}

template <typename CharType>
struct CStringHash
{
    size_t operator()(const CharType* str) const
    {
        // http://www.cse.yorku.ca/~oz/hash.html
        std::size_t Seed = 0;
        while (size_t Ch = *(str++))
            Seed = Seed * 65599 + Ch;
        return Seed;
    }
};

template <typename CharType>
struct CStringCompare
{
    bool operator()(const CharType* str1, const CharType* str2) const
    {
        UNSUPPORTED("Template specialization is not implemented");
        return false;
    }
};

template <>
struct CStringCompare<Char>
{
    bool operator()(const Char* str1, const Char* str2) const
    {
        return strcmp(str1, str2) == 0;
    }
};

/// This helper structure is intended to facilitate using strings as a
/// hash table key. It provides constructors that can make a copy of the
/// source string or just keep a pointer to it, which enables searching in
/// the hash using raw const Char* pointers.
struct HashMapStringKey
{
public:
    // This constructor can perform implicit const Char* -> HashMapStringKey
    // conversion without copying the string.
    HashMapStringKey(const Char* _Str, bool bMakeCopy = false) :
        Str{_Str}
    {
        VERIFY(Str, "String pointer must not be null");

        Ownership_Hash = CStringHash<Char>{}.operator()(Str) & HashMask;
        if (bMakeCopy)
        {
            auto  LenWithZeroTerm = strlen(Str) + 1;
            auto* StrCopy         = new char[LenWithZeroTerm];
            memcpy(StrCopy, Str, LenWithZeroTerm);
            Str = StrCopy;
            Ownership_Hash |= StrOwnershipMask;
        }
    }

    // Make this constructor explicit to avoid unintentional string copies
    explicit HashMapStringKey(const String& Str) :
        HashMapStringKey{Str.c_str(), true}
    {
    }

    HashMapStringKey(HashMapStringKey&& Key) noexcept :
        // clang-format off
        Str {Key.Str},
        Ownership_Hash{Key.Ownership_Hash}
    // clang-format on
    {
        Key.Str            = nullptr;
        Key.Ownership_Hash = 0;
    }

    ~HashMapStringKey()
    {
        if (Str != nullptr && (Ownership_Hash & StrOwnershipMask) != 0)
            delete[] Str;
    }

    // Disable copy constuctor and assignments. The struct is designed
    // to be initialized at creation time only.
    // clang-format off
    HashMapStringKey           (const HashMapStringKey&) = delete;
    HashMapStringKey& operator=(const HashMapStringKey&) = delete;
    HashMapStringKey& operator=(HashMapStringKey&&)      = delete;
    // clang-format on

    bool operator==(const HashMapStringKey& RHS) const
    {
        if (Str == RHS.Str)
            return true;

        if (Str == nullptr)
        {
            VERIFY_EXPR(RHS.Str != nullptr);
            return false;
        }
        else if (RHS.Str == nullptr)
        {
            VERIFY_EXPR(Str != nullptr);
            return false;
        }

        auto Hash    = GetHash();
        auto RHSHash = RHS.GetHash();
        if (Hash != RHSHash)
        {
            VERIFY_EXPR(strcmp(Str, RHS.Str) != 0);
            return false;
        }

        bool IsEqual = strcmp(Str, RHS.Str) == 0;

#if LOG_HASH_CONFLICTS
        if (!IsEqual && Hash == RHSHash)
        {
            LOG_WARNING_MESSAGE("Unequal strings \"", Str, "\" and \"", RHS.Str,
                                "\" have the same hash. You may want to use a better hash function. "
                                "You may disable this warning by defining LOG_HASH_CONFLICTS to 0");
        }
#endif
        return IsEqual;
    }

    bool operator!=(const HashMapStringKey& RHS) const
    {
        return !(*this == RHS);
    }

    size_t GetHash() const
    {
        return Ownership_Hash & HashMask;
    }

    const Char* GetStr() const
    {
        return Str;
    }

    struct Hasher
    {
        size_t operator()(const HashMapStringKey& Key) const
        {
            return Key.GetHash();
        }
    };

private:
    static constexpr size_t StrOwnershipBit  = sizeof(size_t) * 8 - 1;
    static constexpr size_t StrOwnershipMask = size_t{1} << StrOwnershipBit;
    static constexpr size_t HashMask         = ~StrOwnershipMask;

    const Char* Str = nullptr;
    // We will use top bit of the hash to indicate if we own the pointer
    size_t Ownership_Hash = 0;
};

} // namespace Diligent
