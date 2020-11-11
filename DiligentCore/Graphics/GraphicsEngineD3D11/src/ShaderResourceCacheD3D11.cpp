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

#include "ShaderResourceCacheD3D11.hpp"
#include "ShaderResourceLayoutD3D11.hpp"
#include "TextureBaseD3D11.hpp"
#include "BufferD3D11Impl.hpp"
#include "SamplerD3D11Impl.hpp"
#include "MemoryAllocator.h"

namespace Diligent
{
size_t ShaderResourceCacheD3D11::GetRequriedMemorySize(const ShaderResourcesD3D11& Resources)
{
    // clang-format off
    auto CBCount      = Resources.GetMaxCBBindPoint()     + 1;
    auto SRVCount     = Resources.GetMaxSRVBindPoint()    + 1;
    auto SamplerCount = Resources.GetMaxSamplerBindPoint()+ 1;
    auto UAVCount     = Resources.GetMaxUAVBindPoint()    + 1;
    auto MemSize = 
                (sizeof(CachedCB)       + sizeof(ID3D11Buffer*))              * CBCount + 
                (sizeof(CachedResource) + sizeof(ID3D11ShaderResourceView*))  * SRVCount + 
                (sizeof(CachedSampler)  + sizeof(ID3D11SamplerState*))        * SamplerCount + 
                (sizeof(CachedResource) + sizeof(ID3D11UnorderedAccessView*)) * UAVCount;
    // clang-format on
    return MemSize;
}

void ShaderResourceCacheD3D11::Initialize(const ShaderResourcesD3D11& Resources, IMemoryAllocator& MemAllocator)
{
    // clang-format off
    auto CBCount      = Resources.GetMaxCBBindPoint()     + 1;
    auto SRVCount     = Resources.GetMaxSRVBindPoint()    + 1;
    auto SamplerCount = Resources.GetMaxSamplerBindPoint()+ 1;
    auto UAVCount     = Resources.GetMaxUAVBindPoint()    + 1;
    // clang-format on
    Initialize(CBCount, SRVCount, SamplerCount, UAVCount, MemAllocator);
}

void ShaderResourceCacheD3D11::Initialize(Uint32 CBCount, Uint32 SRVCount, Uint32 SamplerCount, Uint32 UAVCount, IMemoryAllocator& MemAllocator)
{
    // http://diligentgraphics.com/diligent-engine/architecture/d3d11/shader-resource-cache/
    if (IsInitialized())
    {
        LOG_ERROR_MESSAGE("Resource cache is already intialized");
        return;
    }

    // clang-format off
    VERIFY(CBCount      <= D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, "Constant buffer count ", CBCount,      " exceeds D3D11 limit ", D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT );
    VERIFY(SRVCount     <= D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT,      "SRV count ",             SRVCount,     " exceeds D3D11 limit ", D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT );
    VERIFY(SamplerCount <= D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT,             "Sampler count ",         SamplerCount, " exceeds D3D11 limit ", D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT );
    VERIFY(UAVCount     <= D3D11_PS_CS_UAV_REGISTER_COUNT,                    "UAV count ",             UAVCount,     " exceeds D3D11 limit ", D3D11_PS_CS_UAV_REGISTER_COUNT );

    // m_CBOffset  = 0
    m_SRVOffset       = static_cast<Uint16>(m_CBOffset      + CBCount      * (sizeof(CachedCB)       + sizeof(ID3D11Buffer*)));
    m_SamplerOffset   = static_cast<Uint16>(m_SRVOffset     + SRVCount     * (sizeof(CachedResource) + sizeof(ID3D11ShaderResourceView*)));
    m_UAVOffset       = static_cast<Uint16>(m_SamplerOffset + SamplerCount * (sizeof(CachedSampler)  + sizeof(ID3D11SamplerState*)));
    m_MemoryEndOffset = static_cast<Uint16>(m_UAVOffset     + UAVCount     * (sizeof(CachedResource) + sizeof(ID3D11UnorderedAccessView*)));

    VERIFY_EXPR(GetCBCount()      == static_cast<Uint32>(CBCount));
    VERIFY_EXPR(GetSRVCount()     == static_cast<Uint32>(SRVCount));
    VERIFY_EXPR(GetSamplerCount() == static_cast<Uint32>(SamplerCount));
    VERIFY_EXPR(GetUAVCount()     == static_cast<Uint32>(UAVCount));

    VERIFY_EXPR(m_pResourceData == nullptr);
    size_t BufferSize =  m_MemoryEndOffset;

    VERIFY_EXPR( BufferSize ==
                (sizeof(CachedCB)       + sizeof(ID3D11Buffer*))              * CBCount + 
                (sizeof(CachedResource) + sizeof(ID3D11ShaderResourceView*))  * SRVCount + 
                (sizeof(CachedSampler)  + sizeof(ID3D11SamplerState*))        * SamplerCount + 
                (sizeof(CachedResource) + sizeof(ID3D11UnorderedAccessView*)) * UAVCount );
    // clang-format on

#ifdef DILIGENT_DEBUG
    m_pdbgMemoryAllocator = &MemAllocator;
#endif
    if (BufferSize > 0)
    {
        m_pResourceData = ALLOCATE(MemAllocator, "Shader resource cache data buffer", Uint8, BufferSize);
        memset(m_pResourceData, 0, BufferSize);
    }

    // Explicitly construct all objects
    if (CBCount != 0)
    {
        CachedCB*      CBs      = nullptr;
        ID3D11Buffer** d3d11CBs = nullptr;
        GetCBArrays(CBs, d3d11CBs);
        for (Uint32 cb = 0; cb < CBCount; ++cb)
            new (CBs + cb) CachedCB;
    }

    if (SRVCount != 0)
    {
        CachedResource*            SRVResources = nullptr;
        ID3D11ShaderResourceView** d3d11SRVs    = nullptr;
        GetSRVArrays(SRVResources, d3d11SRVs);
        for (Uint32 srv = 0; srv < SRVCount; ++srv)
            new (SRVResources + srv) CachedResource;
    }

    if (SamplerCount != 0)
    {
        CachedSampler*       Samplers      = nullptr;
        ID3D11SamplerState** d3d11Samplers = nullptr;
        GetSamplerArrays(Samplers, d3d11Samplers);
        for (Uint32 sam = 0; sam < SamplerCount; ++sam)
            new (Samplers + sam) CachedSampler;
    }

    if (UAVCount != 0)
    {
        CachedResource*             UAVResources = nullptr;
        ID3D11UnorderedAccessView** d3d11UAVs    = nullptr;
        GetUAVArrays(UAVResources, d3d11UAVs);
        for (Uint32 uav = 0; uav < UAVCount; ++uav)
            new (UAVResources + uav) CachedResource;
    }
}

void ShaderResourceCacheD3D11::Destroy(IMemoryAllocator& MemAllocator)
{
    if (!IsInitialized())
        return;

    VERIFY(m_pdbgMemoryAllocator == &MemAllocator, "The allocator does not match the one used to create resources");

    // Explicitly destory all objects
    auto CBCount = GetCBCount();
    if (CBCount != 0)
    {
        CachedCB*      CBs      = nullptr;
        ID3D11Buffer** d3d11CBs = nullptr;
        GetCBArrays(CBs, d3d11CBs);
        for (size_t cb = 0; cb < CBCount; ++cb)
            CBs[cb].~CachedCB();
    }

    auto SRVCount = GetSRVCount();
    if (SRVCount != 0)
    {
        CachedResource*            SRVResources = nullptr;
        ID3D11ShaderResourceView** d3d11SRVs    = nullptr;
        GetSRVArrays(SRVResources, d3d11SRVs);
        for (size_t srv = 0; srv < SRVCount; ++srv)
            SRVResources[srv].~CachedResource();
    }

    auto SamplerCount = GetSamplerCount();
    if (SamplerCount != 0)
    {
        CachedSampler*       Samplers      = nullptr;
        ID3D11SamplerState** d3d11Samplers = nullptr;
        GetSamplerArrays(Samplers, d3d11Samplers);
        for (size_t sam = 0; sam < SamplerCount; ++sam)
            Samplers[sam].~CachedSampler();
    }

    auto UAVCount = GetUAVCount();
    if (UAVCount != 0)
    {
        CachedResource*             UAVResources = nullptr;
        ID3D11UnorderedAccessView** d3d11UAVs    = nullptr;
        GetUAVArrays(UAVResources, d3d11UAVs);
        for (size_t uav = 0; uav < UAVCount; ++uav)
            UAVResources[uav].~CachedResource();
    }

    m_SRVOffset       = InvalidResourceOffset;
    m_SamplerOffset   = InvalidResourceOffset;
    m_UAVOffset       = InvalidResourceOffset;
    m_MemoryEndOffset = InvalidResourceOffset;

    if (m_pResourceData != nullptr)
        MemAllocator.Free(m_pResourceData);
    m_pResourceData = nullptr;
}

ShaderResourceCacheD3D11::~ShaderResourceCacheD3D11()
{
    VERIFY(!IsInitialized(), "Shader resource cache memory must be released with ShaderResourceCacheD3D11::Destroy()");
}

void dbgVerifyResource(ShaderResourceCacheD3D11::CachedResource& Res, ID3D11View* pd3d11View, const char* ViewType)
{
    if (pd3d11View != nullptr)
    {
        VERIFY(Res.pView != nullptr, "Resource view is not initialized");
        VERIFY(Res.pBuffer == nullptr && Res.pTexture != nullptr || Res.pBuffer != nullptr && Res.pTexture == nullptr,
               "Texture and buffer resources are mutually exclusive");
        VERIFY(Res.pd3d11Resource != nullptr, "D3D11 resource is missing");

        CComPtr<ID3D11Resource> pd3d11ActualResource;
        pd3d11View->GetResource(&pd3d11ActualResource);
        VERIFY(pd3d11ActualResource == Res.pd3d11Resource, "Inconsistent D3D11 resource");
        if (Res.pBuffer)
        {
            VERIFY(pd3d11ActualResource == Res.pBuffer->GetD3D11Buffer(), "Inconsistent buffer ", ViewType);
            if (Res.pView)
            {
                RefCntAutoPtr<IBufferViewD3D11> pBufView(Res.pView, IID_BufferViewD3D11);
                VERIFY(pBufView != nullptr, "Provided resource view is not D3D11 buffer view");
                if (pBufView)
                    VERIFY(pBufView->GetBuffer() == Res.pBuffer, "Provided resource view is not a view of the buffer");
            }
        }
        else if (Res.pTexture)
        {
            VERIFY(pd3d11ActualResource == Res.pTexture->GetD3D11Texture(), "Inconsistent texture ", ViewType);
            if (Res.pView)
            {
                RefCntAutoPtr<ITextureViewD3D11> pTexView(Res.pView, IID_TextureViewD3D11);
                VERIFY(pTexView != nullptr, "Provided resource view is not D3D11 texture view");
                if (pTexView)
                    VERIFY(pTexView->GetTexture() == Res.pTexture, "Provided resource view is not a view of the texture");
            }
        }
    }
    else
    {
        VERIFY(Res.pView == nullptr, "Resource view is unexpected");
        VERIFY(Res.pBuffer == nullptr && Res.pTexture == nullptr, "Niether texture nor buffer resource is expected");
        VERIFY(Res.pd3d11Resource == nullptr, "Unexepected D3D11 resource");
    }
}

void ShaderResourceCacheD3D11::dbgVerifyCacheConsistency()
{
    VERIFY(IsInitialized(), "Cache is not initialized");

    CachedCB*                   CBs           = nullptr;
    ID3D11Buffer**              d3d11CBs      = nullptr;
    CachedResource*             SRVResources  = nullptr;
    ID3D11ShaderResourceView**  d3d11SRVs     = nullptr;
    CachedSampler*              Samplers      = nullptr;
    ID3D11SamplerState**        d3d11Samplers = nullptr;
    CachedResource*             UAVResources  = nullptr;
    ID3D11UnorderedAccessView** d3d11UAVs     = nullptr;

    GetCBArrays(CBs, d3d11CBs);
    GetSRVArrays(SRVResources, d3d11SRVs);
    GetSamplerArrays(Samplers, d3d11Samplers);
    GetUAVArrays(UAVResources, d3d11UAVs);

    auto CBCount = GetCBCount();
    for (size_t cb = 0; cb < CBCount; ++cb)
    {
        auto& pBuff      = CBs[cb].pBuff;
        auto* pd3d11Buff = d3d11CBs[cb];
        VERIFY(pBuff == nullptr && pd3d11Buff == nullptr || pBuff != nullptr && pd3d11Buff != nullptr, "CB resource and d3d11 buffer must be set/unset atomically");
        if (pBuff != nullptr && pd3d11Buff != nullptr)
        {
            VERIFY(pd3d11Buff == pBuff->GetD3D11Buffer(), "Inconsistent D3D11 buffer");
        }
    }

    auto SRVCount = GetSRVCount();
    for (size_t srv = 0; srv < SRVCount; ++srv)
    {
        auto& Res       = SRVResources[srv];
        auto* pd3d11SRV = d3d11SRVs[srv];
        dbgVerifyResource(Res, pd3d11SRV, "SRV");
    }

    auto UAVCount = GetUAVCount();
    for (size_t uav = 0; uav < UAVCount; ++uav)
    {
        auto& Res       = UAVResources[uav];
        auto* pd3d11UAV = d3d11UAVs[uav];
        dbgVerifyResource(Res, pd3d11UAV, "UAV");
    }

    auto SamplerCount = GetSamplerCount();
    for (size_t sam = 0; sam < SamplerCount; ++sam)
    {
        auto& pSampler      = Samplers[sam].pSampler;
        auto* pd3d11Sampler = d3d11Samplers[sam];
        VERIFY(pSampler == nullptr && pd3d11Sampler == nullptr || pSampler != nullptr && pd3d11Sampler != nullptr, "CB resource and d3d11 buffer must be set/unset atomically");
        if (pSampler != nullptr && pd3d11Sampler != nullptr)
        {
            VERIFY(pd3d11Sampler == pSampler->GetD3D11SamplerState(), "Inconsistent D3D11 sampler");
        }
    }
}
} // namespace Diligent
