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

#include "APIInfo.h"
#include "BlendState.h"
#include "Buffer.h"
#include "BufferView.h"
#include "DepthStencilState.h"
#include "GraphicsTypes.h"
#include "DeviceContext.h"
#include "InputLayout.h"
#include "PipelineState.h"
#include "RasterizerState.h"
#include "ResourceMapping.h"
#include "Sampler.h"
#include "Shader.h"
#include "Texture.h"
#include "TextureView.h"

namespace Diligent
{

static APIInfo InitAPIInfo()
{
    APIInfo Info    = {};
    Info.StructSize = sizeof(APIInfo);
    Info.APIVersion = DILIGENT_API_VERSION;

#define INIT_STRUCTURE_SIZE(Struct) Info.Struct##Size = sizeof(Struct)
    INIT_STRUCTURE_SIZE(RenderTargetBlendDesc);
    INIT_STRUCTURE_SIZE(BlendStateDesc);
    INIT_STRUCTURE_SIZE(BufferDesc);
    INIT_STRUCTURE_SIZE(BufferData);
    INIT_STRUCTURE_SIZE(BufferFormat);
    INIT_STRUCTURE_SIZE(BufferViewDesc);
    INIT_STRUCTURE_SIZE(StencilOpDesc);
    INIT_STRUCTURE_SIZE(DepthStencilStateDesc);
    INIT_STRUCTURE_SIZE(SamplerCaps);
    INIT_STRUCTURE_SIZE(TextureCaps);
    INIT_STRUCTURE_SIZE(DeviceCaps);
    INIT_STRUCTURE_SIZE(DrawAttribs);
    INIT_STRUCTURE_SIZE(DispatchComputeAttribs);
    INIT_STRUCTURE_SIZE(Viewport);
    INIT_STRUCTURE_SIZE(Rect);
    INIT_STRUCTURE_SIZE(CopyTextureAttribs);
    INIT_STRUCTURE_SIZE(DeviceObjectAttribs);
    INIT_STRUCTURE_SIZE(GraphicsAdapterInfo);
    INIT_STRUCTURE_SIZE(DisplayModeAttribs);
    INIT_STRUCTURE_SIZE(SwapChainDesc);
    INIT_STRUCTURE_SIZE(FullScreenModeDesc);
    INIT_STRUCTURE_SIZE(EngineCreateInfo);
    INIT_STRUCTURE_SIZE(EngineGLCreateInfo);
    INIT_STRUCTURE_SIZE(EngineD3D11CreateInfo);
    INIT_STRUCTURE_SIZE(EngineD3D12CreateInfo);
    INIT_STRUCTURE_SIZE(EngineVkCreateInfo);
    INIT_STRUCTURE_SIZE(EngineMtlCreateInfo);
    INIT_STRUCTURE_SIZE(Box);
    INIT_STRUCTURE_SIZE(TextureFormatAttribs);
    INIT_STRUCTURE_SIZE(TextureFormatInfo);
    INIT_STRUCTURE_SIZE(TextureFormatInfoExt);
    INIT_STRUCTURE_SIZE(StateTransitionDesc);
    INIT_STRUCTURE_SIZE(LayoutElement);
    INIT_STRUCTURE_SIZE(InputLayoutDesc);
    INIT_STRUCTURE_SIZE(SampleDesc);
    INIT_STRUCTURE_SIZE(ShaderResourceVariableDesc);
    INIT_STRUCTURE_SIZE(ImmutableSamplerDesc);
    INIT_STRUCTURE_SIZE(PipelineResourceLayoutDesc);
    INIT_STRUCTURE_SIZE(GraphicsPipelineDesc);
    INIT_STRUCTURE_SIZE(GraphicsPipelineStateCreateInfo);
    INIT_STRUCTURE_SIZE(ComputePipelineStateCreateInfo);
    INIT_STRUCTURE_SIZE(PipelineStateDesc);
    INIT_STRUCTURE_SIZE(RasterizerStateDesc);
    INIT_STRUCTURE_SIZE(ResourceMappingEntry);
    INIT_STRUCTURE_SIZE(ResourceMappingDesc);
    INIT_STRUCTURE_SIZE(SamplerDesc);
    INIT_STRUCTURE_SIZE(ShaderDesc);
    INIT_STRUCTURE_SIZE(ShaderMacro);
    INIT_STRUCTURE_SIZE(ShaderCreateInfo);
    INIT_STRUCTURE_SIZE(ShaderResourceDesc);
    INIT_STRUCTURE_SIZE(DepthStencilClearValue);
    INIT_STRUCTURE_SIZE(OptimizedClearValue);
    INIT_STRUCTURE_SIZE(TextureDesc);
    INIT_STRUCTURE_SIZE(TextureSubResData);
    INIT_STRUCTURE_SIZE(TextureData);
    INIT_STRUCTURE_SIZE(MappedTextureSubresource);
    INIT_STRUCTURE_SIZE(TextureViewDesc);
#undef INIT_STRUCTURE_SIZE

    return Info;
}

const APIInfo& GetAPIInfo()
{
    static const APIInfo Info = InitAPIInfo();
    return Info;
}

} // namespace Diligent
