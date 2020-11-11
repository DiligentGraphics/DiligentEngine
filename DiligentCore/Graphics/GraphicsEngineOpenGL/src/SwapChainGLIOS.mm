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

#include "pch.h"

#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/EAGLDrawable.h>

#include "DeviceContextGLImpl.hpp"
#include "RenderDeviceGLImpl.hpp"
#include "SwapChainGLIOS.hpp"

namespace Diligent
{
SwapChainGLIOS::SwapChainGLIOS(IReferenceCounters*          pRefCounters,
                               const EngineGLCreateInfo&    InitAttribs,
                               const SwapChainDesc&         SCDesc,
                               RenderDeviceGLImpl*          pRenderDeviceGL,
                               DeviceContextGLImpl*         pImmediateContextGL) :
    TSwapChainGLBase( pRefCounters, pRenderDeviceGL, pImmediateContextGL, SCDesc),
    m_ColorRenderBuffer(false),
    m_DepthRenderBuffer(false),
    m_DefaultFBO(false)
{
    if (m_DesiredPreTransform != SURFACE_TRANSFORM_OPTIMAL &&
        m_DesiredPreTransform != SURFACE_TRANSFORM_IDENTITY)
    {
        LOG_WARNING_MESSAGE(GetSurfaceTransformString(m_DesiredPreTransform),
                            " is not an allowed pretransform because OpenGL swap chains only support identity transform. "
                            "Use SURFACE_TRANSFORM_OPTIMAL (recommended) or SURFACE_TRANSFORM_IDENTITY.");
    }
    m_DesiredPreTransform        = SURFACE_TRANSFORM_OPTIMAL;
    m_SwapChainDesc.PreTransform = SURFACE_TRANSFORM_IDENTITY;

    m_CALayer = InitAttribs.Window.pCALayer;
    InitRenderBuffers(true, m_SwapChainDesc.Width, m_SwapChainDesc.Height);
    CreateDummyBuffers(m_pRenderDevice.RawPtr<RenderDeviceGLImpl>());
}

IMPLEMENT_QUERY_INTERFACE( SwapChainGLIOS, IID_SwapChainGL, TSwapChainBase )

void SwapChainGLIOS::Present(Uint32 SyncInterval)
{
    EAGLContext* context = [EAGLContext currentContext];
    glBindRenderbuffer(GL_RENDERBUFFER, m_ColorRenderBuffer);
    [context presentRenderbuffer:GL_RENDERBUFFER];
    //auto *pDeviceGL = ValidatedCast<RenderDeviceGLImpl>(m_pRenderDevice.RawPtr());
    //pDeviceGL->m_GLContext.SwapBuffers();
}

void SwapChainGLIOS::InitRenderBuffers(bool InitFromDrawable, Uint32 &Width, Uint32 &Height)
{
    EAGLContext* context = [EAGLContext currentContext];

    m_DefaultFBO.Release();
    m_DefaultFBO.Create();
    glBindFramebuffer(GL_FRAMEBUFFER, m_DefaultFBO);

    m_ColorRenderBuffer.Release();
    m_ColorRenderBuffer.Create();
    glBindRenderbuffer(GL_RENDERBUFFER, m_ColorRenderBuffer);

    if(InitFromDrawable)
    {
        // This call associates the storage for the current render buffer with the
        // EAGLDrawable (our CAEAGLLayer) allowing us to draw into a buffer that
        // will later be rendered to the screen wherever the layer is (which
        // corresponds with our view).
        id<EAGLDrawable> drawable = (__bridge id<EAGLDrawable>)m_CALayer;
        [context renderbufferStorage:GL_RENDERBUFFER fromDrawable:drawable];
    }
    else
    {
        CAEAGLLayer* layer = (__bridge CAEAGLLayer*)m_CALayer;
        [context renderbufferStorage:GL_RENDERBUFFER fromDrawable:layer];
    }

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_ColorRenderBuffer);

    // Get the drawable buffer's width and height so we can create a depth buffer for the FBO
    GLint backingWidth;
    GLint backingHeight;
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &backingWidth);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &backingHeight);
    Width = backingWidth;
    Height = backingHeight;

    // Create a depth buffer to use with our drawable FBO
    m_DepthRenderBuffer.Release();
    m_DepthRenderBuffer.Create();
    glBindRenderbuffer(GL_RENDERBUFFER, m_DepthRenderBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, backingWidth, backingHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_DepthRenderBuffer);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        LOG_ERROR_AND_THROW("Failed to make complete framebuffer object %x", glCheckFramebufferStatus(GL_FRAMEBUFFER));
    }
}

void SwapChainGLIOS::Resize( Uint32 NewWidth, Uint32 NewHeight, SURFACE_TRANSFORM NewTransform )
{
    InitRenderBuffers(false, NewWidth, NewHeight);
    TSwapChainGLBase::Resize(NewWidth, NewHeight, NewTransform, 0);
}

GLuint SwapChainGLIOS::GetDefaultFBO()const
{
    return m_DefaultFBO;
}
 
void SwapChainGLIOS::SetFullscreenMode(const DisplayModeAttribs &DisplayMode)
{
    UNSUPPORTED("Switching to fullscreen mode is not available on iOS");
}

void SwapChainGLIOS::SetWindowedMode()
{
    UNSUPPORTED("Switching to windowed mode is not available on iOS");
}
 
}
