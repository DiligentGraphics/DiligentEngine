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
/// Definition of the Diligent::IRenderDevice interface and related data structures

#include "../../../Primitives/interface/Object.h"
#include "../../../Platforms/interface/PlatformDefinitions.h"
#include "GraphicsTypes.h"
#include "BlendState.h"
#include "RasterizerState.h"
#include "DepthStencilState.h"
#include "InputLayout.h"
#include "ShaderResourceBinding.h"
#include "ShaderResourceVariable.h"
#include "Shader.h"
#include "Sampler.h"
#include "RenderPass.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

    
/// Sample description

/// This structure is used by GraphicsPipelineDesc to describe multisampling parameters
struct SampleDesc
{
    /// Sample count
    Uint8 Count     DEFAULT_INITIALIZER(1);

    /// Quality
    Uint8 Quality   DEFAULT_INITIALIZER(0);

#if DILIGENT_CPP_INTERFACE
    SampleDesc()noexcept{}

    SampleDesc(Uint8 _Count, Uint8 _Quality) noexcept : 
        Count   {_Count  },
        Quality {_Quality}
    {}
#endif
};
typedef struct SampleDesc SampleDesc;


/// Describes shader variable
struct ShaderResourceVariableDesc
{
    /// Shader stages this resources variable applies to. More than one shader stage can be specified.
    SHADER_TYPE                   ShaderStages DEFAULT_INITIALIZER(SHADER_TYPE_UNKNOWN);

    /// Shader variable name
    const Char*                   Name         DEFAULT_INITIALIZER(nullptr);

    /// Shader variable type. See Diligent::SHADER_RESOURCE_VARIABLE_TYPE for a list of allowed types
    SHADER_RESOURCE_VARIABLE_TYPE Type         DEFAULT_INITIALIZER(SHADER_RESOURCE_VARIABLE_TYPE_STATIC);

#if DILIGENT_CPP_INTERFACE
    ShaderResourceVariableDesc()noexcept{}

    ShaderResourceVariableDesc(SHADER_TYPE _ShaderStages, const Char* _Name, SHADER_RESOURCE_VARIABLE_TYPE _Type)noexcept : 
        ShaderStages{_ShaderStages},
        Name        {_Name        },
        Type        {_Type        }
    {}
#endif
};
typedef struct ShaderResourceVariableDesc ShaderResourceVariableDesc;


/// Immutable sampler description.

/// An immutable sampler is compiled into the pipeline state and can't be changed.
/// It is generally more efficient than a regular sampler and should be used
/// whenever possible.
struct ImmutableSamplerDesc
{
    /// Shader stages that this immutable sampler applies to. More than one shader stage can be specified.
    SHADER_TYPE ShaderStages         DEFAULT_INITIALIZER(SHADER_TYPE_UNKNOWN);

    /// The name of the sampler itself or the name of the texture variable that 
    /// this immutable sampler is assigned to if combined texture samplers are used.
    const Char* SamplerOrTextureName DEFAULT_INITIALIZER(nullptr);

    /// Sampler description
    struct SamplerDesc Desc;

#if DILIGENT_CPP_INTERFACE
    ImmutableSamplerDesc()noexcept{}

    ImmutableSamplerDesc(SHADER_TYPE        _ShaderStages,
                         const Char*        _SamplerOrTextureName,
                         const SamplerDesc& _Desc)noexcept : 
        ShaderStages        {_ShaderStages        },
        SamplerOrTextureName{_SamplerOrTextureName},
        Desc                {_Desc                }
    {}
#endif
};
typedef struct ImmutableSamplerDesc ImmutableSamplerDesc;

/// Pipeline layout description
struct PipelineResourceLayoutDesc
{
    /// Default shader resource variable type. This type will be used if shader
    /// variable description is not found in the Variables array
    /// or if Variables == nullptr
    SHADER_RESOURCE_VARIABLE_TYPE       DefaultVariableType  DEFAULT_INITIALIZER(SHADER_RESOURCE_VARIABLE_TYPE_STATIC);

    /// Number of elements in Variables array            
    Uint32                              NumVariables         DEFAULT_INITIALIZER(0);

    /// Array of shader resource variable descriptions               
    const ShaderResourceVariableDesc*   Variables            DEFAULT_INITIALIZER(nullptr);
                                                            
    /// Number of immutable samplers in ImmutableSamplers array   
    Uint32                              NumImmutableSamplers DEFAULT_INITIALIZER(0);
                                                            
    /// Array of immutable sampler descriptions                
    const ImmutableSamplerDesc*         ImmutableSamplers    DEFAULT_INITIALIZER(nullptr);
};
typedef struct PipelineResourceLayoutDesc PipelineResourceLayoutDesc;


/// Graphics pipeline state description

