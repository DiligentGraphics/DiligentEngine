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

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include <vector>

#include <map>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>

#if PLATFORM_WIN32

#    ifndef GLEW_STATIC
#        define GLEW_STATIC // Must be defined to use static version of glew
#    endif
#    include "GL/glew.h"
// Glew includes <windows.h>
#    ifndef NOMINMAX
#        define NOMINMAX
#    endif
#    include "GL/wglew.h"
#    include <GL/GL.h>

#elif PLATFORM_LINUX

#    ifndef GLEW_STATIC
#        define GLEW_STATIC // Must be defined to use static version of glew
#    endif
#    ifndef GLEW_NO_GLU
#        define GLEW_NO_GLU
#    endif

#    include "GL/glew.h"
#    include "GL/glxew.h"
#    include <GL/glx.h>

// Undefine beautiful defines from GL/glx.h -> X11/Xlib.h
#    ifdef Bool
#        undef Bool
#    endif
#    ifdef True
#        undef True
#    endif
#    ifdef False
#        undef False
#    endif
#    ifdef Status
#        undef Status
#    endif
#    ifdef Success
#        undef Success
#    endif
#    ifdef None
#        undef None
#    endif

#elif PLATFORM_MACOS

#    ifndef GLEW_STATIC
#        define GLEW_STATIC // Must be defined to use static version of glew
#    endif
#    ifndef GLEW_NO_GLU
#        define GLEW_NO_GLU
#    endif

#    include "GL/glew.h"

#elif PLATFORM_ANDROID

#    include <GLES3/gl3.h>
#    include <GLES3/gl3ext.h>
// GLStubs must be included after GLFeatures!
#    include "GLStubsAndroid.h"

#elif PLATFORM_IOS

#    include <OpenGLES/ES3/gl.h>
#    include <OpenGLES/ES3/glext.h>
#    include "GLStubsIOS.h"

#else
#    error Unsupported platform
#endif

#include "Errors.hpp"

#include "PlatformDefinitions.h"
#include "RefCntAutoPtr.hpp"
#include "DebugUtilities.hpp"
#include "GLObjectWrapper.hpp"
#include "ValidatedCast.hpp"
#include "RenderDevice.h"
#include "BaseInterfacesGL.h"

#define CHECK_GL_ERROR(...)                                                                                              \
    do                                                                                                                   \
    {                                                                                                                    \
        auto err = glGetError();                                                                                         \
        if (err != GL_NO_ERROR)                                                                                          \
        {                                                                                                                \
            LogError<false>(/*IsFatal=*/false, __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__, "\nGL Error Code: ", err); \
            UNEXPECTED("Error");                                                                                         \
        }                                                                                                                \
    } while (false)

#define CHECK_GL_ERROR_AND_THROW(...)                                                                                   \
    do                                                                                                                  \
    {                                                                                                                   \
        auto err = glGetError();                                                                                        \
        if (err != GL_NO_ERROR)                                                                                         \
            LogError<true>(/*IsFatal=*/false, __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__, "\nGL Error Code: ", err); \
    } while (false)

#ifdef DILIGENT_DEVELOPMENT
#    define DEV_CHECK_GL_ERROR CHECK_GL_ERROR
#else
#    define DEV_CHECK_GL_ERROR(...) \
        do                          \
        {                           \
        } while (false)
#endif
