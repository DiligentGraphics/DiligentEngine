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

#include "Shader.h"

int TestObjectCInterface(struct IObject* pObject);
int TestDeviceObjectCInterface(struct IDeviceObject* pDeviceObject);

int TestShaderCInterface(struct IShader* pShader)
{
    IObject*                  pUnknown = NULL;
    ReferenceCounterValueType RefCnt1 = 0, RefCnt2 = 0;

    DeviceObjectAttribs Desc;
    Int32               UniqueId = 0;

    ShaderDesc         ShaderDesc;
    Uint32             ResourceCount = 0;
    ShaderResourceDesc ResourceDesc;

    int num_errors =
        TestObjectCInterface((struct IObject*)pShader) +
        TestDeviceObjectCInterface((struct IDeviceObject*)pShader);

    IObject_QueryInterface(pShader, &IID_Unknown, &pUnknown);
    if (pUnknown != NULL)
        IObject_Release(pUnknown);
    else
        ++num_errors;

    RefCnt1 = IObject_AddRef(pShader);
    if (RefCnt1 <= 1)
        ++num_errors;
    RefCnt2 = IObject_Release(pShader);
    if (RefCnt2 <= 0)
        ++num_errors;
    if (RefCnt2 != RefCnt1 - 1)
        ++num_errors;

    Desc = *IDeviceObject_GetDesc(pShader);
    if (Desc.Name == NULL)
        ++num_errors;

    UniqueId = IDeviceObject_GetUniqueID(pShader);
    if (UniqueId == 0)
        ++num_errors;

    ShaderDesc = *IShader_GetDesc(pShader);
    if (ShaderDesc.ShaderType != SHADER_TYPE_VERTEX)
        ++num_errors;

    ResourceCount = IShader_GetResourceCount(pShader);
    if (ResourceCount == 0)
        ++num_errors;

    IShader_GetResourceDesc(pShader, 0, &ResourceDesc);
    if (ResourceDesc.ArraySize == 0)
        ++num_errors;

    return num_errors;
}
