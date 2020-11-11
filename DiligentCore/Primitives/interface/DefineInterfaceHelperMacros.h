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

#include "CommonDefinitions.h"

#ifndef DILIGENT_INTERFACE_NAME
#    error Interface name is undefined
#endif

#pragma push_macro("THIS")
#pragma push_macro("THIS_")
#pragma push_macro("VIRTUAL")
#pragma push_macro("CONST")
#pragma push_macro("PURE")
#pragma push_macro("REF")
#pragma push_macro("METHOD")

#undef THIS
#undef THIS_
#undef VIRTUAL
#undef CONST
#undef PURE
#undef REF
#undef METHOD

#if DILIGENT_C_INTERFACE

#    define THIS  struct DILIGENT_INTERFACE_NAME*
#    define THIS_ struct DILIGENT_INTERFACE_NAME*,
#    define VIRTUAL
#    define CONST
#    define PURE
#    define REF          *
#    define METHOD(Name) (DILIGENT_CALL_TYPE * Name)

// Suppose that DILIGENT_INTERFACE_NAME == Iface, then DILIGENT_END_INTERFACE macro below will expand to the following:
//
//      typedef struct IfaceMethods IfaceMethods;
//      typedef struct IfaceVtbl
//      {
//          IfaceInclusiveMethods;
//      } IfaceVtbl;
//
// IfaceInclusiveMethods macro must be properly defined

// clang-format off
#    define DILIGENT_END_INTERFACE\
        typedef struct DILIGENT_CONCATENATE(DILIGENT_INTERFACE_NAME, Methods) DILIGENT_CONCATENATE(DILIGENT_INTERFACE_NAME, Methods); \
        typedef struct DILIGENT_CONCATENATE(DILIGENT_INTERFACE_NAME, Vtbl)  \
        {                                                                   \
            DILIGENT_CONCATENATE(DILIGENT_INTERFACE_NAME, InclusiveMethods);\
        } DILIGENT_CONCATENATE(DILIGENT_INTERFACE_NAME, Vtbl);
// clang-format on

#else

#    define THIS
#    define THIS_
#    define VIRTUAL      virtual
#    define CONST        const
#    define PURE         = 0
#    define REF          &
#    define METHOD(Name) DILIGENT_CALL_TYPE Name
#    define DILIGENT_END_INTERFACE

#endif
