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

// clang-format off

/// \file
/// Definition of the Diligent::IDeviceContext interface and related data structures

#include "../../../Primitives/interface/Object.h"
#include "../../../Primitives/interface/FlagEnum.h"
#include "GraphicsTypes.h"
#include "Constants.h"
#include "Buffer.h"
#include "InputLayout.h"
#include "Shader.h"
#include "Texture.h"
#include "Sampler.h"
#include "ResourceMapping.h"
#include "TextureView.h"
#include "BufferView.h"
#include "DepthStencilState.h"
#include "BlendState.h"
#include "PipelineState.h"
#include "Fence.h"
#include "Query.h"
#include "RenderPass.h"
#include "Framebuffer.h"
#include "CommandList.h"
#include "SwapChain.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)


// {DC92711B-A1BE-4319-B2BD-C662D1CC19E4}
static const INTERFACE_ID IID_DeviceContext =
    {0xdc92711b, 0xa1be, 0x4319, {0xb2, 0xbd, 0xc6, 0x62, 0xd1, 0xcc, 0x19, 0xe4}};

/// Draw command flags
DILIGENT_TYPED_ENUM(DRAW_FLAGS, Uint8)
{
    /// No flags.
    DRAW_FLAG_NONE                            = 0x00,

    /// Verify the sate of index and vertex buffers (if any) used by the draw 
    /// command. State validation is only performed in debug and development builds 
    /// and the flag has no effect in release build.
    DRAW_FLAG_VERIFY_STATES                   = 0x01,

    /// Verify correctness of parameters passed to the draw command.
    DRAW_FLAG_VERIFY_DRAW_ATTRIBS             = 0x02,

    /// Verify that render targets bound to the context are consistent with the pipeline state.
    DRAW_FLAG_VERIFY_RENDER_TARGETS           = 0x04,

    /// Perform all state validation checks
    DRAW_FLAG_VERIFY_ALL                      = DRAW_FLAG_VERIFY_STATES | DRAW_FLAG_VERIFY_DRAW_ATTRIBS | DRAW_FLAG_VERIFY_RENDER_TARGETS,

    /// Indicates that none of the dynamic resource buffers used by the draw command
    /// have been modified by the CPU since the last command.
    ///
    /// \remarks This flag should be used to improve performance when an application issues a
    ///          series of draw commands that use the same pipeline state and shader resources and
    ///          no dynamic buffers (constant or bound as shader resources) are updated between the
    ///          commands.
    ///          The flag has no effect on dynamic vertex and index buffers.
    ///
    ///          Details
    ///
    ///          D3D12 and Vulkan back-ends have to perform some work to make data in buffers
    ///          available to draw commands. When a dynamic buffer is mapped, the engine allocates
    ///          new memory and assigns a new GPU address to this buffer. When a draw command is issued,
    ///          this GPU address needs to be used. By default the engine assumes that the CPU may
    ///          map the buffer before any command (to write new transformation matrices for example)
    ///          and that all GPU addresses need to always be refreshed. This is not always the case, 
    ///          and the application may use the flag to inform the engine that the data in the buffer 
    ///          stay intact to avoid extra work.\n
    ///          Note that after a new PSO is bound or an SRB is committed, the engine will always set all
    ///          required buffer addresses/offsets regardless of the flag. The flag will only take effect
    ///          on the second and susbequent draw calls that use the same PSO and SRB.\n
    ///          The flag has no effect in D3D11 and OpenGL backends.
    ///
    ///          Implementation details
    ///         
    ///          Vulkan backend allocates VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC descriptors for all uniform (constant), 
    ///          buffers and VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC descriptors for storage buffers.
    ///          Note that HLSL structured buffers are mapped to read-only storage buffers in SPIRV and RW buffers
    ///          are mapped to RW-storage buffers.
    ///          By default, all dynamic descriptor sets that have dynamic buffers bound are updated every time a draw command is
    ///          issued (see PipelineStateVkImpl::BindDescriptorSetsWithDynamicOffsets). When DRAW_FLAG_DYNAMIC_RESOURCE_BUFFERS_INTACT
    ///          is specified, dynamic descriptor sets are only bound by the first draw command that uses the PSO and the SRB.
    ///          The flag avoids binding descriptors with the same offsets if none of the dynamic offsets have changed.
    ///
    ///          Direct3D12 backend binds constant buffers to root views. By default the engine assumes that virtual GPU addresses 
    ///          of all dynamic buffers may change between the draw commands and always binds dynamic buffers to root views
    ///          (see RootSignature::CommitRootViews). When DRAW_FLAG_DYNAMIC_RESOURCE_BUFFERS_INTACT is set, root views are only bound
    ///          by the first draw command that uses the PSO + SRB pair. The flag avoids setting the same GPU virtual addresses when
    ///          they stay unchanged.
    DRAW_FLAG_DYNAMIC_RESOURCE_BUFFERS_INTACT = 0x08
};
DEFINE_FLAG_ENUM_OPERATORS(DRAW_FLAGS)


/// Defines resource state transition mode performed by various commands.

/// Refer to http://diligentgraphics.com/2018/12/09/resource-state-management/ for detailed explanation
/// of resource state management in Diligent Engine.
DILIGENT_TYPED_ENUM(RESOURCE_STATE_TRANSITION_MODE, Uint8)
{
    /// Perform no state transitions and no state validation. 
    /// Resource states are not accessed (either read or written) by the command.
    RESOURCE_STATE_TRANSITION_MODE_NONE = 0,
    
    /// Transition resources to the states required by the specific command.
    /// Resources in unknown state are ignored.
    ///
    /// \note    Any method that uses this mode may alter the state of the resources it works with.
    ///          As automatic state management is not thread-safe, no other thread is allowed to read
    ///          or write the state of the resources being transitioned. 
    ///          If the application intends to use the same resources in other threads simultaneously, it needs to 
    ///          explicitly manage the states using IDeviceContext::TransitionResourceStates() method.
    ///          Refer to http://diligentgraphics.com/2018/12/09/resource-state-management/ for detailed explanation
    ///          of resource state management in Diligent Engine.
    RESOURCE_STATE_TRANSITION_MODE_TRANSITION,

    /// Do not transition, but verify that states are correct.
    /// No validation is performed if the state is unknown to the engine.
    /// This mode only has effect in debug and development builds. No validation 
    /// is performed in release build.
    ///
    /// \note    Any method that uses this mode will read the state of resources it works with.
    ///          As automatic state management is not thread-safe, no other thread is allowed to alter
    ///          the state of resources being used by the command. It is safe to read these states.
    RESOURCE_STATE_TRANSITION_MODE_VERIFY
};


/// Defines the draw command attributes.

/// This structure is used by IDeviceContext::Draw().
struct DrawAttribs
{
    /// The number of vertices to draw.
    Uint32     NumVertices           DEFAULT_INITIALIZER(0);

    /// Additional flags, see Diligent::DRAW_FLAGS.
    DRAW_FLAGS Flags                 DEFAULT_INITIALIZER(DRAW_FLAG_NONE);

    /// The number of instances to draw. If more than one instance is specified,
    /// instanced draw call will be performed.
    Uint32     NumInstances          DEFAULT_INITIALIZER(1);

    /// LOCATION (or INDEX, but NOT the byte offset) of the first vertex in the
    /// vertex buffer to start reading vertices from.
    Uint32     StartVertexLocation   DEFAULT_INITIALIZER(0);

    /// LOCATION (or INDEX, but NOT the byte offset) in the vertex buffer to start
    /// reading instance data from.
    Uint32     FirstInstanceLocation DEFAULT_INITIALIZER(0);


#if DILIGENT_CPP_INTERFACE
    /// Initializes the structure members with default values.

    /// Default values:
    ///
    /// Member                                   | Default value
    /// -----------------------------------------|--------------------------------------
    /// NumVertices                              | 0
    /// Flags                                    | DRAW_FLAG_NONE
    /// NumInstances                             | 1
    /// StartVertexLocation                      | 0
    /// FirstInstanceLocation                    | 0
    DrawAttribs()noexcept{}

    /// Initializes the structure with user-specified values.
    DrawAttribs(Uint32     _NumVertices,
                DRAW_FLAGS _Flags,
                Uint32     _NumInstances          = 1,
                Uint32     _StartVertexLocation   = 0,
                Uint32     _FirstInstanceLocation = 0)noexcept : 
        NumVertices          {_NumVertices          },
        Flags                {_Flags                },
        NumInstances         {_NumInstances         },
        StartVertexLocation  {_StartVertexLocation  },
        FirstInstanceLocation{_FirstInstanceLocation}
    {}
#endif
};
typedef struct DrawAttribs DrawAttribs;


/// Defines the indexed draw command attributes.

/// This structure is used by IDeviceContext::DrawIndexed().
struct DrawIndexedAttribs
{
    /// The number of indices to draw.
    Uint32     NumIndices            DEFAULT_INITIALIZER(0);

    /// The type of elements in the index buffer.
    /// Allowed values: VT_UINT16 and VT_UINT32.
    VALUE_TYPE IndexType             DEFAULT_INITIALIZER(VT_UNDEFINED);

    /// Additional flags, see Diligent::DRAW_FLAGS.
    DRAW_FLAGS Flags                 DEFAULT_INITIALIZER(DRAW_FLAG_NONE);

    /// Number of instances to draw. If more than one instance is specified,
    /// instanced draw call will be performed.
    Uint32     NumInstances          DEFAULT_INITIALIZER(1);

    /// LOCATION (NOT the byte offset) of the first index in
    /// the index buffer to start reading indices from.
    Uint32     FirstIndexLocation    DEFAULT_INITIALIZER(0);

    /// A constant which is added to each index before accessing the vertex buffer.
    Uint32     BaseVertex            DEFAULT_INITIALIZER(0);

