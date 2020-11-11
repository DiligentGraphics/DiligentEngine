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

#include <cstdarg>

// If defined it will include header `<stdint.h>` for fixed sized types otherwise nuklear tries to select the correct type. If that fails it will throw a compiler error and you have to select the correct types yourself.
#define NK_INCLUDE_FIXED_TYPES

// If defined it will include header `<stdio.h>` and provide additional functions depending on file loading.
#define NK_INCLUDE_STANDARD_IO

// If defined it will include header <stdio.h> and provide additional functions depending on file loading.
#define NK_INCLUDE_STANDARD_VARARGS

// If defined it will include header `<stdlib.h>` and provide additional functions to use this library without caring for memory allocation control and therefore ease memory management.
#define NK_INCLUDE_DEFAULT_ALLOCATOR

// Defining this adds a vertex draw command list backend to this library, which allows you to convert queue commands into vertex draw commands. This is mainly if you need a hardware accessible format for OpenGL, DirectX, Vulkan, Metal,...
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT

// Defining this adds `stb_truetype` and `stb_rect_pack` implementation to this library and provides font baking and rendering. If you already have font handling or do not want to use this font handler you don't have to define it.
#define NK_INCLUDE_FONT_BAKING

// Defining this adds the default font: ProggyClean.ttf into this library which can be loaded into a font atlas and allows using this library without having a truetype font
#define NK_INCLUDE_DEFAULT_FONT

#define NK_IMPLEMENTATION
#include "../../../ThirdParty/nuklear/nuklear.h"

#include "NkDiligent.h"

#include "RenderDevice.h"
#include "DeviceContext.h"
#include "RefCntAutoPtr.hpp"
#include "BasicMath.hpp"
#include "CommonlyUsedStates.h"
#include "MapHelper.hpp"

using namespace Diligent;

struct nk_diligent_vertex
{
    float   position[2];
    float   uv[2];
    nk_byte col[4];
};


struct nk_diligent_context
{
    struct nk_context    ctx   = {};
    struct nk_font_atlas atlas = {};
    struct nk_buffer     cmds  = {};

    struct nk_draw_null_texture null = {};

    unsigned int max_vertex_buffer_size = 0;
    unsigned int max_index_buffer_size  = 0;

    Viewport                              viewport;
    RefCntAutoPtr<IRenderDevice>          device;
    RefCntAutoPtr<IPipelineState>         pso;
    RefCntAutoPtr<IBuffer>                const_buffer;
    RefCntAutoPtr<ITextureView>           font_texture_view;
    RefCntAutoPtr<IShaderResourceBinding> srb;
    RefCntAutoPtr<IBuffer>                vertex_buffer;
    RefCntAutoPtr<IBuffer>                index_buffer;
} d3d11;

NK_API struct nk_context* nk_diligent_get_nk_ctx(nk_diligent_context* nk_dlg_ctx)
{
    VERIFY_EXPR(nk_dlg_ctx != nullptr);
    return &nk_dlg_ctx->ctx;
}

static float4x4 nk_get_projection_matrix(int width, int height, bool IsGL)
{
    const float L = 0.0f;
    const float R = (float)width;
    const float T = 0.0f;
    const float B = (float)height;
    // clang-format off
    return float4x4
    {
           2.0f / (R - L),              0.0f,   0.0f,   0.0f,
                     0.0f,    2.0f / (T - B),   0.0f,   0.0f,
                     0.0f,              0.0f,   0.5f,   0.0f,
        (R + L) / (L - R), (T + B) / (B - T),   0.5f,   1.0f
    };
    // clang-format on
}

static const char* NuklearVertexShaderSource = R"(
cbuffer buffer0
{
    float4x4 ProjectionMatrix;
};

struct VS_INPUT
{
    float2 pos : ATTRIB0;
    float2 uv  : ATTRIB1;
    float4 col : ATTRIB2;
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float4 col : COLOR;
    float2 uv  : TEXCOORD;
};

