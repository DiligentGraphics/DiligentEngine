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

// This file is originally based on background_renderer.cc from
// arcore-android-sdk (https://github.com/google-ar/arcore-android-sdk)

#include "BackgroundRenderer.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "RenderDeviceGL.h"

using namespace Diligent;

namespace hello_ar
{

namespace
{

constexpr char kVertexShaderSource[] = R"(
#version 310 es

// It is crucial to use the same precision in vertex and fragment shaders because otherwise
// draw command fails with notorious 1282 error, while debug output prints largely useless
// "unable to validate active pipeline".
precision mediump float;

layout(location = 0) in vec2 in_Position;
layout(location = 1) in vec2 in_TexCoord;
layout(location = 0) out vec2 out_TexCoord;

void main()
{
   gl_Position = vec4(in_Position.xy, 0.0, 1.0);
   out_TexCoord = in_TexCoord;
}
)";

constexpr char kFragmentShaderSource[] = R"(
#version 310 es
#extension GL_OES_EGL_image_external_essl3 : require

// It is crucial to use the same precision in vertex and fragment shaders because otherwise
// draw command fails with notorious 1282 error, while debug output prints largely useless
// "unable to validate active pipeline".
precision mediump float;

layout(location = 0) in vec2 in_TexCoord;
layout(location = 0) out vec4 out_Color;

uniform samplerExternalOES sTexture;

void main()
{
    out_Color = texture(sTexture, in_TexCoord);
}
)";

constexpr int kNumVertices = 4;

} // namespace

void BackgroundRenderer::Initialize(Diligent::IRenderDevice* pDevice)
{
    // Initialize camera texture
    {
        // Create external OES texture
        glGenTextures(1, &m_CameraTextureId);
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, m_CameraTextureId);
        glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Attach Diligent texture to GL handle
        RefCntAutoPtr<IRenderDeviceGL> pDeviceGL{pDevice, IID_RenderDeviceGL};
        RefCntAutoPtr<ITexture>        camera_texture;
        TextureDesc                    TexDesc;
        TexDesc.Name      = "External camera texture";
        TexDesc.Type      = RESOURCE_DIM_TEX_2D;
        TexDesc.BindFlags = BIND_SHADER_RESOURCE;
        TexDesc.Width     = 1920;
        TexDesc.Height    = 1080;
        TexDesc.Format    = TEX_FORMAT_RGBA8_UNORM; // Texture format can't be unknown
        pDeviceGL->CreateTextureFromGLHandle(m_CameraTextureId, GL_TEXTURE_EXTERNAL_OES, TexDesc,
                                             RESOURCE_STATE_SHADER_RESOURCE, &camera_texture);
        m_pCameraTextureSRV = camera_texture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
    }

    // Create pipeline state
    {
        GraphicsPipelineStateCreateInfo PSOCreateInfo;
        PipelineStateDesc&              PSODesc = PSOCreateInfo.PSODesc;
		
        PSODesc.Name = "Backround PSO";

        PSODesc.PipelineType                                        = PIPELINE_TYPE_GRAPHICS;
        PSOCreateInfo.GraphicsPipeline.NumRenderTargets             = 1;
        PSOCreateInfo.GraphicsPipeline.RTVFormats[0]                = TEX_FORMAT_RGBA8_UNORM;
        PSOCreateInfo.GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
        PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

        ShaderCreateInfo ShaderCI;
        ShaderCI.SourceLanguage             = SHADER_SOURCE_LANGUAGE_GLSL_VERBATIM;
        ShaderCI.UseCombinedTextureSamplers = true;

        RefCntAutoPtr<IShader> pVS;
        {
            ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
            ShaderCI.EntryPoint      = "main";
            ShaderCI.Desc.Name       = "Background vertex shader";
            ShaderCI.Source          = kVertexShaderSource;
            pDevice->CreateShader(ShaderCI, &pVS);
            VERIFY(pVS, "Failed to create background vertex shader");
        }

        RefCntAutoPtr<IShader> pPS;
        {
            ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
            ShaderCI.EntryPoint      = "main";
            ShaderCI.Desc.Name       = "Background fragment shader";
            ShaderCI.Source          = kFragmentShaderSource;
            pDevice->CreateShader(ShaderCI, &pPS);
            VERIFY(pPS, "Failed to create background fragment shader");
        }

        LayoutElement LayoutElems[] =
            {
                LayoutElement{0, 0, 2, VT_FLOAT32, False}, // Position
                LayoutElement{1, 0, 2, VT_FLOAT32, False}  // TexCoord
            };

        PSOCreateInfo.pVS = pVS;
        PSOCreateInfo.pPS = pPS;

        PSOCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = LayoutElems;
        PSOCreateInfo.GraphicsPipeline.InputLayout.NumElements    = 2;

        PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
        pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pRenderBackgroundPSO);
        VERIFY(m_pRenderBackgroundPSO, "Failed to create background PSO");

        m_pRenderBackgroundPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "sTexture")->Set(m_pCameraTextureSRV);
        m_pRenderBackgroundPSO->CreateShaderResourceBinding(&m_pRenderBackgroundSRB, true);
    }

    // Create vertex buffer
    {
        BufferDesc VBDesc;
        VBDesc.Name          = "Camera background vertex buffer";
        VBDesc.uiSizeInBytes = sizeof(float) * 4 * kNumVertices;
        VBDesc.BindFlags     = BIND_VERTEX_BUFFER;
        VBDesc.Usage         = USAGE_DEFAULT;
        pDevice->CreateBuffer(VBDesc, nullptr, &m_pVertexBuffer);
        VERIFY(m_pVertexBuffer, "Failed to create background vertex buffer");
    }
}

