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
/// Defines graphics engine utilities

#include "../../GraphicsEngine/interface/Texture.h"
#include "../../GraphicsEngine/interface/Buffer.h"
#include "../../GraphicsEngine/interface/RenderDevice.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

void DILIGENT_GLOBAL_FUNCTION(CreateUniformBuffer)(IRenderDevice*                  pDevice,
                                                   Uint32                          Size,
                                                   const Char*                     Name,
                                                   IBuffer**                       ppBuffer,
                                                   USAGE Usage                     DEFAULT_VALUE(USAGE_DYNAMIC),
                                                   BIND_FLAGS BindFlags            DEFAULT_VALUE(BIND_UNIFORM_BUFFER),
                                                   CPU_ACCESS_FLAGS CPUAccessFlags DEFAULT_VALUE(CPU_ACCESS_WRITE),
                                                   void* pInitialData              DEFAULT_VALUE(nullptr));

void DILIGENT_GLOBAL_FUNCTION(GenerateCheckerBoardPattern)(Uint32         Width,
                                                           Uint32         Height,
                                                           TEXTURE_FORMAT Fmt,
                                                           Uint32         HorzCells,
                                                           Uint32         VertCells,
                                                           Uint8*         pData,
                                                           Uint32         StrideInBytes);

DILIGENT_END_NAMESPACE // namespace Diligent
