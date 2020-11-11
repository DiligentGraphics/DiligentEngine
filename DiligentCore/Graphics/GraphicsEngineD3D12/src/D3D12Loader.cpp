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


#include "D3D12Loader.hpp"
#include "Errors.hpp"

#ifndef NOMINMAX
#    define NOMINMAX
#endif
#include <Windows.h>

namespace Diligent
{

D3D12CreateDeviceProcType           D3D12CreateDevice;
D3D12GetDebugInterfaceProcType      D3D12GetDebugInterface;
D3D12SerializeRootSignatureProcType D3D12SerializeRootSignature;

HMODULE LoadD3D12Dll(const char* DLLPath)
{
#if PLATFORM_WIN32
    auto hModule = LoadLibraryA(DLLPath);

    if (hModule == NULL)
    {
        return NULL;
    }

    bool bAllEntriesLoaded = true;
#    define LOAD_D3D12_ENTRY_POINT(Func)                                                             \
        do                                                                                           \
        {                                                                                            \
            Func = (Func##ProcType)GetProcAddress(hModule, #Func);                                   \
            if (Func == NULL)                                                                        \
            {                                                                                        \
                bAllEntriesLoaded = false;                                                           \
                LOG_ERROR_MESSAGE("Failed to loader '" #Func "' function from '", DLLPath, "' dll"); \
            }                                                                                        \
        } while (false)
#elif PLATFORM_UNIVERSAL_WINDOWS
    HMODULE hModule = (HMODULE)-1;
#    define LOAD_D3D12_ENTRY_POINT(Func) Func = ::Func
#else
#    error Unexpected platform
#endif

    LOAD_D3D12_ENTRY_POINT(D3D12CreateDevice);
    LOAD_D3D12_ENTRY_POINT(D3D12GetDebugInterface);
    LOAD_D3D12_ENTRY_POINT(D3D12SerializeRootSignature);
#undef LOAD_D3D12_ENTRY_POINT

#if PLATFORM_WIN32
    if (!bAllEntriesLoaded)
    {
        FreeLibrary(hModule);
        return NULL;
    }
#endif

    return hModule;
}

} // namespace Diligent