    /// LOCATION (or INDEX, but NOT the byte offset) in the vertex
    /// buffer to start reading instance data from.
    Uint32     FirstInstanceLocation DEFAULT_INITIALIZER(0);


#if DILIGENT_CPP_INTERFACE
    /// Initializes the structure members with default values.

    /// Default values:
    /// Member                                   | Default value
    /// -----------------------------------------|--------------------------------------
    /// NumIndices                               | 0
    /// IndexType                                | VT_UNDEFINED
    /// Flags                                    | DRAW_FLAG_NONE
    /// NumInstances                             | 1
    /// FirstIndexLocation                       | 0
    /// BaseVertex                               | 0
    /// FirstInstanceLocation                    | 0
    DrawIndexedAttribs()noexcept{}

    /// Initializes the structure members with user-specified values.
    DrawIndexedAttribs(Uint32      _NumIndices,
                       VALUE_TYPE  _IndexType,
                       DRAW_FLAGS  _Flags,
                       Uint32      _NumInstances          = 1,
                       Uint32      _FirstIndexLocation    = 0,
                       Uint32      _BaseVertex            = 0,
                       Uint32      _FirstInstanceLocation = 0)noexcept : 
        NumIndices           {_NumIndices           },
        IndexType            {_IndexType            },
        Flags                {_Flags                },
        NumInstances         {_NumInstances         },
        FirstIndexLocation   {_FirstIndexLocation   },
        BaseVertex           {_BaseVertex           },
        FirstInstanceLocation{_FirstInstanceLocation}
    {}
#endif
};
typedef struct DrawIndexedAttribs DrawIndexedAttribs;


/// Defines the indirect draw command attributes.

/// This structure is used by IDeviceContext::DrawIndirect().
struct DrawIndirectAttribs
{
    /// Additional flags, see Diligent::DRAW_FLAGS.
    DRAW_FLAGS Flags                DEFAULT_INITIALIZER(DRAW_FLAG_NONE);

    /// State transition mode for indirect draw arguments buffer.
    RESOURCE_STATE_TRANSITION_MODE IndirectAttribsBufferStateTransitionMode DEFAULT_INITIALIZER(RESOURCE_STATE_TRANSITION_MODE_NONE);

    /// Offset from the beginning of the buffer to the location of draw command attributes.
    Uint32 IndirectDrawArgsOffset   DEFAULT_INITIALIZER(0);
    

#if DILIGENT_CPP_INTERFACE
    /// Initializes the structure members with default values

    /// Default values:
    /// Member                                   | Default value
    /// -----------------------------------------|--------------------------------------
    /// Flags                                    | DRAW_FLAG_NONE
    /// IndirectAttribsBufferStateTransitionMode | RESOURCE_STATE_TRANSITION_MODE_NONE
    /// IndirectDrawArgsOffset                   | 0
    DrawIndirectAttribs()noexcept{}

    /// Initializes the structure members with user-specified values.
    DrawIndirectAttribs(DRAW_FLAGS                     _Flags,
                        RESOURCE_STATE_TRANSITION_MODE _IndirectAttribsBufferStateTransitionMode,
                        Uint32                         _IndirectDrawArgsOffset = 0)noexcept :
        Flags                                   {_Flags                                   },
        IndirectAttribsBufferStateTransitionMode{_IndirectAttribsBufferStateTransitionMode},
        IndirectDrawArgsOffset                  {_IndirectDrawArgsOffset                  }
    {}
#endif
};
typedef struct DrawIndirectAttribs DrawIndirectAttribs;


/// Defines the indexed indirect draw command attributes.

/// This structure is used by IDeviceContext::DrawIndexedIndirect().
struct DrawIndexedIndirectAttribs
{
    /// The type of the elements in the index buffer.
    /// Allowed values: VT_UINT16 and VT_UINT32.
    VALUE_TYPE IndexType            DEFAULT_INITIALIZER(VT_UNDEFINED);

    /// Additional flags, see Diligent::DRAW_FLAGS.
    DRAW_FLAGS Flags                DEFAULT_INITIALIZER(DRAW_FLAG_NONE);

    /// State transition mode for indirect draw arguments buffer.
    RESOURCE_STATE_TRANSITION_MODE IndirectAttribsBufferStateTransitionMode DEFAULT_INITIALIZER(RESOURCE_STATE_TRANSITION_MODE_NONE);

    /// Offset from the beginning of the buffer to the location of draw command attributes.
    Uint32 IndirectDrawArgsOffset        DEFAULT_INITIALIZER(0);


#if DILIGENT_CPP_INTERFACE
    /// Initializes the structure members with default values

    /// Default values:
    /// Member                                   | Default value
    /// -----------------------------------------|--------------------------------------
    /// IndexType                                | VT_UNDEFINED
    /// Flags                                    | DRAW_FLAG_NONE
    /// IndirectAttribsBufferStateTransitionMode | RESOURCE_STATE_TRANSITION_MODE_NONE
    /// IndirectDrawArgsOffset                   | 0
    DrawIndexedIndirectAttribs()noexcept{}

    /// Initializes the structure members with user-specified values.
    DrawIndexedIndirectAttribs(VALUE_TYPE                     _IndexType,
                               DRAW_FLAGS                     _Flags,
                               RESOURCE_STATE_TRANSITION_MODE _IndirectAttribsBufferStateTransitionMode,
                               Uint32                         _IndirectDrawArgsOffset = 0)noexcept : 
        IndexType                               {_IndexType                               },
        Flags                                   {_Flags                                   },
        IndirectAttribsBufferStateTransitionMode{_IndirectAttribsBufferStateTransitionMode},
        IndirectDrawArgsOffset                  {_IndirectDrawArgsOffset                  }
    {}
#endif
};
typedef struct DrawIndexedIndirectAttribs DrawIndexedIndirectAttribs;


/// Defines the mesh draw command attributes.

/// This structure is used by IDeviceContext::DrawMesh().
struct DrawMeshAttribs
{
    /// The number of dispatched groups
    Uint32 ThreadGroupCount DEFAULT_INITIALIZER(1);

    /// Additional flags, see Diligent::DRAW_FLAGS.
    DRAW_FLAGS Flags        DEFAULT_INITIALIZER(DRAW_FLAG_NONE);

#if DILIGENT_CPP_INTERFACE
    /// Initializes the structure members with default values.
    DrawMeshAttribs()noexcept{}

    /// Initializes the structure with user-specified values.
    DrawMeshAttribs(Uint32     _ThreadGroupCount,
                    DRAW_FLAGS _Flags)noexcept :
        ThreadGroupCount {_ThreadGroupCount},
        Flags            {_Flags}
    {}
#endif
};
typedef struct DrawMeshAttribs DrawMeshAttribs;


/// Defines the mesh indirect draw command attributes.

/// This structure is used by IDeviceContext::DrawMeshIndirect().
struct DrawMeshIndirectAttribs
{
    /// Additional flags, see Diligent::DRAW_FLAGS.
    DRAW_FLAGS Flags                DEFAULT_INITIALIZER(DRAW_FLAG_NONE);
    
    /// State transition mode for indirect draw arguments buffer.
    RESOURCE_STATE_TRANSITION_MODE IndirectAttribsBufferStateTransitionMode DEFAULT_INITIALIZER(RESOURCE_STATE_TRANSITION_MODE_NONE);

    /// Offset from the beginning of the buffer to the location of draw command attributes.
    Uint32 IndirectDrawArgsOffset        DEFAULT_INITIALIZER(0);

#if DILIGENT_CPP_INTERFACE
    /// Initializes the structure members with default values

    /// Default values:
    /// Member                                   | Default value
    /// -----------------------------------------|--------------------------------------
    /// Flags                                    | DRAW_FLAG_NONE
    /// IndirectAttribsBufferStateTransitionMode | RESOURCE_STATE_TRANSITION_MODE_NONE
    /// IndirectDrawArgsOffset                   | 0
    DrawMeshIndirectAttribs()noexcept{}

    /// Initializes the structure members with user-specified values.
    DrawMeshIndirectAttribs(DRAW_FLAGS                     _Flags,
                            RESOURCE_STATE_TRANSITION_MODE _IndirectAttribsBufferStateTransitionMode,
                            Uint32                         _IndirectDrawArgsOffset = 0)noexcept : 
        Flags                                   {_Flags                                   },
        IndirectAttribsBufferStateTransitionMode{_IndirectAttribsBufferStateTransitionMode},
        IndirectDrawArgsOffset                  {_IndirectDrawArgsOffset                  }
    {}
#endif
};
typedef struct DrawMeshIndirectAttribs DrawMeshIndirectAttribs;


/// Defines which parts of the depth-stencil buffer to clear.

/// These flags are used by IDeviceContext::ClearDepthStencil().
DILIGENT_TYPED_ENUM(CLEAR_DEPTH_STENCIL_FLAGS, Uint32)
{
    /// Perform no clear.
    CLEAR_DEPTH_FLAG_NONE = 0x00,  

    /// Clear depth part of the buffer.
    CLEAR_DEPTH_FLAG      = 0x01,  

    /// Clear stencil part of the buffer.
    CLEAR_STENCIL_FLAG    = 0x02   
};
DEFINE_FLAG_ENUM_OPERATORS(CLEAR_DEPTH_STENCIL_FLAGS)


/// Describes dispatch command arguments.

/// This structure is used by IDeviceContext::DispatchCompute().
struct DispatchComputeAttribs
{
    Uint32 ThreadGroupCountX DEFAULT_INITIALIZER(1); ///< Number of groups dispatched in X direction.
    Uint32 ThreadGroupCountY DEFAULT_INITIALIZER(1); ///< Number of groups dispatched in Y direction.
    Uint32 ThreadGroupCountZ DEFAULT_INITIALIZER(1); ///< Number of groups dispatched in Z direction.

#if DILIGENT_CPP_INTERFACE
    DispatchComputeAttribs()noexcept{}

