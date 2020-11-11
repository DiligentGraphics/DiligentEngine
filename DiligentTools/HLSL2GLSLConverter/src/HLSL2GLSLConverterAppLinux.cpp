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

#include "HLSL2GLSLConverterApp.h"
#include "Errors.hpp"
#include "RenderDevice.h"
#include "EngineFactoryOpenGL.h"
#include "RefCntAutoPtr.hpp"

#include <GL/glx.h>
#include <GL/gl.h>

typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, int, const int*);

using namespace Diligent;

int main(int argc, char** argv)
{
    HLSL2GLSLConverterApp Converter;

    if (argc == 1)
    {
        Converter.PrintHelp();
        return 0;
    }

    {
        auto ret = Converter.ParseCmdLine(argc, argv);
        if (ret != 0)
            return ret;
    }

    RefCntAutoPtr<IRenderDevice>  pDevice;
    RefCntAutoPtr<IDeviceContext> pContext;
    RefCntAutoPtr<ISwapChain>     pSwapChain;

    Display* display = nullptr;
    Window win = 0;
    if (Converter.NeedsCompileShader())
    {
        display = XOpenDisplay(0);

        // clang-format off
        static int visual_attribs[] =
        {
            GLX_RENDER_TYPE,    GLX_RGBA_BIT,
            GLX_DRAWABLE_TYPE,  GLX_WINDOW_BIT,
            GLX_DOUBLEBUFFER,   true,

            // The largest available total RGBA color buffer size (sum of GLX_RED_SIZE, 
            // GLX_GREEN_SIZE, GLX_BLUE_SIZE, and GLX_ALPHA_SIZE) of at least the minimum
            // size specified for each color component is preferred.
            GLX_RED_SIZE,       8,
            GLX_GREEN_SIZE,     8,
            GLX_BLUE_SIZE,      8,
            GLX_ALPHA_SIZE,     8,

            // The largest available depth buffer of at least GLX_DEPTH_SIZE size is preferred
            GLX_DEPTH_SIZE,     24,

            //GLX_SAMPLE_BUFFERS, 1,
            GLX_SAMPLES, 1,
            None
        };
        // clang-format on

        int          fbcount = 0;
        GLXFBConfig* fbc     = glXChooseFBConfig(display, DefaultScreen(display), visual_attribs, &fbcount);
        if (!fbc)
        {
            LOG_ERROR_MESSAGE("Failed to retrieve a framebuffer config");
            return -1;
        }

        XVisualInfo* vi = glXGetVisualFromFBConfig(display, fbc[0]);

        XSetWindowAttributes swa;
        swa.colormap     = XCreateColormap(display, RootWindow(display, vi->screen), vi->visual, AllocNone);
        swa.border_pixel = 0;
        swa.event_mask =
            StructureNotifyMask |
            ExposureMask |
            KeyPressMask |
            KeyReleaseMask |
            ButtonPressMask |
            ButtonReleaseMask |
            PointerMotionMask;

        win = XCreateWindow(display, RootWindow(display, vi->screen), 0, 0, 1024, 768, 0, vi->depth, InputOutput, vi->visual, CWBorderPixel | CWColormap | CWEventMask, &swa);
        if (!win)
        {
            LOG_ERROR_MESSAGE("Failed to create window.");
            return -1;
        }

        {
            auto SizeHints        = XAllocSizeHints();
            SizeHints->flags      = PMinSize;
            SizeHints->min_width  = 320;
            SizeHints->min_height = 240;
            XSetWMNormalHints(display, win, SizeHints);
            XFree(SizeHints);
        }

        XMapWindow(display, win);

        glXCreateContextAttribsARBProc glXCreateContextAttribsARB = nullptr;
        {
            // Create an oldstyle context first, to get the correct function pointer for glXCreateContextAttribsARB
            GLXContext ctx_old         = glXCreateContext(display, vi, 0, GL_TRUE);
            glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)glXGetProcAddress((const GLubyte*)"glXCreateContextAttribsARB");
            glXMakeCurrent(display, None, NULL);
            glXDestroyContext(display, ctx_old);
        }

        if (glXCreateContextAttribsARB == nullptr)
        {
            LOG_ERROR_MESSAGE("glXCreateContextAttribsARB entry point not found. Aborting.");
            return -1;
        }

        int Flags = GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
#ifdef _DEBUG
        Flags |= GLX_CONTEXT_DEBUG_BIT_ARB;
#endif

        int major_version = 4;
        int minor_version = 3;
        // clang-format off
        static int context_attribs[] =
        {
            GLX_CONTEXT_MAJOR_VERSION_ARB, major_version,
            GLX_CONTEXT_MINOR_VERSION_ARB, minor_version,
            GLX_CONTEXT_FLAGS_ARB, Flags,
            None
        };
        // clang-format on

        GLXContext ctx  = glXCreateContextAttribsARB(display, fbc[0], NULL, 1, context_attribs);
        if (!ctx)
        {
            LOG_ERROR_MESSAGE("Failed to create GL context.");
            return -1;
        }
        XFree(fbc);

        glXMakeCurrent(display, win, ctx);
        
        auto* pFactory = Converter.GetFactoryGL();
        EngineGLCreateInfo CreationAttribs;
        SwapChainDesc      SCDesc;
        CreationAttribs.Window.WindowId = win;
        CreationAttribs.Window.pDisplay = display;
        pFactory->CreateDeviceAndSwapChainGL(
            CreationAttribs, &pDevice, &pContext, SCDesc, &pSwapChain);
    }

    auto ret = Converter.Convert(pDevice);

    if (display != nullptr)
    {
        auto ctx = glXGetCurrentContext();
        glXMakeCurrent(display, None, NULL);
        glXDestroyContext(display, ctx);
        XDestroyWindow(display, win);
        XCloseDisplay(display);
    }

    return ret;
}
