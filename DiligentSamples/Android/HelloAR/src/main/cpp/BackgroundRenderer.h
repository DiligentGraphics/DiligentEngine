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

// This file is originally based on background_renderer.h from
// arcore-android-sdk (https://github.com/google-ar/arcore-android-sdk).

#pragma once

#include "RenderDevice.h"
#include "DeviceContext.h"
#include "TextureView.h"
#include "RefCntAutoPtr.hpp"

#include "arcore_c_api.h"

namespace hello_ar
{

// This class renders the passthrough camera image into the OpenGL frame.
class BackgroundRenderer
{
public:
    BackgroundRenderer()  = default;
    ~BackgroundRenderer() = default;

    // Initializes the object.  Must be called on the OpenGL thread and before any
    // other methods below.
    void Initialize(Diligent::IRenderDevice* pDevice);

    // Draws the background image. This methods must be called for every ArFrame
    // returned by ArSession_update() to catch display geometry change events.
    void Draw(const ArSession* session, const ArFrame* frame, Diligent::IDeviceContext* ctx);

    // Returns the generated texture name for the GL_TEXTURE_EXTERNAL_OES target.
    unsigned int GetTextureId() const
    {
        return m_CameraTextureId;
    }

private:
    Diligent::RefCntAutoPtr<Diligent::ITextureView>           m_pCameraTextureSRV;
    Diligent::RefCntAutoPtr<Diligent::IPipelineState>         m_pRenderBackgroundPSO;
    Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> m_pRenderBackgroundSRB;
    Diligent::RefCntAutoPtr<Diligent::IBuffer>                m_pVertexBuffer;

    unsigned int m_CameraTextureId = 0;

    bool m_UVsInitialized = false;
};

} // namespace hello_ar
