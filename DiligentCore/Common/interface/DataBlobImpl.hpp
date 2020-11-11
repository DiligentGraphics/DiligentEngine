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
/// Implementation for the IDataBlob interface

#include <vector>
#include "../../Primitives/interface/BasicTypes.h"
#include "../../Primitives/interface/DataBlob.h"
#include "ObjectBase.hpp"

namespace Diligent
{

/// Base interface for a data blob
class DataBlobImpl : public Diligent::ObjectBase<IDataBlob>
{
public:
    typedef ObjectBase<IDataBlob> TBase;

    DataBlobImpl(IReferenceCounters* pRefCounters, size_t InitialSize = 0);

    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override;

    /// Sets the size of the internal data buffer
    virtual void DILIGENT_CALL_TYPE Resize(size_t NewSize) override;

    /// Returns the size of the internal data buffer
    virtual size_t DILIGENT_CALL_TYPE GetSize() const override;

    /// Returns the pointer to the internal data buffer
    virtual void* DILIGENT_CALL_TYPE GetDataPtr() override;

    /// Returns const pointer to the internal data buffer
    virtual const void* DILIGENT_CALL_TYPE GetConstDataPtr() const override;

private:
    std::vector<Uint8> m_DataBuff;
};

} // namespace Diligent
