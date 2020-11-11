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

#include "PlatformDefinitions.h"

#if PLATFORM_WIN32

#    include "../Win32/interface/Win32NativeWindow.h"

#elif PLATFORM_UNIVERSAL_WINDOWS

#    include "../UWP/interface/UWPNativeWindow.h"

#elif PLATFORM_ANDROID

#    include "../Android/interface/AndroidNativeWindow.h"

#elif PLATFORM_LINUX

#    include "../Linux/interface/LinuxNativeWindow.h"

#elif PLATFORM_MACOS

#    include "../Apple/interface/MacOSNativeWindow.h"

#elif PLATFORM_IOS

#    include "../Apple/interface/IOSNativeWindow.h"

#else
#    error Unknown platform. Please define one of the following macros as 1: PLATFORM_WIN32, PLATFORM_UNIVERSAL_WINDOWS, PLATFORM_ANDROID, PLATFORM_LINUX, PLATFORM_MACOS, PLATFORM_IOS.
#endif

DILIGENT_BEGIN_NAMESPACE(Diligent)

#if PLATFORM_WIN32

typedef struct Win32NativeWindow NativeWindow;

#elif PLATFORM_UNIVERSAL_WINDOWS

typedef struct UWPNativeWindow NativeWindow;

#elif PLATFORM_ANDROID

typedef struct AndroidNativeWindow NativeWindow;

#elif PLATFORM_LINUX

typedef struct LinuxNativeWindow NativeWindow;

#elif PLATFORM_MACOS

typedef struct MacOSNativeWindow NativeWindow;

#elif PLATFORM_IOS

typedef struct IOSNativeWindow NativeWindow;

#else
#    error Unknown platform. Please define one of the following macros as 1: PLATFORM_WIN32, PLATFORM_UNIVERSAL_WINDOWS, PLATFORM_ANDROID, PLATFORM_LINUX, PLATFORM_MACOS, PLATFORM_IOS.
#endif

DILIGENT_END_NAMESPACE // namespace Diligent
