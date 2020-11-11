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
/// Defines Diligent::IBuffer interface and related data structures

#include "DeviceObject.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {C38C68F2-8A8C-4ED5-B7EE-69126E75DCD8}
static const struct INTERFACE_ID IID_CommandList =
    {0xc38c68f2, 0x8a8c, 0x4ed5, {0xb7, 0xee, 0x69, 0x12, 0x6e, 0x75, 0xdc, 0xd8}};


#if DILIGENT_CPP_INTERFACE

/// Command list interface

/// Command list has no methods. When command list recording is finished, it is executed by
/// IDeviceContext::ExecuteCommandList().
class ICommandList : public IDeviceObject
{
};

#else

struct ICommandList;

//  C requires that a struct or union has at least one member
//struct ICommandListMethods
//{
//};

struct ICommandListVtbl
{
    struct IObjectMethods       Object;
    struct IDeviceObjectMethods DeviceObject;
    //struct ICommandListMethods  CommandList;
};

typedef struct ICommandList
{
    struct ICommandListVtbl* pVtbl;
} ICommandList;

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
