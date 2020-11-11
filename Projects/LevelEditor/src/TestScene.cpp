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

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdio.h>
#include <typeinfo>

#include "TestScene.hpp"
#include "MapHelper.hpp"
#include "GraphicsUtilities.h"
#include "TextureUtilities.h"
#include "TexturedCube.hpp"
#include "Cube.h"
#include "Plane.h"
#include "imgui.h"


namespace ImGui
{
static auto vector_getter = [](void* vec, int idx, const char** out_text) {
    auto& vector = *static_cast<std::vector<std::string>*>(vec);
    if (idx < 0 || idx >= static_cast<int>(vector.size())) { return false; }
    *out_text = vector.at(idx).c_str();
    return true;
};

bool ListBox(const char* label, int* currIndex, std::vector<std::string>& values)
{
    if (values.empty()) { return false; }
    return ListBox(label, currIndex, vector_getter,
                   static_cast<void*>(&values),(int) values.size());
}

} // namespace ImGui
namespace Diligent
{

SampleBase* CreateSample()
{
    return new TestScene();
}

void TestScene::GetEngineInitializationAttribs(RENDER_DEVICE_TYPE DeviceType, EngineCreateInfo& EngineCI, SwapChainDesc& SCDesc)
{
    SampleBase::GetEngineInitializationAttribs(DeviceType, EngineCI, SCDesc);

    EngineCI.Features.DepthClamp = DEVICE_FEATURE_STATE_OPTIONAL;
}

void TestScene::Initialize(const SampleInitInfo& InitInfo)
{
    varInitInfo = InitInfo;
    SampleBase::Initialize(varInitInfo);
    ReadFile("TestLevel.txt",varInitInfo);
   // actors.emplace_back(new Cube(InitInfo));
   // actors.emplace_back(new Cube(InitInfo));
   // actors.emplace_back(new Plane(InitInfo));
    int i = 0;
    for (auto actor : actors)
    {
        if (transforms.size() >= i)
        {
            actor->setTransform(float3(transforms.at(i).x, transforms.at(i).y, transforms.at(i).z));

       //     actor->setTransform(float3(2.0f * i, 0.0f, 0.0f));
        
        }
        else
        {
            actor->setTransform(float3(0.0f, 0.0f, 0.0f));

        }
        i++;
    }

    CreateShadowMapVisPSO();

}

void TestScene::ReadFile(std::string fileName, const SampleInitInfo& InitInfo)
{
    std::ifstream file(fileName.c_str());

    if (!file)
    {
        return ;
    }
    std::string line;

    while (std::getline(file, line))
    {
        std::istringstream ss(line);

        std::stringstream        test(line);
        std::string       actorClass;
        std::getline(test, actorClass, '/');
        std::string xCoord;
        std::string yCoord;
        std::string zCoord;
        std::getline(test, xCoord, ',');
        std::getline(test, yCoord, ',');
        std::getline(test, zCoord, ',');

        float x = std::stof(xCoord.c_str());
        float y = std::stof(yCoord.c_str());
        float z = std::stof(zCoord.c_str());
        CreateAdaptedActor(actorClass, InitInfo);
        transforms.emplace_back(float3(x,y,z));
    }

    int isfserfs = 0;
    isfserfs++;

    file.close();
    //    transforms.push_back(float3(2, 0, 0));
//    transforms.push_back(float3(4, 0, 0));
}

void TestScene::CreateAdaptedActor(std::string actorClass, const SampleInitInfo& InitInfo)
{
    if (actorClass == "Cube") {
        actors.emplace_back(new Cube(InitInfo));
    }
    if (actorClass == "Plane")
    {
        actors.emplace_back(new Plane(InitInfo));
    }
}
    // Render a frame
void TestScene::Render()
{
    // Bind main back buffer
    auto* pRTV = m_pSwapChain->GetCurrentBackBufferRTV();
    auto* pDSV = m_pSwapChain->GetDepthBufferDSV();
    m_pImmediateContext->SetRenderTargets(1, &pRTV, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    const float ClearColor[] = {0.5f, 0.5f, 0.5f, 1.0f};
    m_pImmediateContext->ClearRenderTarget(pRTV, ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    m_pImmediateContext->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    for (auto actor : actors)
    {
        actor->RenderActor(m_CameraViewProjMatrix, false);
    }
}

void TestScene::CreateShadowMapVisPSO()
{
    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    PSOCreateInfo.PSODesc.Name = "Shadow Map Vis PSO";

    // This is a graphics pipeline
    PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    // clang-format off
    // This tutorial renders to a single render target
    PSOCreateInfo.GraphicsPipeline.NumRenderTargets             = 1;
    // Set render target format which is the format of the swap chain's color buffer
    PSOCreateInfo.GraphicsPipeline.RTVFormats[0]                = m_pSwapChain->GetDesc().ColorBufferFormat;
    // Set depth buffer format which is the format of the swap chain's back buffer
    PSOCreateInfo.GraphicsPipeline.DSVFormat                    = m_pSwapChain->GetDesc().DepthBufferFormat;
    // Primitive topology defines what kind of primitives will be rendered by this pipeline state
    PSOCreateInfo.GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    // No cull
    PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
    // Disable depth testing
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = False;
    // clang-format on

    ShaderCreateInfo ShaderCI;
    // Tell the system that the shader source code is in HLSL.
    // For OpenGL, the engine will convert this into GLSL under the hood.
    ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;

    // OpenGL backend requires emulated combined HLSL texture samplers (g_Texture + g_Texture_sampler combination)
    ShaderCI.UseCombinedTextureSamplers = true;

    // Create a shader source stream factory to load shaders from files.
    RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
    m_pEngineFactory->CreateDefaultShaderSourceStreamFactory(nullptr, &pShaderSourceFactory);
    ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;
    // Create shadow map visualization vertex shader
    RefCntAutoPtr<IShader> pShadowMapVisVS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Shadow Map Vis VS";
        ShaderCI.FilePath        = "shadow_map_vis.vsh";
        m_pDevice->CreateShader(ShaderCI, &pShadowMapVisVS);
    }

    // Create shadow map visualization pixel shader
    RefCntAutoPtr<IShader> pShadowMapVisPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Shadow Map Vis PS";
        ShaderCI.FilePath        = "shadow_map_vis.psh";
        m_pDevice->CreateShader(ShaderCI, &pShadowMapVisPS);
    }

    PSOCreateInfo.pVS = pShadowMapVisVS;
    PSOCreateInfo.pPS = pShadowMapVisPS;

    // Define variable type that will be used by default
    PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;

    // clang-format off
    SamplerDesc SamLinearClampDesc
    {
        FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR,
        TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP
    };
    ImmutableSamplerDesc ImtblSamplers[] =
    {
        {SHADER_TYPE_PIXEL, "g_ShadowMap", SamLinearClampDesc}
    };
    // clang-format on
    PSOCreateInfo.PSODesc.ResourceLayout.ImmutableSamplers    = ImtblSamplers;
    PSOCreateInfo.PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);

