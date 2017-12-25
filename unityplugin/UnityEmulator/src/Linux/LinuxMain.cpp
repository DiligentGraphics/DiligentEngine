/*     Copyright 2015-2017 Egor Yusov
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

#include <GL/glx.h>
#include <GL/gl.h>

 // Undef symbols defined by XLib
#ifdef Bool
# undef Bool
#endif
#ifdef True
#   undef True
#endif
#ifdef False
#   undef False
#endif

#include "DeviceCaps.h"
#include "Errors.h"
#include "Timer.h"

#include "IUnityInterface.h"
#include "UnityGraphicsGLCoreES_Emulator.h"
#include "DiligentGraphicsAdapterGL.h"
#include "UnitySceneBase.h"
#include "StringTools.h"

using namespace Diligent;

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

typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
 
extern "C"
{
    void __attribute__((visibility("default"))) UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* unityInterfaces);
    void __attribute__((visibility("default"))) UNITY_INTERFACE_API UnityPluginUnload();
    UnityRenderingEvent __attribute__((visibility("default"))) UNITY_INTERFACE_API GetRenderEventFunc();
}

void* LoadPluginFunction(const char* FunctionName);

bool LoadPlugin()
{
    // Do nothing. Android automagically sets function pointers
    return true;
}

void UnloadPlugin(UnityGraphicsEmulator *GraphicsEmulator)
{
    GraphicsEmulator->InvokeDeviceEventCallback(kUnityGfxDeviceEventShutdown);
    UnityPluginUnload();
}


int main (int argc, char ** argv)
{
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
 
    Window win = XCreateWindow(display, RootWindow(display, vi->screen), 0, 0, 1024, 768, 0, vi->depth, InputOutput, vi->visual, CWBorderPixel|CWColormap|CWEventMask, &swa);
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
  
    GLXContext ctx = glXCreateContextAttribsARB(display, fbc[0], NULL, true, context_attribs);
    if (!ctx)
    {
        LOG_ERROR("Failed to create GL context.");
        return -1;
    }
    XFree(fbc);
    
    glXMakeCurrent(display, win, ctx);
 
    auto &GraphicsGLCoreES_Emulator = UnityGraphicsGLCoreES_Emulator::GetInstance();
    GraphicsGLCoreES_Emulator.InitGLContext(reinterpret_cast<void*>(static_cast<size_t>(win)), display, major_version, minor_version);

    std::unique_ptr<DiligentGraphicsAdapter> pDiligentGraphics(new DiligentGraphicsAdapterGL(GraphicsGLCoreES_Emulator));
    UnityGraphicsEmulator *GraphicsEmulator = &GraphicsGLCoreES_Emulator;
    
    std::unique_ptr<UnitySceneBase> pScene(CreateScene());
    std::string Title = pScene->GetSceneName();
    pScene->SetDiligentGraphicsAdapter(pDiligentGraphics.get());
    pScene->OnGraphicsInitialized();
    //if (DevType == DeviceType::D3D12)
    //{
    //    UnityGraphicsD3D12Emulator::GetInstance().SetTransitionHandler(g_pScene->GetStateTransitionHandler());
    //}

    if (!LoadPlugin())
    {
         return -1;
    }

    pScene->OnPluginLoad(LoadPluginFunction);
    UnityPluginLoad(&GraphicsEmulator->GeUnityInterfaces());

    auto RenderEventFunc = GetRenderEventFunc();

    Timer timer;
    auto PrevTime = timer.GetElapsedTime();
    double filteredFrameTime = 0.0;
 
    while (true) 
    {
        bool EscPressed = false;
        XEvent xev;
        // Handle all events in the queue
        while(XCheckMaskEvent(display, 0xFFFFFFFF, &xev))
        {
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
                    if(GraphicsEmulator)
                    {
                        pDiligentGraphics->PreSwapChainResize();
                        GraphicsEmulator->ResizeSwapChain(static_cast<Uint32>(xce.width), static_cast<Uint32>(xce.height));
                        pDiligentGraphics->PostSwapChainResize();
                        pScene->OnWindowResize(static_cast<Uint32>(xce.width), static_cast<Uint32>(xce.height));
                    }
                    break;
                }

                default:
                {
                    // if (pSample != nullptr )
                    //     pSample->HandleNativeMessage(&xev);
                }
            }
        }

        if(EscPressed)
            break;

        GraphicsEmulator->BeginFrame();
        pDiligentGraphics->BeginFrame();

        auto CurrTime = timer.GetElapsedTime();
        auto ElapsedTime = CurrTime - PrevTime;
        PrevTime = CurrTime;

        pScene->Render(RenderEventFunc, CurrTime, ElapsedTime);

        pDiligentGraphics->EndFrame();
        GraphicsEmulator->EndFrame();

        GraphicsEmulator->Present();

        double filterScale = 0.2;
        filteredFrameTime = filteredFrameTime * (1.0 - filterScale) + filterScale * ElapsedTime;
        std::stringstream fpsCounterSS;
        fpsCounterSS << " - " << std::fixed << std::setprecision(1) << filteredFrameTime * 1000;
        fpsCounterSS << " ms (" << 1.0 / filteredFrameTime << " fps)";
        XStoreName(display, win, (Title + fpsCounterSS.str()).c_str());
    }

    pScene->OnPluginUnload();
    pScene.reset();
    UnloadPlugin(GraphicsEmulator);
 
    pDiligentGraphics.reset();
    GraphicsEmulator->Release();

    ctx = glXGetCurrentContext();
    glXMakeCurrent(display, None, NULL);
    glXDestroyContext(display, ctx);
    XDestroyWindow(display, win);
    XCloseDisplay(display);
}
