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

#ifdef PLATFORM_WIN32
#    ifndef WIN32_LEAN_AND_MEAN
#        define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#    endif

#    ifndef NOMINMAX
#        define NOMINMAX
#    endif
#endif

#include <vector>
#include <exception>
#include <algorithm>

#include "VulkanUtilities/VulkanHeaders.h"

#include "GraphicsTypes.h"

namespace std
{

template <> struct hash<Diligent::TEXTURE_FORMAT>
{
    size_t operator()(const Diligent::TEXTURE_FORMAT& fmt) const
    {
        return hash<size_t>()(size_t{fmt});
    }
};
template <> struct hash<VkFormat>
{
    size_t operator()(const VkFormat& fmt) const
    {
        return hash<int>()(int{fmt});
    }
};

} // namespace std

#include "PlatformDefinitions.h"
#include "Errors.hpp"
#include "RefCntAutoPtr.hpp"
#include "VulkanErrors.hpp"
#include "RenderDeviceBase.hpp"
#include "ValidatedCast.hpp"
