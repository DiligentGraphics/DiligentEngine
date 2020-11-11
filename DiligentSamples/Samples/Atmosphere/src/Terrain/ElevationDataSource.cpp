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

// This file is derived from the open source project provided by Intel Corportaion that
// requires the following notice to be kept:
//--------------------------------------------------------------------------------------
// Copyright 2013 Intel Corporation
// All Rights Reserved
//
// Permission is granted to use, copy, distribute and prepare derivative works of this
// software for any purpose and without fee, provided, that the above copyright notice
// and this statement appear in all copies.  Intel makes no representations about the
// suitability of this software for any purpose.  THIS SOFTWARE IS PROVIDED "AS IS."
// INTEL SPECIFICALLY DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, AND ALL LIABILITY,
// INCLUDING CONSEQUENTIAL AND OTHER INDIRECT DAMAGES, FOR THE USE OF THIS SOFTWARE,
// INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PROPRIETARY RIGHTS, AND INCLUDING THE
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  Intel does not
// assume any responsibility for any errors which may appear in this software nor any
// responsibility to update it.
//--------------------------------------------------------------------------------------

#include <algorithm>
#include <cmath>

#include "ElevationDataSource.hpp"
#include "FileWrapper.hpp"
#include "DataBlobImpl.hpp"
#include "Image.h"
#include "BasicFileStream.hpp"
#include "TextureUtilities.h"
#include "GraphicsAccessories.hpp"