/// This structure describes the graphics pipeline state and is part of the PipelineStateDesc structure.
struct GraphicsPipelineDesc
{
    /// Blend state description.
    BlendStateDesc BlendDesc;

    /// 32-bit sample mask that determines which samples get updated 
    /// in all the active render targets. A sample mask is always applied; 
    /// it is independent of whether multisampling is enabled, and does not 
    /// depend on whether an application uses multisample render targets.
    Uint32 SampleMask DEFAULT_INITIALIZER(0xFFFFFFFF);

    /// Rasterizer state description.
    RasterizerStateDesc RasterizerDesc;

    /// Depth-stencil state description.
    DepthStencilStateDesc DepthStencilDesc;

    /// Input layout, ignored in a mesh pipeline.
    InputLayoutDesc InputLayout;
    //D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBStripCutValue;

    /// Primitive topology type, ignored in a mesh pipeline.
    PRIMITIVE_TOPOLOGY PrimitiveTopology DEFAULT_INITIALIZER(PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    /// The number of viewports used by this pipeline
    Uint8 NumViewports           DEFAULT_INITIALIZER(1);

    /// The number of render targets in the RTVFormats array.
    /// Must be 0 when pRenderPass is not null.
    Uint8 NumRenderTargets       DEFAULT_INITIALIZER(0);

    /// When pRenderPass is not null, the subpass
    /// index within the render pass.
    /// When pRenderPass is null, this member must be 0.
    Uint8 SubpassIndex           DEFAULT_INITIALIZER(0);

    /// Render target formats.
    /// All formats must be TEX_FORMAT_UNKNOWN when pRenderPass is not null.
    TEXTURE_FORMAT RTVFormats[8] DEFAULT_INITIALIZER({});

    /// Depth-stencil format.
    /// Must be TEX_FORMAT_UNKNOWN when pRenderPass is not null.
    TEXTURE_FORMAT DSVFormat     DEFAULT_INITIALIZER(TEX_FORMAT_UNKNOWN);

    /// Multisampling parameters.
    SampleDesc SmplDesc;

    /// Pointer to the render pass object.

    /// When non-null render pass is specified, NumRenderTargets must be 0,
    /// and all RTV formats as well as DSV format must be TEX_FORMAT_UNKNOWN.
    IRenderPass* pRenderPass     DEFAULT_INITIALIZER(nullptr);

    /// Node mask.
    Uint32 NodeMask DEFAULT_INITIALIZER(0);

    //D3D12_CACHED_PIPELINE_STATE CachedPSO;
    //D3D12_PIPELINE_STATE_FLAGS Flags;
};
typedef struct GraphicsPipelineDesc GraphicsPipelineDesc;


/// Pipeline type
DILIGENT_TYPED_ENUM(PIPELINE_TYPE, Uint8)
{
    /// Graphics pipeline, which is used by IDeviceContext::Draw(), IDeviceContext::DrawIndexed(),
    /// IDeviceContext::DrawIndirect(), IDeviceContext::DrawIndexedIndirect().
    PIPELINE_TYPE_GRAPHICS,

    /// Compute pipeline, which is used by IDeviceContext::DispatchCompute(), IDeviceContext::DispatchComputeIndirect().
    PIPELINE_TYPE_COMPUTE,

    /// Mesh pipeline, which is used by IDeviceContext::DrawMesh(), IDeviceContext::DrawMeshIndirect().
    PIPELINE_TYPE_MESH,
};


/// Pipeline state description
struct PipelineStateDesc DILIGENT_DERIVE(DeviceObjectAttribs)

    /// Pipeline type
    PIPELINE_TYPE PipelineType      DEFAULT_INITIALIZER(PIPELINE_TYPE_GRAPHICS);

    /// Shader resource binding allocation granularity

    /// This member defines allocation granularity for internal resources required by the shader resource
    /// binding object instances.
    Uint32 SRBAllocationGranularity DEFAULT_INITIALIZER(1);

    /// Defines which command queues this pipeline state can be used with
    Uint64 CommandQueueMask         DEFAULT_INITIALIZER(1);

    /// Pipeline layout description
    PipelineResourceLayoutDesc ResourceLayout;

#if DILIGENT_CPP_INTERFACE
    bool IsAnyGraphicsPipeline() const { return PipelineType == PIPELINE_TYPE_GRAPHICS || PipelineType == PIPELINE_TYPE_MESH; }
    bool IsComputePipeline()     const { return PipelineType == PIPELINE_TYPE_COMPUTE; }
#endif
};
typedef struct PipelineStateDesc PipelineStateDesc;


