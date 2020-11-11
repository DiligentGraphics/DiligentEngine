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

// clang-format off

#ifndef GLAPIENTRY
#   define GLAPIENTRY GL_APIENTRY
#endif

// Define unsupported formats for OpenGL ES
#ifndef GL_RGBA16
#   define GL_RGBA16 0x805B
#endif

#ifndef GL_RGBA16_SNORM
#   define GL_RGBA16_SNORM 0x8F9B
#endif

#ifndef GL_RG16
#   define GL_RG16 0x822C
#endif

#ifndef GL_RG16_SNORM
#   define GL_RG16_SNORM 0x8F99
#endif

#ifndef GL_R16
#   define GL_R16 0x822A
#endif

#ifndef GL_R16_SNORM
#   define GL_R16_SNORM 0x8F98
#endif

#ifndef GL_COMPRESSED_RGB_S3TC_DXT1_EXT
#   define GL_COMPRESSED_RGB_S3TC_DXT1_EXT 0x83F0
#endif

#ifndef GL_COMPRESSED_SRGB_S3TC_DXT1_EXT
#   define GL_COMPRESSED_SRGB_S3TC_DXT1_EXT 0x8C4C
#endif

#ifndef GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
#   define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#endif

#ifndef GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT
#   define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT 0x8C4E
#endif

#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
#   define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif

#ifndef GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT
#   define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT 0x8C4F
#endif

#ifndef GL_COMPRESSED_RED_RGTC1
#   define GL_COMPRESSED_RED_RGTC1 0x8DBB
#endif

#ifndef GL_COMPRESSED_SIGNED_RED_RGTC1
#   define GL_COMPRESSED_SIGNED_RED_RGTC1 0x8DBC
#endif

#ifndef GL_COMPRESSED_RG_RGTC2
#   define GL_COMPRESSED_RG_RGTC2 0x8DBD
#endif

#ifndef GL_COMPRESSED_SIGNED_RG_RGTC2
#   define GL_COMPRESSED_SIGNED_RG_RGTC2 0x8DBE
#endif

#ifndef GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT
#   define GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT 0x8E8F
#endif

#ifndef GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT
#   define GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT 0x8E8E
#endif

#ifndef GL_COMPRESSED_RGBA_BPTC_UNORM
#   define GL_COMPRESSED_RGBA_BPTC_UNORM 0x8E8C
#endif

#ifndef GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM
#   define GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM 0x8E8D
#endif

#ifndef GL_UNSIGNED_SHORT_5_6_5_REV
#   define GL_UNSIGNED_SHORT_5_6_5_REV 0x8364
#endif

#ifndef GL_UNSIGNED_INT_10_10_10_2
#   define GL_UNSIGNED_INT_10_10_10_2 0x8036
#endif

#ifndef GL_UNSIGNED_SHORT_5_6_5_REV
#   define GL_UNSIGNED_SHORT_5_6_5_REV 0x8364
#endif

#ifndef GL_UNSIGNED_SHORT_1_5_5_5_REV
#   define GL_UNSIGNED_SHORT_1_5_5_5_REV 0x8366
#endif

// Define unsupported shaders
#ifndef GL_GEOMETRY_SHADER
#   define GL_GEOMETRY_SHADER 0x8DD9
#endif

#ifndef GL_TESS_CONTROL_SHADER
#   define GL_TESS_CONTROL_SHADER 0x8E88
#endif

#ifndef GL_TESS_EVALUATION_SHADER
#   define GL_TESS_EVALUATION_SHADER 0x8E87
#endif

// Define unsupported texture filtering modes
#ifndef GL_CLAMP_TO_BORDER
#   define GL_CLAMP_TO_BORDER 0
#endif

#ifndef GL_MIRROR_CLAMP_TO_EDGE
#   define GL_MIRROR_CLAMP_TO_EDGE 0
#endif

// Define unsupported bind points
#ifndef GL_ARB_draw_indirect
#   define GL_ARB_draw_indirect 1
#endif

#ifndef GL_DRAW_INDIRECT_BUFFER
#   define GL_DRAW_INDIRECT_BUFFER 0x8F3F
#endif

#ifndef GL_DISPATCH_INDIRECT_BUFFER
#   define GL_DISPATCH_INDIRECT_BUFFER 0x90EE
#endif

#ifndef GL_TEXTURE_1D_ARRAY
#   define GL_TEXTURE_1D_ARRAY 0x8C18
#endif

#ifndef GL_TEXTURE_BINDING_1D_ARRAY
#   define GL_TEXTURE_BINDING_1D_ARRAY 0x8C1C
#endif

#ifndef GL_TEXTURE_1D
#   define GL_TEXTURE_1D 0x0DE0
#endif

#ifndef GL_TEXTURE_BINDING_1D
#   define GL_TEXTURE_BINDING_1D 0x8068
#endif

#ifndef GL_ARB_texture_storage_multisample
#   define GL_ARB_texture_storage_multisample 1
#endif

#ifndef GL_TEXTURE_2D_MULTISAMPLE
#   define GL_TEXTURE_2D_MULTISAMPLE 0x9100
#endif

#ifndef GL_TEXTURE_BINDING_2D_MULTISAMPLE
#   define GL_TEXTURE_BINDING_2D_MULTISAMPLE 0x9104
#endif

#ifndef GL_TEXTURE_2D_MULTISAMPLE_ARRAY
#   define GL_TEXTURE_2D_MULTISAMPLE_ARRAY 0x9102
#endif

#ifndef GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY
#   define GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY 0x9105
#endif

#ifndef GL_TEXTURE_CUBE_MAP_ARRAY
#   define GL_TEXTURE_CUBE_MAP_ARRAY 0x9009
#endif

#ifndef GL_TEXTURE_BINDING_CUBE_MAP_ARRAY
#   define GL_TEXTURE_BINDING_CUBE_MAP_ARRAY 0x900A
#endif

