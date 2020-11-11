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
/// Diligent API information

#define DILIGENT_API_VERSION 240078

#include "../../../Primitives/interface/BasicTypes.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

/// Diligent API Info. This tructure can be used to verify API compatibility.
struct APIInfo
{
    size_t StructSize                          DEFAULT_INITIALIZER(0);
    int APIVersion                             DEFAULT_INITIALIZER(0);
    size_t RenderTargetBlendDescSize           DEFAULT_INITIALIZER(0);
    size_t BlendStateDescSize                  DEFAULT_INITIALIZER(0);
    size_t BufferDescSize                      DEFAULT_INITIALIZER(0);
    size_t BufferDataSize                      DEFAULT_INITIALIZER(0);
    size_t BufferFormatSize                    DEFAULT_INITIALIZER(0);
    size_t BufferViewDescSize                  DEFAULT_INITIALIZER(0);
    size_t StencilOpDescSize                   DEFAULT_INITIALIZER(0);
    size_t DepthStencilStateDescSize           DEFAULT_INITIALIZER(0);
    size_t SamplerCapsSize                     DEFAULT_INITIALIZER(0);
    size_t TextureCapsSize                     DEFAULT_INITIALIZER(0);
    size_t DeviceCapsSize                      DEFAULT_INITIALIZER(0);
    size_t DrawAttribsSize                     DEFAULT_INITIALIZER(0);
    size_t DispatchComputeAttribsSize          DEFAULT_INITIALIZER(0);
    size_t ViewportSize                        DEFAULT_INITIALIZER(0);
    size_t RectSize                            DEFAULT_INITIALIZER(0);
    size_t CopyTextureAttribsSize              DEFAULT_INITIALIZER(0);
    size_t DeviceObjectAttribsSize             DEFAULT_INITIALIZER(0);
    size_t GraphicsAdapterInfoSize             DEFAULT_INITIALIZER(0);
    size_t DisplayModeAttribsSize              DEFAULT_INITIALIZER(0);
    size_t SwapChainDescSize                   DEFAULT_INITIALIZER(0);
    size_t FullScreenModeDescSize              DEFAULT_INITIALIZER(0);
    size_t EngineCreateInfoSize                DEFAULT_INITIALIZER(0);
    size_t EngineGLCreateInfoSize              DEFAULT_INITIALIZER(0);
    size_t EngineD3D11CreateInfoSize           DEFAULT_INITIALIZER(0);
    size_t EngineD3D12CreateInfoSize           DEFAULT_INITIALIZER(0);
    size_t EngineVkCreateInfoSize              DEFAULT_INITIALIZER(0);
    size_t EngineMtlCreateInfoSize             DEFAULT_INITIALIZER(0);
    size_t BoxSize                             DEFAULT_INITIALIZER(0);
    size_t TextureFormatAttribsSize            DEFAULT_INITIALIZER(0);
    size_t TextureFormatInfoSize               DEFAULT_INITIALIZER(0);
    size_t TextureFormatInfoExtSize            DEFAULT_INITIALIZER(0);
    size_t StateTransitionDescSize             DEFAULT_INITIALIZER(0);
    size_t LayoutElementSize                   DEFAULT_INITIALIZER(0);
    size_t InputLayoutDescSize                 DEFAULT_INITIALIZER(0);
    size_t SampleDescSize                      DEFAULT_INITIALIZER(0);
    size_t ShaderResourceVariableDescSize      DEFAULT_INITIALIZER(0);
    size_t ImmutableSamplerDescSize            DEFAULT_INITIALIZER(0);
    size_t PipelineResourceLayoutDescSize      DEFAULT_INITIALIZER(0);
    size_t GraphicsPipelineDescSize            DEFAULT_INITIALIZER(0);
    size_t GraphicsPipelineStateCreateInfoSize DEFAULT_INITIALIZER(0);
    size_t ComputePipelineStateCreateInfoSize  DEFAULT_INITIALIZER(0);
    size_t PipelineStateDescSize               DEFAULT_INITIALIZER(0);
    size_t RasterizerStateDescSize             DEFAULT_INITIALIZER(0);
    size_t ResourceMappingEntrySize            DEFAULT_INITIALIZER(0);
    size_t ResourceMappingDescSize             DEFAULT_INITIALIZER(0);
    size_t SamplerDescSize                     DEFAULT_INITIALIZER(0);
    size_t ShaderDescSize                      DEFAULT_INITIALIZER(0);
    size_t ShaderMacroSize                     DEFAULT_INITIALIZER(0);
    size_t ShaderCreateInfoSize                DEFAULT_INITIALIZER(0);
    size_t ShaderResourceDescSize              DEFAULT_INITIALIZER(0);
    size_t DepthStencilClearValueSize          DEFAULT_INITIALIZER(0);
    size_t OptimizedClearValueSize             DEFAULT_INITIALIZER(0);
    size_t TextureDescSize                     DEFAULT_INITIALIZER(0);
    size_t TextureSubResDataSize               DEFAULT_INITIALIZER(0);
    size_t TextureDataSize                     DEFAULT_INITIALIZER(0);
    size_t MappedTextureSubresourceSize        DEFAULT_INITIALIZER(0);
    size_t TextureViewDescSize                 DEFAULT_INITIALIZER(0);
};
typedef struct APIInfo APIInfo;

DILIGENT_END_NAMESPACE // namespace Diligent
