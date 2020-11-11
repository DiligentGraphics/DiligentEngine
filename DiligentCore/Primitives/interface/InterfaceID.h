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
#include "BasicTypes.h"

/// Unique identification structures
DILIGENT_BEGIN_NAMESPACE(Diligent)

/// Unique interface identifier
struct INTERFACE_ID
{
    Uint32 Data1;
    Uint16 Data2;
    Uint16 Data3;
    Uint8  Data4[8];

#if DILIGENT_CPP_INTERFACE
    bool operator==(const INTERFACE_ID& rhs) const
    {
        return Data1 == rhs.Data1 &&
            Data2 == rhs.Data2 &&
            Data3 == rhs.Data3 &&
            memcmp(Data4, rhs.Data4, sizeof(Data4)) == 0;
    }
#endif
};
typedef struct INTERFACE_ID INTERFACE_ID;

/// Unknown interface
static const INTERFACE_ID IID_Unknown = {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}};

DILIGENT_END_NAMESPACE // namespace Diligent