/// Pipeline state creation flags
DILIGENT_TYPED_ENUM(PSO_CREATE_FLAGS, Uint32)
{
    /// Null flag.
    PSO_CREATE_FLAG_NONE                              = 0x00,

    /// Ignore missing variables.

    /// By default, the engine outputs a warning for every variable
    /// provided as part of the pipeline resource layout description
    /// that is not found in any of the designated shader stages.
    /// Use this flag to silence these warnings.
    PSO_CREATE_FLAG_IGNORE_MISSING_VARIABLES          = 0x01,

    /// Ignore missing immutable samplers.

    /// By default, the engine outputs a warning for every immutable sampler
    /// provided as part of the pipeline resource layout description
    /// that is not found in any of the designated shader stages.
    /// Use this flag to silence these warnings.
    PSO_CREATE_FLAG_IGNORE_MISSING_IMMUTABLE_SAMPLERS = 0x02,
};
DEFINE_FLAG_ENUM_OPERATORS(PSO_CREATE_FLAGS);


/// Pipeline state creation attributes
struct PipelineStateCreateInfo
{
    /// Pipeline state description
    PipelineStateDesc PSODesc;

    /// Pipeline state creation flags, see Diligent::PSO_CREATE_FLAGS.
    PSO_CREATE_FLAGS  Flags      DEFAULT_INITIALIZER(PSO_CREATE_FLAG_NONE);
};
typedef struct PipelineStateCreateInfo PipelineStateCreateInfo;


/// Graphics pipeline state creation attributes
struct GraphicsPipelineStateCreateInfo DILIGENT_DERIVE(PipelineStateCreateInfo)
    
    /// Graphics pipeline state description.
    GraphicsPipelineDesc GraphicsPipeline; 
     
    /// Vertex shader to be used with the pipeline.
    IShader* pVS DEFAULT_INITIALIZER(nullptr);

    /// Pixel shader to be used with the pipeline.
    IShader* pPS DEFAULT_INITIALIZER(nullptr);

    /// Domain shader to be used with the pipeline.
    IShader* pDS DEFAULT_INITIALIZER(nullptr);

    /// Hull shader to be used with the pipeline.
    IShader* pHS DEFAULT_INITIALIZER(nullptr);

    /// Geometry shader to be used with the pipeline.
    IShader* pGS DEFAULT_INITIALIZER(nullptr);
    
    /// Amplification shader to be used with the pipeline.
    IShader* pAS DEFAULT_INITIALIZER(nullptr);
    
    /// Mesh shader to be used with the pipeline.
    IShader* pMS DEFAULT_INITIALIZER(nullptr);
};
typedef struct GraphicsPipelineStateCreateInfo GraphicsPipelineStateCreateInfo;


/// Compute pipeline state description.
struct ComputePipelineStateCreateInfo DILIGENT_DERIVE(PipelineStateCreateInfo)
    
    /// Compute shader to be used with the pipeline
    IShader* pCS DEFAULT_INITIALIZER(nullptr);

#if DILIGENT_CPP_INTERFACE
    ComputePipelineStateCreateInfo() noexcept
    {
        PSODesc.PipelineType = PIPELINE_TYPE_COMPUTE;
    }
#endif
};
typedef struct ComputePipelineStateCreateInfo ComputePipelineStateCreateInfo;


// {06084AE5-6A71-4FE8-84B9-395DD489A28C}
static const struct INTERFACE_ID IID_PipelineState =
    {0x6084ae5, 0x6a71, 0x4fe8, {0x84, 0xb9, 0x39, 0x5d, 0xd4, 0x89, 0xa2, 0x8c}};

#define DILIGENT_INTERFACE_NAME IPipelineState
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IPipelineStateInclusiveMethods \
    IDeviceObjectInclusiveMethods;     \
    IPipelineStateMethods PipelineState

// clang-format off

