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

#if defined(ANDROID)
#    ifndef PLATFORM_ANDROID
#        define PLATFORM_ANDROID 1
#    endif
#endif

#if !PLATFORM_WIN32 && !PLATFORM_UNIVERSAL_WINDOWS && !PLATFORM_ANDROID && !PLATFORM_LINUX && !PLATFORM_MACOS && !PLATFORM_IOS
#    error Unknown platform. Please define one of the following macros as 1:  PLATFORM_WIN32, PLATFORM_UNIVERSAL_WINDOWS, PLATFORM_ANDROID, PLATFORM_LINUX, PLATFORM_MACOS, PLATFORM_IOS.
#endif

#if PLATFORM_WIN32

#    if PLATFORM_UNIVERSAL_WINDOWS || PLATFORM_ANDROID || PLATFORM_LINUX || PLATFORM_MACOS || PLATFORM_IOS
#        error Conflicting platform macros
#    endif

#    include "../Win32/interface/Win32PlatformDefinitions.h"

#elif PLATFORM_UNIVERSAL_WINDOWS

#    if PLATFORM_WIN32 || PLATFORM_ANDROID || PLATFORM_LINUX || PLATFORM_MACOS || PLATFORM_IOS
#        error Conflicting platform macros
#    endif

#    include "../UWP/interface/UWPDefinitions.h"

#elif PLATFORM_ANDROID

#    if PLATFORM_WIN32 || PLATFORM_UNIVERSAL_WINDOWS || PLATFORM_LINUX || PLATFORM_MACOS || PLATFORM_IOS
#        error Conflicting platform macros
#    endif

#    include "../Android/interface/AndroidPlatformDefinitions.h"

#elif PLATFORM_LINUX

#    if PLATFORM_WIN32 || PLATFORM_UNIVERSAL_WINDOWS || PLATFORM_ANDROID || PLATFORM_MACOS || PLATFORM_IOS
#        error Conflicting platform macros
#    endif

#    include "../Linux/interface/LinuxPlatformDefinitions.h"

#elif PLATFORM_MACOS

#    if PLATFORM_WIN32 || PLATFORM_UNIVERSAL_WINDOWS || PLATFORM_ANDROID || PLATFORM_LINUX || PLATFORM_IOS
#        error Conflicting platform macros
#    endif

#    include "../Apple/interface/ApplePlatformDefinitions.h"

#elif PLATFORM_IOS

#    if PLATFORM_WIN32 || PLATFORM_UNIVERSAL_WINDOWS || PLATFORM_ANDROID || PLATFORM_LINUX || PLATFORM_MACOS
#        error Conflicting platform macros
#    endif

#    include "../Apple/interface/ApplePlatformDefinitions.h"

#else

#    error Unsupported platform

#endif
