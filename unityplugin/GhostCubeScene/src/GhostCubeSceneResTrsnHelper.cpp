/*
 *  Copyright 2019-2021 Diligent Graphics LLC
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
#define NOMINMAX
#include <d3d12.h>

#include "GhostCubeSceneResTrsnHelper.h"
#include "GhostCubeScene.h"
#include "IUnityGraphicsD3D12.h"
#include "TextureD3D12.h"
#include "DeviceContextD3D12.h"
#include "ValidatedCast.hpp"


using namespace Diligent;

void GhostCubeSceneResTrsnHelper::TransitionResources(int stateCount, UnityGraphicsD3D12ResourceState* states)
{
    if (stateCount == 0)
        return;

    auto *pCtx = ValidatedCast<IDeviceContextD3D12>(m_TheScene.m_DiligentGraphics->GetContext());
    for (int i = 0; i < stateCount; ++i)
    {
        auto &ResState = states[i];
        ITextureD3D12 *pMirrorRT = m_TheScene.m_pRenderTarget.RawPtr<ITextureD3D12>();
        ITextureD3D12 *pMirrorDepth = m_TheScene.m_pDepthBuffer.RawPtr<ITextureD3D12>();
        ITextureD3D12 *pResToTransition = nullptr;
        if (ResState.resource == pMirrorRT->GetD3D12Texture())
            pResToTransition = pMirrorRT;
        else if(ResState.resource == pMirrorDepth->GetD3D12Texture())
            pResToTransition = pMirrorDepth;
        else
        {
            UNEXPECTED("Unexpected resource to transition");
        }
        if (pResToTransition)
        {
            pCtx->TransitionTextureState(pResToTransition, ResState.expected);
            pResToTransition->SetD3D12ResourceState(ResState.current);
        }
    }
    pCtx->Flush();
}