    m_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pShadowMapVisPSO);
}


void TestScene::UpdateUI()
{
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Level Editor", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        //index a selectionner avec listbox
        std::vector<std::string> indexesName;
        int size        = (int)actors.size();
        for (int indextemp = 0; indextemp < size; indextemp++) {
            indexesName.emplace_back(std::to_string(indextemp));
        }

        ImGui::ListBox("index actor", &indexActors, indexesName);

    //    ImGui::InputInt("Index acteur", &indexActors);
    //    ImGui::DragInt("drag int", &indexActors, 1);


        //Transform of index selected transform
        if (indexActors > -1) {

        
            float3 coord = actors.at(indexActors)->getCoord();
             float vec4i[4] = {coord.x, coord.y, coord.z, 1};
            if (ImGui::InputFloat3("coord", vec4i))
            {
                coord = float3(vec4i[0], vec4i[1], vec4i[2]);
                actors.at(indexActors)->setTransform(coord);
            }
        }
        
        
        /*
        int size =(int) actors.size();
        if (ImGui::ListBoxHeader("List",size ))
        {
            for (int indes=0; indes <size; indes++)
            {

             //   ImGui::Selectable((std::to_string(indes)).c_str(), false,);
            }
            ImGui::ListBoxFooter();
        }
        */

        //Same line String to write + button to create actor
        
        ImGui::InputText("Name of class", nameSelected, IM_ARRAYSIZE(nameSelected));
        ImGui::SameLine();
        if (ImGui::Button("Create")) {
            std::stringstream ss;
            ss << nameSelected;
            std::string s = ss.str();

            CreateAdaptedActor(s,varInitInfo);

        }

         ImGui::NewLine();
        ImGui::InputText("Level file name", levelName, IM_ARRAYSIZE(levelName));
        ImGui::SameLine();
        if (ImGui::Button("SaveLevel"))
        {
            std::stringstream sslevel;
            sslevel << levelName;
            std::string slevel = sslevel.str();

            SaveLevel(levelName);
        }

    }
    ImGui::End();
}
void TestScene::SaveLevel(std::string fileName)
{
    std::ofstream file(fileName.c_str());
    std::string   line;
    
    for (auto actor : actors)
    {
        line = actor->getClassName();
        line += "/";
        float3 tempCoord = actor->getCoord();
        line += std::to_string(tempCoord.x) + "," +std::to_string(tempCoord.y) + "," +std::to_string(tempCoord.z);
        line += "\n";
        file << line;
    }
    file.close();
}



void TestScene::Update(double CurrTime, double ElapsedTime)
{
    SampleBase::Update(CurrTime, ElapsedTime);
    UpdateUI();
    // Animate the cube
    for (auto actor : actors)
    {
        actor->Update(CurrTime, ElapsedTime);
    }

    float4x4 CameraView = float4x4::Translation(0.f, -5.0f, -10.0f) * float4x4::RotationY(PI_F) * float4x4::RotationX(-PI_F * 0.2);

    // Get pretransform matrix that rotates the scene according the surface orientation
    auto SrfPreTransform = GetSurfacePretransformMatrix(float3{0, 0, 1});

    // Get projection matrix adjusted to the current screen orientation
    auto Proj = GetAdjustedProjectionMatrix(PI_F / 4.0f, 0.1f, 100.f);

    // Compute camera view-projection matrix
    m_CameraViewProjMatrix = CameraView * SrfPreTransform * Proj;
}
} // namespace Diligent
