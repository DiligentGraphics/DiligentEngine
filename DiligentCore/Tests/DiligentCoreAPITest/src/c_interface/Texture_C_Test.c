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

#include "Texture.h"

int TestObjectCInterface(struct IObject* pObject);
int TestDeviceObjectCInterface(struct IDeviceObject* pDeviceObject);

struct ISampler;

int TestTextureCInterface(struct ITexture* pTexture)
{
    IObject*                  pUnknown = NULL;
    ReferenceCounterValueType RefCnt1 = 0, RefCnt2 = 0;

    DeviceObjectAttribs Desc;
    Int32               UniqueId = 0;

    TextureDesc     TexDesc;
    ITextureView *  pView0 = NULL, *pView1 = NULL;
    TextureViewDesc ViewDesc;
    void*           NativeHanlde;
    RESOURCE_STATE  State = RESOURCE_STATE_SHADER_RESOURCE;

    int num_errors =
        TestObjectCInterface((struct IObject*)pTexture) +
        TestDeviceObjectCInterface((struct IDeviceObject*)pTexture);

    IObject_QueryInterface(pTexture, &IID_Unknown, &pUnknown);
    if (pUnknown != NULL)
        IObject_Release(pUnknown);
    else
        ++num_errors;

    RefCnt1 = IObject_AddRef(pTexture);
    if (RefCnt1 <= 1)
        ++num_errors;
    RefCnt2 = IObject_Release(pTexture);
    if (RefCnt2 <= 0)
        ++num_errors;
    if (RefCnt2 != RefCnt1 - 1)
        ++num_errors;

    Desc = *IDeviceObject_GetDesc(pTexture);
    if (Desc.Name == NULL)
        ++num_errors;

    UniqueId = IDeviceObject_GetUniqueID(pTexture);
    if (UniqueId == 0)
        ++num_errors;

    TexDesc = *ITexture_GetDesc(pTexture);
    if (TexDesc._DeviceObjectAttribs.Name == NULL)
        ++num_errors;

    memset(&ViewDesc, 0, sizeof(ViewDesc));
    ViewDesc._DeviceObjectAttribs.Name = "Test SRV";
    ViewDesc.ViewType                  = TEXTURE_VIEW_SHADER_RESOURCE;
    ITexture_CreateView(pTexture, &ViewDesc, &pView0);
    if (pView0 != NULL)
        IObject_Release(pView0);
    else
        ++num_errors;

    pView1 = ITexture_GetDefaultView(pTexture, TEXTURE_VIEW_SHADER_RESOURCE);
    if (pView1 == NULL)
        ++num_errors;

    NativeHanlde = ITexture_GetNativeHandle(pTexture);
    if (NativeHanlde == NULL)
        ++num_errors;

    State = ITexture_GetState(pTexture);
    ITexture_SetState(pTexture, State);

    return num_errors;
}
