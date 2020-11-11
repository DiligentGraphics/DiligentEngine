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

#include "CommonDefinitions.h"

#if DILIGENT_C_INTERFACE
#    include <stdint.h>
#    include <stdbool.h>
#    include <stddef.h>
#else
#    include <cstdint>
#    include <string>
#endif

DILIGENT_BEGIN_NAMESPACE(Diligent)

typedef float Float32; ///< 32-bit float

typedef int64_t Int64; ///< 64-bit signed integer
typedef int32_t Int32; ///< 32-bit signed integer
typedef int16_t Int16; ///< 16-bit signed integer
typedef int8_t  Int8;  ///< 8-bit signed integer

typedef uint64_t Uint64; ///< 64-bit unsigned integer
typedef uint32_t Uint32; ///< 32-bit unsigned integer
typedef uint16_t Uint16; ///< 16-bit unsigned integer
typedef uint8_t  Uint8;  ///< 8-bit unsigned integer

typedef size_t      SizeType;
typedef void*       PVoid;
typedef const void* CPVoid;

typedef bool Bool; ///< Boolean

static const Bool False = false;
static const Bool True  = true;

typedef char Char;
#if !DILIGENT_C_INTERFACE
using String = std::basic_string<Char>; ///< String variable
#endif

DILIGENT_END_NAMESPACE // namespace Diligent
