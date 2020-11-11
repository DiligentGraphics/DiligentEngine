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
/// Declaration of Diligent::CommandListD3D12Impl class

#include "CommandListBase.hpp"
#include "RenderDeviceD3D12Impl.hpp"

namespace Diligent
{

class DeviceContextD3D12Impl;
class CommandContext;

/// Command list implementation in Direct3D12 backend.
class CommandListD3D12Impl final : public CommandListBase<ICommandList, RenderDeviceD3D12Impl>
{
public:
    using TCommandListBase = CommandListBase<ICommandList, RenderDeviceD3D12Impl>;

    CommandListD3D12Impl(IReferenceCounters*                           pRefCounters,
                         RenderDeviceD3D12Impl*                        pDevice,
                         DeviceContextD3D12Impl*                       pDeferredCtx,
                         RenderDeviceD3D12Impl::PooledCommandContext&& pCmdContext) :
        // clang-format off
        TCommandListBase
        {
            pRefCounters,
            pDevice
        },
        m_pDeferredCtx  {pDeferredCtx          },
        m_pCmdContext   {std::move(pCmdContext)}
    // clang-format on
    {
    }

    ~CommandListD3D12Impl()
    {
        if (m_pCmdContext != nullptr)
        {
            LOG_WARNING_MESSAGE("Destroying command list that has not been executed");
            m_pDevice->DisposeCommandContext(std::move(m_pCmdContext));
        }
        else
        {
            VERIFY_EXPR(m_pDeferredCtx == nullptr);
        }
    }

    RenderDeviceD3D12Impl::PooledCommandContext Close(RefCntAutoPtr<DeviceContextD3D12Impl>& pDeferredCtx)
    {
        pDeferredCtx = std::move(m_pDeferredCtx);
        return std::move(m_pCmdContext);
    }

private:
    RefCntAutoPtr<DeviceContextD3D12Impl>       m_pDeferredCtx;
    RenderDeviceD3D12Impl::PooledCommandContext m_pCmdContext;
};

} // namespace Diligent