#ifndef GL_TEXTURE_BUFFER
#   define GL_TEXTURE_BUFFER 0x8C2A
#endif

// Define unsupported pipeline bind flags
#ifndef GL_VERTEX_SHADER_BIT
#   define GL_VERTEX_SHADER_BIT 0x00000001
#endif

#ifndef GL_FRAGMENT_SHADER_BIT
#   define GL_FRAGMENT_SHADER_BIT 0x00000002
#endif

#ifndef GL_GEOMETRY_SHADER_BIT
#   define GL_GEOMETRY_SHADER_BIT 0x00000004
#endif

#ifndef GL_TESS_CONTROL_SHADER_BIT
#   define GL_TESS_CONTROL_SHADER_BIT 0x00000008
#endif

#ifndef GL_TESS_EVALUATION_SHADER_BIT
#   define GL_TESS_EVALUATION_SHADER_BIT 0x00000010
#endif

#ifndef GL_COMPUTE_SHADER_BIT
#   define GL_COMPUTE_SHADER_BIT 0x00000020
#endif

// Define unsupported sampler attributes
#ifndef GL_TEXTURE_LOD_BIAS
#   define GL_TEXTURE_LOD_BIAS 0
#endif

#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#   define GL_TEXTURE_MAX_ANISOTROPY_EXT 0
#endif

#ifndef GL_TEXTURE_BORDER_COLOR
#   define GL_TEXTURE_BORDER_COLOR 0
#endif

// Other unsupported attributes
#ifndef GL_PROGRAM_SEPARABLE
#   define GL_PROGRAM_SEPARABLE 0x8258
#endif

// Define unsupported uniform data types
#ifndef GL_SAMPLER_1D
    #define GL_SAMPLER_1D 0x8B5D
#endif

#ifndef GL_SAMPLER_1D_SHADOW
    #define GL_SAMPLER_1D_SHADOW 0x8B61
#endif

#ifndef GL_SAMPLER_1D_ARRAY 
    #define GL_SAMPLER_1D_ARRAY 0x8DC0
#endif

#ifndef GL_SAMPLER_1D_ARRAY_SHADOW
    #define GL_SAMPLER_1D_ARRAY_SHADOW 0x8DC3
#endif

#ifndef GL_INT_SAMPLER_1D
    #define GL_INT_SAMPLER_1D 0x8DC9
#endif

#ifndef GL_INT_SAMPLER_1D_ARRAY
    #define GL_INT_SAMPLER_1D_ARRAY 0x8DCE
#endif

#ifndef GL_UNSIGNED_INT_SAMPLER_1D
    #define GL_UNSIGNED_INT_SAMPLER_1D 0x8DD1
#endif

#ifndef GL_UNSIGNED_INT_SAMPLER_1D_ARRAY
    #define GL_UNSIGNED_INT_SAMPLER_1D_ARRAY 0x8DD6
#endif

#ifndef GL_SAMPLER_CUBE_MAP_ARRAY
    #define GL_SAMPLER_CUBE_MAP_ARRAY 0x900C
#endif

#ifndef GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW
    #define GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW 0x900D
#endif

#ifndef GL_INT_SAMPLER_CUBE_MAP_ARRAY
    #define GL_INT_SAMPLER_CUBE_MAP_ARRAY 0x900E
#endif

#ifndef GL_UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY
    #define GL_UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY 0x900F
#endif

#ifndef GL_SAMPLER_BUFFER
    #define GL_SAMPLER_BUFFER 0x8DC2
#endif

#ifndef GL_INT_SAMPLER_BUFFER
    #define GL_INT_SAMPLER_BUFFER 0x8DD0
#endif

#ifndef GL_UNSIGNED_INT_SAMPLER_BUFFER
    #define GL_UNSIGNED_INT_SAMPLER_BUFFER 0x8DD8
#endif

#ifndef GL_SAMPLER_2D_MULTISAMPLE
    #define GL_SAMPLER_2D_MULTISAMPLE 0x9108
#endif

#ifndef GL_INT_SAMPLER_2D_MULTISAMPLE 
    #define GL_INT_SAMPLER_2D_MULTISAMPLE 0x9109
#endif

#ifndef GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE
    #define GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE 0x910A
#endif

#ifndef GL_SAMPLER_2D_MULTISAMPLE_ARRAY
    #define GL_SAMPLER_2D_MULTISAMPLE_ARRAY 0x910B
#endif

#ifndef GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY
    #define GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY 0x910C
#endif

#ifndef GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY
    #define GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY 0x910D
#endif

#ifndef GL_SAMPLER_EXTERNAL_OES
    #define GL_SAMPLER_EXTERNAL_OES 0x8D66
#endif


#ifndef GL_IMAGE_1D
    #define GL_IMAGE_1D 0x904C
#endif

#ifndef GL_IMAGE_2D
    #define GL_IMAGE_2D 0x904D
#endif

#ifndef GL_IMAGE_3D
    #define GL_IMAGE_3D 0x904E
#endif

#ifndef GL_IMAGE_2D_RECT
    #define GL_IMAGE_2D_RECT 0x904F
#endif

#ifndef GL_IMAGE_CUBE
    #define GL_IMAGE_CUBE 0x9050
#endif

#ifndef GL_IMAGE_BUFFER
    #define GL_IMAGE_BUFFER 0x9051
#endif

#ifndef GL_IMAGE_1D_ARRAY
    #define GL_IMAGE_1D_ARRAY 0x9052
#endif

#ifndef GL_IMAGE_2D_ARRAY
    #define GL_IMAGE_2D_ARRAY 0x9053
#endif

#ifndef GL_IMAGE_CUBE_MAP_ARRAY
    #define GL_IMAGE_CUBE_MAP_ARRAY 0x9054
#endif

#ifndef GL_IMAGE_2D_MULTISAMPLE
    #define GL_IMAGE_2D_MULTISAMPLE 0x9055
#endif

