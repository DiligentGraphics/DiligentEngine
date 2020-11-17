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

#include "SampleBase.hpp"
#include "BasicMath.hpp"
#include "Camera.h"
#include <vector>


namespace Diligent
{

class Component;

class Actor : public SampleBase
{
public:
    Actor();
    Actor(const SampleInitInfo& InitInfo);
    Actor(const Actor&) = delete;
    Actor& operator=(const Actor&) = delete;

    virtual void Initialize(const SampleInitInfo& InitInfo) override;

    void            Render() override final {};
    virtual void    RenderActor(const Camera& camera, bool IsShadowPass){};
    void            Update(double CurrTime, double ElapsedTime) override final;
    virtual void    UpdateActor(double CurrTime, double ElapsedTime) {}
    void            updateComponents(double CurrTime, double ElapsedTime);

    void addComponent(Component* component);
    void removeComponent(Component* component);

    void computeWorldTransform();

    float      getScale() { return scale; }
    Quaternion getRotation() { return rotation; }
    float3     getPosition() { return position; }

    void setScale(float scaleP) { scale = scaleP; }
    void setRotation(Quaternion rotationP) { rotation = rotationP; }
    void setPosition(float3 positionP) { position = positionP; }

protected:
    RefCntAutoPtr<IPipelineState>         m_pPSO;
    RefCntAutoPtr<IBuffer>                m_VertexBuffer;
    RefCntAutoPtr<IBuffer>                m_IndexBuffer;
    RefCntAutoPtr<IBuffer>                m_VSConstants;
    RefCntAutoPtr<ITextureView>           m_TextureSRV;
    RefCntAutoPtr<IShaderResourceBinding> m_SRB;

    float4x4       m_WorldMatrix;
    float4x4       m_ContextInit = float4x4::Identity();

    float      scale    = 1.0f;
    Quaternion rotation = Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
    float3     position = float3(0.0f, 0.0f, 0.0f);

private:
    virtual void CreatePSO() {}
    virtual void CreateVertexBuffer() {}

    std::vector<Component*> components;
};

} // namespace Diligent
