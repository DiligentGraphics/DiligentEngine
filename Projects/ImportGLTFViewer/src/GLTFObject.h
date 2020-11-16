#pragma once
#include "Actor.h"
#include "GLTFLoader.hpp"
#include "GLTF_PBR_Renderer.hpp"
#include "Camera.h"
#include "EnvMap.h"

namespace Diligent
{

class GLTFObject : public Actor
{
public:
    GLTFObject();
    GLTFObject(const SampleInitInfo& InitInfo);

    void Initialize(const SampleInitInfo& InitInfo) override;

    void setObjectPath(const char* path);

    void RenderActor(const Camera camera, bool IsShadowPass) override;

    void UpdateActor(double CurrTime, double ElapsedTime) override;

protected:
    const char* path;

    BackgroundMode m_BackgroundMode = BackgroundMode::PrefilteredEnvMap;


private:
    void CreatePSO() override;
    void CreateVertexBuffer() override;
    void LoadModel(const char* Path);

    GLTF_PBR_Renderer::RenderInfo m_RenderParams;

    float3 m_LightDirection;
    float4 m_LightColor     = float4(1, 1, 1, 1);
    float  m_LightIntensity = 3.f;
    float  m_EnvMapMipLevel = 1.f;
    int    m_SelectedModel  = 3;

    static const std::pair<const char*, const char*> GLTFModels[];

    bool               m_PlayAnimation  = false;
    int                m_AnimationIndex = 0;
    std::vector<float> m_AnimationTimers;

    std::unique_ptr<GLTF_PBR_Renderer>    m_GLTFRenderer;
    std::unique_ptr<GLTF::Model>          m_Model;

    enum class BoundBoxMode : int {
            None = 0,
            Local,
            Global
    };

    BoundBoxMode m_BoundBoxMode = BoundBoxMode::None;

    Camera camera;

    MouseState m_LastMouseState;
};

} // namespace Diligent