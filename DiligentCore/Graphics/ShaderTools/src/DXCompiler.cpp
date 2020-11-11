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

#include <memory>
#include <mutex>

// Platforms that support DXCompiler.
#if PLATFORM_WIN32
#    include "DXCompilerBaseWin32.hpp"
#elif PLATFORM_UNIVERSAL_WINDOWS
#    include "DXCompilerBaseUWP.hpp"
#elif PLATFORM_LINUX
#    include "DXCompilerBaseLiunx.hpp"
#else
#    error DXC is not supported on this platform
#endif

#include "DataBlobImpl.hpp"
#include "RefCntAutoPtr.hpp"
#include "ShaderToolsCommon.hpp"

#if D3D12_SUPPORTED
#    include <d3d12shader.h>
#endif

#include "HLSLUtils.hpp"

namespace Diligent
{

namespace
{

class DXCompilerImpl final : public DXCompilerBase
{
public:
    DXCompilerImpl(DXCompilerTarget Target, const char* pLibName) :
        m_Target{Target},
        m_LibName{pLibName ? pLibName : (Target == DXCompilerTarget::Direct3D12 ? "dxcompiler" : "spv_dxcompiler")}
    {}

    ShaderVersion GetMaxShaderModel() override final
    {
        Load();
        // mutex is not needed here
        return m_MaxShaderModel;
    }

    bool IsLoaded() override final
    {
        return GetCreateInstaceProc() != nullptr;
    }

    DxcCreateInstanceProc GetCreateInstaceProc()
    {
        return Load();
    }

    bool Compile(const CompileAttribs& Attribs) override final;

    virtual void Compile(const ShaderCreateInfo& ShaderCI,
                         ShaderVersion           ShaderModel,
                         const char*             ExtraDefinitions,
                         IDxcBlob**              ppByteCodeBlob,
                         std::vector<uint32_t>*  pByteCode,
                         IDataBlob**             ppCompilerOutput) noexcept(false) override final;

    virtual void GetD3D12ShaderReflection(IDxcBlob*                pShaderBytecode,
                                          ID3D12ShaderReflection** ppShaderReflection) override final;

private:
    DxcCreateInstanceProc Load()
    {
        std::unique_lock<std::mutex> lock{m_Guard};

        if (m_IsInitialized)
            return m_pCreateInstance;

        m_IsInitialized   = true;
        m_pCreateInstance = DXCompilerBase::Load(m_Target, m_LibName);

        if (m_pCreateInstance)
        {
            CComPtr<IDxcValidator> validator;
            if (SUCCEEDED(m_pCreateInstance(CLSID_DxcValidator, IID_PPV_ARGS(&validator))))
            {
                CComPtr<IDxcVersionInfo> info;
                if (SUCCEEDED(validator->QueryInterface(IID_PPV_ARGS(&info))))
                {
                    info->GetVersion(&m_MajorVer, &m_MinorVer);

                    LOG_INFO_MESSAGE("Loaded DX Shader Compiler, version ", m_MajorVer, ".", m_MinorVer);

                    auto ver = (m_MajorVer << 16) | (m_MinorVer & 0xFFFF);

                    // map known DXC version to maximum SM
                    switch (ver)
                    {
                        case 0x10005: m_MaxShaderModel = {6, 5}; break; // SM 6.5 and SM 6.6 preview
                        case 0x10004: m_MaxShaderModel = {6, 4}; break; // SM 6.4 and SM 6.5 preview
                        case 0x10003:
                        case 0x10002: m_MaxShaderModel = {6, 1}; break; // SM 6.1 and SM 6.2 preview
                        default: m_MaxShaderModel = (ver > 0x10005 ? ShaderVersion{6, 6} : ShaderVersion{6, 0}); break;
                    }
                }
            }
        }

        return m_pCreateInstance;
    }

private:
    DxcCreateInstanceProc  m_pCreateInstance = nullptr;
    bool                   m_IsInitialized   = false;
    ShaderVersion          m_MaxShaderModel;
    std::mutex             m_Guard;
    const String           m_LibName;
    const DXCompilerTarget m_Target;
    // Compiler version
    UINT32 m_MajorVer = 0;
    UINT32 m_MinorVer = 0;
};


class DxcIncludeHandlerImpl final : public IDxcIncludeHandler
{
public:
    explicit DxcIncludeHandlerImpl(IShaderSourceInputStreamFactory* pStreamFactory, CComPtr<IDxcLibrary> pLibrary) :
        m_pLibrary{pLibrary},
        m_pStreamFactory{pStreamFactory},
        m_RefCount{1}
    {
    }

