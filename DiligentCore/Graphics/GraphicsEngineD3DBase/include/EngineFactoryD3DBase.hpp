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

#include "DXGITypeConversions.hpp"
#include "EngineFactoryBase.hpp"

/// \file
/// Implementation of the Diligent::EngineFactoryD3DBase template class

namespace Diligent
{

template <typename BaseInterface, RENDER_DEVICE_TYPE DevType>
class EngineFactoryD3DBase : public EngineFactoryBase<BaseInterface>
{
public:
    using TEngineFactoryBase = EngineFactoryBase<BaseInterface>;

    EngineFactoryD3DBase(const INTERFACE_ID& FactoryIID) :
        TEngineFactoryBase{FactoryIID}
    {}


    virtual void DILIGENT_CALL_TYPE EnumerateAdapters(DIRECT3D_FEATURE_LEVEL MinFeatureLevel,
                                                      Uint32&                NumAdapters,
                                                      GraphicsAdapterInfo*   Adapters) override
    {
        auto DXGIAdapters = FindCompatibleAdapters(MinFeatureLevel);

        if (Adapters == nullptr)
            NumAdapters = static_cast<Uint32>(DXGIAdapters.size());
        else
        {
            NumAdapters = std::min(NumAdapters, static_cast<Uint32>(DXGIAdapters.size()));
            for (Uint32 adapter = 0; adapter < NumAdapters; ++adapter)
            {
                IDXGIAdapter1*     pDXIAdapter = DXGIAdapters[adapter];
                DXGI_ADAPTER_DESC1 AdapterDesc;
                pDXIAdapter->GetDesc1(&AdapterDesc);

                auto& Attribs = Adapters[adapter];

                Attribs = DXGI_ADAPTER_DESC_To_GraphicsAdapterInfo(AdapterDesc);

                Attribs.NumOutputs = 0;
                CComPtr<IDXGIOutput> pOutput;
                while (pDXIAdapter->EnumOutputs(Attribs.NumOutputs, &pOutput) != DXGI_ERROR_NOT_FOUND)
                {
                    ++Attribs.NumOutputs;
                    pOutput.Release();
                };
            }
        }
    }


    virtual void DILIGENT_CALL_TYPE EnumerateDisplayModes(DIRECT3D_FEATURE_LEVEL MinFeatureLevel,
                                                          Uint32                 AdapterId,
                                                          Uint32                 OutputId,
                                                          TEXTURE_FORMAT         Format,
                                                          Uint32&                NumDisplayModes,
                                                          DisplayModeAttribs*    DisplayModes) override
    {
        auto DXGIAdapters = FindCompatibleAdapters(MinFeatureLevel);
        if (AdapterId >= DXGIAdapters.size())
        {
            LOG_ERROR("Incorrect adapter id ", AdapterId);
            return;
        }

        IDXGIAdapter1* pDXIAdapter = DXGIAdapters[AdapterId];

        DXGI_FORMAT          DXIGFormat = TexFormatToDXGI_Format(Format);
        CComPtr<IDXGIOutput> pOutput;
        if (pDXIAdapter->EnumOutputs(OutputId, &pOutput) == DXGI_ERROR_NOT_FOUND)
        {
            DXGI_ADAPTER_DESC1 AdapterDesc;
            pDXIAdapter->GetDesc1(&AdapterDesc);
            char DescriptionMB[_countof(AdapterDesc.Description)];
            WideCharToMultiByte(CP_ACP, 0, AdapterDesc.Description, -1, DescriptionMB, _countof(DescriptionMB), NULL, FALSE);
            LOG_ERROR_MESSAGE("Failed to enumerate output ", OutputId, " of adapter ", AdapterId, " (", DescriptionMB, ')');
            return;
        }

        UINT numModes = 0;
        // Get the number of elements
        auto hr = pOutput->GetDisplayModeList(DXIGFormat, 0, &numModes, NULL);
        if (DisplayModes != nullptr)
        {
            // Get the list
            std::vector<DXGI_MODE_DESC> DXIDisplayModes(numModes);
            hr = pOutput->GetDisplayModeList(DXIGFormat, 0, &numModes, DXIDisplayModes.data());
            for (Uint32 m = 0; m < std::min(NumDisplayModes, numModes); ++m)
            {
                const auto& SrcMode            = DXIDisplayModes[m];
                auto&       DstMode            = DisplayModes[m];
                DstMode.Width                  = SrcMode.Width;
                DstMode.Height                 = SrcMode.Height;
                DstMode.Format                 = DXGI_FormatToTexFormat(SrcMode.Format);
                DstMode.RefreshRateNumerator   = SrcMode.RefreshRate.Numerator;
                DstMode.RefreshRateDenominator = SrcMode.RefreshRate.Denominator;
                DstMode.Scaling                = static_cast<SCALING_MODE>(SrcMode.Scaling);
                DstMode.ScanlineOrder          = static_cast<SCANLINE_ORDER>(SrcMode.ScanlineOrdering);
            }
            NumDisplayModes = std::min(NumDisplayModes, numModes);
        }
        else
        {
            NumDisplayModes = numModes;
        }
    }


