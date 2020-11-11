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

#include "ShaderResourceVariable.h"

int TestObjectCInterface(struct IObject* pObject);

int TestShaderResourceVariableCInterface(struct IShaderResourceVariable* pVar, struct IDeviceObject* pObjectToSet)
{
    IObject*                  pUnknown = NULL;
    ReferenceCounterValueType RefCnt1 = 0, RefCnt2 = 0;

    SHADER_RESOURCE_VARIABLE_TYPE VarType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
    ShaderResourceDesc            ResDesc;
    Uint32                        Index   = 0;
    bool                          IsBound = false;

    int num_errors = TestObjectCInterface((struct IObject*)pVar);

    IObject_QueryInterface(pVar, &IID_Unknown, &pUnknown);
    if (pUnknown != NULL)
        IObject_Release(pUnknown);
    else
        ++num_errors;

    RefCnt1 = IObject_AddRef(pVar);
    if (RefCnt1 <= 1)
        ++num_errors;
    RefCnt2 = IObject_Release(pVar);
    if (RefCnt2 <= 0)
        ++num_errors;
    if (RefCnt2 != RefCnt1 - 1)
        ++num_errors;

    IShaderResourceVariable_Set(pVar, pObjectToSet);
    IShaderResourceVariable_SetArray(pVar, &pObjectToSet, 0, 1);
    VarType = IShaderResourceVariable_GetType(pVar);
    if (VarType != SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC)
        ++num_errors;

    IShaderResourceVariable_GetResourceDesc(pVar, &ResDesc);
    if (ResDesc.Type != SHADER_RESOURCE_TYPE_TEXTURE_SRV)
        ++num_errors;

    Index = IShaderResourceVariable_GetIndex(pVar);

    IsBound = IShaderResourceVariable_IsBound(pVar, 0);
    if (!IsBound)
        ++num_errors;

    return num_errors;
}