    HRESULT STDMETHODCALLTYPE LoadSource(_In_ LPCWSTR pFilename, _COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource) override
    {
        if (pFilename == nullptr)
            return E_FAIL;

        String fileName;
        fileName.resize(wcslen(pFilename));
        for (size_t i = 0; i < fileName.size(); ++i)
        {
            fileName[i] = static_cast<char>(pFilename[i]);
        }

        if (fileName.empty())
        {
            LOG_ERROR("Failed to convert shader include file name ", fileName, ". File name must be ANSI string");
            return E_FAIL;
        }

        // validate file name
        if (fileName.size() > 2 && fileName[0] == '.' && (fileName[1] == '\\' || fileName[1] == '/'))
            fileName.erase(0, 2);

        RefCntAutoPtr<IFileStream> pSourceStream;
        m_pStreamFactory->CreateInputStream(fileName.c_str(), &pSourceStream);
        if (pSourceStream == nullptr)
        {
            LOG_ERROR("Failed to open shader include file ", fileName, ". Check that the file exists");
            return E_FAIL;
        }

        RefCntAutoPtr<IDataBlob> pFileData(MakeNewRCObj<DataBlobImpl>()(0));
        pSourceStream->ReadBlob(pFileData);

        CComPtr<IDxcBlobEncoding> sourceBlob;

        HRESULT hr = m_pLibrary->CreateBlobWithEncodingFromPinned(pFileData->GetDataPtr(), UINT32(pFileData->GetSize()), CP_UTF8, &sourceBlob);
        if (FAILED(hr))
        {
            LOG_ERROR("Failed to allocate space for shader include file ", fileName, ".");
            return E_FAIL;
        }

        m_FileDataCache.push_back(pFileData);

        sourceBlob->QueryInterface(IID_PPV_ARGS(ppIncludeSource));
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override
    {
        return E_FAIL;
    }

    ULONG STDMETHODCALLTYPE AddRef(void) override
    {
        return m_RefCount++;
    }

    ULONG STDMETHODCALLTYPE Release(void) override
    {
        --m_RefCount;
        VERIFY(m_RefCount > 0, "Inconsistent call to Release()");
        return m_RefCount;
    }

private:
    CComPtr<IDxcLibrary>                  m_pLibrary;
    IShaderSourceInputStreamFactory*      m_pStreamFactory;
    ULONG                                 m_RefCount;
    std::vector<RefCntAutoPtr<IDataBlob>> m_FileDataCache;
};

} // namespace


IDXCompiler* CreateDXCompiler(DXCompilerTarget Target, const char* pLibraryName)
{
    return new DXCompilerImpl{Target, pLibraryName};
}

bool DXCompilerImpl::Compile(const CompileAttribs& Attribs)
{
    auto CreateInstance = GetCreateInstaceProc();

    if (CreateInstance == nullptr)
    {
        LOG_ERROR("Failed to load DXCompiler");
        return false;
    }

    DEV_CHECK_ERR(Attribs.Source != nullptr && Attribs.SourceLength > 0, "'Source' must not be null and 'SourceLength' must be greater than 0");
    DEV_CHECK_ERR(Attribs.EntryPoint != nullptr, "'EntryPoint' must not be null");
    DEV_CHECK_ERR(Attribs.Profile != nullptr, "'Profile' must not be null");
    DEV_CHECK_ERR((Attribs.pDefines != nullptr) == (Attribs.DefinesCount > 0), "'DefinesCount' must be 0 if 'pDefines' is null");
    DEV_CHECK_ERR((Attribs.pArgs != nullptr) == (Attribs.ArgsCount > 0), "'ArgsCount' must be 0 if 'pArgs' is null");
    DEV_CHECK_ERR(Attribs.ppBlobOut != nullptr, "'ppBlobOut' must not be null");
    DEV_CHECK_ERR(Attribs.ppCompilerOutput != nullptr, "'ppCompilerOutput' must not be null");

    HRESULT hr;

    // NOTE: The call to DxcCreateInstance is thread-safe, but objects created by DxcCreateInstance aren't thread-safe.
    // Compiler objects should be created and then used on the same thread.
    // https://github.com/microsoft/DirectXShaderCompiler/wiki/Using-dxc.exe-and-dxcompiler.dll#dxcompiler-dll-interface

    CComPtr<IDxcLibrary> library;
    hr = CreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library));
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create DXC Library");
        return false;
    }

    CComPtr<IDxcCompiler> compiler;
    hr = CreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create DXC Compiler");
        return false;
    }

    CComPtr<IDxcBlobEncoding> sourceBlob;
    hr = library->CreateBlobWithEncodingFromPinned(Attribs.Source, UINT32{Attribs.SourceLength}, CP_UTF8, &sourceBlob);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create DXC Blob encoding");
        return false;
    }

    DxcIncludeHandlerImpl IncludeHandler{Attribs.pShaderSourceStreamFactory, library};

    CComPtr<IDxcOperationResult> result;
    hr = compiler->Compile(
        sourceBlob,
        L"",
        Attribs.EntryPoint,
        Attribs.Profile,
        Attribs.pArgs, UINT32{Attribs.ArgsCount},
        Attribs.pDefines, UINT32{Attribs.DefinesCount},
        Attribs.pShaderSourceStreamFactory ? &IncludeHandler : nullptr,
        &result);

    if (SUCCEEDED(hr))
    {
        HRESULT status;
        if (SUCCEEDED(result->GetStatus(&status)))
            hr = status;
    }

    if (result)
    {
        CComPtr<IDxcBlobEncoding> errorsBlob;
        CComPtr<IDxcBlobEncoding> errorsBlobUtf8;
        if (SUCCEEDED(result->GetErrorBuffer(&errorsBlob)) && SUCCEEDED(library->GetBlobAsUtf8(errorsBlob, &errorsBlobUtf8)))
        {
            errorsBlobUtf8->QueryInterface(IID_PPV_ARGS(Attribs.ppCompilerOutput));
        }
    }

    if (FAILED(hr))
    {
        return false;
    }

    CComPtr<IDxcBlob> compiled;
    hr = result->GetResult(&compiled);
    if (FAILED(hr))
        return false;

    // validate and sign
    if (m_Target == DXCompilerTarget::Direct3D12)
    {
        CComPtr<IDxcValidator> validator;
        hr = CreateInstance(CLSID_DxcValidator, IID_PPV_ARGS(&validator));
        if (FAILED(hr))
        {
            LOG_ERROR("Failed to create  DXC Validator");
            return false;
        }

        CComPtr<IDxcOperationResult> validationResult;
        hr = validator->Validate(compiled, DxcValidatorFlags_InPlaceEdit, &validationResult);

        if (validationResult == nullptr || FAILED(hr))
        {
            LOG_ERROR("Failed to validate shader bytecode");
            return false;
        }

        HRESULT status = E_FAIL;
        validationResult->GetStatus(&status);

        if (SUCCEEDED(status))
        {
            CComPtr<IDxcBlob> validated;
            hr = validationResult->GetResult(&validated);
            if (FAILED(hr))
                return false;

            *Attribs.ppBlobOut = validated ? validated.Detach() : compiled.Detach();
            return true;
        }
        else
        {
            CComPtr<IDxcBlobEncoding> validationOutput;
            CComPtr<IDxcBlobEncoding> validationOutputUtf8;
            validationResult->GetErrorBuffer(&validationOutput);
            library->GetBlobAsUtf8(validationOutput, &validationOutputUtf8);

            size_t      ValidationMsgLen = validationOutputUtf8 ? validationOutputUtf8->GetBufferSize() : 0;
            const char* ValidationMsg    = ValidationMsgLen > 0 ? static_cast<const char*>(validationOutputUtf8->GetBufferPointer()) : "";

            LOG_ERROR("Shader validation failed: ", ValidationMsg);
            return false;
        }
    }

    *Attribs.ppBlobOut = compiled.Detach();
    return true;
}

