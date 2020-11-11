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
#include "ShaderResourcesD3D11.hpp"
#include "ShaderBase.hpp"
#include "ShaderResourceCacheD3D11.hpp"
#include "RenderDeviceD3D11Impl.hpp"

namespace Diligent
{


ShaderResourcesD3D11::ShaderResourcesD3D11(RenderDeviceD3D11Impl* pDeviceD3D11Impl,
                                           ID3DBlob*              pShaderBytecode,
                                           const ShaderDesc&      ShdrDesc,
                                           const char*            CombinedSamplerSuffix) :
    ShaderResources{ShdrDesc.ShaderType}
{
    class NewResourceHandler
    {
    public:
        NewResourceHandler(RenderDeviceD3D11Impl* const _pDeviceD3D11Impl,
                           const ShaderDesc&            _ShdrDesc,
                           const char*                  _CombinedSamplerSuffix,
                           ShaderResourcesD3D11&        _Resources) :
            // clang-format off
            pDeviceD3D11Impl     {_pDeviceD3D11Impl     },
            ShdrDesc             {_ShdrDesc             },
            CombinedSamplerSuffix{_CombinedSamplerSuffix},
            Resources            {_Resources            }
        // clang-format on
        {}

        void OnNewCB(const D3DShaderResourceAttribs& CBAttribs)
        {
            VERIFY(CBAttribs.BindPoint + CBAttribs.BindCount - 1 <= MaxAllowedBindPoint, "CB bind point exceeds supported range");
            Resources.m_MaxCBBindPoint = std::max(Resources.m_MaxCBBindPoint, static_cast<MaxBindPointType>(CBAttribs.BindPoint + CBAttribs.BindCount - 1));
        }

        void OnNewTexUAV(const D3DShaderResourceAttribs& TexUAV)
        {
            VERIFY(TexUAV.BindPoint + TexUAV.BindCount - 1 <= MaxAllowedBindPoint, "Tex UAV bind point exceeds supported range");
            Resources.m_MaxUAVBindPoint = std::max(Resources.m_MaxUAVBindPoint, static_cast<MaxBindPointType>(TexUAV.BindPoint + TexUAV.BindCount - 1));
        }

        void OnNewBuffUAV(const D3DShaderResourceAttribs& BuffUAV)
        {
            VERIFY(BuffUAV.BindPoint + BuffUAV.BindCount - 1 <= MaxAllowedBindPoint, "Buff UAV bind point exceeds supported range");
            Resources.m_MaxUAVBindPoint = std::max(Resources.m_MaxUAVBindPoint, static_cast<MaxBindPointType>(BuffUAV.BindPoint + BuffUAV.BindCount - 1));
        }

        void OnNewBuffSRV(const D3DShaderResourceAttribs& BuffSRV)
        {
            VERIFY(BuffSRV.BindPoint + BuffSRV.BindCount - 1 <= MaxAllowedBindPoint, "Buff SRV bind point exceeds supported range");
            Resources.m_MaxSRVBindPoint = std::max(Resources.m_MaxSRVBindPoint, static_cast<MaxBindPointType>(BuffSRV.BindPoint + BuffSRV.BindCount - 1));
        }

        void OnNewSampler(const D3DShaderResourceAttribs& SamplerAttribs)
        {
            VERIFY(SamplerAttribs.BindPoint + SamplerAttribs.BindCount - 1 <= MaxAllowedBindPoint, "Sampler bind point exceeds supported range");
            Resources.m_MaxSamplerBindPoint = std::max(Resources.m_MaxSamplerBindPoint, static_cast<MaxBindPointType>(SamplerAttribs.BindPoint + SamplerAttribs.BindCount - 1));
        }

        void OnNewTexSRV(const D3DShaderResourceAttribs& TexAttribs)
        {
            VERIFY(TexAttribs.BindPoint + TexAttribs.BindCount - 1 <= MaxAllowedBindPoint, "Tex SRV bind point exceeds supported range");
            Resources.m_MaxSRVBindPoint = std::max(Resources.m_MaxSRVBindPoint, static_cast<MaxBindPointType>(TexAttribs.BindPoint + TexAttribs.BindCount - 1));
        }

        ~NewResourceHandler()
        {
        }

    private:
        RenderDeviceD3D11Impl* const pDeviceD3D11Impl;
        const ShaderDesc&            ShdrDesc;
        const char*                  CombinedSamplerSuffix;
        ShaderResourcesD3D11&        Resources;
    };

    CComPtr<ID3D11ShaderReflection> pShaderReflection;
    HRESULT                         hr = D3DReflect(pShaderBytecode->GetBufferPointer(), pShaderBytecode->GetBufferSize(), __uuidof(pShaderReflection), reinterpret_cast<void**>(&pShaderReflection));
    CHECK_D3D_RESULT_THROW(hr, "Failed to get the shader reflection");


    Initialize<D3D11_SHADER_DESC, D3D11_SHADER_INPUT_BIND_DESC>(
        static_cast<ID3D11ShaderReflection*>(pShaderReflection),
        NewResourceHandler{pDeviceD3D11Impl, ShdrDesc, CombinedSamplerSuffix, *this},
        ShdrDesc.Name,
        CombinedSamplerSuffix);
}


ShaderResourcesD3D11::~ShaderResourcesD3D11()
{
}


#ifdef DILIGENT_DEVELOPMENT
static String DbgMakeResourceName(const D3DShaderResourceAttribs& Attr, Uint32 BindPoint)
{
    VERIFY(BindPoint >= Uint32{Attr.BindPoint} && BindPoint < Uint32{Attr.BindPoint} + Attr.BindCount, "Bind point is out of allowed range");
    if (Attr.BindCount == 1)
        return Attr.Name;
    else
        return String(Attr.Name) + '[' + std::to_string(BindPoint - Attr.BindPoint) + ']';
}

void ShaderResourcesD3D11::dvpVerifyCommittedResources(ID3D11Buffer*              CommittedD3D11CBs[],
                                                       ID3D11ShaderResourceView*  CommittedD3D11SRVs[],
                                                       ID3D11Resource*            CommittedD3D11SRVResources[],
                                                       ID3D11SamplerState*        CommittedD3D11Samplers[],
                                                       ID3D11UnorderedAccessView* CommittedD3D11UAVs[],
                                                       ID3D11Resource*            CommittedD3D11UAVResources[],
                                                       ShaderResourceCacheD3D11&  ResourceCache) const
{
    ShaderResourceCacheD3D11::CachedCB*       CachedCBs          = nullptr;
    ID3D11Buffer**                            d3d11CBs           = nullptr;
    ShaderResourceCacheD3D11::CachedResource* CachedSRVResources = nullptr;
    ID3D11ShaderResourceView**                d3d11SRVs          = nullptr;
    ShaderResourceCacheD3D11::CachedSampler*  CachedSamplers     = nullptr;
    ID3D11SamplerState**                      d3d11Samplers      = nullptr;
    ShaderResourceCacheD3D11::CachedResource* CachedUAVResources = nullptr;
    ID3D11UnorderedAccessView**               d3d11UAVs          = nullptr;
    // clang-format off
    ResourceCache.GetCBArrays     (CachedCBs,          d3d11CBs);
    ResourceCache.GetSRVArrays    (CachedSRVResources, d3d11SRVs);
    ResourceCache.GetSamplerArrays(CachedSamplers,     d3d11Samplers);
    ResourceCache.GetUAVArrays    (CachedUAVResources, d3d11UAVs);
    // clang-format on

    ProcessResources(
        [&](const D3DShaderResourceAttribs& cb, Uint32) //
        {
            for (auto BindPoint = cb.BindPoint; BindPoint < cb.BindPoint + cb.BindCount; ++BindPoint)
            {
                if (BindPoint >= ResourceCache.GetCBCount())
                {
                    LOG_ERROR_MESSAGE("Unable to find constant buffer '", DbgMakeResourceName(cb, BindPoint), "' (slot ",
                                      BindPoint, ") in the resource cache: the cache reserves ", ResourceCache.GetCBCount(),
                                      " CB slots only. This should never happen and may be the result of using wrong resource cache.");
                    continue;
                }
                auto& CB = CachedCBs[BindPoint];
                if (CB.pBuff == nullptr)
                {
                    LOG_ERROR_MESSAGE("Constant buffer '", DbgMakeResourceName(cb, BindPoint), "' (slot ",
                                      BindPoint, ") is not initialized in the resource cache.");
                    continue;
                }

                if (!(CB.pBuff->GetDesc().BindFlags & BIND_UNIFORM_BUFFER))
                {
                    LOG_ERROR_MESSAGE("Buffer '", CB.pBuff->GetDesc().Name,
                                      "' committed in the device context as constant buffer to variable '",
                                      DbgMakeResourceName(cb, BindPoint), "' (slot ", BindPoint,
                                      ") in shader '", GetShaderName(), "' was not created with BIND_UNIFORM_BUFFER flag");
                    continue;
                }

                VERIFY_EXPR(d3d11CBs[BindPoint] == CB.pBuff->GetD3D11Buffer());

                if (CommittedD3D11CBs[BindPoint] == nullptr)
                {
                    LOG_ERROR_MESSAGE("No D3D11 resource committed to constant buffer '", DbgMakeResourceName(cb, BindPoint),
                                      "' (slot ", BindPoint, ") in shader '", GetShaderName(), "'");
                    continue;
                }

                if (CommittedD3D11CBs[BindPoint] != d3d11CBs[BindPoint])
                {
                    LOG_ERROR_MESSAGE("D3D11 resource committed to constant buffer '", DbgMakeResourceName(cb, BindPoint),
                                      "' (slot ", BindPoint, ") in shader '", GetShaderName(), "' does not match the resource in the resource cache");
                    continue;
                }
            }
        },

        [&](const D3DShaderResourceAttribs& sam, Uint32) //
        {
            for (auto BindPoint = sam.BindPoint; BindPoint < sam.BindPoint + sam.BindCount; ++BindPoint)
            {
                if (BindPoint >= ResourceCache.GetSamplerCount())
                {
                    LOG_ERROR_MESSAGE("Unable to find sampler '", DbgMakeResourceName(sam, BindPoint), "' (slot ", BindPoint,
                                      ") in the resource cache: the cache reserves ", ResourceCache.GetSamplerCount(),
                                      " Sampler slots only. This should never happen and may be the result of using wrong resource cache.");
                    continue;
                }
                auto& Sam = CachedSamplers[BindPoint];
                if (Sam.pSampler == nullptr)
                {
                    LOG_ERROR_MESSAGE("Sampler '", DbgMakeResourceName(sam, BindPoint), "' (slot ", BindPoint,
                                      ") is not initialized in the resource cache.");
                    continue;
                }
                VERIFY_EXPR(d3d11Samplers[BindPoint] == Sam.pSampler->GetD3D11SamplerState());

                if (CommittedD3D11Samplers[BindPoint] == nullptr)
                {
                    LOG_ERROR_MESSAGE("No D3D11 sampler committed to variable '", DbgMakeResourceName(sam, BindPoint),
                                      "' (slot ", BindPoint, ") in shader '", GetShaderName(), "'");
                    continue;
                }

                if (CommittedD3D11Samplers[BindPoint] != d3d11Samplers[BindPoint])
                {
                    LOG_ERROR_MESSAGE("D3D11 sampler committed to variable '", DbgMakeResourceName(sam, BindPoint),
                                      "' (slot ", BindPoint, ") in shader '", GetShaderName(), "' does not match the resource in the resource cache");
                    continue;
                }
            }
        },

        [&](const D3DShaderResourceAttribs& tex, Uint32) //
        {
            for (auto BindPoint = tex.BindPoint; BindPoint < tex.BindPoint + tex.BindCount; ++BindPoint)
            {
                if (BindPoint >= ResourceCache.GetSRVCount())
                {
                    LOG_ERROR_MESSAGE("Unable to find texture SRV '", DbgMakeResourceName(tex, BindPoint), "' (slot ",
                                      BindPoint, ") in the resource cache: the cache reserves ", ResourceCache.GetSRVCount(),
                                      " SRV slots only. This should never happen and may be the result of using wrong resource cache.");
                    continue;
                }
                auto& SRVRes = CachedSRVResources[BindPoint];
                if (SRVRes.pBuffer != nullptr)
                {
                    LOG_ERROR_MESSAGE("Unexpected buffer bound to variable '", DbgMakeResourceName(tex, BindPoint), "' (slot ",
                                      BindPoint, "). Texture is expected.");
                    continue;
                }
                if (SRVRes.pTexture == nullptr)
                {
                    LOG_ERROR_MESSAGE("Texture '", DbgMakeResourceName(tex, BindPoint), "' (slot ", BindPoint,
                                      ") is not initialized in the resource cache.");
                    continue;
                }

                if (!(SRVRes.pTexture->GetDesc().BindFlags & BIND_SHADER_RESOURCE))
                {
                    LOG_ERROR_MESSAGE("Texture '", SRVRes.pTexture->GetDesc().Name,
                                      "' committed in the device context as SRV to variable '", DbgMakeResourceName(tex, BindPoint),
                                      "' (slot ", BindPoint, ") in shader '", GetShaderName(), "' was not created with BIND_SHADER_RESOURCE flag");
                }

                if (CommittedD3D11SRVs[BindPoint] == nullptr)
                {
                    LOG_ERROR_MESSAGE("No D3D11 resource committed to texture SRV '", DbgMakeResourceName(tex, BindPoint),
                                      "' (slot ", BindPoint, ") in shader '", GetShaderName(), "'");
                    continue;
                }

                if (CommittedD3D11SRVs[BindPoint] != d3d11SRVs[BindPoint])
                {
                    LOG_ERROR_MESSAGE("D3D11 resource committed to texture SRV '", DbgMakeResourceName(tex, BindPoint),
                                      "' (slot ", BindPoint, ") in shader '", GetShaderName(),
                                      "' does not match the resource in the resource cache");
                    continue;
                }
            }

            if (tex.IsCombinedWithSampler())
            {
                const auto& SamAttribs = GetCombinedSampler(tex);
                VERIFY_EXPR(SamAttribs.IsValidBindPoint());
                VERIFY_EXPR(SamAttribs.BindCount == 1 || SamAttribs.BindCount == tex.BindCount);
            }
        },

        [&](const D3DShaderResourceAttribs& uav, Uint32) //
        {
            for (auto BindPoint = uav.BindPoint; BindPoint < uav.BindPoint + uav.BindCount; ++BindPoint)
            {
                if (BindPoint >= ResourceCache.GetUAVCount())
                {
                    LOG_ERROR_MESSAGE("Unable to find texture UAV '", DbgMakeResourceName(uav, BindPoint), "' (slot ",
                                      BindPoint, ") in the resource cache: the cache reserves ", ResourceCache.GetUAVCount(),
                                      " UAV slots only. This should never happen and may be the result of using wrong resource cache.");
                    continue;
                }
                auto& UAVRes = CachedUAVResources[BindPoint];
                if (UAVRes.pBuffer != nullptr)
                {
                    LOG_ERROR_MESSAGE("Unexpected buffer bound to variable '", DbgMakeResourceName(uav, BindPoint),
                                      "' (slot ", BindPoint, "). Texture is expected.");
                    continue;
                }
                if (UAVRes.pTexture == nullptr)
                {
                    LOG_ERROR_MESSAGE("Texture '", DbgMakeResourceName(uav, BindPoint), "' (slot ", BindPoint,
                                      ") is not initialized in the resource cache.");
                    continue;
                }

                if (!(UAVRes.pTexture->GetDesc().BindFlags & BIND_UNORDERED_ACCESS))
                {
                    LOG_ERROR_MESSAGE("Texture '", UAVRes.pTexture->GetDesc().Name,
                                      "' committed in the device context as UAV to variable '", DbgMakeResourceName(uav, BindPoint), "' (slot ", BindPoint, ") in shader '", GetShaderName(), "' was not created with BIND_UNORDERED_ACCESS flag");
                }

                if (CommittedD3D11UAVs[BindPoint] == nullptr)
                {
                    LOG_ERROR_MESSAGE("No D3D11 resource committed to texture UAV '", DbgMakeResourceName(uav, BindPoint),
                                      "' (slot ", BindPoint, ") in shader '", GetShaderName(), "'");
                    continue;
                }

                if (CommittedD3D11UAVs[BindPoint] != d3d11UAVs[BindPoint])
                {
                    LOG_ERROR_MESSAGE("D3D11 resource committed to texture UAV '", DbgMakeResourceName(uav, BindPoint),
                                      "' (slot ", BindPoint, ") in shader '", GetShaderName(), "' does not match the resource in the resource cache");
                    continue;
                }
            }
        },


        [&](const D3DShaderResourceAttribs& buf, Uint32) //
        {
            for (auto BindPoint = buf.BindPoint; BindPoint < buf.BindPoint + buf.BindCount; ++BindPoint)
            {
                if (BindPoint >= ResourceCache.GetSRVCount())
                {
                    LOG_ERROR_MESSAGE("Unable to find buffer SRV '", DbgMakeResourceName(buf, BindPoint), "' (slot ", BindPoint,
                                      ") in the resource cache: the cache reserves ", ResourceCache.GetSRVCount(),
                                      " SRV slots only. This should never happen and may be the result of using wrong resource cache.");
                    continue;
                }
                auto& SRVRes = CachedSRVResources[BindPoint];
                if (SRVRes.pTexture != nullptr)
                {
                    LOG_ERROR_MESSAGE("Unexpected texture bound to variable '", DbgMakeResourceName(buf, BindPoint),
                                      "' (slot ", BindPoint, "). Buffer is expected.");
                    continue;
                }
                if (SRVRes.pBuffer == nullptr)
                {
                    LOG_ERROR_MESSAGE("Buffer '", DbgMakeResourceName(buf, BindPoint), "' (slot ", BindPoint,
                                      ") is not initialized in the resource cache.");
                    continue;
                }

                if (!(SRVRes.pBuffer->GetDesc().BindFlags & BIND_SHADER_RESOURCE))
                {
                    LOG_ERROR_MESSAGE("Buffer '", SRVRes.pBuffer->GetDesc().Name,
                                      "' committed in the device context as SRV to variable '", DbgMakeResourceName(buf, BindPoint),
                                      "' (slot ", BindPoint, ") in shader '", GetShaderName(),
                                      "' was not created with BIND_SHADER_RESOURCE flag");
                }

                if (CommittedD3D11SRVs[BindPoint] == nullptr)
                {
                    LOG_ERROR_MESSAGE("No D3D11 resource committed to buffer SRV '", DbgMakeResourceName(buf, BindPoint),
                                      "' (slot ", BindPoint, ") in shader '", GetShaderName(), "'");
                    continue;
                }

                if (CommittedD3D11SRVs[BindPoint] != d3d11SRVs[BindPoint])
                {
                    LOG_ERROR_MESSAGE("D3D11 resource committed to buffer SRV '", DbgMakeResourceName(buf, BindPoint),
                                      "' (slot ", BindPoint, ") in shader '", GetShaderName(),
                                      "' does not match the resource in the resource cache");
                    continue;
                }
            }
        },

        [&](const D3DShaderResourceAttribs& uav, Uint32) //
        {
            for (auto BindPoint = uav.BindPoint; BindPoint < uav.BindPoint + uav.BindCount; ++BindPoint)
            {
                if (BindPoint >= ResourceCache.GetUAVCount())
                {
                    LOG_ERROR_MESSAGE("Unable to find buffer UAV '", DbgMakeResourceName(uav, BindPoint), "' (slot ", BindPoint,
                                      ") in the resource cache: the cache reserves ", ResourceCache.GetUAVCount(),
                                      " UAV slots only. This should never happen and may be the result of using wrong resource cache.");
                    continue;
                }
                auto& UAVRes = CachedUAVResources[BindPoint];
                if (UAVRes.pTexture != nullptr)
                {
                    LOG_ERROR_MESSAGE("Unexpected texture bound to variable '", DbgMakeResourceName(uav, BindPoint),
                                      "' (slot ", BindPoint, "). Buffer is expected.");
                    return;
                }
                if (UAVRes.pBuffer == nullptr)
                {
                    LOG_ERROR_MESSAGE("Buffer UAV '", DbgMakeResourceName(uav, BindPoint), "' (slot ", BindPoint,
                                      ") is not initialized in the resource cache.");
                    return;
                }

                if (!(UAVRes.pBuffer->GetDesc().BindFlags & BIND_UNORDERED_ACCESS))
                {
                    LOG_ERROR_MESSAGE("Buffer '", UAVRes.pBuffer->GetDesc().Name,
                                      "' committed in the device context as UAV to variable '", DbgMakeResourceName(uav, BindPoint), "' (slot ", BindPoint, ") in shader '", GetShaderName(), "' was not created with BIND_UNORDERED_ACCESS flag");
                }

                if (CommittedD3D11UAVs[BindPoint] == nullptr)
                {
                    LOG_ERROR_MESSAGE("No D3D11 resource committed to buffer UAV '", DbgMakeResourceName(uav, BindPoint),
                                      "' (slot ", BindPoint, ") in shader '", GetShaderName(), "'");
                    return;
                }

                if (CommittedD3D11UAVs[BindPoint] != d3d11UAVs[BindPoint])
                {
                    LOG_ERROR_MESSAGE("D3D11 resource committed to buffer UAV '", DbgMakeResourceName(uav, BindPoint),
                                      "' (slot ", BindPoint, ") in shader '", GetShaderName(),
                                      "' does not match the resource in the resource cache");
                    return;
                }
            }
        } // clang-format off
    ); // clang-format on
}
#endif

} // namespace Diligent