    /// Initializes the structure with user-specified values.
    DispatchComputeAttribs(Uint32 GroupsX, Uint32 GroupsY, Uint32 GroupsZ = 1)noexcept :
        ThreadGroupCountX {GroupsX},
        ThreadGroupCountY {GroupsY},
        ThreadGroupCountZ {GroupsZ}
    {}
#endif
};
typedef struct DispatchComputeAttribs DispatchComputeAttribs;


/// Describes dispatch command arguments.

/// This structure is used by IDeviceContext::DispatchComputeIndirect().
struct DispatchComputeIndirectAttribs
{
    /// State transition mode for indirect dispatch attributes buffer.
    RESOURCE_STATE_TRANSITION_MODE IndirectAttribsBufferStateTransitionMode DEFAULT_INITIALIZER(RESOURCE_STATE_TRANSITION_MODE_NONE);

    /// The offset from the beginning of the buffer to the dispatch command arguments.
    Uint32  DispatchArgsByteOffset    DEFAULT_INITIALIZER(0);

#if DILIGENT_CPP_INTERFACE
    DispatchComputeIndirectAttribs()noexcept{}

    /// Initializes the structure with user-specified values.
    explicit
    DispatchComputeIndirectAttribs(RESOURCE_STATE_TRANSITION_MODE StateTransitionMode,
                                   Uint32                         Offset              = 0) :
        IndirectAttribsBufferStateTransitionMode{StateTransitionMode},
        DispatchArgsByteOffset                  {Offset             }
    {}
#endif
};
typedef struct DispatchComputeIndirectAttribs DispatchComputeIndirectAttribs;


/// Describes multi-sampled texture resolve command arguments.

/// This structure is used by IDeviceContext::ResolveTextureSubresource().
struct ResolveTextureSubresourceAttribs
{
    /// Mip level of the source multi-sampled texture to resolve.
    Uint32 SrcMipLevel   DEFAULT_INITIALIZER(0);

    /// Array slice of the source multi-sampled texture to resolve.
    Uint32 SrcSlice      DEFAULT_INITIALIZER(0);

    /// Source texture state transition mode, see Diligent::RESOURCE_STATE_TRANSITION_MODE.
    RESOURCE_STATE_TRANSITION_MODE SrcTextureTransitionMode DEFAULT_INITIALIZER(RESOURCE_STATE_TRANSITION_MODE_NONE);

    /// Mip level of the destination non-multi-sampled texture.
    Uint32 DstMipLevel   DEFAULT_INITIALIZER(0);

    /// Array slice of the destination non-multi-sampled texture.
    Uint32 DstSlice      DEFAULT_INITIALIZER(0);

    /// Destination texture state transition mode, see Diligent::RESOURCE_STATE_TRANSITION_MODE.
    RESOURCE_STATE_TRANSITION_MODE DstTextureTransitionMode DEFAULT_INITIALIZER(RESOURCE_STATE_TRANSITION_MODE_NONE);

    /// If one or both textures are typeless, specifies the type of the typeless texture.
    /// If both texture formats are not typeless, in which case they must be identical, this member must be
    /// either TEX_FORMAT_UNKNOWN, or match this format.
    TEXTURE_FORMAT Format DEFAULT_INITIALIZER(TEX_FORMAT_UNKNOWN);
};
typedef struct ResolveTextureSubresourceAttribs ResolveTextureSubresourceAttribs;


/// Defines allowed flags for IDeviceContext::SetVertexBuffers() function.
DILIGENT_TYPED_ENUM(SET_VERTEX_BUFFERS_FLAGS, Uint8)
{
    /// No extra operations.
    SET_VERTEX_BUFFERS_FLAG_NONE  = 0x00,

    /// Reset the vertex buffers to only the buffers specified in this
    /// call. All buffers previously bound to the pipeline will be unbound.
    SET_VERTEX_BUFFERS_FLAG_RESET = 0x01
};
DEFINE_FLAG_ENUM_OPERATORS(SET_VERTEX_BUFFERS_FLAGS)


/// Describes the viewport.

/// This structure is used by IDeviceContext::SetViewports().
struct Viewport
{
    /// X coordinate of the left boundary of the viewport.
    Float32 TopLeftX    DEFAULT_INITIALIZER(0.f);

    /// Y coordinate of the top boundary of the viewport.
    /// When defining a viewport, DirectX convention is used:
    /// window coordinate systems originates in the LEFT TOP corner
    /// of the screen with Y axis pointing down.
    Float32 TopLeftY    DEFAULT_INITIALIZER(0.f);

    /// Viewport width.
    Float32 Width       DEFAULT_INITIALIZER(0.f);

    /// Viewport Height.
    Float32 Height      DEFAULT_INITIALIZER(0.f);

    /// Minimum depth of the viewport. Ranges between 0 and 1.
    Float32 MinDepth    DEFAULT_INITIALIZER(0.f);

    /// Maximum depth of the viewport. Ranges between 0 and 1.
    Float32 MaxDepth    DEFAULT_INITIALIZER(1.f);

#if DILIGENT_CPP_INTERFACE
    /// Initializes the structure.
    Viewport(Float32 _TopLeftX,     Float32 _TopLeftY,
             Float32 _Width,        Float32 _Height,
             Float32 _MinDepth = 0, Float32 _MaxDepth = 1)noexcept :
        TopLeftX {_TopLeftX},
        TopLeftY {_TopLeftY},
        Width    {_Width   },
        Height   {_Height  },
        MinDepth {_MinDepth},
        MaxDepth {_MaxDepth}
    {}

    Viewport()noexcept{}
#endif
};
typedef struct Viewport Viewport;


/// Describes the rectangle.

/// This structure is used by IDeviceContext::SetScissorRects().
///
/// \remarks When defining a viewport, Windows convention is used:
///          window coordinate systems originates in the LEFT TOP corner
///          of the screen with Y axis pointing down.
struct Rect
{
    Int32 left   DEFAULT_INITIALIZER(0);  ///< X coordinate of the left boundary of the viewport.
    Int32 top    DEFAULT_INITIALIZER(0);  ///< Y coordinate of the top boundary of the viewport.
    Int32 right  DEFAULT_INITIALIZER(0);  ///< X coordinate of the right boundary of the viewport.
    Int32 bottom DEFAULT_INITIALIZER(0);  ///< Y coordinate of the bottom boundary of the viewport.

#if DILIGENT_CPP_INTERFACE
    /// Initializes the structure
    Rect(Int32 _left, Int32 _top, Int32 _right, Int32 _bottom)noexcept : 
        left   {_left  },
        top    {_top   },
        right  {_right },
        bottom {_bottom}
    {}

    Rect()noexcept{}

    bool IsValid() const
    {
        return right > left && bottom > top;
    }
#endif
};
typedef struct Rect Rect;


/// Defines copy texture command attributes.

/// This structure is used by IDeviceContext::CopyTexture().
struct CopyTextureAttribs
{
    /// Source texture to copy data from.
    ITexture*                      pSrcTexture              DEFAULT_INITIALIZER(nullptr);

    /// Mip level of the source texture to copy data from.
    Uint32                         SrcMipLevel              DEFAULT_INITIALIZER(0);

    /// Array slice of the source texture to copy data from. Must be 0 for non-array textures.
    Uint32                         SrcSlice                 DEFAULT_INITIALIZER(0);
    
    /// Source region to copy. Use nullptr to copy the entire subresource.
    const Box*                     pSrcBox                  DEFAULT_INITIALIZER(nullptr);
    
    /// Source texture state transition mode (see Diligent::RESOURCE_STATE_TRANSITION_MODE).
    RESOURCE_STATE_TRANSITION_MODE SrcTextureTransitionMode DEFAULT_INITIALIZER(RESOURCE_STATE_TRANSITION_MODE_NONE);

    /// Destination texture.
    ITexture*                      pDstTexture              DEFAULT_INITIALIZER(nullptr);

    /// Destination mip level.
    Uint32                         DstMipLevel              DEFAULT_INITIALIZER(0);

    /// Destination array slice. Must be 0 for non-array textures.
    Uint32                         DstSlice                 DEFAULT_INITIALIZER(0);

    /// X offset on the destination subresource.
    Uint32                         DstX                     DEFAULT_INITIALIZER(0);

    /// Y offset on the destination subresource.
    Uint32                         DstY                     DEFAULT_INITIALIZER(0);

    /// Z offset on the destination subresource
    Uint32                         DstZ                     DEFAULT_INITIALIZER(0);

    /// Destination texture state transition mode (see Diligent::RESOURCE_STATE_TRANSITION_MODE).
    RESOURCE_STATE_TRANSITION_MODE DstTextureTransitionMode DEFAULT_INITIALIZER(RESOURCE_STATE_TRANSITION_MODE_NONE);


#if DILIGENT_CPP_INTERFACE
    CopyTextureAttribs()noexcept{}

    CopyTextureAttribs(ITexture*                      _pSrcTexture,
                       RESOURCE_STATE_TRANSITION_MODE _SrcTextureTransitionMode,
                       ITexture*                      _pDstTexture,
                       RESOURCE_STATE_TRANSITION_MODE _DstTextureTransitionMode)noexcept :
        pSrcTexture             {_pSrcTexture             },
        SrcTextureTransitionMode{_SrcTextureTransitionMode},
        pDstTexture             {_pDstTexture             },
        DstTextureTransitionMode{_DstTextureTransitionMode}
    {}
#endif
};
typedef struct CopyTextureAttribs CopyTextureAttribs;


/// BeginRenderPass command attributes.

/// This structure is used by IDeviceContext::BeginRenderPass().
struct BeginRenderPassAttribs
{
    /// Render pass to begin.
    IRenderPass*    pRenderPass     DEFAULT_INITIALIZER(nullptr);

    /// Framebuffer containing the attachments that are used with the render pass.
    IFramebuffer*   pFramebuffer    DEFAULT_INITIALIZER(nullptr);

