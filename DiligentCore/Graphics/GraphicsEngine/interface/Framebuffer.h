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

// clang-format off

/// \file
/// Definition of the Diligent::IFramebuffer interface and related data structures

#include "DeviceObject.h"
#include "RenderPass.h"
#include "TextureView.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {05DA9E47-3CA6-4F96-A967-1DDDC53181A6}
static const struct INTERFACE_ID IID_Framebuffer =
    { 0x5da9e47, 0x3ca6, 0x4f96, { 0xa9, 0x67, 0x1d, 0xdd, 0xc5, 0x31, 0x81, 0xa6 } };

/// Framebuffer description.
struct FramebufferDesc DILIGENT_DERIVE(DeviceObjectAttribs)

    /// Render pass that the framebuffer will be compatible with.
    IRenderPass*         pRenderPass     DEFAULT_INITIALIZER(nullptr);

    /// The number of attachments.
    Uint32               AttachmentCount DEFAULT_INITIALIZER(0);

    /// Pointer to the array of attachments.
    ITextureView* const* ppAttachments   DEFAULT_INITIALIZER(nullptr);

    /// Width of the framebuffer.
    Uint32               Width           DEFAULT_INITIALIZER(0);

    /// Height of the framebuffer.
    Uint32               Height          DEFAULT_INITIALIZER(0);

    /// The number of array slices in the framebuffer.
    Uint32               NumArraySlices  DEFAULT_INITIALIZER(0);
};
typedef struct FramebufferDesc FramebufferDesc;

#if DILIGENT_CPP_INTERFACE

/// Framebuffer interface

/// Framebuffer has no methods.
class IFramebuffer : public IDeviceObject
{
public:
    virtual const FramebufferDesc& DILIGENT_CALL_TYPE GetDesc() const override = 0;
};

#else

struct IFramebuffer;

//  C requires that a struct or union has at least one member
//struct IFramebufferMethods
//{
//};

struct IFramebufferVtbl
{
    struct IObjectMethods       Object;
    struct IDeviceObjectMethods DeviceObject;
    //struct IFramebufferMethods  Framebuffer;
};

typedef struct IFramebuffer
{
    struct IFramebufferVtbl* pVtbl;
} IFramebuffer;

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
