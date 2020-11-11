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

#include "pch.h"
#include "Buffer.h"
#include "GraphicsAccessories.hpp"

namespace Diligent
{

#define LOG_BUFFER_ERROR_AND_THROW(...) LOG_ERROR_AND_THROW("Description of buffer '", (Desc.Name ? Desc.Name : ""), "' is invalid: ", ##__VA_ARGS__)
#define VERIFY_BUFFER(Expr, ...)                     \
    do                                               \
    {                                                \
        if (!(Expr))                                 \
        {                                            \
            LOG_BUFFER_ERROR_AND_THROW(__VA_ARGS__); \
        }                                            \
    } while (false)


void ValidateBufferDesc(const BufferDesc& Desc, const DeviceCaps& deviceCaps)
{
    constexpr Uint32 AllowedBindFlags =
        BIND_VERTEX_BUFFER |
        BIND_INDEX_BUFFER |
        BIND_UNIFORM_BUFFER |
        BIND_SHADER_RESOURCE |
        BIND_STREAM_OUTPUT |
        BIND_UNORDERED_ACCESS |
        BIND_INDIRECT_DRAW_ARGS;

    VERIFY_BUFFER((Desc.BindFlags & ~AllowedBindFlags) == 0, "the following bind flags are not allowed for a buffer: ", GetBindFlagsString(Desc.BindFlags & ~AllowedBindFlags, ", "), '.');

    if ((Desc.BindFlags & BIND_UNORDERED_ACCESS) ||
        (Desc.BindFlags & BIND_SHADER_RESOURCE))
    {
        VERIFY_BUFFER(Desc.Mode > BUFFER_MODE_UNDEFINED && Desc.Mode < BUFFER_MODE_NUM_MODES, GetBufferModeString(Desc.Mode), " is not a valid mode for a buffer created with BIND_SHADER_RESOURCE or BIND_UNORDERED_ACCESS flags.");
        if (Desc.Mode == BUFFER_MODE_STRUCTURED || Desc.Mode == BUFFER_MODE_FORMATTED)
        {
            VERIFY_BUFFER(Desc.ElementByteStride != 0, "element stride must not be zero for structured and formatted buffers.");
        }
        else if (Desc.Mode == BUFFER_MODE_RAW)
        {
        }
    }

    switch (Desc.Usage)
    {
        case USAGE_IMMUTABLE:
        case USAGE_DEFAULT:
            VERIFY_BUFFER(Desc.CPUAccessFlags == CPU_ACCESS_NONE, "static and default buffers can't have any CPU access flags set.");
            break;

        case USAGE_DYNAMIC:
            VERIFY_BUFFER(Desc.CPUAccessFlags == CPU_ACCESS_WRITE, "dynamic buffers require CPU_ACCESS_WRITE flag.");
            break;

        case USAGE_STAGING:
            VERIFY_BUFFER(Desc.CPUAccessFlags == CPU_ACCESS_WRITE || Desc.CPUAccessFlags == CPU_ACCESS_READ,
                          "exactly one of CPU_ACCESS_WRITE or CPU_ACCESS_READ flags must be specified for a staging buffer.");
            VERIFY_BUFFER(Desc.BindFlags == BIND_NONE,
                          "staging buffers cannot be bound to any part of the graphics pipeline and can't have any bind flags set.");
            break;

        case USAGE_UNIFIED:
            VERIFY_BUFFER(deviceCaps.AdapterInfo.UnifiedMemory != 0,
                          "Unified memory is not present on this device. Check the amount of available unified memory "
                          "in the device caps before creating unified buffers.");
            VERIFY_BUFFER(Desc.CPUAccessFlags != CPU_ACCESS_NONE,
                          "at least one of CPU_ACCESS_WRITE or CPU_ACCESS_READ flags must be specified for a unified buffer.");
            if (Desc.CPUAccessFlags & CPU_ACCESS_WRITE)
            {
                VERIFY_BUFFER(deviceCaps.AdapterInfo.UnifiedMemoryCPUAccess & CPU_ACCESS_WRITE,
                              "Unified memory on this device does not support write access. Check the available access flags "
                              "in the device caps before creating unified buffers.");
            }
            if (Desc.CPUAccessFlags & CPU_ACCESS_READ)
            {
                VERIFY_BUFFER(deviceCaps.AdapterInfo.UnifiedMemoryCPUAccess & CPU_ACCESS_READ,
                              "Unified memory on this device does not support read access. Check the available access flags "
                              "in the device caps before creating unified buffers.");
            }
            break;

        default:
            UNEXPECTED("Unknown usage");
    }
}

void ValidateBufferInitData(const BufferDesc& Desc, const BufferData* pBuffData)
{
    if (Desc.Usage == USAGE_IMMUTABLE && (pBuffData == nullptr || pBuffData->pData == nullptr))
        LOG_BUFFER_ERROR_AND_THROW("initial data must not be null as immutable buffers must be initialized at creation time.");

    if (Desc.Usage == USAGE_DYNAMIC && pBuffData != nullptr && pBuffData->pData != nullptr)
        LOG_BUFFER_ERROR_AND_THROW("initial data must be null for dynamic buffers.");

    if (Desc.Usage == USAGE_STAGING)
    {
        if (Desc.CPUAccessFlags == CPU_ACCESS_WRITE)
        {
            VERIFY_BUFFER(pBuffData == nullptr || pBuffData->pData == nullptr,
                          "CPU-writable staging buffers must be updated via map.");
        }
    }
    else if (Desc.Usage == USAGE_UNIFIED)
    {
        if (pBuffData != nullptr && pBuffData->pData != nullptr && (Desc.CPUAccessFlags & CPU_ACCESS_WRITE) == 0)
        {
            LOG_BUFFER_ERROR_AND_THROW("CPU_ACCESS_WRITE flag is required to intialize a unified buffer.");
        }
    }
}

#undef VERIFY_BUFFER
#undef LOG_BUFFER_ERROR_AND_THROW

} // namespace Diligent
