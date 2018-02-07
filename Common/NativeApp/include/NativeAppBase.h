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

struct NativeAppAttributes
{
#ifdef PLATFORM_WIN32
    // * HWND on Win32
    void* NativeWindowHandle = nullptr;
#endif
    int WindowWidth = 0;
    int WindowHeight = 0;
};

class NativeAppBase
{
public:
    virtual ~NativeAppBase() {}

    virtual void ProcessCommandLine(const char *CmdLine) = 0;
    virtual const char* GetAppTitle()const = 0;
    virtual void Initialize(const struct NativeAppAttributes &NativeAppAttribs) = 0;
    virtual void Update(double CurrTime, double ElapsedTime) {};
    virtual void PlatformRender() = 0;
    virtual bool HandleNativeMessage(struct NativeMessage &msg) = 0;
    virtual void Resize(int width, int height) = 0;
};

extern NativeAppBase* CreateApplication();
