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

#include <unordered_map>
#include <string>

#include "IUnityInterface.h"

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API SetMatrixFromUnity 
    (float m00, float m01, float m02, float m03,
     float m10, float m11, float m12, float m13,
     float m20, float m21, float m22, float m23,
     float m30, float m31, float m32, float m33);

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API SetTexturesFromUnity(void* renderTargetHandle, void *depthBufferHandle);

void *LoadPluginFunction(const char *name)
{
    static std::unordered_map<std::string, void*> functions_map = 
    { 
        {"SetMatrixFromUnity", reinterpret_cast<void*>(SetMatrixFromUnity) },
        {"SetTexturesFromUnity", reinterpret_cast<void*>(SetTexturesFromUnity) }
    };
    auto it = functions_map.find(name);
    return it != functions_map.end() ? it->second : nullptr;
}