void DXCompilerImpl::GetD3D12ShaderReflection(IDxcBlob*                pShaderBytecode,
                                              ID3D12ShaderReflection** ppShaderReflection)
{
#if D3D12_SUPPORTED
    try
    {
        auto CreateInstance = GetCreateInstaceProc();
        if (CreateInstance == nullptr)
            return;

#    define FOURCC(a, b, c, d) (uint32_t{((d) << 24) | ((c) << 16) | ((b) << 8) | (a)})
        const uint32_t DFCC_DXIL = FOURCC('D', 'X', 'I', 'L');

        CComPtr<IDxcContainerReflection> pReflection;

        auto hr = CreateInstance(CLSID_DxcContainerReflection, IID_PPV_ARGS(&pReflection));
        if (FAILED(hr))
            LOG_ERROR_AND_THROW("Failed to create shader reflection instance");

        hr = pReflection->Load(pShaderBytecode);
        if (FAILED(hr))
            LOG_ERROR_AND_THROW("Failed to load shader reflection from bytecode");

        UINT32 shaderIdx;

        hr = pReflection->FindFirstPartKind(DFCC_DXIL, &shaderIdx);
        if (SUCCEEDED(hr))
        {
            hr = pReflection->GetPartReflection(shaderIdx, __uuidof(*ppShaderReflection), reinterpret_cast<void**>(ppShaderReflection));
            if (FAILED(hr))
                LOG_ERROR_AND_THROW("Failed to get the shader reflection");
        }
    }
    catch (...)
    {
    }
#endif
}


