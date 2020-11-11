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
#include "ResourceMappingImpl.hpp"
#include "DeviceObjectBase.hpp"

using namespace std;

namespace Diligent
{
ResourceMappingImpl::~ResourceMappingImpl()
{
}

IMPLEMENT_QUERY_INTERFACE(ResourceMappingImpl, IID_ResourceMapping, TObjectBase)

ThreadingTools::LockHelper ResourceMappingImpl::Lock()
{
    return ThreadingTools::LockHelper(m_LockFlag);
}

void ResourceMappingImpl::AddResourceArray(const Char* Name, Uint32 StartIndex, IDeviceObject* const* ppObjects, Uint32 NumElements, bool bIsUnique)
{
    if (Name == nullptr || *Name == 0)
        return;

    auto LockHelper = Lock();
    for (Uint32 Elem = 0; Elem < NumElements; ++Elem)
    {
        auto* pObject = ppObjects[Elem];

        // Try to construct new element in place
        auto Elems =
            m_HashTable.emplace(
                make_pair(Diligent::ResMappingHashKey(Name, true, StartIndex + Elem), // Make a copy of the source string
                          Diligent::RefCntAutoPtr<IDeviceObject>(pObject)));
        // If there is already element with the same name, replace it
        if (!Elems.second && Elems.first->second != pObject)
        {
            if (bIsUnique)
            {
                UNEXPECTED("Resource with the same name already exists");
                LOG_WARNING_MESSAGE(
                    "Resource with name ", Name,
                    " marked is unique, but already present in the hash.\n"
                    "New resource will be used\n.");
            }
            Elems.first->second = pObject;
        }
    }
}

void ResourceMappingImpl::AddResource(const Char* Name, IDeviceObject* pObject, bool bIsUnique)
{
    AddResourceArray(Name, 0, &pObject, 1, bIsUnique);
}

void ResourceMappingImpl::RemoveResourceByName(const Char* Name, Uint32 ArrayIndex)
{
    if (*Name == 0)
        return;

    auto LockHelper = Lock();
    // Remove object with the given name
    // Name will be implicitly converted to HashMapStringKey without making a copy
    m_HashTable.erase(ResMappingHashKey(Name, false, ArrayIndex));
}

void ResourceMappingImpl::GetResource(const Char* Name, IDeviceObject** ppResource, Uint32 ArrayIndex)
{
    VERIFY(Name, "Name is null");
    if (*Name == 0)
        return;

    VERIFY(ppResource, "Null pointer provided");
    if (!ppResource)
        return;

    VERIFY(*ppResource == nullptr, "Overwriting reference to existing object may cause memory leaks");
    *ppResource = nullptr;

    auto LockHelper = Lock();

    // Find an object with the requested name
    // Name will be implicitly converted to HashMapStringKey without making a copy
    auto It = m_HashTable.find(ResMappingHashKey(Name, false, ArrayIndex));
    if (It != m_HashTable.end())
    {
        *ppResource = It->second.RawPtr();
        if (*ppResource)
            (*ppResource)->AddRef();
    }
}

size_t ResourceMappingImpl::GetSize()
{
    return m_HashTable.size();
}
} // namespace Diligent
