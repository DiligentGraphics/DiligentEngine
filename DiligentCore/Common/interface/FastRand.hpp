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

#include "../../Platforms/Basic/interface/DebugUtilities.hpp"

namespace Diligent
{

/// Fast random number generator.
///
/// \note You should probably not use it for your production-grade encryption.
class FastRand
{
public:
    using StateType = unsigned int;

    static constexpr StateType Max = 0x7FFF;

    explicit FastRand(StateType Seed) noexcept :
        State{Seed}
    {
    }

    StateType operator()()
    {
        State = StateType{214013} * State + StateType{2531011};
        return (State >> StateType{16}) & Max;
    }

private:
    StateType State;
};

/// Generates a real random number in [Min, Max] range
template <typename Type>
class FastRandReal : private FastRand
{
public:
    explicit FastRandReal(FastRand::StateType Seed) noexcept :
        FastRand{Seed}
    {}

    FastRandReal(FastRand::StateType Seed, Type _Min, Type _Max) noexcept :
        FastRand{Seed},
        Min{_Min},
        Range{_Max - _Min}
    {}

    Type operator()()
    {
        return (static_cast<Type>(FastRand::operator()()) / static_cast<Type>(FastRand::Max)) * Range + Min;
    }

private:
    const Type Min   = 0.f;
    const Type Range = 1.f;
};

using FastRandFloat  = FastRandReal<float>;
using FastRandDouble = FastRandReal<double>;

/// Generates an integer random number in [Min, Max] range
class FastRandInt : private FastRand
{
public:
    FastRandInt(FastRand::StateType Seed, int _Min, int _Max) noexcept :
        FastRand{Seed},
        Min{_Min},
        Range{_Max - _Min + 1}
    {
        VERIFY_EXPR(_Max > _Min);
        VERIFY(Range <= static_cast<int>(FastRand::Max), "Range is too large");
    }

    int operator()()
    {
        return Min + static_cast<int>(FastRand::operator()()) % Range;
    }

private:
    const int Min;
    const int Range;
};

} // namespace Diligent