void DXCompilerImpl::Compile(const ShaderCreateInfo& ShaderCI,
                             ShaderVersion           ShaderModel,
                             const char*             ExtraDefinitions,
                             IDxcBlob**              ppByteCodeBlob,
                             std::vector<uint32_t>*  pByteCode,
                             IDataBlob**             ppCompilerOutput) noexcept(false)
{
    if (!IsLoaded())
    {
        UNEXPECTED("DX compiler is not loaded");
        return;
    }

    ShaderVersion MaxSM = GetMaxShaderModel();

    // validate shader version
    if (ShaderModel == ShaderVersion{})
    {
        ShaderModel = MaxSM;
    }
    else if (ShaderModel.Major < 6)
    {
        LOG_INFO_MESSAGE("DXC only supports shader model 6.0+. Upgrading the specified shader model ",
                         Uint32{ShaderModel.Major}, '_', Uint32{ShaderModel.Minor}, " to 6_0");
        ShaderModel = ShaderVersion{6, 0};
    }
    else if ((ShaderModel.Major > MaxSM.Major) ||
             (ShaderModel.Major == MaxSM.Major && ShaderModel.Minor > MaxSM.Minor))
    {
        LOG_WARNING_MESSAGE("The maximum supported shader model by DXC is ", Uint32{MaxSM.Major}, '_', Uint32{MaxSM.Minor},
                            ". The specified shader model ", Uint32{ShaderModel.Major}, '_', Uint32{ShaderModel.Minor}, " will be downgraded.");
        ShaderModel = MaxSM;
    }

    const auto         Profile = GetHLSLProfileString(ShaderCI.Desc.ShaderType, ShaderModel);
    const std::wstring wstrProfile{Profile.begin(), Profile.end()};
    const std::wstring wstrEntryPoint{ShaderCI.EntryPoint, ShaderCI.EntryPoint + strlen(ShaderCI.EntryPoint)};

    std::vector<const wchar_t*> DxilArgs;
    if (m_Target == DXCompilerTarget::Direct3D12)
    {
        DxilArgs.push_back(L"-Zpc"); // Matrices in column-major order

        //DxilArgs.push_back(L"-WX");  // Warnings as errors
#ifdef DILIGENT_DEBUG
        DxilArgs.push_back(L"-Zi"); // Debug info
        DxilArgs.push_back(L"-Od"); // Disable optimization
        if (m_MajorVer > 1 || m_MajorVer == 1 && m_MinorVer >= 5)
        {
            // Silence the following warning:
            // no output provided for debug - embedding PDB in shader container.  Use -Qembed_debug to silence this warning.
            DxilArgs.push_back(L"-Qembed_debug");
        }
#else
        DxilArgs.push_back(L"-Od"); // TODO: something goes wrong if optimization is enabled
#endif
    }
    else if (m_Target == DXCompilerTarget::Vulkan)
    {
        DxilArgs.assign(
            {
                L"-spirv",
                L"-fspv-reflect",
                L"-fspv-target-env=vulkan1.0",
                //L"-WX", // Warnings as errors
                L"-O3", // Optimization level 3
            });
    }
    else
    {
        UNEXPECTED("Unknown compiler target");
    }


    CComPtr<IDxcBlob> pDXIL;
    CComPtr<IDxcBlob> pDxcLog;

    IDXCompiler::CompileAttribs CA;

    auto Source = BuildHLSLSourceString(ShaderCI, ExtraDefinitions);

    CA.Source                     = Source.c_str();
    CA.SourceLength               = static_cast<Uint32>(Source.length());
    CA.EntryPoint                 = wstrEntryPoint.c_str();
    CA.Profile                    = wstrProfile.c_str();
    CA.pDefines                   = nullptr;
    CA.DefinesCount               = 0;
    CA.pArgs                      = DxilArgs.data();
    CA.ArgsCount                  = static_cast<Uint32>(DxilArgs.size());
    CA.pShaderSourceStreamFactory = ShaderCI.pShaderSourceStreamFactory;
    CA.ppBlobOut                  = &pDXIL;
    CA.ppCompilerOutput           = &pDxcLog;

    auto result = Compile(CA);
    HandleHLSLCompilerResult(result, pDxcLog.p, Source, ShaderCI.Desc.Name, ppCompilerOutput);

    if (result && pDXIL && pDXIL->GetBufferSize() > 0)
    {
        if (pByteCode != nullptr)
            pByteCode->assign(static_cast<uint32_t*>(pDXIL->GetBufferPointer()),
                              static_cast<uint32_t*>(pDXIL->GetBufferPointer()) + pDXIL->GetBufferSize() / sizeof(uint32_t));

        if (ppByteCodeBlob != nullptr)
            *ppByteCodeBlob = pDXIL.Detach();
    }
}

} // namespace Diligent