#ifndef GL_IMAGE_2D_MULTISAMPLE_ARRAY
    #define GL_IMAGE_2D_MULTISAMPLE_ARRAY 0x9056
#endif

#ifndef GL_INT_IMAGE_1D
    #define GL_INT_IMAGE_1D 0x9057
#endif

#ifndef GL_INT_IMAGE_2D
    #define GL_INT_IMAGE_2D 0x9058
#endif

#ifndef GL_INT_IMAGE_3D
    #define GL_INT_IMAGE_3D 0x9059
#endif

#ifndef GL_INT_IMAGE_2D_RECT
    #define GL_INT_IMAGE_2D_RECT 0x905A
#endif

#ifndef GL_INT_IMAGE_CUBE
    #define GL_INT_IMAGE_CUBE 0x905B
#endif

#ifndef GL_INT_IMAGE_BUFFER
    #define GL_INT_IMAGE_BUFFER 0x905C
#endif

#ifndef GL_INT_IMAGE_1D_ARRAY
    #define GL_INT_IMAGE_1D_ARRAY 0x905D
#endif

#ifndef GL_INT_IMAGE_2D_ARRAY
    #define GL_INT_IMAGE_2D_ARRAY 0x905E
#endif

#ifndef GL_INT_IMAGE_CUBE_MAP_ARRAY
    #define GL_INT_IMAGE_CUBE_MAP_ARRAY 0x905F
#endif

#ifndef GL_INT_IMAGE_2D_MULTISAMPLE
    #define GL_INT_IMAGE_2D_MULTISAMPLE 0x9060
#endif

#ifndef GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY
    #define GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY 0x9061
#endif

#ifndef GL_UNSIGNED_INT_IMAGE_1D
    #define GL_UNSIGNED_INT_IMAGE_1D 0x9062
#endif

#ifndef GL_UNSIGNED_INT_IMAGE_2D
    #define GL_UNSIGNED_INT_IMAGE_2D 0x9063
#endif

#ifndef GL_UNSIGNED_INT_IMAGE_3D
    #define GL_UNSIGNED_INT_IMAGE_3D 0x9064
#endif

#ifndef GL_UNSIGNED_INT_IMAGE_2D_RECT
    #define GL_UNSIGNED_INT_IMAGE_2D_RECT 0x9065
#endif

#ifndef GL_UNSIGNED_INT_IMAGE_CUBE
    #define GL_UNSIGNED_INT_IMAGE_CUBE 0x9066
#endif

#ifndef GL_UNSIGNED_INT_IMAGE_BUFFER
    #define GL_UNSIGNED_INT_IMAGE_BUFFER 0x9067
#endif

#ifndef GL_UNSIGNED_INT_IMAGE_1D_ARRAY
    #define GL_UNSIGNED_INT_IMAGE_1D_ARRAY 0x9068
#endif

#ifndef GL_UNSIGNED_INT_IMAGE_2D_ARRAY
    #define GL_UNSIGNED_INT_IMAGE_2D_ARRAY 0x9069
#endif

#ifndef GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY
    #define GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY 0x906A
#endif

#ifndef GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE
    #define GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE 0x906B
#endif

#ifndef GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY
    #define GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY 0x906C
#endif


// Compute shader stubs
#ifndef GL_ARB_compute_shader
#   define GL_ARB_compute_shader 1
#endif

#ifndef GL_READ_ONLY
#   define GL_READ_ONLY 0x88B8
#endif

#ifndef GL_WRITE_ONLY
#   define GL_WRITE_ONLY 0x88B9
#endif

#ifndef GL_READ_WRITE
#   define GL_READ_WRITE 0x88BA
#endif

#ifndef GL_COMPUTE_SHADER
#   define GL_COMPUTE_SHADER 0x91B9
#endif

#ifndef GL_ES_VERSION_3_1
#define LOAD_GL_BIND_IMAGE_TEXTURE
typedef void (GL_APIENTRY* PFNGLBINDIMAGETEXTUREPROC) (GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format);
extern PFNGLBINDIMAGETEXTUREPROC glBindImageTexture;

#define LOAD_GL_DISPATCH_COMPUTE
typedef void (GL_APIENTRY* PFNGLDISPATCHCOMPUTEPROC) (GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z);
extern PFNGLDISPATCHCOMPUTEPROC glDispatchCompute;

#define LOAD_GL_MEMORY_BARRIER 
typedef void (GL_APIENTRY* PFNGLMEMORYBARRIERPROC) (GLbitfield barriers);
extern PFNGLMEMORYBARRIERPROC glMemoryBarrier;
#endif // GL_ES_VERSION_3_1

#ifndef GL_ARB_shader_image_load_store
#   define GL_ARB_shader_image_load_store 1
#endif

#ifndef GL_ARB_shader_storage_buffer_object
#   define GL_ARB_shader_storage_buffer_object 1
#endif

