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

/// \file
/// Helper function to load engine DLL on Windows

#include <stdlib.h>
#include <stdio.h>

#include "../../../Primitives/interface/CommonDefinitions.h"

#if PLATFORM_UNIVERSAL_WINDOWS
#    include "../../../Common/interface/StringTools.hpp"
#endif

#ifndef NOMINMAX
#    define NOMINMAX
#endif
#include <Windows.h>

DILIGENT_BEGIN_NAMESPACE(Diligent)

inline FARPROC LoadEngineDll(const char* EngineName, const char* GetFactoryFuncName)
{
    const size_t StringBufferSize = 4096;
    char*        LibName          = (char*)malloc(StringBufferSize);
    FARPROC      GetFactoryFunc   = NULL;
    HMODULE      hModule          = NULL;

    // clang-format off
#if _WIN64
    const char* Arch = "_64";
#else
    const char* Arch = "_32";
#endif

#ifdef _DEBUG
    const char* Conf = "d";
#else
    const char* Conf = "r";
#endif

    sprintf_s(LibName, StringBufferSize, "%s%s%s.dll", EngineName, Arch, Conf);

#if PLATFORM_WIN32
    hModule = LoadLibraryA(LibName);
#elif PLATFORM_UNIVERSAL_WINDOWS
    hModule = LoadPackagedLibrary(WidenString(LibName).c_str(), 0);
#else
#    error Unexpected platform
#endif
    // clang-format on

    if (hModule == NULL)
    {
        printf("Failed to load %s library.\n", LibName);
        OutputDebugStringA("Failed to load engine DLL");
        free(LibName);
        return NULL;
    }

    GetFactoryFunc = GetProcAddress(hModule, GetFactoryFuncName);
    if (GetFactoryFunc == NULL)
    {
        printf("Failed to load %s function from %s library.\n", GetFactoryFuncName, LibName);
        OutputDebugStringA("Failed to load engine factory function from library");
        FreeLibrary(hModule);
    }

    free(LibName);
    return GetFactoryFunc;
}

DILIGENT_END_NAMESPACE // namespace Diligent
