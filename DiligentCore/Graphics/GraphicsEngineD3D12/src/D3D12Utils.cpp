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

#include "pch.h"
#include "D3D12Utils.h"

namespace Diligent
{
const Char* GetD3D12DescriptorHeapTypeLiteralName(D3D12_DESCRIPTOR_HEAP_TYPE Type)
{
    static bool        bIsInitialized = false;
    static const Char* HeapTypeNames[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
    if (!bIsInitialized)
    {
        // clang-format off
        HeapTypeNames[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] = "D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV";
        HeapTypeNames[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER]     = "D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER";
        HeapTypeNames[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]         = "D3D12_DESCRIPTOR_HEAP_TYPE_RTV";
        HeapTypeNames[D3D12_DESCRIPTOR_HEAP_TYPE_DSV]         = "D3D12_DESCRIPTOR_HEAP_TYPE_DSV";
        // clang-format on

        bIsInitialized = true;
    }
    VERIFY_EXPR(Type >= 0 && Type < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES);
    return HeapTypeNames[Type];
}
} // namespace Diligent
