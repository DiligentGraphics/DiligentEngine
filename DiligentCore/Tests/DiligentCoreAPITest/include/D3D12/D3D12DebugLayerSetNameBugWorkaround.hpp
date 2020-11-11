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

#include <memory>

#include "RenderDevice.h"

namespace Diligent
{

namespace Testing
{

// There is a bug in D3D12 debug layer as of build version 10.0.18362: SetName() method
// is not protected by a mutex internally. This, in combination with the fact that root signatures are
// de-duplicated by D3D12 runtime, causes the following problem:
// when multiple threads attempt to create the same root signature, the run-time returns the same
// object. Calling SetName() results in a race condition because multiple threads attempt to resize/write
// to the buffer without a mutex.

// As a workaround, we create the root signature ahead of time and reserve enough space for the name
// to avoid memory allocation.

class D3D12DebugLayerSetNameBugWorkaround
{
public:
    explicit D3D12DebugLayerSetNameBugWorkaround(IRenderDevice* pDevice);
    ~D3D12DebugLayerSetNameBugWorkaround();

private:
    struct RootSignatureWrapper;
    std::unique_ptr<RootSignatureWrapper> m_RootSignature;
};

} // namespace Testing

} // namespace Diligent
