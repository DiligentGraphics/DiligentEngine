/*     Copyright 2015-2018 Egor Yusov
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF ANY PROPRIETARY RIGHTS.
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
/// Definition of the Diligent::IRenderDeviceGLES interface

#include "RenderDeviceGL.h"
#include <android/native_window.h>
#include <EGL/egl.h>

namespace Diligent
{

// {F705A0D9-2023-4DE1-8B3C-C56E4CEB8DB7}
static const Diligent::INTERFACE_ID IID_RenderDeviceGLES =
    {0xf705a0d9, 0x2023, 0x4de1, {0x8b, 0x3c, 0xc5, 0x6e, 0x4c, 0xeb, 0x8d, 0xb7}};

/// Interface to the render device object implemented in OpenGLES
class IRenderDeviceGLES : public Diligent::IRenderDeviceGL
{
public:
    virtual bool Invalidate() = 0;

    virtual void Suspend() = 0;

    virtual EGLint Resume(ANativeWindow* window) = 0;
};

} // namespace Diligent
