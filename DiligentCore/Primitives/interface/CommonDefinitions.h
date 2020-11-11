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

/// \file
/// Common definitions

#ifndef DILIGENT_C_INTERFACE
#    ifdef __cplusplus
#        define DILIGENT_C_INTERFACE 0
#    else
#        define DILIGENT_C_INTERFACE 1
#    endif
#endif

#ifdef _MSC_VER
// Note that MSVC x86 compiler by default uses __this call for class member functions
#    define DILIGENT_CALL_TYPE __cdecl
#else
#    define DILIGENT_CALL_TYPE
#endif

#if DILIGENT_C_INTERFACE

#    define DILIGENT_BEGIN_NAMESPACE(Name)
#    define DILIGENT_END_NAMESPACE

#    define DILIGENT_TYPED_ENUM(EnumName, EnumType) \
        typedef EnumType EnumName;                  \
        enum _##EnumName

#    define DILIGENT_DERIVE(TypeName) \
        {                             \
            struct TypeName _##TypeName;

#    define DEFAULT_INITIALIZER(x)

#    define DILIGENT_GLOBAL_FUNCTION(FuncName) Diligent_##FuncName

#    define DILIGENT_BEGIN_INTERFACE(Iface, Base) \
        typedef struct Iface                      \
        {                                         \
            struct Iface##Vtbl* pVtbl;            \
        } Iface;                                  \
        struct Iface##Methods

#    define DEFAULT_VALUE(x)

#    define CALL_IFACE_METHOD(Iface, Method, This, ...) (This)->pVtbl->Iface.Method((I##Iface*)(This), ##__VA_ARGS__)

// Two levels of indirection are required to concatenate expanded macros
#    define DILIGENT_CONCATENATE0(X, Y) X##Y
#    define DILIGENT_CONCATENATE(X, Y)  DILIGENT_CONCATENATE0(X, Y)

#else

#    define DILIGENT_BEGIN_NAMESPACE(Name) \
        namespace Name                     \
        {

#    define DILIGENT_END_NAMESPACE }

#    define DILIGENT_TYPED_ENUM(EnumName, EnumType) enum EnumName : EnumType

#    define DILIGENT_DERIVE(TypeName) : public TypeName \
        {

#    define DEFAULT_INITIALIZER(x) = x

#    define DILIGENT_GLOBAL_FUNCTION(FuncName) FuncName

#    define DILIGENT_BEGIN_INTERFACE(Name, Base) struct Name : public Base

#    define DEFAULT_VALUE(x) = x

#endif

#if DILIGENT_C_INTERFACE
#    define DILIGENT_CPP_INTERFACE 0
#else
#    define DILIGENT_CPP_INTERFACE 1
#endif
