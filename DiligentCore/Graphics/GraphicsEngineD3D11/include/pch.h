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

#include "targetver.h"

#ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#endif

#ifndef NOMINMAX
#    define NOMINMAX
#endif

#include "PlatformDefinitions.h"

#include <vector>
#include <exception>
#include <algorithm>

#if PLATFORM_WIN32
#    ifndef D3D11_VERSION
#        define D3D11_VERSION 0
#    endif
#elif PLATFORM_UNIVERSAL_WINDOWS
#    ifndef D3D11_VERSION
#        define D3D11_VERSION 2
#    endif
#endif

#if D3D11_VERSION == 0
#    include <d3d11.h>
#elif D3D11_VERSION == 1
#    include <d3d11_1.h>
#elif D3D11_VERSION == 2
#    include <d3d11_2.h>
#elif D3D11_VERSION == 3
#    include <d3d11_3.h>
#elif D3D11_VERSION == 4
#    include <d3d11_4.h>
#endif

#include "EngineD3D11Defines.h"
#include "Errors.hpp"
#include "RefCntAutoPtr.hpp"
#include "DebugUtilities.hpp"
#include "D3DErrors.hpp"
#include "RenderDeviceBase.hpp"
#include "D3D11TypeConversions.hpp"
#include "ValidatedCast.hpp"
#include <atlcomcli.h>
