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

#include "../../GraphicsEngine/interface/RenderDevice.h"
#include "../../GraphicsEngine/interface/DeviceContext.h"

namespace Diligent
{

// clang-format off

/// Upload buffer description
struct UploadBufferDesc
{
    Uint32         Width       = 0;
    Uint32         Height      = 0;
    Uint32         Depth       = 1;
    Uint32         MipLevels   = 1;
    Uint32         ArraySize   = 1;
    TEXTURE_FORMAT Format      = TEX_FORMAT_UNKNOWN;

    bool operator == (const UploadBufferDesc &rhs) const
    {
        return Width  == rhs.Width  && 
                Height == rhs.Height &&
                Depth  == rhs.Depth  &&
                Format == rhs.Format;
    }
};
// clang-format on

class IUploadBuffer : public IObject
{
public:
    virtual void                     WaitForCopyScheduled()                  = 0;
    virtual MappedTextureSubresource GetMappedData(Uint32 Mip, Uint32 Slice) = 0;
    virtual const UploadBufferDesc&  GetDesc() const                         = 0;
};

/// Texture uploader description.
struct TextureUploaderDesc
{
};


/// Texture uploader statistics.
struct TextureUploaderStats
{
    Uint32 NumPendingOperations = 0;
};

/// Asynchronous texture uplader
class ITextureUploader : public IObject
{
public:
    /// Executes pending render-thread operations
    virtual void RenderThreadUpdate(IDeviceContext* pContext) = 0;


    /// Allocates upload buffer

    /// \param [in]  pContext   - Pointer to the device context when the method is executed by
    ///                           render thread, or null when it is called from a worker thread,
    ///                           see remarks.
    /// \param [in]  Desc       - Buffer description, see Diligent::UploadBufferDesc.
    /// \param [out] ppBuffer   - Memory address where pointer to the created upload buffer
    ///                           object will be written to.
    ///
    /// \remarks  When the method is called from a worker thread (pContext is null),
    ///           it may enqueue a render-thread operation and block until the operation is
    ///           complete. If in this case the method is in fact called from the render thread,
    ///           it may never return causing a deadlock. Always provide non-null device context
    ///           when calling the method from the render thread. On the other hand, always
    ///           pass null when calling the method from a worker thread to avoid
    ///           synchronization issues, which may result in an undefined behavior.
    ///
    ///           The method can be safely called from multiple threads simultaneously.
    ///           However, if pContext is not null, the application is responsible for synchronizing
    ///           the access to the context.
    virtual void AllocateUploadBuffer(IDeviceContext*         pContext,
                                      const UploadBufferDesc& Desc,
                                      IUploadBuffer**         ppBuffer) = 0;


    /// Schedules a GPU copy or executes the copy immediately.

    /// \param [in] pContext      - Pointer to the device context when the method is executed by
    ///                             render thread, or null when it is called from a worker thread,
    ///                             see remarks.
    /// \param [in] pDstTexture   - Destination texture for copy operation.
    /// \param [in] ArraySlice    - Destination array slice. When multiple slices
    ///                             are copied, the starting slice.
    /// \param [in] MipLevel      - Destination mip level. When multiple mip levels are copied,
    ///                             the starting mip level.
    /// \param [in] pUploadBuffer - Upload buffer to copy data from.
    ///
    /// \remarks  When the method is called from a worker thread (pContext is null),
    ///           it may enqueue a render-thread operation and block until the operation is
    ///           complete. If in this case the method is in fact called from the render thread,
    ///           it may never return causing a deadlock. Always provide non-null device context
    ///           when calling the method from the render thread. On the other hand, always
    ///           pass null when calling the method from a worker thread to avoid
    ///           synchronization issues, which may result in an undefined behavior.
    virtual void ScheduleGPUCopy(IDeviceContext* pContext,
                                 ITexture*       pDstTexture,
                                 Uint32          ArraySlice,
                                 Uint32          MipLevel,
                                 IUploadBuffer*  pUploadBuffer) = 0;


    /// Recycles upload buffer to make it available for future operations.

    /// \param [in] pUploadBuffer - Upload buffer to recycle.
    virtual void RecycleBuffer(IUploadBuffer* pUploadBuffer) = 0;


    /// Returns texture uploader statistics, see Diligent::TextureUploaderStats.
    virtual TextureUploaderStats GetStats() = 0;
};

void CreateTextureUploader(IRenderDevice* pDevice, const TextureUploaderDesc& Desc, ITextureUploader** ppUploader);

} // namespace Diligent
