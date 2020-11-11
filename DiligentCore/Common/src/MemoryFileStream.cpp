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

#include <algorithm>
#include "pch.h"

#include "MemoryFileStream.hpp"

namespace Diligent
{

MemoryFileStream::MemoryFileStream(IReferenceCounters* pRefCounters,
                                   IDataBlob*          pData) :
    TBase{pRefCounters},
    m_DataBlob{pData}
{
}

IMPLEMENT_QUERY_INTERFACE(MemoryFileStream, IID_FileStream, TBase)

bool MemoryFileStream::Read(void* Data, size_t Size)
{
    VERIFY_EXPR(m_CurrentOffset <= m_DataBlob->GetSize());
    auto  BytesLeft   = m_DataBlob->GetSize() - m_CurrentOffset;
    auto  BytesToRead = std::min(BytesLeft, Size);
    auto* pSrcData    = reinterpret_cast<const Uint8*>(m_DataBlob->GetDataPtr()) + m_CurrentOffset;
    memcpy(Data, pSrcData, BytesToRead);
    m_CurrentOffset += BytesToRead;
    return Size == BytesToRead;
}

void MemoryFileStream::ReadBlob(IDataBlob* pData)
{
    auto BytesLeft = m_DataBlob->GetSize() - m_CurrentOffset;
    pData->Resize(BytesLeft);
    auto res = Read(pData->GetDataPtr(), pData->GetSize());
    VERIFY_EXPR(res);
    (void)res;
}

bool MemoryFileStream::Write(const void* Data, size_t Size)
{
    if (m_CurrentOffset + Size > m_DataBlob->GetSize())
    {
        m_DataBlob->Resize(m_CurrentOffset + Size);
    }
    auto* DstData = reinterpret_cast<Uint8*>(m_DataBlob->GetDataPtr()) + m_CurrentOffset;
    memcpy(DstData, Data, Size);
    m_CurrentOffset += Size;
    return true;
}

bool MemoryFileStream::IsValid()
{
    return !!m_DataBlob;
}

size_t MemoryFileStream::GetSize()
{
    return m_DataBlob->GetSize();
}

} // namespace Diligent
