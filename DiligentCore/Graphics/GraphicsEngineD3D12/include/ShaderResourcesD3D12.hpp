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

/// \file
/// Declaration of Diligent::ShaderResourcesD3D12 class

//  ShaderResourcesD3D12 are created by ShaderD3D12Impl instances. They are then referenced by ShaderResourceLayoutD3D12 objects, which are in turn
//  created by instances of PipelineStatesD3D12Impl and ShaderD3D12Impl
//
//    _________________
//   |                 |
//   | ShaderD3D12Impl |
//   |_________________|
//            |
//            |shared_ptr
//    ________V_____________                  _____________________________________________________________________
//   |                      |  unique_ptr    |        |           |           |           |           |            |
//   | ShaderResourcesD3D12 |--------------->|   CBs  |  TexSRVs  |  TexUAVs  |  BufSRVs  |  BufUAVs  |  Samplers  |
//   |______________________|                |________|___________|___________|___________|___________|____________|
//            A                                         A                              A                   A
//            |                                          \                            /                     \
//            |shared_ptr                                Ref                        Ref                     Ref
//    ________|__________________                  ________\________________________/_________________________\_________________________________________
//   |                           |   unique_ptr   |                   |                 |               |                  |                 |          |
//   | ShaderResourceLayoutD3D12 |--------------->|   SRV_CBV_UAV[0]  |  SRV_CBV_UAV[1] |       ...     |    Sampler[0]    |    Sampler[1]   |   ...    |
//   |___________________________|                |___________________|_________________|_______________|__________________|_________________|__________|
//              A                                           |                                                    A
//              |                                           |___________________SamplerId________________________|
//              |
//    __________|_____________
//   |                        |
//   | PipelineStateD3D12Impl |
//   |________________________|
//
//
//
//  One ShaderResourcesD3D12 instance can be referenced by multiple objects
//
//
//             ________________________           _<m_pShaderResourceLayouts>_          _____<m_pShaderVarMgrs>_____       ________________________________
//            |                        |         |                            |        |                            |     |                                |
//            | PipelineStateD3D12Impl |========>| ShaderResourceLayoutD3D12  |<-------| ShaderVariableManagerD3D12 |<====| ShaderResourceBindingD3D12Impl |
//            |________________________|         |____________________________|        |____________________________|     |________________________________|
//                                                            |            A
//                                                            |shared_ptr   \         
//             _________________                   ___________V__________    \          _____<m_pShaderVarMgrs>_____       ________________________________
//            |                 |  shared_ptr     |                      |    \        |                            |     |                                |
//            | ShaderD3D12Impl |---------------->| ShaderResourcesD3D12 |     '-------| ShaderVariableManagerD3D12 |<====| ShaderResourceBindingD3D12Impl |
//            |_________________|                 |______________________|             |____________________________|     |________________________________|
//                    |      |___________________               A
//                    |                          |              |
//                    V                          V              |shared_ptr
//  _______<m_StaticVarsMgr>____         ___<m_StaticResLayout>_|___
// |                            |       |                           |
// | ShaderVariableManagerD3D12 |------>| ShaderResourceLayoutD3D12 |
// |____________________________|       |___________________________|
//

#include "ShaderResources.hpp"

namespace Diligent
{

/// Diligent::ShaderResourcesD3D12 class
class ShaderResourcesD3D12 final : public ShaderResources
{
public:
    // Loads shader resources from the compiled shader bytecode
    ShaderResourcesD3D12(ID3DBlob*          pShaderBytecode,
                         const ShaderDesc&  ShdrDesc,
                         const char*        CombinedSamplerSuffix,
                         class IDXCompiler* pDXCompiler);

    // clang-format off
    ShaderResourcesD3D12             (const ShaderResourcesD3D12&)  = delete;
    ShaderResourcesD3D12             (      ShaderResourcesD3D12&&) = delete;
    ShaderResourcesD3D12& operator = (const ShaderResourcesD3D12&)  = delete;
    ShaderResourcesD3D12& operator = (      ShaderResourcesD3D12&&) = delete;
    // clang-format on
};

} // namespace Diligent
