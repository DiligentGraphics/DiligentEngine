/*     Copyright 2015-2018 Egor Yusov
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF ANY PROPRIETARY RIGHTS.
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

#include "Errors.hpp"

#ifndef GLAPIENTRY
#    define GLAPIENTRY
#endif

#define glUseProgramStages          glUseProgramStagesEXT
#define glActiveShaderProgram       glActiveShaderProgramEXT
#define glCreateShaderProgramv      glCreateShaderProgramvEXT
#define glBindProgramPipeline       glBindProgramPipelineEXT
#define glDeleteProgramPipelines    glDeleteProgramPipelinesEXT
#define glGenProgramPipelines       glGenProgramPipelinesEXT
#define glIsProgramPipeline         glIsProgramPipelineEXT
#define glProgramParameteri         glProgramParameteriEXT
#define glGetProgramPipelineiv      glGetProgramPipelineivEXT
#define glValidateProgramPipeline   glValidateProgramPipelineEXT
#define glGetProgramPipelineInfoLog glGetProgramPipelineInfoLogEXT
#define glProgramUniform1i          glProgramUniform1iEXT

#define GL_VERTEX_SHADER_BIT        GL_VERTEX_SHADER_BIT_EXT
#define GL_FRAGMENT_SHADER_BIT      GL_FRAGMENT_SHADER_BIT_EXT
#define GL_ALL_SHADER_BITS          GL_ALL_SHADER_BITS_EXT
#define GL_PROGRAM_SEPARABLE        GL_PROGRAM_SEPARABLE_EXT
#define GL_ACTIVE_PROGRAM           GL_ACTIVE_PROGRAM_EXT
#define GL_PROGRAM_PIPELINE_BINDING GL_PROGRAM_PIPELINE_BINDING_EXT

#define GL_ARB_shader_image_load_store      0
#define GL_ARB_shader_storage_buffer_object 0
#define GL_ARB_tessellation_shader          0
#define GL_ARB_draw_indirect                0
#define GL_ARB_compute_shader               0
#define GL_ARB_program_interface_query      0
#define GL_ARB_internalformat_query2        0
#define GL_ARB_texture_storage_multisample  0

#ifndef GL_CLAMP_TO_BORDER
#    define GL_CLAMP_TO_BORDER GL_CLAMP_TO_EDGE
#endif

#ifndef GL_MIRROR_CLAMP_TO_EDGE
#    define GL_MIRROR_CLAMP_TO_EDGE GL_CLAMP_TO_EDGE
#endif

#ifndef GL_UNSIGNED_INT_10_10_10_2
#    define GL_UNSIGNED_INT_10_10_10_2 0x8036
#endif

#ifndef GL_UNSIGNED_SHORT_5_6_5_REV
#    define GL_UNSIGNED_SHORT_5_6_5_REV 0x8364
#endif

#ifndef GL_TEXTURE_BUFFER
#    define GL_TEXTURE_BUFFER 0
#endif

#define glTexBuffer(...) 0

#ifndef GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER
#    define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER 0x8CDB
#endif

#ifndef GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER
#    define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER 0x8CDC
#endif

#ifndef GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS
#    define GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS 0x8DA8
#endif

#ifndef GL_DRAW_INDIRECT_BUFFER
#    define GL_DRAW_INDIRECT_BUFFER 0
#endif

#ifndef GL_GEOMETRY_SHADER
#    define GL_GEOMETRY_SHADER 0
#endif

#ifndef GL_TESS_CONTROL_SHADER
#    define GL_TESS_CONTROL_SHADER 0
#endif

#ifndef GL_TESS_EVALUATION_SHADER
#    define GL_TESS_EVALUATION_SHADER 0
#endif

#ifndef GL_COMPUTE_SHADER
#    define GL_COMPUTE_SHADER 0
#endif

#ifndef GL_GEOMETRY_SHADER_BIT
#    define GL_GEOMETRY_SHADER_BIT 0
#endif

#ifndef GL_TESS_CONTROL_SHADER_BIT
#    define GL_TESS_CONTROL_SHADER_BIT 0
#endif

#ifndef GL_TESS_EVALUATION_SHADER_BIT
#    define GL_TESS_EVALUATION_SHADER_BIT 0
#endif

#ifndef GL_COMPUTE_SHADER_BIT
#    define GL_COMPUTE_SHADER_BIT 0
#endif

#ifndef GL_SAMPLER_BUFFER
#    define GL_SAMPLER_BUFFER 0x8DC2
#endif

#ifndef GL_INT_SAMPLER_BUFFER
#    define GL_INT_SAMPLER_BUFFER 0x8DD0
#endif

#ifndef GL_UNSIGNED_INT_SAMPLER_BUFFER
#    define GL_UNSIGNED_INT_SAMPLER_BUFFER 0x8DD8
#endif

#ifndef GL_SAMPLER_EXTERNAL_OES
#    define GL_SAMPLER_EXTERNAL_OES 0x8D66
#endif


// Polygon mode
#ifndef GL_POINT
#    define GL_POINT 0x1B00
#endif

#ifndef GL_LINE
#    define GL_LINE 0x1B01
#endif

#ifndef GL_FILL
#    define GL_FILL 0x1B02
#endif

#ifndef GL_DEPTH_CLAMP
#    define GL_DEPTH_CLAMP 0
#endif


// Define unsupported formats for OpenGL ES
#ifndef GL_RGBA16
#    define GL_RGBA16 0x805B
#endif

#ifndef GL_RGBA16_SNORM
#    define GL_RGBA16_SNORM 0x8F9B
#endif

#ifndef GL_RG16
#    define GL_RG16 0x822C
#endif

#ifndef GL_RG16_SNORM
#    define GL_RG16_SNORM 0x8F99
#endif

#ifndef GL_R16
#    define GL_R16 0x822A
#endif

#ifndef GL_R16_SNORM
#    define GL_R16_SNORM 0x8F98
#endif

#ifndef GL_COMPRESSED_RGB_S3TC_DXT1_EXT
#    define GL_COMPRESSED_RGB_S3TC_DXT1_EXT 0x83F0
#endif

#ifndef GL_COMPRESSED_SRGB_S3TC_DXT1_EXT
#    define GL_COMPRESSED_SRGB_S3TC_DXT1_EXT 0x8C4C
#endif

#ifndef GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
#    define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#endif

#ifndef GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT
#    define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT 0x8C4E
#endif

#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
#    define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif

#ifndef GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT
#    define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT 0x8C4F
#endif

#ifndef GL_COMPRESSED_RED_RGTC1
#    define GL_COMPRESSED_RED_RGTC1 0x8DBB
#endif

#ifndef GL_COMPRESSED_SIGNED_RED_RGTC1
#    define GL_COMPRESSED_SIGNED_RED_RGTC1 0x8DBC
#endif

#ifndef GL_COMPRESSED_RG_RGTC2
#    define GL_COMPRESSED_RG_RGTC2 0x8DBD
#endif

#ifndef GL_COMPRESSED_SIGNED_RG_RGTC2
#    define GL_COMPRESSED_SIGNED_RG_RGTC2 0x8DBE
#endif

#ifndef GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT
#    define GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT 0x8E8F
#endif

#ifndef GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT
#    define GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT 0x8E8E
#endif

#ifndef GL_COMPRESSED_RGBA_BPTC_UNORM
#    define GL_COMPRESSED_RGBA_BPTC_UNORM 0x8E8C
#endif

#ifndef GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM
#    define GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM 0x8E8D
#endif

#ifndef GL_UNSIGNED_SHORT_5_6_5_REV
#    define GL_UNSIGNED_SHORT_5_6_5_REV 0x8364
#endif

#ifndef GL_UNSIGNED_INT_10_10_10_2
#    define GL_UNSIGNED_INT_10_10_10_2 0x8036
#endif

#ifndef GL_UNSIGNED_SHORT_5_6_5_REV
#    define GL_UNSIGNED_SHORT_5_6_5_REV 0x8364
#endif

#ifndef GL_UNSIGNED_SHORT_1_5_5_5_REV
#    define GL_UNSIGNED_SHORT_1_5_5_5_REV 0x8366
#endif


// Define unsupported uniform data types
#ifndef GL_SAMPLER_1D
#    define GL_SAMPLER_1D 0x8B5D
#endif

#ifndef GL_SAMPLER_1D_SHADOW
#    define GL_SAMPLER_1D_SHADOW 0x8B61
#endif

#ifndef GL_SAMPLER_1D_ARRAY
#    define GL_SAMPLER_1D_ARRAY 0x8DC0
#endif

#ifndef GL_SAMPLER_1D_ARRAY_SHADOW
#    define GL_SAMPLER_1D_ARRAY_SHADOW 0x8DC3
#endif

#ifndef GL_INT_SAMPLER_1D
#    define GL_INT_SAMPLER_1D 0x8DC9
#endif

#ifndef GL_INT_SAMPLER_1D_ARRAY
#    define GL_INT_SAMPLER_1D_ARRAY 0x8DCE
#endif

#ifndef GL_UNSIGNED_INT_SAMPLER_1D
#    define GL_UNSIGNED_INT_SAMPLER_1D 0x8DD1
#endif

#ifndef GL_UNSIGNED_INT_SAMPLER_1D_ARRAY
#    define GL_UNSIGNED_INT_SAMPLER_1D_ARRAY 0x8DD6
#endif

#ifndef GL_SAMPLER_CUBE_MAP_ARRAY
#    define GL_SAMPLER_CUBE_MAP_ARRAY 0x900C
#endif

#ifndef GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW
#    define GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW 0x900D
#endif

#ifndef GL_INT_SAMPLER_CUBE_MAP_ARRAY
#    define GL_INT_SAMPLER_CUBE_MAP_ARRAY 0x900E
#endif

#ifndef GL_UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY
#    define GL_UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY 0x900F
#endif

#ifndef GL_SAMPLER_BUFFER
#    define GL_SAMPLER_BUFFER 0x8DC2
#endif

#ifndef GL_INT_SAMPLER_BUFFER
#    define GL_INT_SAMPLER_BUFFER 0x8DD0
#endif

#ifndef GL_UNSIGNED_INT_SAMPLER_BUFFER
#    define GL_UNSIGNED_INT_SAMPLER_BUFFER 0x8DD8
#endif

#ifndef GL_SAMPLER_2D_MULTISAMPLE
#    define GL_SAMPLER_2D_MULTISAMPLE 0x9108
#endif

#ifndef GL_INT_SAMPLER_2D_MULTISAMPLE
#    define GL_INT_SAMPLER_2D_MULTISAMPLE 0x9109
#endif

#ifndef GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE
#    define GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE 0x910A
#endif

#ifndef GL_SAMPLER_2D_MULTISAMPLE_ARRAY
#    define GL_SAMPLER_2D_MULTISAMPLE_ARRAY 0x910B
#endif

#ifndef GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY
#    define GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY 0x910C
#endif

#ifndef GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY
#    define GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY 0x910D
#endif


// Blend functions
#ifndef GL_SRC1_COLOR
#    define GL_SRC1_COLOR 0x88F9
#endif

#ifndef GL_ONE_MINUS_SRC1_COLOR
#    define GL_ONE_MINUS_SRC1_COLOR 0x88FA
#endif

#ifndef GL_SOURCE1_ALPHA
#    define GL_SOURCE1_ALPHA 0x8589
#endif

#ifndef GL_SRC1_ALPHA
#    define GL_SRC1_ALPHA GL_SOURCE1_ALPHA
#endif

#ifndef GL_ONE_MINUS_SRC1_ALPHA
#    define GL_ONE_MINUS_SRC1_ALPHA 0x88FB
#endif


#ifndef GL_READ_ONLY
#    define GL_READ_ONLY 0x88B8
#endif

#ifndef GL_WRITE_ONLY
#    define GL_WRITE_ONLY 0x88B9
#endif

#ifndef GL_READ_WRITE
#    define GL_READ_WRITE 0x88BA
#endif



// Define unsupported sampler attributes
#ifndef GL_TEXTURE_LOD_BIAS
#    define GL_TEXTURE_LOD_BIAS 0
#endif

#ifndef GL_TEXTURE_BORDER_COLOR
#    define GL_TEXTURE_BORDER_COLOR 0
#endif




#ifndef GL_TEXTURE_1D
#    define GL_TEXTURE_1D 0x0DE0
#endif

#ifndef GL_TEXTURE_1D_ARRAY
#    define GL_TEXTURE_1D_ARRAY 0x8C18
#endif

#ifndef GL_TEXTURE_BINDING_1D_ARRAY
#    define GL_TEXTURE_BINDING_1D_ARRAY 0x8C1C
#endif

#ifndef GL_TEXTURE_BINDING_1D
#    define GL_TEXTURE_BINDING_1D 0x8068
#endif

#ifndef GL_TEXTURE_2D_MULTISAMPLE
#    define GL_TEXTURE_2D_MULTISAMPLE 0x9100
#endif

#ifndef GL_TEXTURE_BINDING_2D_MULTISAMPLE
#    define GL_TEXTURE_BINDING_2D_MULTISAMPLE 0x9104
#endif

#ifndef GL_TEXTURE_2D_MULTISAMPLE_ARRAY
#    define GL_TEXTURE_2D_MULTISAMPLE_ARRAY 0x9102
#endif

#ifndef GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY
#    define GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY 0x9105
#endif

#ifndef GL_TEXTURE_CUBE_MAP_ARRAY
#    define GL_TEXTURE_CUBE_MAP_ARRAY 0x9009
#endif

#ifndef GL_TEXTURE_BINDING_CUBE_MAP_ARRAY
#    define GL_TEXTURE_BINDING_CUBE_MAP_ARRAY 0x900A
#endif

//#ifndef GL_TEXTURE_BUFFER
//#   define GL_TEXTURE_BUFFER 0x8C2A
//#endif


#ifndef GL_TEXTURE_WIDTH
#    define GL_TEXTURE_WIDTH 0
#endif

#ifndef GL_TEXTURE_HEIGHT
#    define GL_TEXTURE_HEIGHT 0
#endif

#ifndef GL_TEXTURE_DEPTH
#    define GL_TEXTURE_DEPTH 0
#endif

#ifndef GL_TEXTURE_INTERNAL_FORMAT
#    define GL_TEXTURE_INTERNAL_FORMAT 0
#endif

#ifndef GL_TEXTURE_INTERNAL_FORMAT
#    define GL_TEXTURE_INTERNAL_FORMAT 0
#endif




#ifndef GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT
#    define GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT 0x00000001
#endif
#ifndef GL_ELEMENT_ARRAY_BARRIER_BIT
#    define GL_ELEMENT_ARRAY_BARRIER_BIT 0x00000002
#endif
#ifndef GL_UNIFORM_BARRIER_BIT
#    define GL_UNIFORM_BARRIER_BIT 0x00000004
#endif
#ifndef GL_TEXTURE_FETCH_BARRIER_BIT
#    define GL_TEXTURE_FETCH_BARRIER_BIT 0x00000008
#endif
#ifndef GL_SHADER_IMAGE_ACCESS_BARRIER_BIT
#    define GL_SHADER_IMAGE_ACCESS_BARRIER_BIT 0x00000020
#endif
#ifndef GL_COMMAND_BARRIER_BIT
#    define GL_COMMAND_BARRIER_BIT 0x00000040
#endif
#ifndef GL_PIXEL_BUFFER_BARRIER_BIT
#    define GL_PIXEL_BUFFER_BARRIER_BIT 0x00000080
#endif
#ifndef GL_TEXTURE_UPDATE_BARRIER_BIT
#    define GL_TEXTURE_UPDATE_BARRIER_BIT 0x00000100
#endif
#ifndef GL_BUFFER_UPDATE_BARRIER_BIT
#    define GL_BUFFER_UPDATE_BARRIER_BIT 0x00000200
#endif
#ifndef GL_FRAMEBUFFER_BARRIER_BIT
#    define GL_FRAMEBUFFER_BARRIER_BIT 0x00000400
#endif
#ifndef GL_TRANSFORM_FEEDBACK_BARRIER_BIT
#    define GL_TRANSFORM_FEEDBACK_BARRIER_BIT 0x00000800
#endif
#ifndef GL_ATOMIC_COUNTER_BARRIER_BIT
#    define GL_ATOMIC_COUNTER_BARRIER_BIT 0x00001000
#endif
#ifndef GL_SHADER_STORAGE_BARRIER_BIT
#    define GL_SHADER_STORAGE_BARRIER_BIT 0x00002000
#endif
#ifndef GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT
#    define GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT 0x00004000
#endif
#ifndef GL_QUERY_BUFFER_BARRIER_BIT
#    define GL_QUERY_BUFFER_BARRIER_BIT 0x00008000
#endif
#ifndef GL_ALL_BARRIER_BITS
#    define GL_ALL_BARRIER_BITS 0xFFFFFFFF
#endif


#ifndef GL_SAMPLES_PASSED
#    define GL_SAMPLES_PASSED 0
#endif

#ifndef GL_PRIMITIVES_GENERATED
#    define GL_PRIMITIVES_GENERATED 0
#endif


// Define unsupported GL function stubs
template <typename T>
void UnsupportedGLFunctionStub(const T& Name)
{
    LOG_ERROR_MESSAGE(Name, "() is not supported on iOS!\n");
}

#define glDrawElementsInstancedBaseVertexBaseInstance(...) UnsupportedGLFunctionStub("glDrawElementsInstancedBaseVertexBaseInstance")
#define glDrawElementsInstancedBaseVertex(...)             UnsupportedGLFunctionStub("glDrawElementsInstancedBaseVertex")
#define glDrawElementsInstancedBaseInstance(...)           UnsupportedGLFunctionStub("glDrawElementsInstancedBaseInstance")
#define glDrawArraysInstancedBaseInstance(...)             UnsupportedGLFunctionStub("glDrawArraysInstancedBaseInstance")
#define glDrawElementsBaseVertex(...)                      UnsupportedGLFunctionStub("glDrawElementsBaseVertex")
#define glTextureView(...)                                 UnsupportedGLFunctionStub("glTextureView")
#define glTexStorage1D(...)                                UnsupportedGLFunctionStub("glTexStorage1D")
#define glTexSubImage1D(...)                               UnsupportedGLFunctionStub("glTexSubImage1D")
#define glTexStorage3DMultisample(...)                     UnsupportedGLFunctionStub("glTexStorage3DMultisample")
#define glViewportIndexedf(...)                            UnsupportedGLFunctionStub("glViewportIndexedf")
#define glScissorIndexed(...)                              UnsupportedGLFunctionStub("glScissorIndexed")
static void (*glPolygonMode)(GLenum face, GLenum mode) = nullptr;
#define glEnablei(...)                UnsupportedGLFunctionStub("glEnablei")
#define glBlendFuncSeparatei(...)     UnsupportedGLFunctionStub("glBlendFuncSeparatei")
#define glBlendEquationSeparatei(...) UnsupportedGLFunctionStub("glBlendEquationSeparatei")
#define glDisablei(...)               UnsupportedGLFunctionStub("glDisablei")
#define glColorMaski(...)             UnsupportedGLFunctionStub("glColorMaski")
#define glFramebufferTexture(...)     UnsupportedGLFunctionStub("glFramebufferTexture")
#define glFramebufferTexture1D(...)   UnsupportedGLFunctionStub("glFramebufferTexture1D")
static void (*glGetQueryObjectui64v)(GLuint id, GLenum pname, GLuint64* params) = nullptr;