#ifndef GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT
#   define GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT   0x00000001
#endif
#ifndef GL_ELEMENT_ARRAY_BARRIER_BIT
#   define GL_ELEMENT_ARRAY_BARRIER_BIT         0x00000002
#endif
#ifndef GL_UNIFORM_BARRIER_BIT
#   define GL_UNIFORM_BARRIER_BIT               0x00000004
#endif
#ifndef GL_TEXTURE_FETCH_BARRIER_BIT
#   define GL_TEXTURE_FETCH_BARRIER_BIT         0x00000008
#endif
#ifndef GL_SHADER_IMAGE_ACCESS_BARRIER_BIT
#   define GL_SHADER_IMAGE_ACCESS_BARRIER_BIT   0x00000020
#endif
#ifndef GL_COMMAND_BARRIER_BIT
#   define GL_COMMAND_BARRIER_BIT               0x00000040
#endif
#ifndef GL_PIXEL_BUFFER_BARRIER_BIT
#   define GL_PIXEL_BUFFER_BARRIER_BIT          0x00000080
#endif
#ifndef GL_TEXTURE_UPDATE_BARRIER_BIT
#   define GL_TEXTURE_UPDATE_BARRIER_BIT        0x00000100
#endif
#ifndef GL_BUFFER_UPDATE_BARRIER_BIT
#   define GL_BUFFER_UPDATE_BARRIER_BIT         0x00000200
#endif
#ifndef GL_FRAMEBUFFER_BARRIER_BIT
#   define GL_FRAMEBUFFER_BARRIER_BIT           0x00000400
#endif
#ifndef GL_TRANSFORM_FEEDBACK_BARRIER_BIT
#   define GL_TRANSFORM_FEEDBACK_BARRIER_BIT    0x00000800
#endif
#ifndef GL_ATOMIC_COUNTER_BARRIER_BIT
#   define GL_ATOMIC_COUNTER_BARRIER_BIT        0x00001000
#endif
#ifndef GL_SHADER_STORAGE_BARRIER_BIT
#   define GL_SHADER_STORAGE_BARRIER_BIT        0x00002000
#endif
#ifndef GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT
#   define GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT  0x00004000
#endif
#ifndef GL_QUERY_BUFFER_BARRIER_BIT
#   define GL_QUERY_BUFFER_BARRIER_BIT          0x00008000
#endif
#ifndef GL_ALL_BARRIER_BITS
#   define GL_ALL_BARRIER_BITS                  0xFFFFFFFF
#endif


//--------------------------- Texture queries -----------------------------
#ifndef GL_TEXTURE_WIDTH
#   define GL_TEXTURE_WIDTH 0x1000
#endif

#ifndef GL_TEXTURE_HEIGHT
#   define GL_TEXTURE_HEIGHT 0x1001
#endif

#ifndef GL_TEXTURE_DEPTH
#   define GL_TEXTURE_DEPTH 0x8071
#endif

#ifndef GL_TEXTURE_INTERNAL_FORMAT
#   define GL_TEXTURE_INTERNAL_FORMAT 0x1003
#endif


//------------------------ Program interface query ------------------------
#ifndef GL_UNIFORM
#   define GL_UNIFORM 0x92E1
#endif

#ifndef GL_UNIFORM_BLOCK
#   define GL_UNIFORM_BLOCK 0x92E2
#endif

#ifndef GL_PROGRAM_INPUT
#   define GL_PROGRAM_INPUT 0x92E3
#endif

#ifndef GL_PROGRAM_OUTPUT
#   define GL_PROGRAM_OUTPUT 0x92E4
#endif

#ifndef GL_BUFFER_VARIABLE
#   define GL_BUFFER_VARIABLE 0x92E5
#endif

#ifndef GL_ARB_program_interface_query
#   define GL_ARB_program_interface_query 1
#endif

#ifndef GL_SHADER_STORAGE_BLOCK
#   define GL_SHADER_STORAGE_BLOCK 0x92E6
#endif

#ifndef GL_IS_PER_PATCH
#   define GL_IS_PER_PATCH 0x92E7
#endif

#ifndef GL_VERTEX_SUBROUTINE
#   define GL_VERTEX_SUBROUTINE 0x92E8
#endif

#ifndef GL_TESS_CONTROL_SUBROUTINE
#   define GL_TESS_CONTROL_SUBROUTINE 0x92E9
#endif

#ifndef GL_TESS_EVALUATION_SUBROUTINE
#   define GL_TESS_EVALUATION_SUBROUTINE 0x92EA
#endif

#ifndef GL_GEOMETRY_SUBROUTINE
#   define GL_GEOMETRY_SUBROUTINE 0x92EB
#endif

#ifndef GL_FRAGMENT_SUBROUTINE
#   define GL_FRAGMENT_SUBROUTINE 0x92EC
#endif

#ifndef GL_COMPUTE_SUBROUTINE
#   define GL_COMPUTE_SUBROUTINE 0x92ED
#endif

#ifndef GL_VERTEX_SUBROUTINE_UNIFORM
#   define GL_VERTEX_SUBROUTINE_UNIFORM 0x92EE
#endif

#ifndef GL_TESS_CONTROL_SUBROUTINE_UNIFORM
#   define GL_TESS_CONTROL_SUBROUTINE_UNIFORM 0x92EF
#endif

#ifndef GL_TESS_EVALUATION_SUBROUTINE_UNIFORM
#   define GL_TESS_EVALUATION_SUBROUTINE_UNIFORM 0x92F0
#endif

#ifndef GL_GEOMETRY_SUBROUTINE_UNIFORM
#   define GL_GEOMETRY_SUBROUTINE_UNIFORM 0x92F1
#endif

#ifndef GL_FRAGMENT_SUBROUTINE_UNIFORM
#   define GL_FRAGMENT_SUBROUTINE_UNIFORM 0x92F2
#endif

#ifndef GL_COMPUTE_SUBROUTINE_UNIFORM
#   define GL_COMPUTE_SUBROUTINE_UNIFORM 0x92F3
#endif

#ifndef GL_TRANSFORM_FEEDBACK_VARYING
#   define GL_TRANSFORM_FEEDBACK_VARYING 0x92F4
#endif

#ifndef GL_ACTIVE_RESOURCES
#   define GL_ACTIVE_RESOURCES 0x92F5
#endif

#ifndef GL_MAX_NAME_LENGTH
#   define GL_MAX_NAME_LENGTH 0x92F6
#endif

#ifndef GL_MAX_NUM_ACTIVE_VARIABLES
#   define GL_MAX_NUM_ACTIVE_VARIABLES 0x92F7
#endif

#ifndef GL_MAX_NUM_COMPATIBLE_SUBROUTINES
#   define GL_MAX_NUM_COMPATIBLE_SUBROUTINES 0x92F8
#endif

#ifndef GL_NAME_LENGTH
#   define GL_NAME_LENGTH 0x92F9
#endif

#ifndef GL_TYPE
#   define GL_TYPE 0x92FA
#endif