void BackgroundRenderer::Draw(const ArSession* session, const ArFrame* frame, Diligent::IDeviceContext* ctx)
{
    // If display rotation changed (also includes view size change), we need to
    // re-query the uv coordinates for the on-screen portion of the camera image.
    int32_t geometry_changed = 0;
    ArFrame_getDisplayGeometryChanged(session, frame, &geometry_changed);
    if (geometry_changed != 0 || !m_UVsInitialized)
    {
        const GLfloat kVertices[kNumVertices * 2] = {-1.0f, -1.0f, +1.0f, -1.0f, -1.0f, +1.0f, +1.0f, +1.0f};

        GLfloat TransformedUVs[kNumVertices * 2];
        ArFrame_transformCoordinates2d(
            session, frame, AR_COORDINATES_2D_OPENGL_NORMALIZED_DEVICE_COORDINATES,
            kNumVertices, kVertices, AR_COORDINATES_2D_TEXTURE_NORMALIZED,
            TransformedUVs);
        m_UVsInitialized = true;
        GLfloat VBData[kNumVertices * 4];
        for (int i = 0; i < kNumVertices; ++i)
        {
            VBData[i * 4 + 0] = kVertices[i * 2 + 0];
            VBData[i * 4 + 1] = kVertices[i * 2 + 1];
            VBData[i * 4 + 2] = TransformedUVs[i * 2 + 0];
            VBData[i * 4 + 3] = TransformedUVs[i * 2 + 1];
        }
        ctx->UpdateBuffer(m_pVertexBuffer, 0, sizeof(VBData), VBData, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    }

    int64_t FrameTimestamp;
    ArFrame_getTimestamp(session, frame, &FrameTimestamp);
    if (FrameTimestamp == 0)
    {
        // Suppress rendering if the camera did not produce the first frame yet.
        // This is to avoid drawing possible leftover data from previous sessions if
        // the texture is reused.
        return;
    }

    ctx->SetPipelineState(m_pRenderBackgroundPSO);
    ctx->CommitShaderResources(m_pRenderBackgroundSRB, RESOURCE_STATE_TRANSITION_MODE_NONE);
    IBuffer* pVBs[]    = {m_pVertexBuffer};
    Uint32   Offsets[] = {0};
    ctx->SetVertexBuffers(0, 1, pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    DrawAttribs Attribs{4, DRAW_FLAG_NONE};
    ctx->Draw(Attribs);
}

} // namespace hello_ar
