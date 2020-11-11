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

#include "D3D11/TestingEnvironmentD3D11.hpp"

#include "RenderDeviceD3D11.h"
#include "DeviceContextD3D11.h"

#include <d3dcompiler.h>

namespace Diligent
{

namespace Testing
{

void CreateTestingSwapChainD3D11(IRenderDevice*       pDevice,
                                 IDeviceContext*      pContext,
                                 const SwapChainDesc& SCDesc,
                                 ISwapChain**         ppSwapChain);


TestingEnvironmentD3D11::TestingEnvironmentD3D11(const CreateInfo&    CI,
                                                 const SwapChainDesc& SCDesc) :
    TestingEnvironment{CI, SCDesc}
{
    RefCntAutoPtr<IRenderDeviceD3D11>  pRenderDeviceD3D11{m_pDevice, IID_RenderDeviceD3D11};
    RefCntAutoPtr<IDeviceContextD3D11> pContextD3D11{m_pDeviceContext, IID_DeviceContextD3D11};

    m_pd3d11Device  = pRenderDeviceD3D11->GetD3D11Device();
    m_pd3d11Context = pContextD3D11->GetD3D11DeviceContext();

    {
        D3D11_RASTERIZER_DESC RSDesc = {};

        RSDesc.CullMode = D3D11_CULL_NONE;
        RSDesc.FillMode = D3D11_FILL_SOLID;
        auto hr         = m_pd3d11Device->CreateRasterizerState(&RSDesc, &m_pd3d11NoCullRS);
        VERIFY_EXPR(SUCCEEDED(hr));
    }

    {
        D3D11_DEPTH_STENCIL_DESC DSDesc = {};

        DSDesc.DepthEnable = False;
        auto hr            = m_pd3d11Device->CreateDepthStencilState(&DSDesc, &m_pd3d11DisableDepthDSS);
        VERIFY_EXPR(SUCCEEDED(hr));
    }

    {
        D3D11_BLEND_DESC BSDesc = {};

        BSDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        auto hr = m_pd3d11Device->CreateBlendState(&BSDesc, &m_pd3d11DefaultBS);
        VERIFY_EXPR(SUCCEEDED(hr));
    }

    if (m_pSwapChain == nullptr)
    {
        CreateTestingSwapChainD3D11(m_pDevice, m_pDeviceContext, SCDesc, &m_pSwapChain);
    }
}

HRESULT CompileD3DShader(const std::string&      Source,
                         LPCSTR                  strFunctionName,
                         const D3D_SHADER_MACRO* pDefines,
                         LPCSTR                  profile,
                         ID3DBlob**              ppBlobOut)
{
    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(DILIGENT_DEBUG)
    // Set the D3D10_SHADER_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows
    // the shaders to be optimized and to run exactly the way they will run in
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#else
    // Warning: do not use this flag as it causes shader compiler to fail the compilation and
    // report strange errors:
    // dwShaderFlags |= D3D10_SHADER_OPTIMIZATION_LEVEL3;
#endif

    CComPtr<ID3DBlob> pCompilerOutput;

    auto hr = D3DCompile(Source.c_str(), Source.length(), NULL, pDefines, nullptr, strFunctionName, profile, dwShaderFlags, 0, ppBlobOut, &pCompilerOutput);

    const char* CompilerMsg = pCompilerOutput ? reinterpret_cast<const char*>(pCompilerOutput->GetBufferPointer()) : nullptr;
    if (FAILED(hr))
    {
        LOG_ERROR_AND_THROW("Failed to compile D3D shader :\n", (CompilerMsg != nullptr ? CompilerMsg : "<no compiler log available>"));
    }
    else if (CompilerMsg != nullptr)
    {
        LOG_INFO_MESSAGE("Shader compiler output:\n", CompilerMsg);
    }

    return hr;
}

CComPtr<ID3D11VertexShader> TestingEnvironmentD3D11::CreateVertexShader(const std::string&      Source,
                                                                        LPCSTR                  strFunctionName,
                                                                        const D3D_SHADER_MACRO* pDefines,
                                                                        LPCSTR                  profile)
{
    CComPtr<ID3DBlob>           pByteCode;
    CComPtr<ID3D11VertexShader> pVS;

    auto hr = CompileD3DShader(Source, strFunctionName, pDefines, profile, &pByteCode);
    if (FAILED(hr))
    {
        ADD_FAILURE() << "Failed to compile vertex shader";
        return pVS;
    }

    hr = m_pd3d11Device->CreateVertexShader(pByteCode->GetBufferPointer(), pByteCode->GetBufferSize(), NULL, &pVS);
    if (FAILED(hr))
    {
        ADD_FAILURE() << "Failed to create vertex shader";
    }
    return pVS;
}

CComPtr<ID3D11PixelShader> TestingEnvironmentD3D11::CreatePixelShader(const std::string&      Source,
                                                                      LPCSTR                  strFunctionName,
                                                                      const D3D_SHADER_MACRO* pDefines,
                                                                      LPCSTR                  profile)
{
    CComPtr<ID3DBlob>          pByteCode;
    CComPtr<ID3D11PixelShader> pPS;

    auto hr = CompileD3DShader(Source, strFunctionName, pDefines, profile, &pByteCode);
    if (FAILED(hr))
    {
        ADD_FAILURE() << "Failed to compile pixel shader";
        return pPS;
    }

    hr = m_pd3d11Device->CreatePixelShader(pByteCode->GetBufferPointer(), pByteCode->GetBufferSize(), NULL, &pPS);
    if (FAILED(hr))
    {
        ADD_FAILURE() << "Failed to create pixel shader";
    }
    return pPS;
}

CComPtr<ID3D11GeometryShader> TestingEnvironmentD3D11::CreateGeometryShader(const std::string&      Source,
                                                                            LPCSTR                  strFunctionName,
                                                                            const D3D_SHADER_MACRO* pDefines,
                                                                            LPCSTR                  profile)
{
    CComPtr<ID3DBlob>             pByteCode;
    CComPtr<ID3D11GeometryShader> pGS;

    auto hr = CompileD3DShader(Source, strFunctionName, pDefines, profile, &pByteCode);
    if (FAILED(hr))
    {
        ADD_FAILURE() << "Failed to compile geometry shader";
        return pGS;
    }

    hr = m_pd3d11Device->CreateGeometryShader(pByteCode->GetBufferPointer(), pByteCode->GetBufferSize(), NULL, &pGS);
    if (FAILED(hr))
    {
        ADD_FAILURE() << "Failed to create geometry shader";
    }
    return pGS;
}

CComPtr<ID3D11DomainShader> TestingEnvironmentD3D11::CreateDomainShader(const std::string&      Source,
                                                                        LPCSTR                  strFunctionName,
                                                                        const D3D_SHADER_MACRO* pDefines,
                                                                        LPCSTR                  profile)
{
    CComPtr<ID3DBlob>           pByteCode;
    CComPtr<ID3D11DomainShader> pDS;

    auto hr = CompileD3DShader(Source, strFunctionName, pDefines, profile, &pByteCode);
    if (FAILED(hr))
    {
        ADD_FAILURE() << "Failed to compile domain shader";
        return pDS;
    }

    hr = m_pd3d11Device->CreateDomainShader(pByteCode->GetBufferPointer(), pByteCode->GetBufferSize(), NULL, &pDS);
    if (FAILED(hr))
    {
        ADD_FAILURE() << "Failed to create domain shader";
    }
    return pDS;
}

CComPtr<ID3D11HullShader> TestingEnvironmentD3D11::CreateHullShader(const std::string&      Source,
                                                                    LPCSTR                  strFunctionName,
                                                                    const D3D_SHADER_MACRO* pDefines,
                                                                    LPCSTR                  profile)
{
    CComPtr<ID3DBlob>         pByteCode;
    CComPtr<ID3D11HullShader> pDS;

    auto hr = CompileD3DShader(Source, strFunctionName, pDefines, profile, &pByteCode);
    if (FAILED(hr))
    {
        ADD_FAILURE() << "Failed to compile hull shader";
        return pDS;
    }

    hr = m_pd3d11Device->CreateHullShader(pByteCode->GetBufferPointer(), pByteCode->GetBufferSize(), NULL, &pDS);
    if (FAILED(hr))
    {
        ADD_FAILURE() << "Failed to create hull shader";
    }
    return pDS;
}

CComPtr<ID3D11ComputeShader> TestingEnvironmentD3D11::CreateComputeShader(const std::string&      Source,
                                                                          LPCSTR                  strFunctionName,
                                                                          const D3D_SHADER_MACRO* pDefines,
                                                                          LPCSTR                  profile)
{
    CComPtr<ID3DBlob>            pByteCode;
    CComPtr<ID3D11ComputeShader> pCS;

    auto hr = CompileD3DShader(Source, strFunctionName, pDefines, profile, &pByteCode);
    if (FAILED(hr))
    {
        ADD_FAILURE() << "Failed to compile hull shader";
        return pCS;
    }

    hr = m_pd3d11Device->CreateComputeShader(pByteCode->GetBufferPointer(), pByteCode->GetBufferSize(), NULL, &pCS);
    if (FAILED(hr))
    {
        ADD_FAILURE() << "Failed to create hull shader";
    }
    return pCS;
}


void TestingEnvironmentD3D11::Reset()
{
    TestingEnvironment::Reset();
    m_pd3d11Context->ClearState();
}

TestingEnvironment* CreateTestingEnvironmentD3D11(const TestingEnvironment::CreateInfo& CI,
                                                  const SwapChainDesc&                  SCDesc)
{
    return new TestingEnvironmentD3D11{CI, SCDesc};
}

} // namespace Testing

} // namespace Diligent
