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

#include <memory>
#include <iomanip>

#include "PlatformDefinitions.h"
#include "NativeAppBase.h"
#include "StringTools.h"
#include "Timer.h"
#include "Errors.h"


#ifndef GLX_CONTEXT_MAJOR_VERSION_ARB
#   define GLX_CONTEXT_MAJOR_VERSION_ARB       0x2091
#endif

#ifndef GLX_CONTEXT_MINOR_VERSION_ARB
#   define GLX_CONTEXT_MINOR_VERSION_ARB       0x2092
#endif

#ifndef GLX_CONTEXT_FLAGS_ARB
#   define GLX_CONTEXT_FLAGS_ARB               0x2094
#endif

#ifndef GLX_CONTEXT_DEBUG_BIT_ARB
#   define GLX_CONTEXT_DEBUG_BIT_ARB           0x0001
#endif

typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, int, const int*);

static constexpr uint16_t WindowWidth = 1024;
static constexpr uint16_t WindowHeight = 768;

using namespace Diligent;

namespace
{
    
class WindowTitleHelper
{
public:
    WindowTitleHelper(std::string _Title) : 
        Title(std::move(_Title))
    {}
    
    std::string GetTitleWithFPS(double ElapsedTime)
    {
        double filterScale = 0.2;
        FilteredFrameTime = FilteredFrameTime * (1.0 - filterScale) + filterScale * ElapsedTime;
        std::stringstream TitleWithFpsSS;
        TitleWithFpsSS << Title;
        TitleWithFpsSS << " - " << std::fixed << std::setprecision(1) << FilteredFrameTime * 1000;
        TitleWithFpsSS << " ms (" << 1.0 / FilteredFrameTime << " fps)";
        return TitleWithFpsSS.str();
    }

private:
    const std::string Title;
    double FilteredFrameTime = 0.0;
};

}

#if VULKAN_SUPPORTED

struct XCBInfo
{
    xcb_connection_t* connection = nullptr;
    uint32_t window = 0;
    uint16_t width = 0;
    uint16_t height = 0;
    xcb_intern_atom_reply_t* atom_wm_delete_window = nullptr;
};

XCBInfo InitXCBConnectionAndWindow(const std::string& Title)
{
    XCBInfo info;

    int scr = 0;
    info.connection = xcb_connect(nullptr, &scr);
    if (info.connection == nullptr || xcb_connection_has_error(info.connection))
    {
        std::cerr << "Unable to make an XCB connection\n";
        exit(-1);
    }

    const xcb_setup_t* setup = xcb_get_setup(info.connection);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    while (scr-- > 0)
        xcb_screen_next(&iter);

    auto screen = iter.data;

    info.width = WindowWidth;
    info.height = WindowHeight;

    uint32_t value_mask, value_list[32];

    info.window = xcb_generate_id(info.connection);

    value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    value_list[0] = screen->black_pixel;
    value_list[1] =
        XCB_EVENT_MASK_KEY_RELEASE |
		XCB_EVENT_MASK_KEY_PRESS |
		XCB_EVENT_MASK_EXPOSURE |
		XCB_EVENT_MASK_STRUCTURE_NOTIFY |
		XCB_EVENT_MASK_POINTER_MOTION |
		XCB_EVENT_MASK_BUTTON_PRESS |
		XCB_EVENT_MASK_BUTTON_RELEASE;

    xcb_create_window(info.connection, XCB_COPY_FROM_PARENT, info.window, screen->root, 0, 0, info.width, info.height, 0,
                        XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, value_mask, value_list);

    // Magic code that will send notification when window is destroyed
    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(info.connection, 1, 12, "WM_PROTOCOLS");
    xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(info.connection, cookie, 0);

    xcb_intern_atom_cookie_t cookie2 = xcb_intern_atom(info.connection, 0, 16, "WM_DELETE_WINDOW");
    info.atom_wm_delete_window = xcb_intern_atom_reply(info.connection, cookie2, 0);

    xcb_change_property(info.connection, XCB_PROP_MODE_REPLACE, info.window, (*reply).atom, 4, 32, 1,
                        &(*info.atom_wm_delete_window).atom);
    free(reply);

    xcb_change_property(info.connection, XCB_PROP_MODE_REPLACE, info.window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING,
                        8, Title.length(), Title.c_str() );

    xcb_map_window(info.connection, info.window);

    // Force the x/y coordinates to 100,100 results are identical in consecutive
    // runs
    const uint32_t coords[] = {100, 100};
    xcb_configure_window(info.connection, info.window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, coords);
    xcb_flush(info.connection);

    xcb_generic_event_t *e;
    while ((e = xcb_wait_for_event(info.connection)))
    {
        if ((e->response_type & ~0x80) == XCB_EXPOSE) break;
    }
    return info;
}

