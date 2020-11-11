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

#include <d3d12.h>

namespace Diligent
{

using D3D12CreateDeviceProcType = HRESULT(WINAPI*)(
    _In_opt_ IUnknown*      pAdapter,
    D3D_FEATURE_LEVEL       MinimumFeatureLevel,
    _In_ REFIID             riid,
    _COM_Outptr_opt_ void** ppDevice);

using D3D12GetDebugInterfaceProcType = HRESULT(WINAPI*)(
    _In_ REFIID             riid,
    _COM_Outptr_opt_ void** ppvDebug);

using D3D12SerializeRootSignatureProcType = HRESULT(WINAPI*)(
    _In_ const D3D12_ROOT_SIGNATURE_DESC* pRootSignature,
    _In_ D3D_ROOT_SIGNATURE_VERSION       Version,
    _Out_ ID3DBlob**                                   ppBlob,
    _Always_(_Outptr_opt_result_maybenull_) ID3DBlob** ppErrorBlob);

extern D3D12CreateDeviceProcType           D3D12CreateDevice;
extern D3D12GetDebugInterfaceProcType      D3D12GetDebugInterface;
extern D3D12SerializeRootSignatureProcType D3D12SerializeRootSignature;

HMODULE LoadD3D12Dll(const char* DLLPath = "d3d12.dll");

} // namespace Diligent
