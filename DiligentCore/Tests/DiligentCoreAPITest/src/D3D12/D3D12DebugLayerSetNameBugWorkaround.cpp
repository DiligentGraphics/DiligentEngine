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

#include "D3D12/D3D12DebugLayerSetNameBugWorkaround.hpp"

#include "RefCntAutoPtr.hpp"

#ifndef NOMINMAX
#    define NOMINMAX
#endif

#include <d3d12.h>
#include <atlcomcli.h>

#include "RenderDeviceD3D12.h"

namespace Diligent
{

namespace Testing
{

struct D3D12DebugLayerSetNameBugWorkaround::RootSignatureWrapper
{
    CComPtr<ID3D12RootSignature> pRootSignature;
};

D3D12DebugLayerSetNameBugWorkaround::D3D12DebugLayerSetNameBugWorkaround(IRenderDevice* pDevice) :
    m_RootSignature(new RootSignatureWrapper)
{
    if (pDevice->GetDeviceCaps().DevType != RENDER_DEVICE_TYPE_D3D12)
        return;

    RefCntAutoPtr<IRenderDeviceD3D12> pDeviceD3D12(pDevice, IID_RenderDeviceD3D12);
    VERIFY_EXPR(pDeviceD3D12);
    auto pd3d12Device = pDeviceD3D12->GetD3D12Device();

    D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc = {};

    RootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    CComPtr<ID3DBlob> signature;
    D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, nullptr);
    auto& pRootSign = m_RootSignature->pRootSignature;
    pd3d12Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), __uuidof(pRootSign), reinterpret_cast<void**>(static_cast<ID3D12RootSignature**>(&pRootSign)));
    pRootSign->SetName(L"A long string to make sure there is enough space reserved in the buffer to avoid resize when SetName is called "
                       "and it is accessed simultaneously from multiple threads without a mutex");
}

D3D12DebugLayerSetNameBugWorkaround::~D3D12DebugLayerSetNameBugWorkaround()
{
}

} // namespace Testing

} // namespace Diligent
