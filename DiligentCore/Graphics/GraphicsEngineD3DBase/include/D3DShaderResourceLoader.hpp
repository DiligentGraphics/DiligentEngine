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
#include <vector>
#include <unordered_set>

#include "Shader.h"
#include "StringTools.hpp"

/// \file
/// D3D shader resource loading

namespace Diligent
{

struct D3DShaderResourceCounters
{
    Uint32 NumCBs      = 0;
    Uint32 NumTexSRVs  = 0;
    Uint32 NumTexUAVs  = 0;
    Uint32 NumBufSRVs  = 0;
    Uint32 NumBufUAVs  = 0;
    Uint32 NumSamplers = 0;
};

template <typename D3D_SHADER_DESC,
          typename D3D_SHADER_INPUT_BIND_DESC,

          typename TShaderReflection,
          typename THandleShaderDesc,
          typename TOnResourcesCounted,
          typename TOnNewCB,
          typename TOnNewTexUAV,
          typename TOnNewBuffUAV,
          typename TOnNewBuffSRV,
          typename TOnNewSampler,
          typename TOnNewTexSRV>
void LoadD3DShaderResources(TShaderReflection*  pShaderReflection,
                            THandleShaderDesc   HandleShaderDesc,
                            TOnResourcesCounted OnResourcesCounted,
                            TOnNewCB            OnNewCB,
                            TOnNewTexUAV        OnNewTexUAV,
                            TOnNewBuffUAV       OnNewBuffUAV,
                            TOnNewBuffSRV       OnNewBuffSRV,
                            TOnNewSampler       OnNewSampler,
                            TOnNewTexSRV        OnNewTexSRV)
{
    D3D_SHADER_DESC shaderDesc = {};
    pShaderReflection->GetDesc(&shaderDesc);

    HandleShaderDesc(shaderDesc);

    std::vector<D3DShaderResourceAttribs, STDAllocatorRawMem<D3DShaderResourceAttribs>> Resources(STD_ALLOCATOR_RAW_MEM(D3DShaderResourceAttribs, GetRawAllocator(), "Allocator for vector<D3DShaderResourceAttribs>"));
    Resources.reserve(shaderDesc.BoundResources);
    std::unordered_set<std::string> ResourceNamesTmpPool;

    D3DShaderResourceCounters RC;

    size_t ResourceNamesPoolSize = 0;
    // Number of resources to skip (used for array resources)
    UINT SkipCount = 1;
    for (UINT Res = 0; Res < shaderDesc.BoundResources; Res += SkipCount)
    {
        D3D_SHADER_INPUT_BIND_DESC BindingDesc = {};
        pShaderReflection->GetResourceBindingDesc(Res, &BindingDesc);

        std::string Name(BindingDesc.Name);

        SkipCount = 1;

        UINT BindCount = BindingDesc.BindCount;

        // Handle arrays
        // For shader models 5_0 and before, every resource array element is enumerated individually.
        // For instance, if the following texture array is defined in the shader:
        //
        //     Texture2D<float3> g_tex2DDiffuse[4];
        //
        // The reflection system will enumerate 4 resources with the following names:
        // "g_tex2DDiffuse[0]"
        // "g_tex2DDiffuse[1]"
        // "g_tex2DDiffuse[2]"
        // "g_tex2DDiffuse[3]"
        //
        // Notice that if some array element is not used by the shader, it will not be enumerated

        auto OpenBracketPos = Name.find('[');
        if (String::npos != OpenBracketPos)
        {
            VERIFY(BindCount == 1, "When array elements are enumerated individually, BindCount is expected to always be 1");

            // Name == "g_tex2DDiffuse[0]"
            //                        ^
            //                   OpenBracketPos
            Name.erase(OpenBracketPos, Name.length() - OpenBracketPos);
            // Name == "g_tex2DDiffuse"
            VERIFY_EXPR(Name.length() == OpenBracketPos);
#ifdef DILIGENT_DEBUG
            for (const auto& ExistingRes : Resources)
            {
                VERIFY(Name.compare(ExistingRes.Name) != 0, "Resource with the same name has already been enumerated. All array elements are expected to be enumerated one after another");
            }
#endif
            for (UINT ArrElem = Res + 1; ArrElem < shaderDesc.BoundResources; ++ArrElem)
            {
                D3D_SHADER_INPUT_BIND_DESC ArrElemBindingDesc = {};
                pShaderReflection->GetResourceBindingDesc(ArrElem, &ArrElemBindingDesc);

                // Make sure this case is handled correctly:
                // "g_tex2DDiffuse[.]" != "g_tex2DDiffuse2[.]"
                if (strncmp(Name.c_str(), ArrElemBindingDesc.Name, OpenBracketPos) == 0 && ArrElemBindingDesc.Name[OpenBracketPos] == '[')
                {
                    //g_tex2DDiffuse[2]
                    //               ^
                    UINT Ind  = atoi(ArrElemBindingDesc.Name + OpenBracketPos + 1);
                    BindCount = std::max(BindCount, Ind + 1);
                    VERIFY(ArrElemBindingDesc.BindPoint == BindingDesc.BindPoint + Ind,
                           "Array elements are expected to use contigous bind points.\n",
                           BindingDesc.Name, " uses slot ", BindingDesc.BindPoint, ", so ", ArrElemBindingDesc.Name,
                           " is expected to use slot ", BindingDesc.BindPoint + Ind, " while ", ArrElemBindingDesc.BindPoint,
                           " is actually used");

                    // Note that skip count may not necessarily be the same as BindCount.
                    // If some array elements are not used by the shader, the reflection system skips them
                    ++SkipCount;
                }
                else
                {
                    break;
                }
            }
        }

        switch (BindingDesc.Type)
        {
            // clang-format off
            case D3D_SIT_CBUFFER:                       ++RC.NumCBs;                                                                           break;
            case D3D_SIT_TBUFFER:                       UNSUPPORTED( "TBuffers are not supported" );                                           break;
            case D3D_SIT_TEXTURE:                       ++(BindingDesc.Dimension == D3D_SRV_DIMENSION_BUFFER ? RC.NumBufSRVs : RC.NumTexSRVs); break;
            case D3D_SIT_SAMPLER:                       ++RC.NumSamplers;                                                                      break;
            case D3D_SIT_UAV_RWTYPED:                   ++(BindingDesc.Dimension == D3D_SRV_DIMENSION_BUFFER ? RC.NumBufUAVs : RC.NumTexUAVs); break;
            case D3D_SIT_STRUCTURED:                    ++RC.NumBufSRVs;                                                                       break;
            case D3D_SIT_UAV_RWSTRUCTURED:              ++RC.NumBufUAVs;                                                                       break;
            case D3D_SIT_BYTEADDRESS:                   ++RC.NumBufSRVs;                                                                       break;
            case D3D_SIT_UAV_RWBYTEADDRESS:             ++RC.NumBufUAVs;                                                                       break;
            case D3D_SIT_UAV_APPEND_STRUCTURED:         UNSUPPORTED( "Append structured buffers are not supported" );                          break;
            case D3D_SIT_UAV_CONSUME_STRUCTURED:        UNSUPPORTED( "Consume structured buffers are not supported" );                         break;
            case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER: UNSUPPORTED( "RW structured buffers with counter are not supported" );                 break;
            // clang-format on
            default: UNEXPECTED("Unexpected resource type");
        }
        ResourceNamesPoolSize += Name.length() + 1;
        auto it = ResourceNamesTmpPool.emplace(std::move(Name));
        Resources.emplace_back(
            it.first->c_str(),
            BindingDesc.BindPoint,
            BindCount,
            BindingDesc.Type,
            BindingDesc.Dimension,
            D3DShaderResourceAttribs::InvalidSamplerId);
    }

    OnResourcesCounted(RC, ResourceNamesPoolSize);

    std::vector<size_t, STDAllocatorRawMem<size_t>> TexSRVInds(STD_ALLOCATOR_RAW_MEM(size_t, GetRawAllocator(), "Allocator for vector<size_t>"));
    TexSRVInds.reserve(RC.NumTexSRVs);

    for (size_t ResInd = 0; ResInd < Resources.size(); ++ResInd)
    {
        const auto& Res = Resources[ResInd];
        switch (Res.GetInputType())
        {
            case D3D_SIT_CBUFFER:
            {
                OnNewCB(Res);
                break;
            }

            case D3D_SIT_TBUFFER:
            {
                UNSUPPORTED("TBuffers are not supported");
                break;
            }

            case D3D_SIT_TEXTURE:
            {
                if (Res.GetSRVDimension() == D3D_SRV_DIMENSION_BUFFER)
                {
                    OnNewBuffSRV(Res);
                }
                else
                {
                    // Texture SRVs must be processed all samplers are initialized
                    TexSRVInds.push_back(ResInd);
                }
                break;
            }

            case D3D_SIT_SAMPLER:
            {
                OnNewSampler(Res);
                break;
            }

            case D3D_SIT_UAV_RWTYPED:
            {
                if (Res.GetSRVDimension() == D3D_SRV_DIMENSION_BUFFER)
                {
                    OnNewBuffUAV(Res);
                }
                else
                {
                    OnNewTexUAV(Res);
                }
                break;
            }

            case D3D_SIT_STRUCTURED:
            {
                OnNewBuffSRV(Res);
                break;
            }

            case D3D_SIT_UAV_RWSTRUCTURED:
            {
                OnNewBuffUAV(Res);
                break;
            }

            case D3D_SIT_BYTEADDRESS:
            {
                OnNewBuffSRV(Res);
                break;
            }

            case D3D_SIT_UAV_RWBYTEADDRESS:
            {
                OnNewBuffUAV(Res);
                break;
            }

            case D3D_SIT_UAV_APPEND_STRUCTURED:
            {
                UNSUPPORTED("Append structured buffers are not supported");
                break;
            }

            case D3D_SIT_UAV_CONSUME_STRUCTURED:
            {
                UNSUPPORTED("Consume structured buffers are not supported");
                break;
            }

            case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
            {
                UNSUPPORTED("RW structured buffers with counter are not supported");
                break;
            }

            default:
            {
                UNEXPECTED("Unexpected resource input type");
            }
        }
    }

    // Process texture SRVs. We need to do this after all samplers are initialized
    for (auto TexSRVInd : TexSRVInds)
    {
        OnNewTexSRV(Resources[TexSRVInd]);
    }
}

} // namespace Diligent
