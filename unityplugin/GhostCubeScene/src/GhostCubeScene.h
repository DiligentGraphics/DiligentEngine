/*     Copyright 2015-2019 Egor Yusov
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

#include "UnitySceneBase.h"
#include "IUnityInterface.h"
#include "BasicMath.h"

using TSetMatrixFromUnity = void (UNITY_INTERFACE_API *) (float m00, float m01, float m02, float m03,
                                                          float m10, float m11, float m12, float m13,
                                                          float m20, float m21, float m22, float m23,
                                                          float m30, float m31, float m32, float m33);
using TSetTexturesFromUnity = void (UNITY_INTERFACE_API *)(void* renderTargetHandle, void *depthBufferHandle);

class GhostCubeScene final : public UnitySceneBase
{
public:
    virtual void OnPluginLoad(TLoadPluginFunction LoadPluginFunctionCallback)override final;

    virtual void OnPluginUnload()override final;

    virtual void OnGraphicsInitialized()override final;

    virtual void Render(UnityRenderingEvent RenderEventFunc)override final;
    virtual void Update(double CurrTime, double ElapsedTime)override final;

    virtual const char* GetSceneName()const override final { return "Ghost Cube Scene"; }

    virtual const char* GetPluginName()const override final { return "GhostCubePlugin"; }

private:
    friend class GhostCubeSceneResTrsnHelper;

    TSetMatrixFromUnity SetMatrixFromUnity = nullptr;
    TSetTexturesFromUnity SetTexturesFromUnity = nullptr;
    Diligent::float4x4 m_CubeWorldView;

    Diligent::RefCntAutoPtr<Diligent::ITexture> m_pRenderTarget;
    Diligent::RefCntAutoPtr<Diligent::ITexture> m_pDepthBuffer;
    Diligent::RefCntAutoPtr<Diligent::IBuffer> m_pMirrorVSConstants;
    Diligent::RefCntAutoPtr<Diligent::IPipelineState> m_pMirrorPSO;
    Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> m_pMirrorSRB;
};
