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


// The source code in this file is derived from ColorBuffer.h and GraphicsCore.h developed by Minigraph
// Original source files header:

//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard
//


#pragma once

/// \file
/// Implementation of mipmap generation routines


namespace Diligent
{
class GenerateMipsHelper
{
public:
    GenerateMipsHelper(ID3D12Device* pd3d12Device);

    void GenerateMips(ID3D12Device* pd3d12Device, class TextureViewD3D12Impl* pTexView, class CommandContext& Ctx) const;

private:
    CComPtr<ID3D12RootSignature> m_pGenerateMipsRS;
    CComPtr<ID3D12PipelineState> m_pGenerateMipsLinearPSO[4];
    CComPtr<ID3D12PipelineState> m_pGenerateMipsGammaPSO[4];
};
} // namespace Diligent
