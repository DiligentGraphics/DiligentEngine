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

#include "DeviceObjectBase.hpp"
#include "RenderDeviceVkImpl.hpp"
#include "VulkanUtilities/VulkanLogicalDevice.hpp"

namespace Diligent
{

template <typename VulkanObjectWrapperType>
class ManagedVulkanObject : public DeviceObjectBase<IDeviceObject, RenderDeviceVkImpl, DeviceObjectAttribs>
{
public:
    using TDeviceObjectBase = DeviceObjectBase<IDeviceObject, RenderDeviceVkImpl, DeviceObjectAttribs>;

    ManagedVulkanObject(IReferenceCounters*        pRefCounters,
                        RenderDeviceVkImpl*        pDevice,
                        const DeviceObjectAttribs& ObjDesc,
                        VulkanObjectWrapperType&&  ObjectWrapper,
                        bool                       bIsDeviceInternal = false) :
        TDeviceObjectBase{pRefCounters, pDevice, ObjDesc, bIsDeviceInternal},
        m_VkObject{std::move(ObjectWrapper)}
    {
    }

    ~ManagedVulkanObject()
    {
        m_pDevice->SafeReleaseDeviceObject(std::move(m_VkObject), ~Uint64{0});
    }

    static void Create(RenderDeviceVkImpl*       pDevice,
                       VulkanObjectWrapperType&& ObjectWrapper,
                       const char*               Name,
                       ManagedVulkanObject**     ppManagedObject)
    {
        DeviceObjectAttribs Desc;
        Desc.Name = Name;
        auto* pObj(NEW_RC_OBJ(GetRawAllocator(), "ManagedVulkanObject instance", ManagedVulkanObject)(pDevice, Desc, std::move(ObjectWrapper)));
        pObj->QueryInterface(IID_DeviceObject, reinterpret_cast<IObject**>(ppManagedObject));
    }

    typename VulkanObjectWrapperType::VkObjectType Get() const
    {
        return m_VkObject;
    }

private:
    VulkanObjectWrapperType m_VkObject;
};

using ManagedSemaphore = ManagedVulkanObject<VulkanUtilities::SemaphoreWrapper>;

} // namespace Diligent