void vs(in  VS_INPUT vs_input,
        out PS_INPUT vs_output)
{
    vs_output.pos = mul(ProjectionMatrix, float4(vs_input.pos.xy, 0.0, 1.0));
    vs_output.col = vs_input.col;
    vs_output.uv  = vs_input.uv;
}
)";


static const char* NuklearPixelShaderSource = R"(
Texture2D<float4> texture0;
SamplerState      texture0_sampler;

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float4 col : COLOR;
    float2 uv  : TEXCOORD;
};

float4 ps(PS_INPUT vs_output) : SV_Target
{
    return vs_output.col * texture0.Sample(texture0_sampler, vs_output.uv);
}
)";

NK_API struct nk_diligent_context* nk_diligent_init(IRenderDevice* device,
                                                    unsigned int   width,
                                                    unsigned int   height,
                                                    TEXTURE_FORMAT BackBufferFmt,
                                                    TEXTURE_FORMAT DepthBufferFmt,
                                                    unsigned int   max_vertex_buffer_size,
                                                    unsigned int   max_index_buffer_size)
{
    nk_diligent_context* nk_dlg_ctx = new nk_diligent_context;

    nk_dlg_ctx->device                 = device;
    nk_dlg_ctx->max_vertex_buffer_size = max_vertex_buffer_size;
    nk_dlg_ctx->max_index_buffer_size  = max_index_buffer_size;

    nk_init_default(&nk_dlg_ctx->ctx, 0);
    //nk_dlg_ctx->ctx.clip.copy     = nk_diligent_clipboard_copy;
    //nk_dlg_ctx->ctx.clip.paste    = nk_diligent_clipboard_paste;
    nk_dlg_ctx->ctx.clip.userdata = nk_handle_ptr(0);

    nk_buffer_init_default(&nk_dlg_ctx->cmds);

    GraphicsPipelineStateCreateInfo PSOCreateInfo;
    PipelineStateDesc&              PSODesc          = PSOCreateInfo.PSODesc;
    GraphicsPipelineDesc&           GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
    GraphicsPipeline.RasterizerDesc.ScissorEnable = True;
    GraphicsPipeline.DepthStencilDesc.DepthEnable = False;
    GraphicsPipeline.NumRenderTargets             = 1;
    GraphicsPipeline.RTVFormats[0]                = BackBufferFmt;
    GraphicsPipeline.DSVFormat                    = DepthBufferFmt;
    GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    ShaderCreateInfo ShaderCI;
    ShaderCI.UseCombinedTextureSamplers = True;
    ShaderCI.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;

    RefCntAutoPtr<IShader> pVS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.Desc.Name       = "Nuklear VS";
        ShaderCI.EntryPoint      = "vs";
        ShaderCI.Source          = NuklearVertexShaderSource;
        device->CreateShader(ShaderCI, &pVS);
    }

    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.Desc.Name       = "Nuklear PS";
        ShaderCI.EntryPoint      = "ps";
        ShaderCI.Source          = NuklearPixelShaderSource;
        device->CreateShader(ShaderCI, &pPS);
    }

    // clang-format off
    LayoutElement Elements[] = 
    {
        { 0, 0, 2, VT_FLOAT32},
        { 1, 0, 2, VT_FLOAT32},
        { 2, 0, 4, VT_UINT8, True},
    };
    // clang-format on
    GraphicsPipeline.InputLayout.NumElements    = _countof(Elements);
    GraphicsPipeline.InputLayout.LayoutElements = Elements;

    {
        float4x4 proj = nk_get_projection_matrix(width, height, device->GetDeviceCaps().IsGLDevice());

        BufferDesc CBDesc;
        CBDesc.BindFlags     = BIND_UNIFORM_BUFFER;
        CBDesc.uiSizeInBytes = sizeof(proj);
        CBDesc.Usage         = USAGE_DEFAULT;
        BufferData InitData(&proj, sizeof(proj));

        device->CreateBuffer(CBDesc, &InitData, &nk_dlg_ctx->const_buffer);
    }


    auto& RT0Blend = GraphicsPipeline.BlendDesc.RenderTargets[0];

    RT0Blend.BlendEnable           = True;
    RT0Blend.SrcBlend              = BLEND_FACTOR_SRC_ALPHA;
    RT0Blend.DestBlend             = BLEND_FACTOR_INV_SRC_ALPHA;
    RT0Blend.BlendOp               = BLEND_OPERATION_ADD;
    RT0Blend.SrcBlendAlpha         = BLEND_FACTOR_INV_SRC_ALPHA;
    RT0Blend.DestBlendAlpha        = BLEND_FACTOR_ZERO;
    RT0Blend.BlendOpAlpha          = BLEND_OPERATION_ADD;
    RT0Blend.RenderTargetWriteMask = COLOR_MASK_ALL;

    // clang-format off
    ImmutableSamplerDesc ImtblSamplers[] = 
    {
        {SHADER_TYPE_PIXEL, "texture0", Sam_LinearClamp}
    };

    ShaderResourceVariableDesc Variables[] =
    {
        {SHADER_TYPE_PIXEL, "texture0", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE}
    };
    // clang-format on

    PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);
    PSODesc.ResourceLayout.ImmutableSamplers    = ImtblSamplers;
    PSODesc.ResourceLayout.NumVariables         = _countof(Variables);
    PSODesc.ResourceLayout.Variables            = Variables;

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    device->CreateGraphicsPipelineState(PSOCreateInfo, &nk_dlg_ctx->pso);
    nk_dlg_ctx->pso->GetStaticVariableByName(SHADER_TYPE_VERTEX, "buffer0")->Set(nk_dlg_ctx->const_buffer);

    {
        BufferDesc VBDesc;
        VBDesc.Name           = "Nuklear vertex buffer";
        VBDesc.BindFlags      = BIND_VERTEX_BUFFER;
        VBDesc.uiSizeInBytes  = max_vertex_buffer_size;
        VBDesc.Usage          = USAGE_DYNAMIC;
        VBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        device->CreateBuffer(VBDesc, nullptr, &nk_dlg_ctx->vertex_buffer);
    }

    {
        BufferDesc IBDesc;
        IBDesc.Name           = "Nuklear index buffer";
        IBDesc.BindFlags      = BIND_INDEX_BUFFER;
        IBDesc.uiSizeInBytes  = max_index_buffer_size;
        IBDesc.Usage          = USAGE_DYNAMIC;
        IBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        device->CreateBuffer(IBDesc, nullptr, &nk_dlg_ctx->index_buffer);
    }

    nk_dlg_ctx->viewport.TopLeftX = 0.0f;
    nk_dlg_ctx->viewport.TopLeftY = 0.0f;
    nk_dlg_ctx->viewport.Width    = static_cast<float>(width);
    nk_dlg_ctx->viewport.Height   = static_cast<float>(height);
    nk_dlg_ctx->viewport.MinDepth = 0.0f;
    nk_dlg_ctx->viewport.MaxDepth = 1.0f;

    return nk_dlg_ctx;
}