#ifndef GL_ARRAY_SIZE
#   define GL_ARRAY_SIZE 0x92FB
#endif

#ifndef GL_OFFSET
#   define GL_OFFSET 0x92FC
#endif

#ifndef GL_BLOCK_INDEX
#   define GL_BLOCK_INDEX 0x92FD
#endif

#ifndef GL_ARRAY_STRIDE
#   define GL_ARRAY_STRIDE 0x92FE
#endif

#ifndef GL_MATRIX_STRIDE
#   define GL_MATRIX_STRIDE 0x92FF
#endif

#ifndef GL_IS_ROW_MAJOR
#   define GL_IS_ROW_MAJOR 0x9300
#endif

#ifndef GL_ATOMIC_COUNTER_BUFFER_INDEX
#   define GL_ATOMIC_COUNTER_BUFFER_INDEX 0x9301
#endif

#ifndef GL_BUFFER_BINDING
#   define GL_BUFFER_BINDING 0x9302
#endif

#ifndef GL_BUFFER_DATA_SIZE
#   define GL_BUFFER_DATA_SIZE 0x9303
#endif

#ifndef GL_NUM_ACTIVE_VARIABLES
#   define GL_NUM_ACTIVE_VARIABLES 0x9304
#endif

#ifndef GL_ACTIVE_VARIABLES
#   define GL_ACTIVE_VARIABLES 0x9305
#endif

#ifndef GL_REFERENCED_BY_VERTEX_SHADER
#   define GL_REFERENCED_BY_VERTEX_SHADER 0x9306
#endif

#ifndef GL_REFERENCED_BY_TESS_CONTROL_SHADER
# define GL_REFERENCED_BY_TESS_CONTROL_SHADER 0x9307
#endif

#ifndef GL_REFERENCED_BY_TESS_EVALUATION_SHADER
#   define GL_REFERENCED_BY_TESS_EVALUATION_SHADER 0x9308
#endif

#ifndef GL_REFERENCED_BY_GEOMETRY_SHADER
# define GL_REFERENCED_BY_GEOMETRY_SHADER 0x9309
#endif

#ifndef GL_REFERENCED_BY_FRAGMENT_SHADER
#   define GL_REFERENCED_BY_FRAGMENT_SHADER 0x930A
#endif

#ifndef GL_REFERENCED_BY_COMPUTE_SHADER
#   define GL_REFERENCED_BY_COMPUTE_SHADER 0x930B
#endif

#ifndef GL_TOP_LEVEL_ARRAY_SIZE
# define GL_TOP_LEVEL_ARRAY_SIZE 0x930C
#endif

#ifndef GL_TOP_LEVEL_ARRAY_STRIDE
#   define GL_TOP_LEVEL_ARRAY_STRIDE 0x930D
#endif

#ifndef GL_LOCATION
#   define GL_LOCATION 0x930E
#endif

#ifndef GL_LOCATION_INDEX
#   define GL_LOCATION_INDEX 0x930F
#endif



// ---------------------  Shader storage buffer -----------------------

#ifndef GL_MAX_COMBINED_SHADER_OUTPUT_RESOURCES
#   define GL_MAX_COMBINED_SHADER_OUTPUT_RESOURCES 0x8F39
#endif

#ifndef GL_SHADER_STORAGE_BUFFER
#   define GL_SHADER_STORAGE_BUFFER 0x90D2
#endif

#ifndef GL_SHADER_STORAGE_BUFFER_BINDING
#   define GL_SHADER_STORAGE_BUFFER_BINDING 0x90D3
#endif

#ifndef GL_SHADER_STORAGE_BUFFER_START
#   define GL_SHADER_STORAGE_BUFFER_START 0x90D4
#endif

#ifndef GL_SHADER_STORAGE_BUFFER_SIZE
#   define GL_SHADER_STORAGE_BUFFER_SIZE 0x90D5
#endif

#ifndef GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS
#   define GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS 0x90D6
#endif

#ifndef GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS
#   define GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS 0x90D7
#endif

#ifndef GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS
#   define GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS 0x90D8
#endif

#ifndef GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS
#   define GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS 0x90D9
#endif

#ifndef GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS
#   define GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS 0x90DA
#endif

#ifndef GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS
#   define GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS 0x90DB
#endif

#ifndef GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS
#   define GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS 0x90DC
#endif

#ifndef GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS
#   define GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS 0x90DD
#endif

#ifndef GL_MAX_SHADER_STORAGE_BLOCK_SIZE
#   define GL_MAX_SHADER_STORAGE_BLOCK_SIZE 0x90DE
#endif

#ifndef GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT
#   define GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT 0x90DF
#endif


// Polygon mode
#ifndef GL_POINT
#   define GL_POINT 0x1B00
#endif              
                    
#ifndef GL_LINE     
#   define GL_LINE  0x1B01
#endif              
                    
#ifndef GL_FILL     
#   define GL_FILL  0x1B02
#endif


#ifndef GL_DEPTH_CLAMP
#   define GL_DEPTH_CLAMP 0
#endif

// Blend functions
#ifndef GL_SRC1_COLOR
#   define GL_SRC1_COLOR 0x88F9
#endif

#ifndef GL_ONE_MINUS_SRC1_COLOR
#   define GL_ONE_MINUS_SRC1_COLOR 0x88FA
#endif

#ifndef GL_SOURCE1_ALPHA
#   define GL_SOURCE1_ALPHA 0x8589
#endif

#ifndef GL_SRC1_ALPHA
#   define GL_SRC1_ALPHA GL_SOURCE1_ALPHA
#endif

#ifndef GL_ONE_MINUS_SRC1_ALPHA
#   define GL_ONE_MINUS_SRC1_ALPHA 0x88FB
#endif

/* ---------------------- GL_ARB_internalformat_query2 --------------------- */
#ifndef GL_ARB_internalformat_query2
#   define GL_ARB_internalformat_query2 1
#endif

