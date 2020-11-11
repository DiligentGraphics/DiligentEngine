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

#include "ShaderResourceBinding.h"

int TestObjectCInterface(struct IObject* pObject);

int TestShaderResourceBindingCInterface(struct IShaderResourceBinding* pSRB)
{
    IObject*                  pUnknown = NULL;
    ReferenceCounterValueType RefCnt1 = 0, RefCnt2 = 0;

    struct IPipelineState*   pPSO     = NULL;
    IShaderResourceVariable* pVar     = NULL;
    Uint32                   VarCount = 0;

    int num_errors = TestObjectCInterface((struct IObject*)pSRB);

    IObject_QueryInterface(pSRB, &IID_Unknown, &pUnknown);
    if (pUnknown != NULL)
        IObject_Release(pUnknown);
    else
        ++num_errors;

    RefCnt1 = IObject_AddRef(pSRB);
    if (RefCnt1 <= 1)
        ++num_errors;
    RefCnt2 = IObject_Release(pSRB);
    if (RefCnt2 <= 0)
        ++num_errors;
    if (RefCnt2 != RefCnt1 - 1)
        ++num_errors;

    pPSO = IShaderResourceBinding_GetPipelineState(pSRB);
    if (pPSO == NULL)
        ++num_errors;

    pVar = IShaderResourceBinding_GetVariableByName(pSRB, SHADER_TYPE_VERTEX, "g_tex2D_Mut");
    if (pVar == NULL)
        ++num_errors;

    VarCount = IShaderResourceBinding_GetVariableCount(pSRB, SHADER_TYPE_VERTEX);
    if (VarCount == 0)
        ++num_errors;

    pVar = IShaderResourceBinding_GetVariableByIndex(pSRB, SHADER_TYPE_VERTEX, 0);
    if (pVar == NULL)
        ++num_errors;

    IShaderResourceBinding_InitializeStaticResources(pSRB, pPSO);

    return num_errors;
}

void TestShaderResourceBindingC_API(struct IShaderResourceBinding* pSRB)
{
    struct IResourceMapping* pResMapping = NULL;
    IShaderResourceBinding_BindResources(pSRB, SHADER_TYPE_VERTEX, pResMapping, BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED);
}