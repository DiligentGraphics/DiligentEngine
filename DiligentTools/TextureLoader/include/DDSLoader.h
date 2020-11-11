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

//--------------------------------------------------------------------------------------
// File: DDSLoader.h
//
// Functions for loading a DDS texture and creating a Direct3D 11 runtime resource for it
//
// Note these functions are useful as a light-weight runtime loader for DDS files. For
// a full-featured DDS file reader, writer, and texture processing pipeline see
// the 'Texconv' sample and the 'DirectXTex' library.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkId=248926
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#pragma once

#include "RenderDevice.h"
#include "Texture.h"

void CreateDDSTextureFromMemory(
    Diligent::IRenderDevice* pDevice,
    const Diligent::Uint8*   ddsData,
    size_t                   ddsDataSize,
    Diligent::ITexture**     texture,
    size_t                   maxsize,
    /*D2D1_ALPHA_MODE* alphaMode,*/
    const char* name);

void CreateDDSTextureFromMemoryEx(
    Diligent::IRenderDevice*     pDevice,
    const Diligent::Uint8*       ddsData,
    size_t                       ddsDataSize,
    size_t                       maxsize,
    Diligent::USAGE              usage,
    const char*                  name,
    Diligent::BIND_FLAGS         bindFlags,
    Diligent::CPU_ACCESS_FLAGS   cpuAccessFlags,
    Diligent::MISC_TEXTURE_FLAGS miscFlags,
    bool                         forceSRGB,
    Diligent::ITexture**         texture /*,
    D2D1_ALPHA_MODE* alphaMode*/
);