#ifndef GL_INTERNALFORMAT_SUPPORTED
#   define GL_INTERNALFORMAT_SUPPORTED 0x826F
#endif

// ---------------------  Framebuffer SRGB -----------------------
#ifndef GL_FRAMEBUFFER_SRGB
#   define GL_FRAMEBUFFER_SRGB 0x8DB9
#endif

// -------------------- Incomplete FBO error codes ---------------
#ifndef GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS
#   define GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS 0x8DA8
#endif

#ifndef GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER
#   define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER 0x8CDB
#endif

#ifndef GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER
#   define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER 0x8CDC
#endif

/* ---------------------- ARB_tessellation_shader --------------------- */
#ifndef GL_ARB_tessellation_shader
#   define GL_ARB_tessellation_shader 1
#endif

#ifndef GL_PATCHES
#   define GL_PATCHES 0xE
#endif

#ifndef GL_PATCH_VERTICES
#   define GL_PATCH_VERTICES 0x8E72
#endif


/* ------------------------------ GL_KHR_debug ----------------------------- */
#ifndef GL_KHR_debug
#   define GL_KHR_debug 1
#endif

#ifndef GL_DEBUG_OUTPUT
#   define GL_DEBUG_OUTPUT 0x92E0
#endif

#ifndef GL_DEBUG_OUTPUT_SYNCHRONOUS
#   define GL_DEBUG_OUTPUT_SYNCHRONOUS 0x8242
#endif

#ifndef GL_DEBUG_SOURCE_API
#   define GL_DEBUG_SOURCE_API 0x8246
#endif

#ifndef GL_DEBUG_SOURCE_WINDOW_SYSTEM
#   define GL_DEBUG_SOURCE_WINDOW_SYSTEM 0x8247
#endif

#ifndef GL_DEBUG_SOURCE_SHADER_COMPILER
#   define GL_DEBUG_SOURCE_SHADER_COMPILER 0x8248
#endif

#ifndef GL_DEBUG_SOURCE_THIRD_PARTY
#   define GL_DEBUG_SOURCE_THIRD_PARTY 0x8249
#endif

#ifndef GL_DEBUG_SOURCE_APPLICATION
#   define GL_DEBUG_SOURCE_APPLICATION 0x824A
#endif

#ifndef GL_DEBUG_SOURCE_OTHER
#   define GL_DEBUG_SOURCE_OTHER 0x824B
#endif



#ifndef GL_DEBUG_TYPE_ERROR
#   define GL_DEBUG_TYPE_ERROR 0x824C
#endif

#ifndef GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR
#   define GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR 0x824D
#endif

#ifndef GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR
#   define GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR 0x824E
#endif

#ifndef GL_DEBUG_TYPE_PORTABILITY
#   define GL_DEBUG_TYPE_PORTABILITY 0x824F
#endif

#ifndef GL_DEBUG_TYPE_PERFORMANCE
#   define GL_DEBUG_TYPE_PERFORMANCE 0x8250
#endif

#ifndef GL_DEBUG_TYPE_MARKER
#   define GL_DEBUG_TYPE_MARKER 0x8268
#endif

#ifndef GL_DEBUG_TYPE_PUSH_GROUP
#   define GL_DEBUG_TYPE_PUSH_GROUP 0x8269
#endif

#ifndef GL_DEBUG_TYPE_POP_GROUP
#   define GL_DEBUG_TYPE_POP_GROUP 0x826A
#endif

#ifndef GL_DEBUG_TYPE_OTHER
#   define GL_DEBUG_TYPE_OTHER 0x8251
#endif



#ifndef GL_DEBUG_SEVERITY_HIGH
#   define GL_DEBUG_SEVERITY_HIGH 0x9146
#endif

#ifndef GL_DEBUG_SEVERITY_MEDIUM
#   define GL_DEBUG_SEVERITY_MEDIUM 0x9147
#endif

#ifndef GL_DEBUG_SEVERITY_LOW
#   define GL_DEBUG_SEVERITY_LOW 0x9148
#endif

#ifndef GL_DEBUG_SEVERITY_NOTIFICATION
#   define GL_DEBUG_SEVERITY_NOTIFICATION 0x826B
#endif


/* ------------------------------ GL_EXT_disjoint_timer_query ----------------------------- */
#ifndef GL_TIMESTAMP
#   define GL_TIMESTAMP 0x8E28
#endif

#ifndef GL_TIME_ELAPSED
#   define GL_TIME_ELAPSED 0x88BF
#endif



// Define unsupported GL function stubs
template<typename T>
void UnsupportedGLFunctionStub( const T &Name )
{
    LOG_ERROR_MESSAGE( Name, "() is not supported in this API!\n" );
}

#define glTexStorage1D(...) UnsupportedGLFunctionStub("glTexStorage1D")
#define glTexSubImage1D(...) UnsupportedGLFunctionStub("glTexSubImage1D")

