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

/// \file
/// Definition of the Diligent::MapHelper helper template class

#include "../../../Platforms/Basic/interface/DebugUtilities.hpp"
#include "../../../Common/interface/RefCntAutoPtr.hpp"
#include "../../GraphicsEngine/interface/DeviceContext.h"
#include "../../GraphicsEngine/interface/Buffer.h"

namespace Diligent
{

/// Facilitates resource mapping

/// \tparam DataType - type of the mapped data.
///
/// This class is designed to automate resource mapping/unmapping process.
/// The class automatically unmaps the resource when the class instance gets out of scope.\n
/// Usage example:
///
///     {
///         MapHelper<float> UniformData( pDeviceContext, pUniformBuff, MAP_WRITE, MAP_FLAG_DISCARD );
///         UniformData[0] = UniformData[1] = UniformData[2] = UniformData[3] = 1;
///     }
template <typename DataType, bool KeepStrongReferences = false>
class MapHelper
{
public:
    /// Initializes the class member with null values
    // clang-format off
    MapHelper() :
        m_pBuffer     {nullptr},
        m_pContext    {nullptr},
        m_pMappedData {nullptr},
        m_MapType     {static_cast<MAP_TYPE>(-1)},
        m_MapFlags    {static_cast<Uint32>(-1)  }
    // clang-format on
    {
    }

    /// Initializes the object and maps the provided resource.
    /// See Map() for details.
    MapHelper(IDeviceContext* pContext, IBuffer* pBuffer, MAP_TYPE MapType, MAP_FLAGS MapFlags) :
        MapHelper()
    {
        Map(pContext, pBuffer, MapType, MapFlags);
    }

    /// Move constructor: takes over resource ownership from Helper
    // clang-format off
    MapHelper(MapHelper&& Helper) :
        m_pBuffer       {std::move(Helper.m_pBuffer)    },
        m_pMappedData   {std::move(Helper.m_pMappedData)},
        m_pContext      {std::move(Helper.m_pContext)   },
        m_MapType       {std::move(Helper.m_MapType)    },
        m_MapFlags      {std::move(Helper.m_MapFlags)   }
    // clang-format on
    {
        Helper.m_pBuffer     = nullptr;
        Helper.m_pContext    = nullptr;
        Helper.m_pMappedData = nullptr;
        Helper.m_MapType     = static_cast<MAP_TYPE>(-1);
        Helper.m_MapFlags    = static_cast<Uint32>(-1);
    }

    /// Move-assignement operator: takes over resource ownership from Helper
    MapHelper& operator=(MapHelper&& Helper)
    {
        m_pBuffer     = std::move(Helper.m_pBuffer);
        m_pMappedData = std::move(Helper.m_pMappedData);
        m_pContext    = std::move(Helper.m_pContext);
        m_MapType     = std::move(Helper.m_MapType);
        m_MapFlags    = std::move(Helper.m_MapFlags);

        Helper.m_pBuffer     = nullptr;
        Helper.m_pContext    = nullptr;
        Helper.m_pMappedData = nullptr;
        Helper.m_MapType     = static_cast<MAP_TYPE>(-1);
        Helper.m_MapFlags    = static_cast<Uint32>(-1);

        return *this;
    }

    /// Maps the provided resource.

    /// \param pContext - Pointer to the device context to perform mapping with.
    /// \param pBuffer - Pointer to the buffer interface to map.
    /// \param MapType - Type of the map operation, see Diligent::MAP_TYPE for details.
    /// \param MapFlags - Additional map flags, see Diligent::MAP_FLAGS.
    void Map(IDeviceContext* pContext, IBuffer* pBuffer, MAP_TYPE MapType, MAP_FLAGS MapFlags)
    {
        VERIFY(!m_pBuffer && !m_pMappedData && !m_pContext, "Object already mapped");
        Unmap();
#ifdef DILIGENT_DEBUG
        {
            auto& BuffDesc = pBuffer->GetDesc();
            VERIFY(sizeof(DataType) <= BuffDesc.uiSizeInBytes, "Data type size exceeds buffer size");
        }
#endif
        pContext->MapBuffer(pBuffer, MapType, MapFlags, (PVoid&)m_pMappedData);
        if (m_pMappedData != nullptr)
        {
            m_pContext = pContext;
            m_pBuffer  = pBuffer;
            m_MapType  = MapType;
            m_MapFlags = MapFlags;
        }
    }

    /// Unamps the resource and resets the object state to default.
    void Unmap()
    {
        if (m_pBuffer)
        {
            m_pContext->UnmapBuffer(m_pBuffer, m_MapType);
            m_pBuffer  = nullptr;
            m_MapType  = static_cast<MAP_TYPE>(-1);
            m_MapFlags = static_cast<Uint32>(-1);
        }
        m_pContext    = nullptr;
        m_pMappedData = nullptr;
    }

    /// Converts mapped data pointer to DataType*
    operator DataType*() { return m_pMappedData; }

    /// Converts mapped data pointer to const DataType*
    operator const DataType*() const { return m_pMappedData; }

    /// Operator ->
    DataType* operator->() { return m_pMappedData; }

    /// Operator const ->
    const DataType* operator->() const { return m_pMappedData; }

    /// Unamps the resource
    ~MapHelper()
    {
        Unmap();
    }

private:
    MapHelper(const MapHelper&) = delete;
    MapHelper& operator=(const MapHelper&) = delete;

    template <typename PtrType, bool UseStrongReference>
    struct PtrTypeSelector
    {};

    template <typename PtrType>
    struct PtrTypeSelector<PtrType, true>
    {
        typedef RefCntAutoPtr<PtrType> Type;
    };

    template <typename PtrType>
    struct PtrTypeSelector<PtrType, false>
    {
        typedef PtrType* Type;
    };

    /// Pointer / strong auto pointer to the resource
    typename PtrTypeSelector<IBuffer, KeepStrongReferences>::Type m_pBuffer;

    /// Pointer / strong auto pointer to the context
    typename PtrTypeSelector<IDeviceContext, KeepStrongReferences>::Type m_pContext;

    /// Pointer to the mapped data
    DataType* m_pMappedData;

    MAP_TYPE m_MapType;

    Uint32 m_MapFlags;
};

} // namespace Diligent