void DestroyXCBConnectionAndWindow(XCBInfo &info)
{
    xcb_destroy_window(info.connection, info.window);
    xcb_disconnect(info.connection);
}

int xcb_main()
{
    std::unique_ptr<NativeAppBase> TheApp(CreateApplication());

    std::string Title = TheApp->GetAppTitle();
    Title += " (Vulkan)";
    auto xcbInfo = InitXCBConnectionAndWindow(Title);
    TheApp->InitVulkan(xcbInfo.connection, xcbInfo.window);

    xcb_flush(xcbInfo.connection);

    Timer timer;
    auto PrevTime = timer.GetElapsedTime();
    WindowTitleHelper TitleHelper(Title);

    while (true)
    {
        xcb_generic_event_t* event = nullptr;
        bool Quit = false;
        while ((event = xcb_poll_for_event(xcbInfo.connection)) != nullptr)
        {
            TheApp->HandleXCBEvent(event);
            switch (event->response_type & 0x7f)
            {
                case XCB_CLIENT_MESSAGE:
                    if ((*(xcb_client_message_event_t*)event).data.data32[0] ==
                        (*xcbInfo.atom_wm_delete_window).atom)
                    {
                        Quit = true;
                    }
                break;

                case XCB_KEY_RELEASE:
                {
                    const auto* keyEvent = reinterpret_cast<const xcb_key_release_event_t*>(event);
                    switch (keyEvent->detail)
                    {
                        #define KEY_ESCAPE 0x9
                        case KEY_ESCAPE:
                            Quit = true;
                            break;
                    }
                }
                break;

                case XCB_DESTROY_NOTIFY:
                    Quit = true;
                break;

                case XCB_CONFIGURE_NOTIFY:
                {
                    const auto* cfgEvent = reinterpret_cast<const xcb_configure_notify_event_t*>(event);
                    if ((cfgEvent->width != xcbInfo.width) || (cfgEvent->height != xcbInfo.height))
                    {
                        xcbInfo.width  = cfgEvent->width;
                        xcbInfo.height = cfgEvent->height;
                        if ((xcbInfo.width > 0) && (xcbInfo.height > 0))
                        {
                            TheApp->WindowResize(xcbInfo.width, xcbInfo.height);
                        }
                    }
                }
                break;
                                    
                default:
                    break;
            }
            free(event);
        }

        if (Quit)
            break;

        // Render the scene
        auto CurrTime = timer.GetElapsedTime();
        auto ElapsedTime = CurrTime - PrevTime;
        PrevTime = CurrTime;

        TheApp->Update(CurrTime, ElapsedTime);

        TheApp->Render();
        
        TheApp->Present();

        auto TitleWithFPS = TitleHelper.GetTitleWithFPS(ElapsedTime);
        xcb_change_property(xcbInfo.connection, XCB_PROP_MODE_REPLACE, xcbInfo.window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING,
                            8, TitleWithFPS.length(), TitleWithFPS.c_str() );
        xcb_flush(xcbInfo.connection);
    }

    TheApp.reset();
    DestroyXCBConnectionAndWindow(xcbInfo);

    return 0;
}

#endif


