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

#include <unordered_map>
#include <vector>
#include <memory>

#include <D3Dcompiler.h>

#include <atlcomcli.h>
#include "dxc/dxcapi.h"

#include "D3DErrors.hpp"
#include "DataBlobImpl.hpp"
#include "RefCntAutoPtr.hpp"
#include "ShaderD3DBase.hpp"
#include "DXCompiler.hpp"
#include "HLSLUtils.hpp"

namespace Diligent
{

class D3DIncludeImpl : public ID3DInclude
{
public:
    D3DIncludeImpl(IShaderSourceInputStreamFactory* pStreamFactory) :
        m_pStreamFactory{pStreamFactory}
    {
    }

    STDMETHOD(Open)
    (THIS_ D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes)
    {
        RefCntAutoPtr<IFileStream> pSourceStream;
        m_pStreamFactory->CreateInputStream(pFileName, &pSourceStream);
        if (pSourceStream == nullptr)
        {
            LOG_ERROR("Failed to open shader include file ", pFileName, ". Check that the file exists");
            return E_FAIL;
        }

        RefCntAutoPtr<IDataBlob> pFileData(MakeNewRCObj<DataBlobImpl>{}(0));
        pSourceStream->ReadBlob(pFileData);
        *ppData = pFileData->GetDataPtr();
        *pBytes = static_cast<UINT>(pFileData->GetSize());

        m_DataBlobs.insert(std::make_pair(*ppData, pFileData));

        return S_OK;
    }

    STDMETHOD(Close)
    (THIS_ LPCVOID pData)
    {
        m_DataBlobs.erase(pData);
        return S_OK;
    }

private:
    IShaderSourceInputStreamFactory*                      m_pStreamFactory;
    std::unordered_map<LPCVOID, RefCntAutoPtr<IDataBlob>> m_DataBlobs;
};

static HRESULT CompileShader(const char*             Source,
                             size_t                  SourceLength,
                             const ShaderCreateInfo& ShaderCI,
                             LPCSTR                  profile,
                             ID3DBlob**              ppBlobOut,
                             ID3DBlob**              ppCompilerOutput)
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

    D3DIncludeImpl IncludeImpl{ShaderCI.pShaderSourceStreamFactory};
    return D3DCompile(Source, SourceLength, NULL, nullptr, &IncludeImpl, ShaderCI.EntryPoint, profile, dwShaderFlags, 0, ppBlobOut, ppCompilerOutput);
}

ShaderD3DBase::ShaderD3DBase(const ShaderCreateInfo& ShaderCI, const ShaderVersion ShaderModel, IDXCompiler* DxCompiler)
{
    if (ShaderCI.Source || ShaderCI.FilePath)
    {
        DEV_CHECK_ERR(ShaderCI.ByteCode == nullptr, "'ByteCode' must be null when shader is created from the source code or a file");
        DEV_CHECK_ERR(ShaderCI.ByteCodeSize == 0, "'ByteCodeSize' must be 0 when shader is created from the source code or a file");
        DEV_CHECK_ERR(ShaderCI.EntryPoint != nullptr, "Entry point must not be null");

        bool UseDXC = false;

        // validate compiler type
        switch (ShaderCI.ShaderCompiler)
        {
            case SHADER_COMPILER_DEFAULT:
                UseDXC = false;
                break;

            case SHADER_COMPILER_DXC:
                UseDXC = DxCompiler != nullptr && DxCompiler->IsLoaded();
                if (!UseDXC)
                    LOG_WARNING_MESSAGE("DXC compiler is not available. Using default shader compiler");
                break;

            case SHADER_COMPILER_FXC:
                UseDXC = false;
                break;

            default: UNEXPECTED("Unsupported shader compiler");
        }

        if (UseDXC)
        {
            VERIFY_EXPR(__uuidof(ID3DBlob) == __uuidof(IDxcBlob));
            DxCompiler->Compile(ShaderCI, ShaderModel, nullptr, reinterpret_cast<IDxcBlob**>(&m_pShaderByteCode), nullptr, ShaderCI.ppCompilerOutput);
        }
        else
        {
            std::string strShaderProfile = GetHLSLProfileString(ShaderCI.Desc.ShaderType, ShaderModel);

            String ShaderSource = BuildHLSLSourceString(ShaderCI);

            CComPtr<ID3DBlob> CompilerOutput;

            auto hr = CompileShader(ShaderSource.c_str(), ShaderSource.length(), ShaderCI, strShaderProfile.c_str(), &m_pShaderByteCode, &CompilerOutput);
            HandleHLSLCompilerResult(SUCCEEDED(hr), CompilerOutput.p, ShaderSource, ShaderCI.Desc.Name, ShaderCI.ppCompilerOutput);
        }
    }
    else if (ShaderCI.ByteCode)
    {
        DEV_CHECK_ERR(ShaderCI.ByteCodeSize != 0, "ByteCode size must be greater than 0");
        CHECK_D3D_RESULT_THROW(D3DCreateBlob(ShaderCI.ByteCodeSize, &m_pShaderByteCode), "Failed to create D3D blob");
        memcpy(m_pShaderByteCode->GetBufferPointer(), ShaderCI.ByteCode, ShaderCI.ByteCodeSize);
    }
    else
    {
        LOG_ERROR_AND_THROW("Shader source must be provided through one of the 'Source', 'FilePath' or 'ByteCode' members");
    }
}

} // namespace Diligent
