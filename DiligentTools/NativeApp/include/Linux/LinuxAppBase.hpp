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


#include <GL/glx.h>
#include <GL/gl.h>

// Undef symbols defined by XLib
#ifdef Bool
#    undef Bool
#endif
#ifdef True
#    undef True
#endif
#ifdef False
#    undef False
#endif
#ifdef None
#    undef None
#endif

#if VULKAN_SUPPORTED
#    include <xcb/xcb.h>
#endif

#include "AppBase.hpp"

namespace Diligent
{

/// Base class for iOS applications.
class LinuxAppBase : public AppBase
{
public:
    /// Called when GL context is initialized

    /// An application must override this method to perform requred
    /// initialization operations after OpenGL context has been initialized
    /// by the framework
    /// \param [in] display - XLib display.
    /// \param [in] window  - XLib window.
    /// \return     true if the initialization was successful and false otherwise
    virtual bool OnGLContextCreated(Display* display, Window window) = 0;

    /// Handles an XLib event.

    /// An application may override this method to handle XLib events.
    /// \param [in] xev - XLib event
    virtual int HandleXEvent(XEvent* xev) { return 0; }

#if VULKAN_SUPPORTED
    /// Called by the framework to initialize Vulkan.

    /// An application must override this method to initialize Vulkan.
    /// \param [in] connection - XCB connection
    /// \param [in] window     - XCB window
    /// \return     true if the initialization was successful and false otherwise
    virtual bool InitVulkan(xcb_connection_t* connection, uint32_t window) = 0;

    /// Handles an XCB event.

    /// An application may override this method to handle XCB events.
    /// \param [in] event - XCB event
    virtual void HandleXCBEvent(xcb_generic_event_t* event) {}
#endif
};

} // namespace Diligent