    /// The number of elements in pClearValues array.
    Uint32 ClearValueCount          DEFAULT_INITIALIZER(0);

    /// A pointer to an array of ClearValueCount OptimizedClearValue structures that contains
    /// clear values for each attachment, if the attachment uses a LoadOp value of ATTACHMENT_LOAD_OP_CLEAR
    /// or if the attachment has a depth/stencil format and uses a StencilLoadOp value of ATTACHMENT_LOAD_OP_CLEAR.
    /// The array is indexed by attachment number. Only elements corresponding to cleared attachments are used.
    /// Other elements of pClearValues are ignored.
    OptimizedClearValue* pClearValues   DEFAULT_INITIALIZER(nullptr);

    /// Framebuffer attachments state transition mode before the render pass begins.

    /// This parameter also indicates how attachment states should be handled when
    /// transitioning between subpasses as well as after the render pass ends.
    /// When RESOURCE_STATE_TRANSITION_MODE_TRANSITION is used, attachment states will be
    /// updated so that they match the state in the current subpass as well as the final states
    /// specified by the render pass when the pass ends.
    /// Note that resources are always transitioned. The flag only indicates if the internal
    /// state variables should be updated.
    /// When RESOURCE_STATE_TRANSITION_MODE_NONE or RESOURCE_STATE_TRANSITION_MODE_VERIFY is used,
    /// internal state variables are not updated and it is the application responsibility to set them
    /// manually to match the actual states.
    RESOURCE_STATE_TRANSITION_MODE StateTransitionMode DEFAULT_INITIALIZER(RESOURCE_STATE_TRANSITION_MODE_NONE);
};
typedef struct BeginRenderPassAttribs BeginRenderPassAttribs;

#define DILIGENT_INTERFACE_NAME IDeviceContext
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IDeviceContextInclusiveMethods  \
    IObjectInclusiveMethods;            \
    IDeviceContextMethods DeviceContext

/// Device context interface.

