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

#include "pch.h"

#include <d3dcompiler.h>
#include "ShaderResourcesD3D12.hpp"
#include "ShaderD3DBase.hpp"
#include "ShaderBase.hpp"
#include "DXCompiler.hpp"

#include "dxc/DxilContainer/DxilContainer.h"

namespace Diligent
{

static bool IsDXILBytecode(ID3DBlob* pBytecodeBlob)
{
    const auto* data_begin = reinterpret_cast<const uint8_t*>(pBytecodeBlob->GetBufferPointer());
    const auto* data_end   = data_begin + pBytecodeBlob->GetBufferSize();
    const auto* ptr        = data_begin;

    if (ptr + sizeof(hlsl::DxilContainerHeader) > data_end)
    {
        // No space for the container header
        return false;
    }

    // A DXIL container is composed of a header, a sequence of part lengths, and a sequence of parts.
    // https://github.com/microsoft/DirectXShaderCompiler/blob/master/docs/DXIL.rst#dxil-container-format
    const auto& ContainerHeader = *reinterpret_cast<const hlsl::DxilContainerHeader*>(ptr);
    if (ContainerHeader.HeaderFourCC != hlsl::DFCC_Container)
    {
        // Incorrect FourCC
        return false;
    }

    if (ContainerHeader.Version.Major != hlsl::DxilContainerVersionMajor)
    {
        LOG_WARNING_MESSAGE("Unable to parse DXIL container: the container major version is ", Uint32{ContainerHeader.Version.Major},
                            " while ", Uint32{hlsl::DxilContainerVersionMajor}, " is expected");
        return false;
    }

    // The header is followed by uint32_t PartOffset[PartCount];
    // The offset is to a DxilPartHeader.
    ptr += sizeof(hlsl::DxilContainerHeader);
    if (ptr + sizeof(uint32_t) * ContainerHeader.PartCount > data_end)
    {
        // No space for offsets
        return false;
    }

    const auto* PartOffsets = reinterpret_cast<const uint32_t*>(ptr);
    for (uint32_t part = 0; part < ContainerHeader.PartCount; ++part)
    {
        const auto Offset = PartOffsets[part];
        if (data_begin + Offset + sizeof(hlsl::DxilPartHeader) > data_end)
        {
            // No space for the part header
            return false;
        }

        const auto& PartHeader = *reinterpret_cast<const hlsl::DxilPartHeader*>(data_begin + Offset);
        if (PartHeader.PartFourCC == hlsl::DFCC_DXIL)
        {
            // We found DXIL part
            return true;
        }
    }

    return false;
}

ShaderResourcesD3D12::ShaderResourcesD3D12(ID3DBlob*         pShaderBytecode,
                                           const ShaderDesc& ShdrDesc,
                                           const char*       CombinedSamplerSuffix,
                                           IDXCompiler*      pDXCompiler) :
    ShaderResources{ShdrDesc.ShaderType}
{
    class NewResourceHandler
    {
    public:
        // clang-format off
        void OnNewCB     (const D3DShaderResourceAttribs& CBAttribs)     {}
        void OnNewTexUAV (const D3DShaderResourceAttribs& TexUAV)        {}
        void OnNewBuffUAV(const D3DShaderResourceAttribs& BuffUAV)       {}
        void OnNewBuffSRV(const D3DShaderResourceAttribs& BuffSRV)       {}
        void OnNewSampler(const D3DShaderResourceAttribs& SamplerAttribs){}
        void OnNewTexSRV (const D3DShaderResourceAttribs& TexAttribs)    {}
        // clang-format on
    };

    CComPtr<ID3D12ShaderReflection> pShaderReflection;
    if (IsDXILBytecode(pShaderBytecode))
    {
        VERIFY(pDXCompiler != nullptr, "DXC is not initialized");
        pDXCompiler->GetD3D12ShaderReflection(reinterpret_cast<IDxcBlob*>(pShaderBytecode), &pShaderReflection);
        if (!pShaderReflection)
        {
            LOG_ERROR_AND_THROW("Failed to read shader reflection from DXIL container");
        }
    }
    else
    {
        // Use D3D compiler to get reflection.
        auto hr = D3DReflect(pShaderBytecode->GetBufferPointer(), pShaderBytecode->GetBufferSize(), __uuidof(pShaderReflection), reinterpret_cast<void**>(&pShaderReflection));
        CHECK_D3D_RESULT_THROW(hr, "Failed to get the shader reflection");
    }

    Initialize<D3D12_SHADER_DESC, D3D12_SHADER_INPUT_BIND_DESC, ID3D12ShaderReflection>(
        pShaderReflection,
        NewResourceHandler{},
        ShdrDesc.Name,
        CombinedSamplerSuffix);
}

} // namespace Diligent
