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

#include <string>

#include "TestingEnvironment.hpp"

#ifndef NOMINMAX
#    define NOMINMAX
#endif

#include <d3d12.h>
#include <d3dcompiler.h>
#include <atlcomcli.h>
#include "DXCompiler.hpp"

namespace Diligent
{

namespace Testing
{

// Implemented in TestingEnvironmentD3D11.cpp
HRESULT CompileD3DShader(const std::string&      Source,
                         LPCSTR                  strFunctionName,
                         const D3D_SHADER_MACRO* pDefines,
                         LPCSTR                  profile,
                         ID3DBlob**              ppBlobOut);

class TestingEnvironmentD3D12 final : public TestingEnvironment
{
public:
    using CreateInfo = TestingEnvironment::CreateInfo;
    TestingEnvironmentD3D12(const CreateInfo&    CI,
                            const SwapChainDesc& SCDesc);
    ~TestingEnvironmentD3D12();

    static TestingEnvironmentD3D12* GetInstance() { return ValidatedCast<TestingEnvironmentD3D12>(TestingEnvironment::GetInstance()); }

    ID3D12Device* GetD3D12Device()
    {
        return m_pd3d12Device;
    }

    //  The allocator is currently never reset, which is not an issue in this testing system.
    CComPtr<ID3D12GraphicsCommandList> CreateGraphicsCommandList();

    void IdleCommandQueue(ID3D12CommandQueue* pd3d12Queue);

    HRESULT CompileDXILShader(const std::string& Source,
                              LPCWSTR            strFunctionName,
                              const DxcDefine*   Defines,
                              Uint32             DefinesCount,
                              LPCWSTR            profile,
                              ID3DBlob**         ppBlobOut);

private:
    CComPtr<ID3D12Device>           m_pd3d12Device;
    CComPtr<ID3D12CommandAllocator> m_pd3d12CmdAllocator;
    CComPtr<ID3D12Fence>            m_pd3d12Fence;

    UINT64 m_NextFenceValue = 1;

    HANDLE m_WaitForGPUEventHandle = {};

    std::unique_ptr<IDXCompiler> m_pDxCompiler;
};

} // namespace Testing

} // namespace Diligent