/// \remarks Device context keeps strong references to all objects currently bound to 
///          the pipeline: buffers, states, samplers, shaders, etc.
///          The context also keeps strong reference to the device and
///          the swap chain.
DILIGENT_BEGIN_INTERFACE(IDeviceContext, IObject)
{
    /// Sets the pipeline state.

    /// \param [in] pPipelineState - Pointer to IPipelineState interface to bind to the context.
    VIRTUAL void METHOD(SetPipelineState)(THIS_
                                          IPipelineState* pPipelineState) PURE;


    /// Transitions shader resources to the states required by Draw or Dispatch command.
    ///
    /// \param [in] pPipelineState         - Pipeline state object that was used to create the shader resource binding.
    /// \param [in] pShaderResourceBinding - Shader resource binding whose resources will be transitioned.
    ///
    /// \remarks This method explicitly transitiones all resources except ones in unknown state to the states required 
    ///          by Draw or Dispatch command.
    ///          If this method was called, there is no need to use Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION
    ///          when calling IDeviceContext::CommitShaderResources()
    ///
    /// \remarks Resource state transitioning is not thread safe. As the method may alter the states 
    ///          of resources referenced by the shader resource binding, no other thread is allowed to read or 
    ///          write these states.
    ///
    ///          If the application intends to use the same resources in other threads simultaneously, it needs to 
    ///          explicitly manage the states using IDeviceContext::TransitionResourceStates() method.
    ///          Refer to http://diligentgraphics.com/2018/12/09/resource-state-management/ for detailed explanation
    ///          of resource state management in Diligent Engine.
    VIRTUAL void METHOD(TransitionShaderResources)(THIS_ 
                                                   IPipelineState*         pPipelineState, 
                                                   IShaderResourceBinding* pShaderResourceBinding) PURE;

    /// Commits shader resources to the device context.

    /// \param [in] pShaderResourceBinding - Shader resource binding whose resources will be committed.
    ///                                      If pipeline state contains no shader resources, this parameter
    ///                                      can be null.
    /// \param [in] StateTransitionMode    - State transition mode (see Diligent::RESOURCE_STATE_TRANSITION_MODE).
    ///
    /// \remarks Pipeline state object that was used to create the shader resource binding must be bound 
    ///          to the pipeline when CommitShaderResources() is called. If no pipeline state object is bound
    ///          or the pipeline state object does not match the shader resource binding, the method will fail.\n
    ///          If Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION mode is used,
    ///          the engine will also transition all shader resources to required states. If the flag
    ///          is not set, it is assumed that all resources are already in correct states.\n
    ///          Resources can be explicitly transitioned to required states by calling 
    ///          IDeviceContext::TransitionShaderResources() or IDeviceContext::TransitionResourceStates().\n
    ///
    /// \remarks Automatic resource state transitioning is not thread-safe.
    ///
    ///          - If Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION mode is used, the method may alter the states 
    ///            of resources referenced by the shader resource binding and no other thread is allowed to read or write these states.
    ///
    ///          - If Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY mode is used, the method will read the states, so no other thread
    ///            should alter the states by calling any of the methods that use Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION mode.
    ///            It is safe for other threads to read the states.
    ///
    ///          - If Diligent::RESOURCE_STATE_TRANSITION_MODE_NONE mode is used, the method does not access the states of resources.
    ///
    ///          If the application intends to use the same resources in other threads simultaneously, it should manage the states
    ///          manually by setting the state to Diligent::RESOURCE_STATE_UNKNOWN (which will disable automatic state 
    ///          management) using IBuffer::SetState() or ITexture::SetState() and explicitly transitioning the states with 
    ///          IDeviceContext::TransitionResourceStates().
    ///          Refer to http://diligentgraphics.com/2018/12/09/resource-state-management/ for detailed explanation
    ///          of resource state management in Diligent Engine.
    ///
    ///          If an application calls any method that changes the state of any resource after it has been committed, the
    ///          application is responsible for transitioning the resource back to correct state using one of the available methods
    ///          before issuing the next draw or dispatch command.
    VIRTUAL void METHOD(CommitShaderResources)(THIS_
                                               IShaderResourceBinding*        pShaderResourceBinding,
                                               RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) PURE;

    /// Sets the stencil reference value.

    /// \param [in] StencilRef - Stencil reference value.
    VIRTUAL void METHOD(SetStencilRef)(THIS_
                                       Uint32 StencilRef) PURE;

    
    /// \param [in] pBlendFactors - Array of four blend factors, one for each RGBA component. 
    ///                             Theses factors are used if the blend state uses one of the 
    ///                             Diligent::BLEND_FACTOR_BLEND_FACTOR or 
    ///                             Diligent::BLEND_FACTOR_INV_BLEND_FACTOR 
    ///                             blend factors. If nullptr is provided,
    ///                             default blend factors array {1,1,1,1} will be used.
    VIRTUAL void METHOD(SetBlendFactors)(THIS_
                                         const float* pBlendFactors DEFAULT_VALUE(nullptr)) PURE;


    /// Binds vertex buffers to the pipeline.

    /// \param [in] StartSlot           - The first input slot for binding. The first vertex buffer is 
    ///                                   explicitly bound to the start slot; each additional vertex buffer 
    ///                                   in the array is implicitly bound to each subsequent input slot. 
    /// \param [in] NumBuffersSet       - The number of vertex buffers in the array.
    /// \param [in] ppBuffers           - A pointer to an array of vertex buffers. 
    ///                                   The buffers must have been created with the Diligent::BIND_VERTEX_BUFFER flag.
    /// \param [in] pOffsets            - Pointer to an array of offset values; one offset value for each buffer 
    ///                                   in the vertex-buffer array. Each offset is the number of bytes between 
    ///                                   the first element of a vertex buffer and the first element that will be 
    ///                                   used. If this parameter is nullptr, zero offsets for all buffers will be used.
    /// \param [in] StateTransitionMode - State transition mode for buffers being set (see Diligent::RESOURCE_STATE_TRANSITION_MODE).
    /// \param [in] Flags               - Additional flags. See Diligent::SET_VERTEX_BUFFERS_FLAGS for a list of allowed values.
    ///                                   
    /// \remarks The device context keeps strong references to all bound vertex buffers.
    ///          Thus a buffer cannot be released until it is unbound from the context.\n
    ///          It is suggested to specify Diligent::SET_VERTEX_BUFFERS_FLAG_RESET flag
    ///          whenever possible. This will assure that no buffers from previous draw calls
    ///          are bound to the pipeline.
    ///
    /// \remarks When StateTransitionMode is Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, the method will 
    ///          transition all buffers in known states to Diligent::RESOURCE_STATE_VERTEX_BUFFER. Resource state 
    ///          transitioning is not thread safe, so no other thread is allowed to read or write the states of 
    ///          these buffers.
    ///
    ///          If the application intends to use the same resources in other threads simultaneously, it needs to 
    ///          explicitly manage the states using IDeviceContext::TransitionResourceStates() method.
    ///          Refer to http://diligentgraphics.com/2018/12/09/resource-state-management/ for detailed explanation
    ///          of resource state management in Diligent Engine.
    VIRTUAL void METHOD(SetVertexBuffers)(THIS_
                                          Uint32                         StartSlot, 
                                          Uint32                         NumBuffersSet, 
                                          IBuffer**                      ppBuffers, 
                                          Uint32*                        pOffsets,
                                          RESOURCE_STATE_TRANSITION_MODE StateTransitionMode,
                                          SET_VERTEX_BUFFERS_FLAGS       Flags) PURE;


    /// Invalidates the cached context state.

    /// This method should be called by an application to invalidate 
    /// internal cached states.
    VIRTUAL void METHOD(InvalidateState)(THIS) PURE;


    /// Binds an index buffer to the pipeline.
    
    /// \param [in] pIndexBuffer        - Pointer to the index buffer. The buffer must have been created 
    ///                                   with the Diligent::BIND_INDEX_BUFFER flag.
    /// \param [in] ByteOffset          - Offset from the beginning of the buffer to 
    ///                                   the start of index data.
    /// \param [in] StateTransitionMode - State transiton mode for the index buffer to bind (see Diligent::RESOURCE_STATE_TRANSITION_MODE).
    ///
    /// \remarks The device context keeps strong reference to the index buffer.
    ///          Thus an index buffer object cannot be released until it is unbound 
    ///          from the context.
    ///
    /// \remarks When StateTransitionMode is Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, the method will 
    ///          transition the buffer to Diligent::RESOURCE_STATE_INDEX_BUFFER (if its state is not unknown). Resource 
    ///          state transitioning is not thread safe, so no other thread is allowed to read or write the state of 
    ///          the buffer.
    ///
    ///          If the application intends to use the same resource in other threads simultaneously, it needs to 
    ///          explicitly manage the states using IDeviceContext::TransitionResourceStates() method.
    ///          Refer to http://diligentgraphics.com/2018/12/09/resource-state-management/ for detailed explanation
    ///          of resource state management in Diligent Engine.
    VIRTUAL void METHOD(SetIndexBuffer)(THIS_
                                        IBuffer*                       pIndexBuffer,
                                        Uint32                         ByteOffset,
                                        RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) PURE;


    /// Sets an array of viewports.

    /// \param [in] NumViewports - Number of viewports to set.
    /// \param [in] pViewports   - An array of Viewport structures describing the viewports to bind.
    /// \param [in] RTWidth      - Render target width. If 0 is provided, width of the currently bound render target will be used.
    /// \param [in] RTHeight     - Render target height. If 0 is provided, height of the currently bound render target will be used.
    ///
    /// \remarks
    /// DirectX and OpenGL use different window coordinate systems. In DirectX, the coordinate system origin
    /// is in the left top corner of the screen with Y axis pointing down. In OpenGL, the origin
    /// is in the left bottom corener of the screen with Y axis pointing up. Render target size is 
    /// required to convert viewport from DirectX to OpenGL coordinate system if OpenGL device is used.\n\n
    /// All viewports must be set atomically as one operation. Any viewports not 
    /// defined by the call are disabled.\n\n
    /// You can set the viewport size to match the currently bound render target using the
    /// following call:
    ///
    ///     pContext->SetViewports(1, nullptr, 0, 0);
    VIRTUAL void METHOD(SetViewports)(THIS_
                                      Uint32          NumViewports,
                                      const Viewport* pViewports, 
                                      Uint32          RTWidth, 
                                      Uint32          RTHeight) PURE;


    /// Sets active scissor rects.

    /// \param [in] NumRects - Number of scissor rectangles to set.
    /// \param [in] pRects   - An array of Rect structures describing the scissor rectangles to bind.
    /// \param [in] RTWidth  - Render target width. If 0 is provided, width of the currently bound render target will be used.
    /// \param [in] RTHeight - Render target height. If 0 is provided, height of the currently bound render target will be used.
    ///
    /// \remarks
    /// DirectX and OpenGL use different window coordinate systems. In DirectX, the coordinate system origin
    /// is in the left top corner of the screen with Y axis pointing down. In OpenGL, the origin
    /// is in the left bottom corener of the screen with Y axis pointing up. Render target size is 
    /// required to convert viewport from DirectX to OpenGL coordinate system if OpenGL device is used.\n\n
    /// All scissor rects must be set atomically as one operation. Any rects not 
    /// defined by the call are disabled.
    VIRTUAL void METHOD(SetScissorRects)(THIS_
                                         Uint32      NumRects,
                                         const Rect* pRects,
                                         Uint32      RTWidth,
                                         Uint32      RTHeight) PURE;


    /// Binds one or more render targets and the depth-stencil buffer to the context. It also
    /// sets the viewport to match the first non-null render target or depth-stencil buffer.

    /// \param [in] NumRenderTargets    - Number of render targets to bind.
    /// \param [in] ppRenderTargets     - Array of pointers to ITextureView that represent the render 
    ///                                   targets to bind to the device. The type of each view in the 
    ///                                   array must be Diligent::TEXTURE_VIEW_RENDER_TARGET.
    /// \param [in] pDepthStencil       - Pointer to the ITextureView that represents the depth stencil to 
    ///                                   bind to the device. The view type must be
    ///                                   Diligent::TEXTURE_VIEW_DEPTH_STENCIL.
    /// \param [in] StateTransitionMode - State transition mode of the render targets and depth stencil buffer being set (see Diligent::RESOURCE_STATE_TRANSITION_MODE).
    /// 
    /// \remarks     The device context will keep strong references to all bound render target 
    ///              and depth-stencil views. Thus these views (and consequently referenced textures) 
    ///              cannot be released until they are unbound from the context.\n
    ///              Any render targets not defined by this call are set to nullptr.\n\n
    ///
    /// \remarks When StateTransitionMode is Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, the method will 
    ///          transition all render targets in known states to Diligent::RESOURCE_STATE_REDER_TARGET,
    ///          and the depth-stencil buffer to Diligent::RESOURCE_STATE_DEPTH_WRITE state.
    ///          Resource state transitioning is not thread safe, so no other thread is allowed to read or write 
    ///          the states of resources used by the command.
    ///
    ///          If the application intends to use the same resource in other threads simultaneously, it needs to 
    ///          explicitly manage the states using IDeviceContext::TransitionResourceStates() method.
    ///          Refer to http://diligentgraphics.com/2018/12/09/resource-state-management/ for detailed explanation
    ///          of resource state management in Diligent Engine.
    VIRTUAL void METHOD(SetRenderTargets)(THIS_
                                          Uint32                         NumRenderTargets,
                                          ITextureView*                  ppRenderTargets[],
                                          ITextureView*                  pDepthStencil,
                                          RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) PURE;


    /// Begins a new render pass.

    /// \param [in] Attribs - The command attributes, see Diligent::BeginRenderPassAttribs for details.
    VIRTUAL void METHOD(BeginRenderPass)(THIS_
                                         const BeginRenderPassAttribs REF Attribs) PURE;
    

    /// Transitions to the next subpass in the render pass instance.
    VIRTUAL void METHOD(NextSubpass)(THIS) PURE;


    /// Ends current render pass.
    VIRTUAL void METHOD(EndRenderPass)(THIS) PURE;


    /// Executes a draw command.

    /// \param [in] Attribs - Draw command attributes, see Diligent::DrawAttribs for details.
    ///
    /// \remarks  If Diligent::DRAW_FLAG_VERIFY_STATES flag is set, the method reads the state of vertex
    ///           buffers, so no other threads are allowed to alter the states of the same resources.
    ///           It is OK to read these states.
    ///          
    ///           If the application intends to use the same resources in other threads simultaneously, it needs to 
    ///           explicitly manage the states using IDeviceContext::TransitionResourceStates() method.
    VIRTUAL void METHOD(Draw)(THIS_
                              const DrawAttribs REF Attribs) PURE;


    /// Executes an indexed draw command.

    /// \param [in] Attribs - Draw command attributes, see Diligent::DrawIndexedAttribs for details.
    ///
    /// \remarks  If Diligent::DRAW_FLAG_VERIFY_STATES flag is set, the method reads the state of vertex/index
    ///           buffers, so no other threads are allowed to alter the states of the same resources.
    ///           It is OK to read these states.
    ///          
    ///           If the application intends to use the same resources in other threads simultaneously, it needs to 
    ///           explicitly manage the states using IDeviceContext::TransitionResourceStates() method.
    VIRTUAL void METHOD(DrawIndexed)(THIS_
                                     const DrawIndexedAttribs REF Attribs) PURE;


    /// Executes an indirect draw command.

    /// \param [in] Attribs        - Structure describing the command attributes, see Diligent::DrawIndirectAttribs for details.
    /// \param [in] pAttribsBuffer - Pointer to the buffer, from which indirect draw attributes will be read.
    ///                              The buffer must contain the following arguments at the specified offset:
    ///                                  Uint32 NumVertices;
    ///                                  Uint32 NumInstances;
    ///                                  Uint32 StartVertexLocation;
    ///                                  Uint32 FirstInstanceLocation;
    ///
    /// \remarks  If IndirectAttribsBufferStateTransitionMode member is Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
    ///           the method may transition the state of the indirect draw arguments buffer. This is not a thread safe operation, 
    ///           so no other thread is allowed to read or write the state of the buffer.
    ///
    ///           If Diligent::DRAW_FLAG_VERIFY_STATES flag is set, the method reads the state of vertex/index
    ///           buffers, so no other threads are allowed to alter the states of the same resources.
    ///           It is OK to read these states.
    ///          
    ///           If the application intends to use the same resources in other threads simultaneously, it needs to 
    ///           explicitly manage the states using IDeviceContext::TransitionResourceStates() method.
    VIRTUAL void METHOD(DrawIndirect)(THIS_
                                      const DrawIndirectAttribs REF Attribs,
                                      IBuffer*                      pAttribsBuffer) PURE;


    /// Executes an indexed indirect draw command.

    /// \param [in] Attribs        - Structure describing the command attributes, see Diligent::DrawIndexedIndirectAttribs for details.
    /// \param [in] pAttribsBuffer - Pointer to the buffer, from which indirect draw attributes will be read.
    ///                              The buffer must contain the following arguments at the specified offset:
    ///                                  Uint32 NumIndices;
    ///                                  Uint32 NumInstances;
    ///                                  Uint32 FirstIndexLocation;
    ///                                  Uint32 BaseVertex;
    ///                                  Uint32 FirstInstanceLocation
    ///
    /// \remarks  If IndirectAttribsBufferStateTransitionMode member is Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
    ///           the method may transition the state of the indirect draw arguments buffer. This is not a thread safe operation, 
    ///           so no other thread is allowed to read or write the state of the buffer.
    ///
    ///           If Diligent::DRAW_FLAG_VERIFY_STATES flag is set, the method reads the state of vertex/index
    ///           buffers, so no other threads are allowed to alter the states of the same resources.
    ///           It is OK to read these states.
    ///          
    ///           If the application intends to use the same resources in other threads simultaneously, it needs to 
    ///           explicitly manage the states using IDeviceContext::TransitionResourceStates() method.
    VIRTUAL void METHOD(DrawIndexedIndirect)(THIS_
                                             const DrawIndexedIndirectAttribs REF Attribs,
                                             IBuffer*                             pAttribsBuffer) PURE;
    

    /// Executes a mesh draw command.
    
    /// \param [in] Attribs - Draw command attributes, see Diligent::DrawMeshAttribs for details.
    /// 
    /// \remarks  For compatibility between Direct3D12 and Vulkan, only a single work group dimension is used.
    ///           Also in the shader, 'numthreads' and 'local_size' attributes must define only the first dimension,
    ///           for example: '[numthreads(ThreadCount, 1, 1)]' or 'layout(local_size_x = ThreadCount) in'.
    VIRTUAL void METHOD(DrawMesh)(THIS_
                                  const DrawMeshAttribs REF Attribs) PURE;
    

    /// Executes an mesh indirect draw command.
    
    /// \param [in] Attribs        - Structure describing the command attributes, see Diligent::DrawMeshIndirectAttribs for details.
    /// \param [in] pAttribsBuffer - Pointer to the buffer, from which indirect draw attributes will be read.
    ///                              The buffer must contain the following arguments at the specified offset:
    ///                                Direct3D12:
    ///                                     Uint32 ThreadGroupCountX;
    ///                                     Uint32 ThreadGroupCountY;
    ///                                     Uint32 ThreadGroupCountZ;
    ///                                Vulkan:
    ///                                     Uint32 TaskCount;
    ///                                     Uint32 FirstTask;
    /// 
    /// \remarks  For compatibility between Direct3D12 and Vulkan and with direct call (DrawMesh) use the first element in the structure,
    ///           for example: Direct3D12 {TaskCount, 1, 1}, Vulkan {TaskCount, 0}.
    /// 
    /// \remarks  If IndirectAttribsBufferStateTransitionMode member is Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
    ///           the method may transition the state of the indirect draw arguments buffer. This is not a thread safe operation, 
    ///           so no other thread is allowed to read or write the state of the buffer.
    /// 
    ///           If the application intends to use the same resources in other threads simultaneously, it needs to 
    ///           explicitly manage the states using IDeviceContext::TransitionResourceStates() method.
    VIRTUAL void METHOD(DrawMeshIndirect)(THIS_
                                          const DrawMeshIndirectAttribs REF Attribs,
                                          IBuffer*                          pAttribsBuffer) PURE;


    /// Executes a dispatch compute command.
    
    /// \param [in] Attribs - Dispatch command attributes, see Diligent::DispatchComputeAttribs for details.
    VIRTUAL void METHOD(DispatchCompute)(THIS_
                                         const DispatchComputeAttribs REF Attribs) PURE;


    /// Executes an indirect dispatch compute command.
    
    /// \param [in] Attribs        - The command attributes, see Diligent::DispatchComputeIndirectAttribs for details.
    /// \param [in] pAttribsBuffer - Pointer to the buffer containing indirect dispatch attributes.
    ///                              The buffer must contain the following arguments at the specified offset:
    ///                                 Uint32 ThreadGroupCountX;
    ///                                 Uint32 ThreadGroupCountY;
    ///                                 Uint32 ThreadGroupCountZ;
    ///
    /// \remarks  If IndirectAttribsBufferStateTransitionMode member is Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
    ///           the method may transition the state of indirect dispatch arguments buffer. This is not a thread safe operation, 
    ///           so no other thread is allowed to read or write the state of the same resource.
    ///          
    ///           If the application intends to use the same resources in other threads simultaneously, it needs to 
    ///           explicitly manage the states using IDeviceContext::TransitionResourceStates() method.
    VIRTUAL void METHOD(DispatchComputeIndirect)(THIS_
                                                 const DispatchComputeIndirectAttribs REF Attribs,
                                                 IBuffer*                                 pAttribsBuffer) PURE;


    /// Clears a depth-stencil view.
    
    /// \param [in] pView               - Pointer to ITextureView interface to clear. The view type must be 
    ///                                   Diligent::TEXTURE_VIEW_DEPTH_STENCIL.
    /// \param [in] StateTransitionMode - state transition mode of the depth-stencil buffer to clear.
    /// \param [in] ClearFlags          - Idicates which parts of the buffer to clear, see Diligent::CLEAR_DEPTH_STENCIL_FLAGS.
    /// \param [in] fDepth              - Value to clear depth part of the view with.
    /// \param [in] Stencil             - Value to clear stencil part of the view with.
    ///
    /// \remarks The full extent of the view is always cleared. Viewport and scissor settings are not applied.
    /// \note The depth-stencil view must be bound to the pipeline for clear operation to be performed.
    ///
    /// \remarks When StateTransitionMode is Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, the method will 
    ///          transition the state of the texture to the state required by clear operation. 
    ///          In Direct3D12, this satate is always Diligent::RESOURCE_STATE_DEPTH_WRITE, however in Vulkan
    ///          the state depends on whether the depth buffer is bound to the pipeline.
    ///
    ///          Resource state transitioning is not thread safe, so no other thread is allowed to read or write 
    ///          the state of resources used by the command.
    ///          Refer to http://diligentgraphics.com/2018/12/09/resource-state-management/ for detailed explanation
    ///          of resource state management in Diligent Engine.
    VIRTUAL void METHOD(ClearDepthStencil)(THIS_
                                           ITextureView*                  pView,
                                           CLEAR_DEPTH_STENCIL_FLAGS      ClearFlags,
                                           float                          fDepth,
                                           Uint8                          Stencil,
                                           RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) PURE;


    /// Clears a render target view

    /// \param [in] pView               - Pointer to ITextureView interface to clear. The view type must be 
    ///                                   Diligent::TEXTURE_VIEW_RENDER_TARGET.
    /// \param [in] RGBA                - A 4-component array that represents the color to fill the render target with.
    ///                                   If nullptr is provided, the default array {0,0,0,0} will be used.
    /// \param [in] StateTransitionMode - Defines required state transitions (see Diligent::RESOURCE_STATE_TRANSITION_MODE)
    ///
    /// \remarks The full extent of the view is always cleared. Viewport and scissor settings are not applied.
    ///
    ///          The render target view must be bound to the pipeline for clear operation to be performed in OpenGL backend.
    ///
    /// \remarks When StateTransitionMode is Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, the method will 
    ///          transition the texture to the state required by the command. Resource state transitioning is not 
    ///          thread safe, so no other thread is allowed to read or write the states of the same textures.
    ///
    ///          If the application intends to use the same resource in other threads simultaneously, it needs to 
    ///          explicitly manage the states using IDeviceContext::TransitionResourceStates() method.
    ///
    /// \note    In D3D12 backend clearing render targets requires textures to always be transitioned to 
    ///          Diligent::RESOURCE_STATE_RENDER_TARGET state. In Vulkan backend however this depends on whether a 
    ///          render pass has been started. To clear render target outside of a render pass, the texture must be transitioned to
    ///          Diligent::RESOURCE_STATE_COPY_DEST state. Inside a render pass it must be in Diligent::RESOURCE_STATE_RENDER_TARGET
    ///          state. When using Diligent::RESOURCE_STATE_TRANSITION_TRANSITION mode, the engine takes care of proper
    ///          resource state transition, otherwise it is the responsibility of the application.
    VIRTUAL void METHOD(ClearRenderTarget)(THIS_
                                           ITextureView*                  pView,
                                           const float*                   RGBA, 
                                           RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) PURE;


    /// Finishes recording commands and generates a command list.
    
    /// \param [out] ppCommandList - Memory location where pointer to the recorded command list will be written.
    VIRTUAL void METHOD(FinishCommandList)(THIS_
                                           ICommandList** ppCommandList) PURE;


    /// Executes recorded commands in a command list.

    /// \param [in] pCommandList - Pointer to the command list to executre.
    /// \remarks After command list is executed, it is no longer valid and should be released.
    VIRTUAL void METHOD(ExecuteCommandList)(THIS_
                                            ICommandList* pCommandList) PURE;


    /// Tells the GPU to set a fence to a specified value after all previous work has completed.

    /// \note The method does not flush the context (an application can do this explcitly if needed)
    ///       and the fence will be signaled only when the command context is flushed next time.
    ///       If an application needs to wait for the fence in a loop, it must flush the context
    ///       after signalling the fence.
    ///
    /// \param [in] pFence - The fence to signal
    /// \param [in] Value  - The value to set the fence to. This value must be greater than the
    ///                      previously signaled value on the same fence.
    VIRTUAL void METHOD(SignalFence)(THIS_
                                     IFence*    pFence,
                                     Uint64     Value) PURE;


    /// Waits until the specified fence reaches or exceeds the specified value, on the host.

    /// \note The method blocks the execution of the calling thread until the wait is complete.
    ///
    /// \param [in] pFence       - The fence to wait.
    /// \param [in] Value        - The value that the context is waiting for the fence to reach.
    /// \param [in] FlushContext - Whether to flush the commands in the context before initiating the wait.
    ///
    /// \remarks    Wait is only allowed for immediate contexts.\n
    ///             When FlushContext is true, the method flushes the context before initiating the wait 
    ///             (see IDeviceContext::Flush()), so an application must explicitly reset the PSO and 
    ///             bind all required shader resources after waiting for the fence.\n
    ///             If FlushContext is false, the commands preceding the fence (including signaling the fence itself)
    ///             may not have been submitted to the GPU and the method may never return.  If an application does 
    ///             not explicitly flush the context, it should typically set FlushContext to true.\n
    ///             If the value the context is waiting for has never been signaled, the method
    ///             may never return.\n
    ///             The fence can only be waited for from the same context it has
    ///             previously been signaled.
    VIRTUAL void METHOD(WaitForFence)(THIS_
                                      IFence*   pFence,
                                      Uint64    Value,
                                      bool      FlushContext) PURE;


    /// Submits all outstanding commands for execution to the GPU and waits until they are complete.

    /// \note The method blocks the execution of the calling thread until the wait is complete.
    ///
    /// \remarks    Only immediate contexts can be idled.\n
    ///             The methods implicitly flushes the context (see IDeviceContext::Flush()), so an 
    ///             application must explicitly reset the PSO and bind all required shader resources after 
    ///             idling the context.\n
    VIRTUAL void METHOD(WaitForIdle)(THIS) PURE;


    /// Marks the beginning of a query.

    /// \param [in] pQuery - A pointer to a query object.
    ///
    /// \remarks    Only immediate contexts can begin a query.
    ///
    ///             Vulkan requires that a query must either begin and end inside the same
    ///             subpass of a render pass instance, or must both begin and end outside of
    ///             a render pass instance. This means that an application must either begin
    ///             and end a query while preserving render targets, or begin it when no render
    ///             targets are bound to the context. In the latter case the engine will automaticaly
    ///             end the render pass, if needed, when the query is ended.
    ///             Also note that resource transitions must be performed outside of a render pass,
    ///             and may thus require ending current render pass.
    ///             To explicitly end current render pass, call
    ///             SetRenderTargets(0, nullptr, nullptr, RESOURCE_STATE_TRANSITION_MODE_NONE).
    ///
    /// \warning    OpenGL and Vulkan do not support nested queries of the same type.
    VIRTUAL void METHOD(BeginQuery)(THIS_
                                    IQuery* pQuery) PURE;


    /// Marks the end of a query.

    /// \param [in] pQuery - A pointer to a query object.
    ///
    /// \remarks    A query must be ended by the same context that began it.
    ///
    ///             In Direct3D12 and Vulkan, queries (except for timestamp queries)
    ///             cannot span command list boundaries, so the engine will never flush
    ///             the context even if the number of commands exceeds the user-specified limit
    ///             when there is an active query.
    ///             It is an error to explicitly flush the context while a query is active. 
    ///
    ///             All queries must be ended when IDeviceContext::FinishFrame() is called.
    VIRTUAL void METHOD(EndQuery)(THIS_
                                  IQuery* pQuery) PURE;


    /// Submits all pending commands in the context for execution to the command queue.

    /// \remarks    Only immediate contexts can be flushed.\n
    ///             Internally the method resets the state of the current command list/buffer.
    ///             When the next draw command is issued, the engine will restore all states 
    ///             (rebind render targets and depth-stencil buffer as well as index and vertex buffers,
    ///             restore viewports and scissor rects, etc.) except for the pipeline state and shader resource
    ///             bindings. An application must explicitly reset the PSO and bind all required shader 
    ///             resources after flushing the context.
    VIRTUAL void METHOD(Flush)(THIS) PURE;


    /// Updates the data in the buffer.

    /// \param [in] pBuffer             - Pointer to the buffer to updates.
    /// \param [in] Offset              - Offset in bytes from the beginning of the buffer to the update region.
    /// \param [in] Size                - Size in bytes of the data region to update.
    /// \param [in] pData               - Pointer to the data to write to the buffer.
    /// \param [in] StateTransitionMode - Buffer state transition mode (see Diligent::RESOURCE_STATE_TRANSITION_MODE)
    VIRTUAL void METHOD(UpdateBuffer)(THIS_
                                      IBuffer*                       pBuffer,
                                      Uint32                         Offset,
                                      Uint32                         Size,
                                      const void*                    pData,
                                      RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) PURE;


    /// Copies the data from one buffer to another.

    /// \param [in] pSrcBuffer              - Source buffer to copy data from.
    /// \param [in] SrcOffset               - Offset in bytes from the beginning of the source buffer to the beginning of data to copy.
    /// \param [in] SrcBufferTransitionMode - State transition mode of the source buffer (see Diligent::RESOURCE_STATE_TRANSITION_MODE).
    /// \param [in] pDstBuffer              - Destination buffer to copy data to.
    /// \param [in] DstOffset               - Offset in bytes from the beginning of the destination buffer to the beginning 
    ///                                       of the destination region.
    /// \param [in] Size                    - Size in bytes of data to copy.
    /// \param [in] DstBufferTransitionMode - State transition mode of the destination buffer (see Diligent::RESOURCE_STATE_TRANSITION_MODE).
    VIRTUAL void METHOD(CopyBuffer)(THIS_
                                    IBuffer*                       pSrcBuffer,
                                    Uint32                         SrcOffset,
                                    RESOURCE_STATE_TRANSITION_MODE SrcBufferTransitionMode,
                                    IBuffer*                       pDstBuffer,
                                    Uint32                         DstOffset,
                                    Uint32                         Size,
                                    RESOURCE_STATE_TRANSITION_MODE DstBufferTransitionMode) PURE;


    /// Maps the buffer.

    /// \param [in] pBuffer      - Pointer to the buffer to map.
    /// \param [in] MapType      - Type of the map operation. See Diligent::MAP_TYPE.
    /// \param [in] MapFlags     - Special map flags. See Diligent::MAP_FLAGS.
    /// \param [out] pMappedData - Reference to the void pointer to store the address of the mapped region.
    VIRTUAL void METHOD(MapBuffer)(THIS_
                                   IBuffer*     pBuffer,
                                   MAP_TYPE     MapType,
                                   MAP_FLAGS    MapFlags,
                                   PVoid REF    pMappedData) PURE;


    /// Unmaps the previously mapped buffer.

    /// \param [in] pBuffer - Pointer to the buffer to unmap.
    /// \param [in] MapType - Type of the map operation. This parameter must match the type that was 
    ///                       provided to the Map() method. 
    VIRTUAL void METHOD(UnmapBuffer)(THIS_
                                     IBuffer*   pBuffer,
                                     MAP_TYPE   MapType) PURE;


    /// Updates the data in the texture.

    /// \param [in] pTexture    - Pointer to the device context interface to be used to perform the operation.
    /// \param [in] MipLevel    - Mip level of the texture subresource to update.
    /// \param [in] Slice       - Array slice. Should be 0 for non-array textures.
    /// \param [in] DstBox      - Destination region on the texture to update.
    /// \param [in] SubresData  - Source data to copy to the texture.
    /// \param [in] SrcBufferTransitionMode - If pSrcBuffer member of TextureSubResData structure is not null, this 
    ///                                       parameter defines state transition mode of the source buffer. 
    ///                                       If pSrcBuffer is null, this parameter is ignored.
    /// \param [in] TextureTransitionMode   - Texture state transition mode (see Diligent::RESOURCE_STATE_TRANSITION_MODE)
    VIRTUAL void METHOD(UpdateTexture)(THIS_
                                       ITexture*                        pTexture,
                                       Uint32                           MipLevel,
                                       Uint32                           Slice,
                                       const Box REF                    DstBox,
                                       const TextureSubResData REF      SubresData,
                                       RESOURCE_STATE_TRANSITION_MODE   SrcBufferTransitionMode,
                                       RESOURCE_STATE_TRANSITION_MODE   TextureTransitionMode) PURE;


    /// Copies data from one texture to another.

    /// \param [in] CopyAttribs - Structure describing copy command attributes, see Diligent::CopyTextureAttribs for details.
    VIRTUAL void METHOD(CopyTexture)(THIS_
                                     const CopyTextureAttribs REF CopyAttribs) PURE;


    /// Maps the texture subresource.

    /// \param [in] pTexture    - Pointer to the texture to map.
    /// \param [in] MipLevel    - Mip level to map.
    /// \param [in] ArraySlice  - Array slice to map. This parameter must be 0 for non-array textures.
    /// \param [in] MapType     - Type of the map operation. See Diligent::MAP_TYPE.
    /// \param [in] MapFlags    - Special map flags. See Diligent::MAP_FLAGS.
    /// \param [in] pMapRegion  - Texture region to map. If this parameter is null, the entire subresource is mapped.
    /// \param [out] MappedData - Mapped texture region data
    ///
    /// \remarks This method is supported in D3D11, D3D12 and Vulkan backends. In D3D11 backend, only the entire 
    ///          subresource can be mapped, so pMapRegion must either be null, or cover the entire subresource.
    ///          In D3D11 and Vulkan backends, dynamic textures are no different from non-dynamic textures, and mapping 
    ///          with MAP_FLAG_DISCARD has exactly the same behavior.
    VIRTUAL void METHOD(MapTextureSubresource)(THIS_
                                               ITexture*                    pTexture,
                                               Uint32                       MipLevel,
                                               Uint32                       ArraySlice,
                                               MAP_TYPE                     MapType,
                                               MAP_FLAGS                    MapFlags,
                                               const Box*                   pMapRegion,
                                               MappedTextureSubresource REF MappedData) PURE;


    /// Unmaps the texture subresource.
    VIRTUAL void METHOD(UnmapTextureSubresource)(THIS_
                                                 ITexture* pTexture,
                                                 Uint32    MipLevel,
                                                 Uint32    ArraySlice) PURE;

    
    /// Generates a mipmap chain.

    /// \param [in] pTextureView - Texture view to generate mip maps for.
    /// \remarks This function can only be called for a shader resource view.
    ///          The texture must be created with MISC_TEXTURE_FLAG_GENERATE_MIPS flag.
    VIRTUAL void METHOD(GenerateMips)(THIS_
                                      ITextureView* pTextureView) PURE;

    
    /// Finishes the current frame and releases dynamic resources allocated by the context.

    /// For immediate context, this method is called automatically by ISwapChain::Present() of the primary
    /// swap chain, but can also be called explicitly. For deferred contexts, the method must be called by the
    /// application to release dynamic resources. The method has some overhead, so it is better to call it once
    /// per frame, though it can be called with different frequency. Note that unless the GPU is idled,
    /// the resources may actually be released several frames after the one they were used in last time.
    /// \note After the call all dynamic resources become invalid and must be written again before the next use. 
    ///       Also, all committed resources become invalid.\n
    ///       For deferred contexts, this method must be called after all command lists referencing dynamic resources
    ///       have been executed through immediate context.\n
    ///       The method does not Flush() the context.
    VIRTUAL void METHOD(FinishFrame)(THIS) PURE;


    /// Transitions resource states.

    /// \param [in] BarrierCount      - Number of barriers in pResourceBarriers array
    /// \param [in] pResourceBarriers - Pointer to the array of resource barriers
    /// \remarks When both old and new states are RESOURCE_STATE_UNORDERED_ACCESS, the engine
    ///          executes UAV barrier on the resource. The barrier makes sure that all UAV accesses 
    ///          (reads or writes) are complete before any future UAV accesses (read or write) can begin.\n
    /// 
    ///          There are two main usage scenarios for this method:
    ///          1. An application knows specifics of resource state transitions not available to the engine.
    ///             For example, only single mip level needs to be transitioned.
    ///          2. An application manages resource states in multiple threads in parallel.
    ///         
    ///          The method always reads the states of all resources to transition. If the state of a resource is managed
    ///          by multiple threads in parallel, the resource must first be transitioned to unknown state
    ///          (Diligent::RESOURCE_STATE_UNKNOWN) to disable automatic state management in the engine.
    ///          
    ///          When StateTransitionDesc::UpdateResourceState is set to true, the method may update the state of the
    ///          corresponding resource which is not thread safe. No other threads should read or write the sate of that 
    ///          resource.
    ///
    /// \note    Any method that uses Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION mode may alter
    ///          the state of resources it works with. Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY mode
    ///          makes the method read the states, but not write them. When Diligent::RESOURCE_STATE_TRANSITION_MODE_NONE
    ///          is used, the method assumes the states are guaranteed to be correct and does not read or write them.
    ///          It is the responsibility of the application to make sure this is indeed true.
    ///
    ///          Refer to http://diligentgraphics.com/2018/12/09/resource-state-management/ for detailed explanation
    ///          of resource state management in Diligent Engine.
    VIRTUAL void METHOD(TransitionResourceStates)(THIS_
                                                  Uint32               BarrierCount,
                                                  StateTransitionDesc* pResourceBarriers) PURE;


    /// Resolves a multi-sampled texture subresource into a non-multi-sampled texture subresource.

    /// \param [in] pSrcTexture    - Source multi-sampled texture.
    /// \param [in] pDstTexture    - Destination non-multi-sampled texture.
    /// \param [in] ResolveAttribs - Resolve command attributes, see Diligent::ResolveTextureSubresourceAttribs for details.
    VIRTUAL void METHOD(ResolveTextureSubresource)(THIS_
                                                   ITexture*                                  pSrcTexture,
                                                   ITexture*                                  pDstTexture,
                                                   const ResolveTextureSubresourceAttribs REF ResolveAttribs) PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define IDeviceContext_SetPipelineState(This, ...)          CALL_IFACE_METHOD(DeviceContext, SetPipelineState,          This, __VA_ARGS__)
#    define IDeviceContext_TransitionShaderResources(This, ...) CALL_IFACE_METHOD(DeviceContext, TransitionShaderResources, This, __VA_ARGS__)
#    define IDeviceContext_CommitShaderResources(This, ...)     CALL_IFACE_METHOD(DeviceContext, CommitShaderResources,     This, __VA_ARGS__)
#    define IDeviceContext_SetStencilRef(This, ...)             CALL_IFACE_METHOD(DeviceContext, SetStencilRef,             This, __VA_ARGS__)
#    define IDeviceContext_SetBlendFactors(This, ...)           CALL_IFACE_METHOD(DeviceContext, SetBlendFactors,           This, __VA_ARGS__)
#    define IDeviceContext_SetVertexBuffers(This, ...)          CALL_IFACE_METHOD(DeviceContext, SetVertexBuffers,          This, __VA_ARGS__)
#    define IDeviceContext_InvalidateState(This)                CALL_IFACE_METHOD(DeviceContext, InvalidateState,           This)
#    define IDeviceContext_SetIndexBuffer(This, ...)            CALL_IFACE_METHOD(DeviceContext, SetIndexBuffer,            This, __VA_ARGS__)
#    define IDeviceContext_SetViewports(This, ...)              CALL_IFACE_METHOD(DeviceContext, SetViewports,              This, __VA_ARGS__)
#    define IDeviceContext_SetScissorRects(This, ...)           CALL_IFACE_METHOD(DeviceContext, SetScissorRects,           This, __VA_ARGS__)
#    define IDeviceContext_SetRenderTargets(This, ...)          CALL_IFACE_METHOD(DeviceContext, SetRenderTargets,          This, __VA_ARGS__)
#    define IDeviceContext_Draw(This, ...)                      CALL_IFACE_METHOD(DeviceContext, Draw,                      This, __VA_ARGS__)
#    define IDeviceContext_DrawIndexed(This, ...)               CALL_IFACE_METHOD(DeviceContext, DrawIndexed,               This, __VA_ARGS__)
#    define IDeviceContext_DrawIndirect(This, ...)              CALL_IFACE_METHOD(DeviceContext, DrawIndirect,              This, __VA_ARGS__)
#    define IDeviceContext_DrawIndexedIndirect(This, ...)       CALL_IFACE_METHOD(DeviceContext, DrawIndexedIndirect,       This, __VA_ARGS__)
#    define IDeviceContext_DispatchCompute(This, ...)           CALL_IFACE_METHOD(DeviceContext, DispatchCompute,           This, __VA_ARGS__)
#    define IDeviceContext_DispatchComputeIndirect(This, ...)   CALL_IFACE_METHOD(DeviceContext, DispatchComputeIndirect,   This, __VA_ARGS__)
#    define IDeviceContext_ClearDepthStencil(This, ...)         CALL_IFACE_METHOD(DeviceContext, ClearDepthStencil,         This, __VA_ARGS__)
#    define IDeviceContext_ClearRenderTarget(This, ...)         CALL_IFACE_METHOD(DeviceContext, ClearRenderTarget,         This, __VA_ARGS__)
#    define IDeviceContext_FinishCommandList(This, ...)         CALL_IFACE_METHOD(DeviceContext, FinishCommandList,         This, __VA_ARGS__)
#    define IDeviceContext_ExecuteCommandList(This, ...)        CALL_IFACE_METHOD(DeviceContext, ExecuteCommandList,        This, __VA_ARGS__)
#    define IDeviceContext_SignalFence(This, ...)               CALL_IFACE_METHOD(DeviceContext, SignalFence,               This, __VA_ARGS__)
#    define IDeviceContext_WaitForFence(This, ...)              CALL_IFACE_METHOD(DeviceContext, WaitForFence,              This, __VA_ARGS__)
#    define IDeviceContext_WaitForIdle(This, ...)               CALL_IFACE_METHOD(DeviceContext, WaitForIdle,               This, __VA_ARGS__)
#    define IDeviceContext_BeginQuery(This, ...)                CALL_IFACE_METHOD(DeviceContext, BeginQuery,                This, __VA_ARGS__)
#    define IDeviceContext_EndQuery(This, ...)                  CALL_IFACE_METHOD(DeviceContext, EndQuery,                  This, __VA_ARGS__)
#    define IDeviceContext_Flush(This, ...)                     CALL_IFACE_METHOD(DeviceContext, Flush,                     This, __VA_ARGS__)
#    define IDeviceContext_UpdateBuffer(This, ...)              CALL_IFACE_METHOD(DeviceContext, UpdateBuffer,              This, __VA_ARGS__)
#    define IDeviceContext_CopyBuffer(This, ...)                CALL_IFACE_METHOD(DeviceContext, CopyBuffer,                This, __VA_ARGS__)
#    define IDeviceContext_MapBuffer(This, ...)                 CALL_IFACE_METHOD(DeviceContext, MapBuffer,                 This, __VA_ARGS__)
#    define IDeviceContext_UnmapBuffer(This, ...)               CALL_IFACE_METHOD(DeviceContext, UnmapBuffer,               This, __VA_ARGS__)
#    define IDeviceContext_UpdateTexture(This, ...)             CALL_IFACE_METHOD(DeviceContext, UpdateTexture,             This, __VA_ARGS__)
#    define IDeviceContext_CopyTexture(This, ...)               CALL_IFACE_METHOD(DeviceContext, CopyTexture,               This, __VA_ARGS__)
#    define IDeviceContext_MapTextureSubresource(This, ...)     CALL_IFACE_METHOD(DeviceContext, MapTextureSubresource,     This, __VA_ARGS__)
#    define IDeviceContext_UnmapTextureSubresource(This, ...)   CALL_IFACE_METHOD(DeviceContext, UnmapTextureSubresource,   This, __VA_ARGS__)
#    define IDeviceContext_GenerateMips(This, ...)              CALL_IFACE_METHOD(DeviceContext, GenerateMips,              This, __VA_ARGS__)
#    define IDeviceContext_FinishFrame(This)                    CALL_IFACE_METHOD(DeviceContext, FinishFrame,               This)
#    define IDeviceContext_TransitionResourceStates(This, ...)  CALL_IFACE_METHOD(DeviceContext, TransitionResourceStates,  This, __VA_ARGS__)
#    define IDeviceContext_ResolveTextureSubresource(This, ...) CALL_IFACE_METHOD(DeviceContext, ResolveTextureSubresource, This, __VA_ARGS__)

// clang-format on

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