#ifndef GL_ES_VERSION_3_1

    #define LOAD_GEN_PROGRAM_PIPELINES
    typedef void (GL_APIENTRY* PFNGLGENPROGRAMPIPELINESPROC) (GLsizei n, GLuint* pipelines);    
    extern PFNGLGENPROGRAMPIPELINESPROC glGenProgramPipelines;

    #define LOAD_GL_DELETE_PROGRAM_PIPELINES
    typedef void (GL_APIENTRY* PFNGLDELETEPROGRAMPIPELINESPROC) (GLsizei n, const GLuint* pipelines);
    extern PFNGLDELETEPROGRAMPIPELINESPROC glDeleteProgramPipelines;

    #define LOAD_GL_BIND_PROGRAM_PIPELINE
    typedef void (GL_APIENTRY* PFNGLBINDPROGRAMPIPELINEPROC) (GLuint pipeline);
    extern PFNGLBINDPROGRAMPIPELINEPROC glBindProgramPipeline;

    #define LOAD_DRAW_ELEMENTS_INDIRECT
    typedef void (GL_APIENTRY* PFNGLDRAWELEMENTSINDIRECTPROC) (GLenum mode, GLenum type, const GLvoid *indirect);
    extern PFNGLDRAWELEMENTSINDIRECTPROC glDrawElementsIndirect;

    #define LOAD_DRAW_ARRAYS_INDIRECT
    typedef void (GL_APIENTRY* PFNGLDRAWARRAYSINDIRECTPROC)( GLenum mode, const GLvoid *indirect );
    extern PFNGLDRAWARRAYSINDIRECTPROC glDrawArraysIndirect;

    #define LOAD_DISPATCH_COMPUTE_INDIRECT
    typedef void (GL_APIENTRY* PFNGLDISPATCHCOMPUTEINDIRECTPROC) (GLintptr indirect);
    extern PFNGLDISPATCHCOMPUTEINDIRECTPROC glDispatchComputeIndirect;

    #define LOAD_GL_USE_PROGRAM_STAGES
    typedef void (GL_APIENTRY* PFNGLUSEPROGRAMSTAGESPROC) (GLuint pipeline, GLbitfield stages, GLuint program);
    extern PFNGLUSEPROGRAMSTAGESPROC glUseProgramStages;

    #define LOAD_GL_TEX_STORAGE_2D_MULTISAMPLE
    typedef void (GL_APIENTRY* PFNGLTEXSTORAGE2DMULTISAMPLEPROC) (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations);
    extern PFNGLTEXSTORAGE2DMULTISAMPLEPROC glTexStorage2DMultisample;

    #define LOAD_GL_PROGRAM_UNIFORM_1I
    typedef void (GL_APIENTRY* PFNGLPROGRAMUNIFORM1IPROC) (GLuint program, GLint location, GLint x);
    extern PFNGLPROGRAMUNIFORM1IPROC glProgramUniform1i;

    #define LOAD_GL_GET_PROGRAM_INTERFACEIV
    typedef void (GL_APIENTRY* PFNGLGETPROGRAMINTERFACEIVPROC) (GLuint program, GLenum programInterface, GLenum pname, GLint* params);
    extern PFNGLGETPROGRAMINTERFACEIVPROC glGetProgramInterfaceiv;

    #define LOAD_GL_GET_PROGRAM_RESOURCE_NAME
    typedef void (GL_APIENTRY* PFNGLGETPROGRAMRESOURCENAMEPROC) (GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei* length, GLchar *name);
    extern PFNGLGETPROGRAMRESOURCENAMEPROC glGetProgramResourceName;

    #define LOAD_GL_GET_PROGRAM_RESOURCE_INDEX
    typedef GLuint (GL_APIENTRY* PFNGLGETPROGRAMRESOURCEINDEXPROC) (GLuint program, GLenum programInterface, const GLchar *name);
    extern PFNGLGETPROGRAMRESOURCEINDEXPROC glGetProgramResourceIndex;

    #define LOAD_GL_GET_PROGRAM_RESOURCEIV
    typedef void (GL_APIENTRY* PFNGLGETPROGRAMRESOURCEIVPROC) (GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum* props, GLsizei bufSize, GLsizei *length, GLint *params);
    extern PFNGLGETPROGRAMRESOURCEIVPROC glGetProgramResourceiv;

    #define LOAD_GET_TEX_LEVEL_PARAMETER_IV
    typedef void (GL_APIENTRY* PFNGLGETTEXLEVELPARAMETERIVPROC) (GLenum target, GLint level, GLenum pname, GLint *params);
    extern PFNGLGETTEXLEVELPARAMETERIVPROC glGetTexLevelParameteriv;

    #define LOAD_GL_SHADER_STORAGE_BLOCK_BINDING
    typedef void (GL_APIENTRY* PFNGLSHADERSTORAGEBLOCKBINDINGPROC) (GLuint program, GLuint storageBlockIndex, GLuint storageBlockBinding);
    extern PFNGLSHADERSTORAGEBLOCKBINDINGPROC glShaderStorageBlockBinding;

#endif //GL_ES_VERSION_3_1

#define LOAD_GL_TEX_BUFFER
typedef void (GL_APIENTRY* PFNGLTEXBUFFERPROC) (GLenum, GLenum, GLuint);
extern PFNGLTEXBUFFERPROC glTexBuffer;

#define LOAD_GL_VIEWPORT_INDEXEDF
typedef void (GL_APIENTRY* PFNGLVIEWPORTINDEXEDFPROC) (GLuint index, GLfloat x, GLfloat y, GLfloat w, GLfloat h);
extern PFNGLVIEWPORTINDEXEDFPROC glViewportIndexedf;

#define LOAD_GL_SCISSOR_INDEXED
typedef void (GL_APIENTRY* PFNGLSCISSORINDEXEDPROC) (GLuint index, GLint left, GLint bottom, GLsizei width, GLsizei height);
extern PFNGLSCISSORINDEXEDPROC glScissorIndexed;

#define LOAD_GL_POLYGON_MODE
typedef void (GL_APIENTRY* PFNGLPOLYGONMODE) (GLenum face, GLenum mode);
extern PFNGLPOLYGONMODE glPolygonMode;

#define LOAD_GL_ENABLEI
typedef void (GL_APIENTRY* PFNGLENABLEIPROC) (GLenum, GLuint);
extern PFNGLENABLEIPROC glEnablei;

