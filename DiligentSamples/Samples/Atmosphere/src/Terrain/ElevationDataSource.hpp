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

#pragma once

#include <vector>
#include "BasicTypes.h"
#include "BasicMath.hpp"
#include "HierarchyArray.hpp"
#include "DynamicQuadTreeNode.hpp"

namespace Diligent
{

// Class implementing elevation data source
class ElevationDataSource
{
public:
    // Creates data source from the specified raw data file
    ElevationDataSource(const Char* strSrcDemFile);
    virtual ~ElevationDataSource(void);

    void GetDataPtr(const Uint16*& pDataPtr, size_t& Pitch);

    // Returns minimal height of the whole terrain
    Uint16 GetGlobalMinElevation() const;

    // Returns maximal height of the whole terrain
    Uint16 GetGlobalMaxElevation() const;

    void RecomputePatchMinMaxElevations(const QuadTreeNodeLocation& pos);

    void SetOffsets(int iColOffset, int iRowOffset)
    {
        m_iColOffset = iColOffset;
        m_iRowOffset = iRowOffset;
    }
    void GetOffsets(int& iColOffset, int& iRowOffset) const
    {
        iColOffset = m_iColOffset;
        iRowOffset = m_iRowOffset;
    }

    float GetInterpolatedHeight(float fCol, float fRow, int iStep = 1) const;

    float3 ComputeSurfaceNormal(float fCol, float fRow, float fSampleSpacing, float fHeightScale, int iStep = 1) const;

    unsigned int GetNumCols() const { return m_iNumCols; }
    unsigned int GetNumRows() const { return m_iNumRows; }

private:
    inline Uint16  GetElevSample(Int32 i, Int32 j) const;
    inline Uint16& GetElevSample(Int32 i, Int32 j);

    // Calculates min/max elevations for all patches in the tree
    void CalculateMinMaxElevations();

    // Hierarchy array storing minimal and maximal heights for quad tree nodes
    HierarchyArray<std::pair<Uint16, Uint16>> m_MinMaxElevation;

    int m_iNumLevels;
    int m_iPatchSize;
    int m_iColOffset, m_iRowOffset;

    // The whole terrain height map
    std::vector<Uint16> m_TheHeightMap;
    Uint32              m_iNumCols, m_iNumRows, m_iStride;
};

} // namespace Diligent
