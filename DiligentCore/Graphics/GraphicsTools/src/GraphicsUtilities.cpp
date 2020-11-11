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
#include <algorithm>
#include <cmath>

#include "GraphicsUtilities.h"
#include "DebugUtilities.hpp"
#include "GraphicsAccessories.hpp"
#include "ColorConversion.h"

#define PI_F 3.1415926f

namespace Diligent
{

void CreateUniformBuffer(IRenderDevice*   pDevice,
                         Uint32           Size,
                         const Char*      Name,
                         IBuffer**        ppBuffer,
                         USAGE            Usage,
                         BIND_FLAGS       BindFlags,
                         CPU_ACCESS_FLAGS CPUAccessFlags,
                         void*            pInitialData)
{
    BufferDesc CBDesc;
    CBDesc.Name           = Name;
    CBDesc.uiSizeInBytes  = Size;
    CBDesc.Usage          = Usage;
    CBDesc.BindFlags      = BindFlags;
    CBDesc.CPUAccessFlags = CPUAccessFlags;

    BufferData InitialData;
    if (pInitialData != nullptr)
    {
        InitialData.pData    = pInitialData;
        InitialData.DataSize = Size;
    }
    pDevice->CreateBuffer(CBDesc, pInitialData != nullptr ? &InitialData : nullptr, ppBuffer);
}

template <class TConverter>
void GenerateCheckerBoardPatternInternal(Uint32 Width, Uint32 Height, TEXTURE_FORMAT Fmt, Uint32 HorzCells, Uint32 VertCells, Uint8* pData, Uint32 StrideInBytes, TConverter Converter)
{
    const auto& FmtAttribs = GetTextureFormatAttribs(Fmt);
    for (Uint32 y = 0; y < Height; ++y)
    {
        for (Uint32 x = 0; x < Width; ++x)
        {
            float horzWave   = sin((static_cast<float>(x) + 0.5f) / static_cast<float>(Width) * PI_F * static_cast<float>(HorzCells));
            float vertWave   = sin((static_cast<float>(y) + 0.5f) / static_cast<float>(Height) * PI_F * static_cast<float>(VertCells));
            float val        = horzWave * vertWave;
            val              = std::max(std::min(val * 20.f, +1.f), -1.f);
            val              = val * 0.5f + 1.f;
            val              = val * 0.5f + 0.25f;
            Uint8* pDstTexel = pData + x * Uint32{FmtAttribs.NumComponents} * Uint32{FmtAttribs.ComponentSize} + y * StrideInBytes;
            Converter(pDstTexel, Uint32{FmtAttribs.NumComponents}, val);
        }
    }
}

void GenerateCheckerBoardPattern(Uint32 Width, Uint32 Height, TEXTURE_FORMAT Fmt, Uint32 HorzCells, Uint32 VertCells, Uint8* pData, Uint32 StrideInBytes)
{
    const auto& FmtAttribs = GetTextureFormatAttribs(Fmt);
    switch (FmtAttribs.ComponentType)
    {
        case COMPONENT_TYPE_UINT:
        case COMPONENT_TYPE_UNORM:
            GenerateCheckerBoardPatternInternal(
                Width, Height, Fmt, HorzCells, VertCells, pData, StrideInBytes,
                [](Uint8* pDstTexel, Uint32 NumComponents, float fVal) //
                {
                    Uint8 uVal = static_cast<Uint8>(fVal * 255.f);
                    for (Uint32 c = 0; c < NumComponents; ++c)
                        pDstTexel[c] = uVal;
                } //
            );
            break;

        case COMPONENT_TYPE_UNORM_SRGB:
            GenerateCheckerBoardPatternInternal(
                Width, Height, Fmt, HorzCells, VertCells, pData, StrideInBytes,
                [](Uint8* pDstTexel, Uint32 NumComponents, float fVal) //
                {
                    Uint8 uVal = static_cast<Uint8>(FastLinearToSRGB(fVal) * 255.f);
                    for (Uint32 c = 0; c < NumComponents; ++c)
                        pDstTexel[c] = uVal;
                } //
            );
            break;

        case COMPONENT_TYPE_FLOAT:
            GenerateCheckerBoardPatternInternal(
                Width, Height, Fmt, HorzCells, VertCells, pData, StrideInBytes,
                [](Uint8* pDstTexel, Uint32 NumComponents, float fVal) //
                {
                    for (Uint32 c = 0; c < NumComponents; ++c)
                        (reinterpret_cast<float*>(pDstTexel))[c] = fVal;
                } //
            );
            break;

        default:
            UNSUPPORTED("Unsupported component type");
            return;
    }
}

} // namespace Diligent


extern "C"
{
    void Diligent_CreateUniformBuffer(Diligent::IRenderDevice*   pDevice,
                                      Diligent::Uint32           Size,
                                      const Diligent::Char*      Name,
                                      Diligent::IBuffer**        ppBuffer,
                                      Diligent::USAGE            Usage,
                                      Diligent::BIND_FLAGS       BindFlags,
                                      Diligent::CPU_ACCESS_FLAGS CPUAccessFlags,
                                      void*                      pInitialData)
    {
        Diligent::CreateUniformBuffer(pDevice, Size, Name, ppBuffer, Usage, BindFlags, CPUAccessFlags, pInitialData);
    }

    void Diligent_GenerateCheckerBoardPattern(Diligent::Uint32         Width,
                                              Diligent::Uint32         Height,
                                              Diligent::TEXTURE_FORMAT Fmt,
                                              Diligent::Uint32         HorzCells,
                                              Diligent::Uint32         VertCells,
                                              Diligent::Uint8*         pData,
                                              Diligent::Uint32         StrideInBytes)
    {
        Diligent::GenerateCheckerBoardPattern(Width, Height, Fmt, HorzCells, VertCells, pData, StrideInBytes);
    }
}