namespace Diligent
{

// Creates data source from the specified raw data file
ElevationDataSource::ElevationDataSource(const Char* strSrcDemFile) :
    m_iNumLevels(0),
    m_iPatchSize(128),
    m_iColOffset(0),
    m_iRowOffset(0)
{

#if 1
    RefCntAutoPtr<Image> pHeightMap;
    CreateImageFromFile(strSrcDemFile, &pHeightMap);

    const auto& ImgInfo    = pHeightMap->GetDesc();
    auto*       pImageData = pHeightMap->GetData();
    // Calculate minimal number of columns and rows
    // in the form 2^n+1 that encompass the data
    m_iNumCols = 1;
    m_iNumRows = 1;
    while (m_iNumCols + 1 < ImgInfo.Width || m_iNumRows + 1 < ImgInfo.Height)
    {
        m_iNumCols *= 2;
        m_iNumRows *= 2;
    }

    m_iNumLevels = 1;
    while ((m_iPatchSize << (m_iNumLevels - 1)) < (int)m_iNumCols ||
           (m_iPatchSize << (m_iNumLevels - 1)) < (int)m_iNumRows)
        m_iNumLevels++;

    m_iNumCols++;
    m_iNumRows++;
    m_iStride = (m_iNumCols + 1) & (-2);

    // Load the data
    m_TheHeightMap.resize(size_t{m_iStride} * size_t{m_iNumRows});

    VERIFY(ImgInfo.ComponentType == VT_UINT16 && ImgInfo.NumComponents == 1, "Unexpected scanline size: 16-bit single-channel image is expected");
    auto* pSrcImgData = reinterpret_cast<Uint8*>(pImageData->GetDataPtr());
    for (Uint32 row = 0; row < ImgInfo.Height; row++, pSrcImgData += ImgInfo.RowStride)
    {
        memcpy(&m_TheHeightMap[row * m_iStride], pSrcImgData, size_t{ImgInfo.Width} * size_t{GetValueSize(ImgInfo.ComponentType)});
    }

    // Duplicate the last row and column
    for (Uint32 iRow = 0; iRow < ImgInfo.Height; iRow++)
        for (Uint32 iCol = ImgInfo.Width; iCol < m_iNumCols; iCol++)
            GetElevSample(iCol, iRow) = GetElevSample((ImgInfo.Width - 1), iRow);

    for (Uint32 iCol = 0; iCol < m_iNumCols; iCol++)
        for (Uint32 iRow = ImgInfo.Height; iRow < m_iNumRows; iRow++)
            GetElevSample(iCol, iRow) = GetElevSample(iCol, ImgInfo.Height - 1);

#else
    m_iStride  = 2048;
    m_iNumRows = m_iNumCols = 2048;
    m_iNumLevels            = 5;
    m_TheHeightMap.resize(m_iStride * m_iNumRows);
    for (Uint32 j = 0; j < m_iNumRows; ++j)
    {
        for (Uint32 i = 0; i < m_iNumCols; ++i)
        {
            float x = (float)i / (float)m_iNumRows;
            float y = (float)j / (float)m_iNumCols;
            float A = 1;
            float F = 1;
            float h = 0;
            for (int Octave = 0; Octave < 8; ++Octave, A *= 0.7f, F *= 1.8f)
            {
                h += A * (sin(x * F) * sin(y * 1.5f * F) + 0.5f * (sin((x + y) * 1.3f * F) + cos((x * y) * 1.7f * F)));
                h = fabs(h - 0.5f);
            }
            h = fabs(h) * 32000.f;
            h = std::min(h, (float)std::numeric_limits<Uint16>::max());

            GetElevSample(i, j) = (Uint16)h;
        }
    }
#endif
    m_MinMaxElevation.Resize(m_iNumLevels);

    // Calcualte min/max elevations
    CalculateMinMaxElevations();
}

ElevationDataSource::~ElevationDataSource(void)
{
}

Uint16 ElevationDataSource::GetGlobalMinElevation() const
{
    return m_MinMaxElevation[QuadTreeNodeLocation()].first;
}

Uint16 ElevationDataSource::GetGlobalMaxElevation() const
{
    return m_MinMaxElevation[QuadTreeNodeLocation()].second;
}

int MirrorCoord(int iCoord, int iDim)
{
    iCoord      = std::abs(iCoord);
    int iPeriod = iCoord / iDim;
    iCoord      = iCoord % iDim;
    if (iPeriod & 0x01)
    {
        iCoord = (iDim - 1) - iCoord;
    }
    return iCoord;
}

inline Uint16& ElevationDataSource::GetElevSample(Int32 i, Int32 j)
{
    return m_TheHeightMap[i + j * m_iStride];
}

inline Uint16 ElevationDataSource::GetElevSample(Int32 i, Int32 j) const
{
    return m_TheHeightMap[i + j * m_iStride];
}

float ElevationDataSource::GetInterpolatedHeight(float fCol, float fRow, int iStep) const
{
    float fCol0    = floor(fCol);
    float fRow0    = floor(fRow);
    int   iCol0    = static_cast<int>(fCol0);
    int   iRow0    = static_cast<int>(fRow0);
    iCol0          = (iCol0 / iStep) * iStep;
    iRow0          = (iRow0 / iStep) * iStep;
    float fHWeight = (fCol - (float)iCol0) / (float)iStep;
    float fVWeight = (fRow - (float)iRow0) / (float)iStep;
    iCol0 += m_iColOffset;
    iRow0 += m_iRowOffset;
    //if( iCol0 < 0 || iCol0 >= (int)m_iNumCols || iRow0 < 0 || iRow0 >= (int)m_iNumRows )
    //    return -FLT_MAX;

    int iCol1 = iCol0 + iStep; //std::min(iCol0+iStep, (int)m_iNumCols-1);
    int iRow1 = iRow0 + iStep; //std::min(iRow0+iStep, (int)m_iNumRows-1);

    iCol0 = MirrorCoord(iCol0, m_iNumCols);
    iCol1 = MirrorCoord(iCol1, m_iNumCols);
    iRow0 = MirrorCoord(iRow0, m_iNumRows);
    iRow1 = MirrorCoord(iRow1, m_iNumRows);

    Uint16 H00 = GetElevSample(iCol0, iRow0);
    Uint16 H10 = GetElevSample(iCol1, iRow0);
    Uint16 H01 = GetElevSample(iCol0, iRow1);
    Uint16 H11 = GetElevSample(iCol1, iRow1);

    float fInterpolatedHeight = (H00 * (1 - fHWeight) + H10 * fHWeight) * (1 - fVWeight) +
        (H01 * (1 - fHWeight) + H11 * fHWeight) * fVWeight;
    return fInterpolatedHeight;
}

float3 ElevationDataSource::ComputeSurfaceNormal(float fCol, float fRow, float fSampleSpacing, float fHeightScale, int iStep) const
{
    float Height1 = GetInterpolatedHeight(fCol + (float)iStep, fRow, iStep);
    float Height2 = GetInterpolatedHeight(fCol - (float)iStep, fRow, iStep);
    float Height3 = GetInterpolatedHeight(fCol, fRow + (float)iStep, iStep);
    float Height4 = GetInterpolatedHeight(fCol, fRow - (float)iStep, iStep);

    float3 Grad;
    Grad.x = Height2 - Height1;
    Grad.y = Height4 - Height3;
    Grad.z = (float)iStep * fSampleSpacing * 2.f;

    Grad.x *= fHeightScale;
    Grad.y *= fHeightScale;
    float3 Normal = normalize(Grad);

    return Normal;
}

void ElevationDataSource::RecomputePatchMinMaxElevations(const QuadTreeNodeLocation& pos)
{
    if (pos.level == m_iNumLevels - 1)
    {
        std::pair<Uint16, Uint16>& CurrPatchMinMaxElev = m_MinMaxElevation[QuadTreeNodeLocation(pos.horzOrder, pos.vertOrder, pos.level)];

        int iStartCol = pos.horzOrder * m_iPatchSize;
        int iStartRow = pos.vertOrder * m_iPatchSize;

        CurrPatchMinMaxElev.first = CurrPatchMinMaxElev.second = GetElevSample(iStartCol, iStartRow);
        for (int iRow = iStartRow; iRow <= iStartRow + m_iPatchSize; iRow++)
            for (int iCol = iStartCol; iCol <= iStartCol + m_iPatchSize; iCol++)
            {
                Uint16 CurrElev            = GetElevSample(std::min(iCol, (Int32)m_iNumCols - 1), std::min(iRow, (Int32)m_iNumRows - 1));
                CurrPatchMinMaxElev.first  = std::min(CurrPatchMinMaxElev.first, CurrElev);
                CurrPatchMinMaxElev.second = std::max(CurrPatchMinMaxElev.second, CurrElev);
            }
    }
    else
    {
        std::pair<Uint16, Uint16>& CurrPatchMinMaxElev = m_MinMaxElevation[pos];
        std::pair<Uint16, Uint16>& LBChildMinMaxElev   = m_MinMaxElevation[GetChildLocation(pos, 0)];
        std::pair<Uint16, Uint16>& RBChildMinMaxElev   = m_MinMaxElevation[GetChildLocation(pos, 1)];
        std::pair<Uint16, Uint16>& LTChildMinMaxElev   = m_MinMaxElevation[GetChildLocation(pos, 2)];
        std::pair<Uint16, Uint16>& RTChildMinMaxElev   = m_MinMaxElevation[GetChildLocation(pos, 3)];

        CurrPatchMinMaxElev.first = std::min(LBChildMinMaxElev.first, RBChildMinMaxElev.first);
        CurrPatchMinMaxElev.first = std::min(CurrPatchMinMaxElev.first, LTChildMinMaxElev.first);
        CurrPatchMinMaxElev.first = std::min(CurrPatchMinMaxElev.first, RTChildMinMaxElev.first);

        CurrPatchMinMaxElev.second = std::max(LBChildMinMaxElev.second, RBChildMinMaxElev.second);
        CurrPatchMinMaxElev.second = std::max(CurrPatchMinMaxElev.second, LTChildMinMaxElev.second);
        CurrPatchMinMaxElev.second = std::max(CurrPatchMinMaxElev.second, RTChildMinMaxElev.second);
    }
}

// Calculates min/max elevations for the hierarchy
void ElevationDataSource::CalculateMinMaxElevations()
{
    // Calculate min/max elevations starting from the finest level
    for (HierarchyReverseIterator it(m_iNumLevels); it.IsValid(); it.Next())
    {
        RecomputePatchMinMaxElevations(it);
    }
}

void ElevationDataSource::GetDataPtr(const Uint16*& pDataPtr, size_t& Pitch)
{
    pDataPtr = &m_TheHeightMap[0];
    Pitch    = m_iStride;
}

} // namespace Diligent