int x_main()
{
    std::unique_ptr<NativeAppBase> TheApp(CreateApplication());
    Display *display = XOpenDisplay(0);

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
 
    int fbcount = 0;
    GLXFBConfig *fbc = glXChooseFBConfig(display, DefaultScreen(display), visual_attribs, &fbcount);
    if (!fbc)
    {
        LOG_ERROR_MESSAGE("Failed to retrieve a framebuffer config");
        return -1;
    }
 
    XVisualInfo *vi = glXGetVisualFromFBConfig(display, fbc[0]);
 
    XSetWindowAttributes swa;
    swa.colormap = XCreateColormap(display, RootWindow(display, vi->screen), vi->visual, AllocNone);
    swa.border_pixel = 0;
    swa.event_mask = 
        StructureNotifyMask |  
        ExposureMask |  
        KeyPressMask | 
        KeyReleaseMask |
        ButtonPressMask | 
        ButtonReleaseMask | 
        PointerMotionMask;
 
    Window win = XCreateWindow(display, RootWindow(display, vi->screen), 0, 0, WindowWidth, WindowHeight, 0, vi->depth, InputOutput, vi->visual, CWBorderPixel|CWColormap|CWEventMask, &swa);
    if (!win)
    {
        LOG_ERROR_MESSAGE("Failed to create window.");
        return -1;
    }
 
    XMapWindow(display, win);
 
    glXCreateContextAttribsARBProc glXCreateContextAttribsARB = nullptr;
    {
        // Create an oldstyle context first, to get the correct function pointer for glXCreateContextAttribsARB
        GLXContext ctx_old = glXCreateContext(display, vi, 0, GL_TRUE);
        glXCreateContextAttribsARB =  (glXCreateContextAttribsARBProc)glXGetProcAddress((const GLubyte*)"glXCreateContextAttribsARB");
        glXMakeCurrent(display, None, NULL);
        glXDestroyContext(display, ctx_old);
    }
 
    if (glXCreateContextAttribsARB == nullptr)
    {
        LOG_ERROR("glXCreateContextAttribsARB entry point not found. Aborting.");
        return -1;
    }
 
    int Flags = GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
#ifdef _DEBUG
     Flags |= GLX_CONTEXT_DEBUG_BIT_ARB;
#endif 
    
    int major_version = 4;
    int minor_version = 3;
    static int context_attribs[] =
    {
        GLX_CONTEXT_MAJOR_VERSION_ARB, major_version,
        GLX_CONTEXT_MINOR_VERSION_ARB, minor_version,
        GLX_CONTEXT_FLAGS_ARB, Flags,
        None
    };
  
    constexpr int True = 1;
    GLXContext ctx = glXCreateContextAttribsARB(display, fbc[0], NULL, True, context_attribs);
    if (!ctx)
    {
        LOG_ERROR("Failed to create GL context.");
        return -1;
    }
    XFree(fbc);
    

    glXMakeCurrent(display, win, ctx);
    TheApp->OnGLContextCreated(display, win);
    std::string Title = TheApp->GetAppTitle();
    Title += " (OpenGL)";
 
    Timer timer;
    auto PrevTime = timer.GetElapsedTime();
    WindowTitleHelper TitleHelper(Title);
 
    while (true) 
    {
        bool EscPressed = false;
        XEvent xev;
        // Handle all events in the queue
        while(XCheckMaskEvent(display, 0xFFFFFFFF, &xev))
        {
            TheApp->HandleXEvent(&xev);
            switch(xev.type)
            {
                case KeyPress:
                {
                    KeySym keysym;
                    char buffer[80];
                    int num_char = XLookupString((XKeyEvent *)&xev, buffer, _countof(buffer), &keysym, 0);
                    EscPressed = (keysym==XK_Escape);
                }
                
                case ConfigureNotify:
                {
                    XConfigureEvent &xce = reinterpret_cast<XConfigureEvent &>(xev);
                    if(xce.width != 0 && xce.height != 0)
                        TheApp->WindowResize(xce.width, xce.height);
                    break;
                }
            }
        }

        if(EscPressed)
            break;

        // Render the scene
        auto CurrTime = timer.GetElapsedTime();
        auto ElapsedTime = CurrTime - PrevTime;
        PrevTime = CurrTime;

        TheApp->Update(CurrTime, ElapsedTime);

        TheApp->Render();
        
        TheApp->Present();

        auto TitleWithFPS = TitleHelper.GetTitleWithFPS(ElapsedTime);
        XStoreName(display, win, TitleWithFPS.c_str());
    }

    TheApp.reset();
 
    ctx = glXGetCurrentContext();
    glXMakeCurrent(display, None, NULL);
    glXDestroyContext(display, ctx);
    XDestroyWindow(display, win);
    XCloseDisplay(display);
}

int main (int argc, char ** argv)
{
    bool UseVulkan = false;

#ifdef VULKAN_SUPPORTED
    UseVulkan = true;
    if (argc > 1)
    {
        const auto* Key = "mode=";
        const auto* pos = strstr(argv[1], Key);
        if (pos != nullptr)
        {
            pos += strlen(Key);
            if (strcasecmp(pos, "GL") == 0)
            {
                UseVulkan = false;
            }
            else if (strcasecmp(pos, "VK") == 0)
            {
                UseVulkan = true;
            }
            else
            {
                std::cerr << "Unknown device type. Only the following types are supported: GL, VK";
                return -1;
            }
        }
    }

    if (UseVulkan)
    {
        return xcb_main();
    }
#endif

    return x_main();
}