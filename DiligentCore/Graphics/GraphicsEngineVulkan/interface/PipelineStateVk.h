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
/// Definition of the Diligent::IPipeplineStateVk interface

#include "../../GraphicsEngine/interface/PipelineState.h"
#include "RenderPassVk.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {2FEA0868-0932-412A-9F0A-7CEA7E61B5E0}
static const INTERFACE_ID IID_PipelineStateVk =
    {0x2fea0868, 0x932, 0x412a, {0x9f, 0xa, 0x7c, 0xea, 0x7e, 0x61, 0xb5, 0xe0}};

#define DILIGENT_INTERFACE_NAME IPipelineStateVk
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IPipelineStateVkInclusiveMethods \
    IPipelineStateInclusiveMethods;      \
    IPipelineStateVkMethods PipelineStateVk

/// Exposes Vulkan-specific functionality of a pipeline state object.
DILIGENT_BEGIN_INTERFACE(IPipelineStateVk, IPipelineState)
{
    /// Returns the pointer to the internal render pass object.
    VIRTUAL IRenderPassVk* METHOD(GetRenderPass)(THIS) CONST PURE;

    /// Returns handle to a vulkan pipeline pass object.
    VIRTUAL VkPipeline METHOD(GetVkPipeline)(THIS) CONST PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define IPipelineStateVk_GetRenderPass(This) CALL_IFACE_METHOD(PipelineStateVk, GetRenderPass, This)
#    define IPipelineStateVk_GetVkPipeline(This) CALL_IFACE_METHOD(PipelineStateVk, GetVkPipeline, This)

// clang-format on

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