NK_API void
nk_diligent_render(struct nk_diligent_context* nk_dlg_ctx,
                   IDeviceContext*             device_ctx,
                   enum nk_anti_aliasing       AA)
{
    const float blend_factors[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    Uint32      offsets[]        = {0};
    IBuffer*    pVBs[]           = {nk_dlg_ctx->vertex_buffer};
    device_ctx->SetVertexBuffers(0, 1, pVBs, offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    device_ctx->SetIndexBuffer(nk_dlg_ctx->index_buffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    device_ctx->SetPipelineState(nk_dlg_ctx->pso);
    device_ctx->CommitShaderResources(nk_dlg_ctx->srb, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    device_ctx->SetBlendFactors(blend_factors);

    DrawIndexedAttribs Attribs;
    Attribs.Flags     = DRAW_FLAG_VERIFY_STATES;
    Attribs.IndexType = VT_UINT16;

    device_ctx->SetViewports(1, &nk_dlg_ctx->viewport, static_cast<Uint32>(nk_dlg_ctx->viewport.Width), static_cast<Uint32>(nk_dlg_ctx->viewport.Height));


    // Convert from command queue into draw list and draw to screen
    // Load draw vertices & elements directly into vertex + element buffer
    const struct nk_draw_command* cmd = nullptr;

    Uint32 offset = 0;
    {
        MapHelper<nk_diligent_vertex> vertices(device_ctx, nk_dlg_ctx->vertex_buffer, MAP_WRITE, MAP_FLAG_DISCARD);
        MapHelper<Uint16>             indices(device_ctx, nk_dlg_ctx->index_buffer, MAP_WRITE, MAP_FLAG_DISCARD);

        // fill converting configuration
        struct nk_convert_config config;
        // clang-format off
        NK_STORAGE const struct nk_draw_vertex_layout_element vertex_layout[] =
        {
            {NK_VERTEX_POSITION, NK_FORMAT_FLOAT,    NK_OFFSETOF(struct nk_diligent_vertex, position)},
            {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT,    NK_OFFSETOF(struct nk_diligent_vertex, uv)},
            {NK_VERTEX_COLOR,    NK_FORMAT_R8G8B8A8, NK_OFFSETOF(struct nk_diligent_vertex, col)},
            {NK_VERTEX_LAYOUT_END}
        };
        // clang-format on
        memset(&config, 0, sizeof(config));
        config.vertex_layout        = vertex_layout;
        config.vertex_size          = sizeof(struct nk_diligent_vertex);
        config.vertex_alignment     = NK_ALIGNOF(struct nk_diligent_vertex);
        config.global_alpha         = 1.0f;
        config.shape_AA             = AA;
        config.line_AA              = AA;
        config.circle_segment_count = 22;
        config.curve_segment_count  = 22;
        config.arc_segment_count    = 22;
        config.null                 = nk_dlg_ctx->null;

        // setup buffers to load vertices and elements
        struct nk_buffer vbuf, ibuf;
        nk_buffer_init_fixed(&vbuf, vertices, (size_t)nk_dlg_ctx->max_vertex_buffer_size);
        nk_buffer_init_fixed(&ibuf, indices, (size_t)nk_dlg_ctx->max_index_buffer_size);
        nk_convert(&nk_dlg_ctx->ctx, &nk_dlg_ctx->cmds, &vbuf, &ibuf, &config);
    }

    // iterate over and execute each draw command
    nk_draw_foreach(cmd, &nk_dlg_ctx->ctx, &nk_dlg_ctx->cmds)
    {
        auto* texture_view = reinterpret_cast<ITextureView*>(cmd->texture.ptr);
        VERIFY(texture_view == nk_dlg_ctx->font_texture_view, "Unexpected font texture view");
        (void)texture_view;
        if (!cmd->elem_count) continue;

        Rect scissor;
        scissor.left   = std::max(static_cast<Int32>(cmd->clip_rect.x), 0);
        scissor.right  = std::max(static_cast<Int32>(cmd->clip_rect.x + cmd->clip_rect.w), scissor.left);
        scissor.top    = std::max(static_cast<Int32>(cmd->clip_rect.y), 0);
        scissor.bottom = std::max(static_cast<Int32>(cmd->clip_rect.y + cmd->clip_rect.h), scissor.top);

        Attribs.NumIndices         = cmd->elem_count;
        Attribs.FirstIndexLocation = offset;
        //ID3D11DeviceContext_PSSetShaderResources(context, 0, 1, &texture_view);
        device_ctx->SetScissorRects(1, &scissor, static_cast<Uint32>(nk_dlg_ctx->viewport.Width), static_cast<Uint32>(nk_dlg_ctx->viewport.Height));
        device_ctx->DrawIndexed(Attribs);
        offset += cmd->elem_count;
    }
    nk_clear(&nk_dlg_ctx->ctx);
}


NK_API void nk_diligent_resize(nk_diligent_context* nk_dlg_ctx, IDeviceContext* device_ctx, unsigned int width, unsigned int height)
{
    VERIFY_EXPR(nk_dlg_ctx != nullptr && nk_dlg_ctx->device != nullptr);

    nk_dlg_ctx->viewport.Width  = (float)width;
    nk_dlg_ctx->viewport.Height = (float)height;

    float4x4 proj = nk_get_projection_matrix(width, height, nk_dlg_ctx->device->GetDeviceCaps().IsGLDevice());

    device_ctx->UpdateBuffer(nk_dlg_ctx->const_buffer, 0, sizeof(proj), &proj, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}


NK_API void
nk_diligent_font_stash_begin(nk_diligent_context* nk_dlg_ctx, nk_font_atlas** atlas)
{
    VERIFY_EXPR(nk_dlg_ctx != nullptr && nk_dlg_ctx->device != nullptr);

    nk_font_atlas_init_default(&nk_dlg_ctx->atlas);
    nk_font_atlas_begin(&nk_dlg_ctx->atlas);
    *atlas = &nk_dlg_ctx->atlas;
}


NK_API void
nk_diligent_font_stash_end(nk_diligent_context* nk_dlg_ctx,
                           IDeviceContext*      device_ctx)
{
    VERIFY_EXPR(nk_dlg_ctx != nullptr && nk_dlg_ctx->device != nullptr);

    const void* image;
    int         w, h;
    image = nk_font_atlas_bake(&nk_dlg_ctx->atlas, &w, &h, NK_FONT_ATLAS_RGBA32);

    // upload font to texture and create texture view
    RefCntAutoPtr<ITexture> font_texture;

    TextureDesc desc;
    desc.Name      = "Nuklear font texture";
    desc.Type      = RESOURCE_DIM_TEX_2D;
    desc.Width     = static_cast<Uint32>(w);
    desc.Height    = static_cast<Uint32>(h);
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format    = TEX_FORMAT_RGBA8_UNORM;
    desc.Usage     = USAGE_IMMUTABLE;
    desc.BindFlags = BIND_SHADER_RESOURCE;

    TextureSubResData mip0data[] =
        {
            {image, desc.Width * 4}};
    TextureData data(mip0data, _countof(mip0data));
    nk_dlg_ctx->device->CreateTexture(desc, &data, &font_texture);

    nk_dlg_ctx->font_texture_view = font_texture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);

    nk_font_atlas_end(&nk_dlg_ctx->atlas, nk_handle_ptr(nk_dlg_ctx->font_texture_view), &nk_dlg_ctx->null);
    if (nk_dlg_ctx->atlas.default_font)
        nk_style_set_font(&nk_dlg_ctx->ctx, &nk_dlg_ctx->atlas.default_font->handle);

    nk_dlg_ctx->pso->CreateShaderResourceBinding(&nk_dlg_ctx->srb, true);
    nk_dlg_ctx->srb->GetVariableByName(SHADER_TYPE_PIXEL, "texture0")->Set(nk_dlg_ctx->font_texture_view);
}

NK_API
void nk_diligent_shutdown(nk_diligent_context* nk_dlg_ctx)
{
    if (nk_dlg_ctx != nullptr)
    {
        nk_font_atlas_clear(&nk_dlg_ctx->atlas);
        nk_buffer_free(&nk_dlg_ctx->cmds);
        nk_free(&nk_dlg_ctx->ctx);

        delete nk_dlg_ctx;
    }
}
