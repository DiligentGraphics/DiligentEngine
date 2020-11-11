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

#include "BufferGL.h"
#include "BufferBase.hpp"
#include "GLObjectWrapper.hpp"
#include "AsyncWritableResource.hpp"
#include "BaseInterfacesGL.h"
#include "BufferViewGLImpl.hpp"
#include "RenderDeviceGLImpl.hpp"
#include "GLContextState.hpp"

namespace Diligent
{

class FixedBlockMemoryAllocator;

/// Buffer object implementation in OpenGL backend.
class BufferGLImpl final : public BufferBase<IBufferGL, RenderDeviceGLImpl, BufferViewGLImpl, FixedBlockMemoryAllocator>, public AsyncWritableResource
{
public:
    using TBufferBase = BufferBase<IBufferGL, RenderDeviceGLImpl, BufferViewGLImpl, FixedBlockMemoryAllocator>;

    BufferGLImpl(IReferenceCounters*        pRefCounters,
                 FixedBlockMemoryAllocator& BuffViewObjMemAllocator,
                 RenderDeviceGLImpl*        pDeviceGL,
                 GLContextState&            CtxState,
                 const BufferDesc&          BuffDesc,
                 const BufferData*          pBuffData,
                 bool                       bIsDeviceInternal);

    BufferGLImpl(IReferenceCounters*        pRefCounters,
                 FixedBlockMemoryAllocator& BuffViewObjMemAllocator,
                 class RenderDeviceGLImpl*  pDeviceGL,
                 GLContextState&            CtxState,
                 const BufferDesc&          BuffDesc,
                 GLuint                     GLHandle,
                 bool                       bIsDeviceInternal);

    ~BufferGLImpl();

    /// Queries the specific interface, see IObject::QueryInterface() for details
    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override;

    void UpdateData(GLContextState& CtxState, Uint32 Offset, Uint32 Size, const void* pData);
    void CopyData(GLContextState& CtxState, BufferGLImpl& SrcBufferGL, Uint32 SrcOffset, Uint32 DstOffset, Uint32 Size);
    void Map(GLContextState& CtxState, MAP_TYPE MapType, Uint32 MapFlags, PVoid& pMappedData);
    void MapRange(GLContextState& CtxState, MAP_TYPE MapType, Uint32 MapFlags, Uint32 Offset, Uint32 Length, PVoid& pMappedData);
    void Unmap(GLContextState& CtxState);

    void BufferMemoryBarrier(Uint32 RequiredBarriers, class GLContextState& GLContextState);

    const GLObjectWrappers::GLBufferObj& GetGLHandle() { return m_GlBuffer; }

    /// Implementation of IBufferGL::GetGLBufferHandle().
    virtual GLuint DILIGENT_CALL_TYPE GetGLBufferHandle() override final { return GetGLHandle(); }

    /// Implementation of IBuffer::GetNativeHandle() in OpenGL backend.
    virtual void* DILIGENT_CALL_TYPE GetNativeHandle() override final
    {
        return reinterpret_cast<void*>(static_cast<size_t>(GetGLBufferHandle()));
    }

private:
    virtual void CreateViewInternal(const struct BufferViewDesc& ViewDesc, IBufferView** ppView, bool bIsDefaultView) override;

    friend class DeviceContextGLImpl;
    friend class VAOCache;

    GLObjectWrappers::GLBufferObj m_GlBuffer;
    const Uint32                  m_BindTarget;
    const GLenum                  m_GLUsageHint;
};

} // namespace Diligent
