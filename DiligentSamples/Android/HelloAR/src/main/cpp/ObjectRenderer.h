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

// This file is originally based on obj_renderer.h from
// arcore-android-sdk (https://github.com/google-ar/arcore-android-sdk)

#pragma once

#include "RenderDevice.h"
#include "DeviceContext.h"
#include "Buffer.h"
#include "RefCntAutoPtr.hpp"
#include "BasicMath.hpp"

#include "arcore_c_api.h"

namespace hello_ar
{

// PlaneRenderer renders ARCore plane type.
class ObjRenderer
{
public:
    ObjRenderer();
    ~ObjRenderer() = default;

    // Crates the resources. Must be called on the OpenGL thread prior to any other calls.
    void Initialize(Diligent::IRenderDevice* pDevice);

    // Draws the model.
    void Draw(Diligent::IDeviceContext* pContext,
              const Diligent::float4x4& projection_mat,
              const Diligent::float4x4& view_mat,
              const Diligent::float4x4& model_mat,
              const float*              color_correction4,
              const float*              object_color4);

private:
    Diligent::RefCntAutoPtr<Diligent::IPipelineState>         m_pObjectPSO;
    Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> m_pObjectSRB;
    Diligent::RefCntAutoPtr<Diligent::IBuffer>                m_pShaderConstants;
    Diligent::RefCntAutoPtr<Diligent::IBuffer>                m_pCubeVertexBuffer;
    Diligent::RefCntAutoPtr<Diligent::IBuffer>                m_pCubeIndexBuffer;

    Diligent::float4x4 m_CubeTransformMat;
};

} // namespace hello_ar