#define LOAD_GL_BLEND_FUNC_SEPARATEI
typedef void (GL_APIENTRY* PFNGLBLENDFUNCSEPARATEIPROC) (GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
extern PFNGLBLENDFUNCSEPARATEIPROC glBlendFuncSeparatei;

#define LOAD_GL_BLEND_EQUATION_SEPARATEI
typedef void (GL_APIENTRY* PFNGLBLENDEQUATIONSEPARATEIPROC) (GLuint buf, GLenum modeRGB, GLenum modeAlpha);
extern PFNGLBLENDEQUATIONSEPARATEIPROC glBlendEquationSeparatei;

#define LOAD_GL_DISABLEI
typedef void (GL_APIENTRY* PFNGLDISABLEIPROC) (GLenum, GLuint);
extern PFNGLDISABLEIPROC glDisablei;

#define LOAD_GL_COLOR_MASKI
typedef void (GL_APIENTRY* PFNGLCOLORMASKIPROC) (GLuint, GLboolean, GLboolean, GLboolean, GLboolean);
extern PFNGLCOLORMASKIPROC glColorMaski;

#define LOAD_GL_PATCH_PARAMTER_I
typedef void (GL_APIENTRY* PFNGLPATCHPARAMETERIPROC) (GLenum pname, GLint value);
extern PFNGLPATCHPARAMETERIPROC glPatchParameteri;

#define LOAD_GL_FRAMEBUFFER_TEXTURE
typedef void (GL_APIENTRY* PFNGLFRAMEBUFFERTEXTUREPROC) (GLenum, GLenum, GLuint, GLint);
extern PFNGLFRAMEBUFFERTEXTUREPROC glFramebufferTexture;

#define LOAD_GL_FRAMEBUFFER_TEXTURE_1D
typedef void (GL_APIENTRY* PFNGLFRAMEBUFFERTEXTURE1DPROC) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
extern PFNGLFRAMEBUFFERTEXTURE1DPROC glFramebufferTexture1D;

#define LOAD_GL_FRAMEBUFFER_TEXTURE_3D
typedef void (GL_APIENTRY* PFNGLFRAMEBUFFERTEXTURE3DPROC) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint layer);
extern PFNGLFRAMEBUFFERTEXTURE3DPROC glFramebufferTexture3D;

#ifndef GL_ARB_copy_image
#   define GL_ARB_copy_image 1
#endif

#define LOAD_GL_COPY_IMAGE_SUB_DATA
typedef void (GL_APIENTRY* PFNGLCOPYIMAGESUBDATAPROC) (GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth);
extern PFNGLCOPYIMAGESUBDATAPROC glCopyImageSubData;

#define LOAD_GL_TEX_STORAGE_3D_MULTISAMPLE
typedef void (GL_APIENTRY* PFNGLTEXSTORAGE3DMULTISAMPLEPROC) (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations);
extern PFNGLTEXSTORAGE3DMULTISAMPLEPROC glTexStorage3DMultisample;

#define LOAD_GL_TEXTURE_VIEW
typedef void (GL_APIENTRY* PFNGLTEXTUREVIEWPROC) (GLuint texture, GLenum target, GLuint origtexture, GLenum internalformat, GLuint minlevel, GLuint numlevels, GLuint minlayer, GLuint numlayers);
extern PFNGLTEXTUREVIEWPROC glTextureView;


#define LOAD_GL_DRAW_ELEMENTS_INSTANCED_BASE_VERTEX_BASE_INSTANCE
typedef void (GL_APIENTRY* PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCEPROC) (GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex, GLuint baseinstance);
extern PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCEPROC glDrawElementsInstancedBaseVertexBaseInstance;

#define LOAD_GL_DRAW_ELEMENTS_INSTANCED_BASE_VERTEX
typedef void (GL_APIENTRY* PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC) (GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex);
extern PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC glDrawElementsInstancedBaseVertex;

#define LOAD_GL_DRAW_ELEMENTS_INSTANCED_BASE_INSTANCE
typedef void (GL_APIENTRY* PFNGLDRAWELEMENTSINSTANCEDBASEINSTANCEPROC) (GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLuint baseinstance);
extern PFNGLDRAWELEMENTSINSTANCEDBASEINSTANCEPROC glDrawElementsInstancedBaseInstance;

#define LOAD_GL_DRAW_ARRAYS_INSTANCED_BASE_INSTANCE
typedef void (GL_APIENTRY* PFNGLDRAWARRAYSINSTANCEDBASEINSTANCEPROC) (GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance);
extern PFNGLDRAWARRAYSINSTANCEDBASEINSTANCEPROC glDrawArraysInstancedBaseInstance;

#define LOAD_GL_DRAW_ELEMENTS_BASE_VERTEX
typedef void (GL_APIENTRY* PFNGLDRAWELEMENTSBASEVERTEXPROC) (GLenum mode, GLsizei count, GLenum type, const void *indices, GLint basevertex);
extern PFNGLDRAWELEMENTSBASEVERTEXPROC glDrawElementsBaseVertex;


#define LOAD_GL_GET_QUERY_OBJECT_UI64V
typedef void (GL_APIENTRY* PFNGLGETQUERYOBJECTUI64VPROC) (GLuint id, GLenum pname, GLuint64* params);
extern PFNGLGETQUERYOBJECTUI64VPROC glGetQueryObjectui64v;

#define LOAD_GL_QUERY_COUNTER
typedef void (GL_APIENTRY* PFNGLQUERYCOUNTERPROC) (GLuint id, GLenum target);
extern PFNGLQUERYCOUNTERPROC glQueryCounter;


#ifndef GL_ES_VERSION_3_2

    typedef void (GL_APIENTRY* GLDEBUGPROC) (GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);

    #define LOAD_DEBUG_MESSAGE_CALLBACK
    typedef void (GL_APIENTRY* PFNGLDEBUGMESSAGECALLBACKPROC) (GLDEBUGPROC callback, const void *userParam);
    extern PFNGLDEBUGMESSAGECALLBACKPROC glDebugMessageCallback;

    #define LOAD_DEBUG_MESSAGE_CONTROL
    typedef void (GL_APIENTRY* PFNGLDEBUGMESSAGECONTROLPROC) (GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint* ids, GLboolean enabled);
    extern PFNGLDEBUGMESSAGECONTROLPROC glDebugMessageControl;
#endif

void LoadGLFunctions();
