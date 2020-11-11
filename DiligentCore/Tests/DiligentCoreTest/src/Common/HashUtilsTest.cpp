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

#include <unordered_map>

#include "HashUtils.hpp"

#include "gtest/gtest.h"

using namespace Diligent;

namespace
{

TEST(Common_HashUtils, HashMapStringKey)
{
    {
        const char* Str = "Test String";

        HashMapStringKey Key1{Str};
        EXPECT_EQ(Key1.GetStr(), Str);
        EXPECT_STREQ(Key1.GetStr(), Str);

        HashMapStringKey Key2{Str, true};
        EXPECT_NE(Key2.GetStr(), Str);
        EXPECT_STREQ(Key2.GetStr(), Str);

        EXPECT_EQ(Key1, Key1);
        EXPECT_EQ(Key2, Key2);
        EXPECT_EQ(Key1, Key2);

        HashMapStringKey Key3{std::string{Str}};
        EXPECT_NE(Key3.GetStr(), Str);
        EXPECT_STREQ(Key3.GetStr(), Str);

        EXPECT_EQ(Key3, Key1);
        EXPECT_EQ(Key3, Key2);
        EXPECT_EQ(Key3, Key3);
    }

    {
        const char*      Str1 = "Test String 1";
        const char*      Str2 = "Test String 2";
        HashMapStringKey Key1{Str1};
        HashMapStringKey Key2{Str2, true};
        EXPECT_NE(Key1, Key2);

        HashMapStringKey Key3{std::move(Key1)};
        EXPECT_NE(Key1, Key2);
        EXPECT_NE(Key2, Key1);

        HashMapStringKey Key4{std::move(Key2)};
        EXPECT_EQ(Key1, Key2);
        EXPECT_EQ(Key2, Key1);
        EXPECT_NE(Key3, Key4);
    }

    {
        std::unordered_map<HashMapStringKey, int, HashMapStringKey::Hasher> TestMap;

        const char* Str1 = "String1";
        const char* Str2 = "String2";
        const char* Str3 = "String3";
        const int   Val1 = 1;
        const int   Val2 = 2;

        auto it_ins = TestMap.emplace(HashMapStringKey{Str1, true}, Val1);
        EXPECT_TRUE(it_ins.second);
        EXPECT_NE(it_ins.first->first.GetStr(), Str1);
        EXPECT_STREQ(it_ins.first->first.GetStr(), Str1);

        it_ins = TestMap.emplace(Str2, Val2);
        EXPECT_TRUE(it_ins.second);
        EXPECT_EQ(it_ins.first->first, Str2);

        auto it = TestMap.find(Str1);
        ASSERT_NE(it, TestMap.end());
        EXPECT_EQ(it->second, Val1);
        EXPECT_NE(it->first.GetStr(), Str1);
        EXPECT_STREQ(it->first.GetStr(), Str1);

        it = TestMap.find(Str2);
        ASSERT_NE(it, TestMap.end());
        EXPECT_EQ(it->second, Val2);
        EXPECT_EQ(it->first.GetStr(), Str2);

        it = TestMap.find(Str3);
        EXPECT_EQ(it, TestMap.end());

        it = TestMap.find(HashMapStringKey{std::string{Str3}});
        EXPECT_EQ(it, TestMap.end());
    }
}

} // namespace