/// Pipeline state interface
DILIGENT_BEGIN_INTERFACE(IPipelineState, IDeviceObject)
{
#if DILIGENT_CPP_INTERFACE
    /// Returns the pipeline description used to create the object
    virtual const PipelineStateDesc& METHOD(GetDesc)() const override = 0;
#endif

    /// Returns the graphics pipeline description used to create the object.
    /// This method must only be called for a graphics or mesh pipeline.
    VIRTUAL const GraphicsPipelineDesc REF METHOD(GetGraphicsPipelineDesc)(THIS) CONST PURE;

    /// Binds resources for all shaders in the pipeline state

    /// \param [in] ShaderFlags - Flags that specify shader stages, for which resources will be bound.
    ///                           Any combination of Diligent::SHADER_TYPE may be used.
    /// \param [in] pResourceMapping - Pointer to the resource mapping interface.
    /// \param [in] Flags - Additional flags. See Diligent::BIND_SHADER_RESOURCES_FLAGS.
    VIRTUAL void METHOD(BindStaticResources)(THIS_
                                             Uint32             ShaderFlags,
                                             IResourceMapping*  pResourceMapping,
                                             Uint32             Flags) PURE;


    /// Returns the number of static shader resource variables.

    /// \param [in] ShaderType - Type of the shader.
    /// \remark Only static variables (that can be accessed directly through the PSO) are counted.
    ///         Mutable and dynamic variables are accessed through Shader Resource Binding object.
    VIRTUAL Uint32 METHOD(GetStaticVariableCount)(THIS_
                                                  SHADER_TYPE ShaderType) CONST PURE;


    /// Returns static shader resource variable. If the variable is not found,
    /// returns nullptr.
    
    /// \param [in] ShaderType - Type of the shader to look up the variable. 
    ///                          Must be one of Diligent::SHADER_TYPE.
    /// \param [in] Name - Name of the variable.
    /// \remark The method does not increment the reference counter
    ///         of the returned interface.
    VIRTUAL IShaderResourceVariable* METHOD(GetStaticVariableByName)(THIS_
                                                                     SHADER_TYPE ShaderType,
                                                                     const Char* Name) PURE;


    /// Returns static shader resource variable by its index.

    /// \param [in] ShaderType - Type of the shader to look up the variable. 
    ///                          Must be one of Diligent::SHADER_TYPE.
    /// \param [in] Index - Shader variable index. The index must be between
    ///                     0 and the total number of variables returned by 
    ///                     GetStaticVariableCount().
    /// \remark Only static shader resource variables can be accessed through this method.
    ///         Mutable and dynamic variables are accessed through Shader Resource 
    ///         Binding object
    VIRTUAL IShaderResourceVariable* METHOD(GetStaticVariableByIndex)(THIS_
                                                                      SHADER_TYPE ShaderType,
                                                                      Uint32      Index) PURE;


    /// Creates a shader resource binding object

    /// \param [out] ppShaderResourceBinding - memory location where pointer to the new shader resource
    ///                                        binding object is written.
    /// \param [in] InitStaticResources      - if set to true, the method will initialize static resources in
    ///                                        the created object, which has the exact same effect as calling 
    ///                                        IShaderResourceBinding::InitializeStaticResources().
    VIRTUAL void METHOD(CreateShaderResourceBinding)(THIS_
                                                     IShaderResourceBinding** ppShaderResourceBinding,
                                                     bool                     InitStaticResources DEFAULT_VALUE(false)) PURE;


    /// Checks if this pipeline state object is compatible with another PSO

    /// If two pipeline state objects are compatible, they can use shader resource binding
    /// objects interchangebly, i.e. SRBs created by one PSO can be committed
    /// when another PSO is bound.
    /// \param [in] pPSO - Pointer to the pipeline state object to check compatibility with
    /// \return     true if this PSO is compatbile with pPSO. false otherwise.
    /// \remarks    The function only checks that shader resource layouts are compatible, but 
    ///             does not check if resource types match. For instance, if a pixel shader in one PSO
    ///             uses a texture at slot 0, and a pixel shader in another PSO uses texture array at slot 0,
    ///             the pipelines will be compatible. However, if you try to use SRB object from the first pipeline
    ///             to commit resources for the second pipeline, a runtime error will occur.\n
    ///             The function only checks compatibility of shader resource layouts. It does not take
    ///             into account vertex shader input layout, number of outputs, etc.
    VIRTUAL bool METHOD(IsCompatibleWith)(THIS_
                                          const struct IPipelineState* pPSO) CONST PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define IPipelineState_GetDesc(This) (const struct PipelineStateDesc*)IDeviceObject_GetDesc(This)

#    define IPipelineState_GetGraphicsPipelineDesc(This)          CALL_IFACE_METHOD(PipelineState, GetGraphicsPipelineDesc,     This)
#    define IPipelineState_BindStaticResources(This, ...)         CALL_IFACE_METHOD(PipelineState, BindStaticResources,         This, __VA_ARGS__)
#    define IPipelineState_GetStaticVariableCount(This, ...)      CALL_IFACE_METHOD(PipelineState, GetStaticVariableCount,      This, __VA_ARGS__)
#    define IPipelineState_GetStaticVariableByName(This, ...)     CALL_IFACE_METHOD(PipelineState, GetStaticVariableByName,     This, __VA_ARGS__)
#    define IPipelineState_GetStaticVariableByIndex(This, ...)    CALL_IFACE_METHOD(PipelineState, GetStaticVariableByIndex,    This, __VA_ARGS__)
#    define IPipelineState_CreateShaderResourceBinding(This, ...) CALL_IFACE_METHOD(PipelineState, CreateShaderResourceBinding, This, __VA_ARGS__)
#    define IPipelineState_IsCompatibleWith(This, ...)            CALL_IFACE_METHOD(PipelineState, IsCompatibleWith,            This, __VA_ARGS__)

// clang-format on

#endif

DILIGENT_END_NAMESPACE