    std::vector<CComPtr<IDXGIAdapter1>> FindCompatibleAdapters(DIRECT3D_FEATURE_LEVEL MinFeatureLevel)
    {
        std::vector<CComPtr<IDXGIAdapter1>> DXGIAdapters;

        CComPtr<IDXGIFactory2> pFactory;
        if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory2), (void**)&pFactory)))
        {
            LOG_ERROR_MESSAGE("Failed to create DXGI Factory");
            return std::move(DXGIAdapters);
        }

        CComPtr<IDXGIAdapter1> pDXIAdapter;

        auto d3dFeatureLevel = GetD3DFeatureLevel(MinFeatureLevel);
        UINT adapter         = 0;
        for (; pFactory->EnumAdapters1(adapter, &pDXIAdapter) != DXGI_ERROR_NOT_FOUND; ++adapter, pDXIAdapter.Release())
        {
            DXGI_ADAPTER_DESC1 AdapterDesc;
            pDXIAdapter->GetDesc1(&AdapterDesc);
            bool IsCompatibleAdapter = CheckAdapterCompatibility<DevType>(pDXIAdapter, d3dFeatureLevel);
            if (IsCompatibleAdapter)
            {
                DXGIAdapters.emplace_back(std::move(pDXIAdapter));
            }
        }

        return std::move(DXGIAdapters);
    }


protected:
    static D3D_FEATURE_LEVEL GetD3DFeatureLevel(DIRECT3D_FEATURE_LEVEL FeatureLevel)
    {
        switch (FeatureLevel)
        {
            case DIRECT3D_FEATURE_LEVEL_10_0: return D3D_FEATURE_LEVEL_10_0;
            case DIRECT3D_FEATURE_LEVEL_10_1: return D3D_FEATURE_LEVEL_10_1;
            case DIRECT3D_FEATURE_LEVEL_11_0: return D3D_FEATURE_LEVEL_11_0;
            case DIRECT3D_FEATURE_LEVEL_11_1: return D3D_FEATURE_LEVEL_11_1;
#if defined(_WIN32_WINNT_WIN10) && (_WIN32_WINNT >= _WIN32_WINNT_WIN10)
            case DIRECT3D_FEATURE_LEVEL_12_0: return D3D_FEATURE_LEVEL_12_0;
            case DIRECT3D_FEATURE_LEVEL_12_1: return D3D_FEATURE_LEVEL_12_1;
#endif

            default:
                UNEXPECTED("Unknown DIRECT3D_FEATURE_LEVEL ", static_cast<Uint32>(FeatureLevel));
                return D3D_FEATURE_LEVEL_11_0;
        }
    }

private:
    template <RENDER_DEVICE_TYPE DevType>
    bool CheckAdapterCompatibility(IDXGIAdapter1*    pDXGIAdapter,
                                   D3D_FEATURE_LEVEL FeatureLevels);

    template <>
    bool CheckAdapterCompatibility<RENDER_DEVICE_TYPE_D3D11>(IDXGIAdapter1*    pDXGIAdapter,
                                                             D3D_FEATURE_LEVEL FeatureLevel)
    {
        auto hr = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_NULL, // There is no need to create a real hardware device.
            0,
            0,                 // Flags.
            &FeatureLevel,     // Feature levels.
            1,                 // Number of feature levels
            D3D11_SDK_VERSION, // Always set this to D3D11_SDK_VERSION for Windows Store apps.
            nullptr,           // No need to keep the D3D device reference.
            nullptr,           // Feature level of the created adapter.
            nullptr            // No need to keep the D3D device context reference.
        );
        return SUCCEEDED(hr);
    }

    template <>
    bool CheckAdapterCompatibility<RENDER_DEVICE_TYPE_D3D12>(IDXGIAdapter1*    pDXGIAdapter,
                                                             D3D_FEATURE_LEVEL FeatureLevel)
    {
        auto hr = D3D12CreateDevice(pDXGIAdapter, FeatureLevel, _uuidof(ID3D12Device), nullptr);
        return SUCCEEDED(hr);
    }
};

} // namespace Diligent
