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

#include "pch.h"
#include "GLStubsAndroid.h"
#include <EGL/egl.h>

// clang-format off

#define DECLARE_GL_FUNCTION(Func, FuncType, ...)\
    FuncType Func = nullptr;                    \
    void Func##Stub(__VA_ARGS__)                \
    {                                           \
        UnsupportedGLFunctionStub(#Func);       \
    }

#ifdef LOAD_GL_BIND_IMAGE_TEXTURE
    DECLARE_GL_FUNCTION( glBindImageTexture, PFNGLBINDIMAGETEXTUREPROC, GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format )
#endif

#ifdef LOAD_GL_DISPATCH_COMPUTE
    DECLARE_GL_FUNCTION( glDispatchCompute, PFNGLDISPATCHCOMPUTEPROC, GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z )
#endif

#ifdef LOAD_GL_PROGRAM_UNIFORM_1I
    DECLARE_GL_FUNCTION( glProgramUniform1i, PFNGLPROGRAMUNIFORM1IPROC, GLuint program, GLint location, GLint x )
#endif

#ifdef LOAD_GL_MEMORY_BARRIER 
    DECLARE_GL_FUNCTION( glMemoryBarrier, PFNGLMEMORYBARRIERPROC, GLbitfield barriers )
#endif

#ifdef LOAD_DRAW_ELEMENTS_INDIRECT
    DECLARE_GL_FUNCTION( glDrawElementsIndirect, PFNGLDRAWELEMENTSINDIRECTPROC, GLenum mode, GLenum type, const GLvoid *indirect )
#endif

#ifdef LOAD_DRAW_ARRAYS_INDIRECT
    DECLARE_GL_FUNCTION( glDrawArraysIndirect, PFNGLDRAWARRAYSINDIRECTPROC, GLenum mode, const GLvoid *indirect )
#endif

#ifdef LOAD_GEN_PROGRAM_PIPELINES
    DECLARE_GL_FUNCTION( glGenProgramPipelines, PFNGLGENPROGRAMPIPELINESPROC, GLsizei n, GLuint* pipelines )
#endif

#ifdef LOAD_GL_DELETE_PROGRAM_PIPELINES
    DECLARE_GL_FUNCTION( glDeleteProgramPipelines, PFNGLDELETEPROGRAMPIPELINESPROC, GLsizei n, const GLuint* pipelines )
#endif

#ifdef LOAD_GL_BIND_PROGRAM_PIPELINE
    DECLARE_GL_FUNCTION( glBindProgramPipeline, PFNGLBINDPROGRAMPIPELINEPROC, GLuint pipeline )
#endif

#ifdef LOAD_GL_USE_PROGRAM_STAGES
    DECLARE_GL_FUNCTION( glUseProgramStages, PFNGLUSEPROGRAMSTAGESPROC, GLuint pipeline, GLbitfield stages, GLuint program )
#endif

#ifdef LOAD_GL_TEX_STORAGE_2D_MULTISAMPLE
    DECLARE_GL_FUNCTION( glTexStorage2DMultisample, PFNGLTEXSTORAGE2DMULTISAMPLEPROC, GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations )
#endif

#ifdef LOAD_GL_GET_PROGRAM_INTERFACEIV
    DECLARE_GL_FUNCTION( glGetProgramInterfaceiv, PFNGLGETPROGRAMINTERFACEIVPROC, GLuint program, GLenum programInterface, GLenum pname, GLint* params )
#endif

#ifdef LOAD_GL_GET_PROGRAM_RESOURCE_NAME
    DECLARE_GL_FUNCTION( glGetProgramResourceName, PFNGLGETPROGRAMRESOURCENAMEPROC, GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei* length, GLchar *name )
#endif

#ifdef LOAD_GL_GET_PROGRAM_RESOURCE_INDEX
    PFNGLGETPROGRAMRESOURCEINDEXPROC glGetProgramResourceIndex = nullptr;
    GLuint glGetProgramResourceIndexStub(GLuint program, GLenum programInterface, const GLchar *name)
    {
        UnsupportedGLFunctionStub("glGetProgramResourceIndex");
        return 0;
    }
#endif

#ifdef LOAD_GL_GET_PROGRAM_RESOURCEIV
    DECLARE_GL_FUNCTION( glGetProgramResourceiv, PFNGLGETPROGRAMRESOURCEIVPROC, GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum* props, GLsizei bufSize, GLsizei *length, GLint *params )
#endif

#ifdef LOAD_DISPATCH_COMPUTE_INDIRECT
    DECLARE_GL_FUNCTION( glDispatchComputeIndirect, PFNGLDISPATCHCOMPUTEINDIRECTPROC, GLintptr indirect )
#endif

#ifdef LOAD_GL_TEX_BUFFER
    DECLARE_GL_FUNCTION( glTexBuffer, PFNGLTEXBUFFERPROC, GLenum, GLenum, GLuint)
#endif

#ifdef LOAD_GL_POLYGON_MODE
    PFNGLPOLYGONMODE glPolygonMode = nullptr;
#endif

#ifdef LOAD_GL_ENABLEI
    DECLARE_GL_FUNCTION( glEnablei, PFNGLENABLEIPROC, GLenum, GLuint)
#endif

#ifdef LOAD_GL_BLEND_FUNC_SEPARATEI 
    DECLARE_GL_FUNCTION( glBlendFuncSeparatei, PFNGLBLENDFUNCSEPARATEIPROC, GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)
#endif

#ifdef LOAD_GL_BLEND_EQUATION_SEPARATEI
    DECLARE_GL_FUNCTION( glBlendEquationSeparatei, PFNGLBLENDEQUATIONSEPARATEIPROC, GLuint buf, GLenum modeRGB, GLenum modeAlpha)
#endif

#ifdef LOAD_GL_DISABLEI
    DECLARE_GL_FUNCTION( glDisablei, PFNGLDISABLEIPROC, GLenum, GLuint)
#endif

#ifdef LOAD_GL_COLOR_MASKI
    DECLARE_GL_FUNCTION( glColorMaski, PFNGLCOLORMASKIPROC, GLuint, GLboolean, GLboolean, GLboolean, GLboolean)
#endif

#ifdef LOAD_GL_VIEWPORT_INDEXEDF
    DECLARE_GL_FUNCTION( glViewportIndexedf, PFNGLVIEWPORTINDEXEDFPROC, GLuint index, GLfloat x, GLfloat y, GLfloat w, GLfloat h)
#endif

#ifdef LOAD_GL_SCISSOR_INDEXED
    DECLARE_GL_FUNCTION( glScissorIndexed, PFNGLSCISSORINDEXEDPROC, GLuint index, GLint left, GLint bottom, GLsizei width, GLsizei height)
#endif

#ifdef LOAD_GL_FRAMEBUFFER_TEXTURE
    DECLARE_GL_FUNCTION( glFramebufferTexture, PFNGLFRAMEBUFFERTEXTUREPROC, GLenum, GLenum, GLuint, GLint)
#endif

#ifdef LOAD_GL_FRAMEBUFFER_TEXTURE_1D
    DECLARE_GL_FUNCTION( glFramebufferTexture1D, PFNGLFRAMEBUFFERTEXTURE1DPROC, GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
#endif

#ifdef LOAD_GL_FRAMEBUFFER_TEXTURE_3D
    DECLARE_GL_FUNCTION( glFramebufferTexture3D, PFNGLFRAMEBUFFERTEXTURE3DPROC, GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint layer )
#endif

#ifdef LOAD_GL_COPY_IMAGE_SUB_DATA
    DECLARE_GL_FUNCTION( glCopyImageSubData, PFNGLCOPYIMAGESUBDATAPROC, GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth )
#endif

#ifdef LOAD_GL_PATCH_PARAMTER_I
    DECLARE_GL_FUNCTION( glPatchParameteri, PFNGLPATCHPARAMETERIPROC, GLenum pname, GLint value )
#endif

#ifdef LOAD_GET_TEX_LEVEL_PARAMETER_IV
    DECLARE_GL_FUNCTION( glGetTexLevelParameteriv, PFNGLGETTEXLEVELPARAMETERIVPROC, GLenum target, GLint level, GLenum pname, GLint *params )
#endif

#ifdef LOAD_GL_SHADER_STORAGE_BLOCK_BINDING
    DECLARE_GL_FUNCTION( glShaderStorageBlockBinding, PFNGLSHADERSTORAGEBLOCKBINDINGPROC, GLuint program, GLuint storageBlockIndex, GLuint storageBlockBinding )
#endif

#ifdef LOAD_GL_TEX_STORAGE_3D_MULTISAMPLE
    DECLARE_GL_FUNCTION( glTexStorage3DMultisample, PFNGLTEXSTORAGE3DMULTISAMPLEPROC, GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations )
#endif

#ifdef LOAD_GL_TEXTURE_VIEW
    DECLARE_GL_FUNCTION( glTextureView, PFNGLTEXTUREVIEWPROC, GLuint texture, GLenum target, GLuint origtexture, GLenum internalformat, GLuint minlevel, GLuint numlevels, GLuint minlayer, GLuint numlayers)
#endif

#ifdef LOAD_GL_DRAW_ELEMENTS_INSTANCED_BASE_VERTEX_BASE_INSTANCE
    DECLARE_GL_FUNCTION( glDrawElementsInstancedBaseVertexBaseInstance, PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCEPROC, GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex, GLuint baseinstance)
#endif

#ifdef LOAD_GL_DRAW_ELEMENTS_INSTANCED_BASE_VERTEX
    DECLARE_GL_FUNCTION( glDrawElementsInstancedBaseVertex, PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC, GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex)
#endif

#ifdef LOAD_GL_DRAW_ELEMENTS_INSTANCED_BASE_INSTANCE
    DECLARE_GL_FUNCTION( glDrawElementsInstancedBaseInstance, PFNGLDRAWELEMENTSINSTANCEDBASEINSTANCEPROC, GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLuint baseinstance)
#endif

#ifdef LOAD_GL_DRAW_ARRAYS_INSTANCED_BASE_INSTANCE
    DECLARE_GL_FUNCTION( glDrawArraysInstancedBaseInstance, PFNGLDRAWARRAYSINSTANCEDBASEINSTANCEPROC, GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance)
#endif

#ifdef LOAD_GL_DRAW_ELEMENTS_BASE_VERTEX
    DECLARE_GL_FUNCTION( glDrawElementsBaseVertex, PFNGLDRAWELEMENTSBASEVERTEXPROC, GLenum mode, GLsizei count, GLenum type, const void *indices, GLint basevertex)
#endif

#ifdef LOAD_DEBUG_MESSAGE_CALLBACK
    DECLARE_GL_FUNCTION( glDebugMessageCallback, PFNGLDEBUGMESSAGECALLBACKPROC, GLDEBUGPROC callback, const void *userParam)
#endif

#ifdef LOAD_DEBUG_MESSAGE_CONTROL
    DECLARE_GL_FUNCTION( glDebugMessageControl, PFNGLDEBUGMESSAGECONTROLPROC, GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint* ids, GLboolean enabled);
#endif

#ifdef LOAD_GL_GET_QUERY_OBJECT_UI64V
    DECLARE_GL_FUNCTION( glGetQueryObjectui64v, PFNGLGETQUERYOBJECTUI64VPROC, GLuint id, GLenum pname, GLuint64* params)
#endif

#ifdef LOAD_GL_QUERY_COUNTER
    DECLARE_GL_FUNCTION( glQueryCounter, PFNGLQUERYCOUNTERPROC, GLuint id, GLenum target)
#endif


void LoadGLFunctions()
{

#define LOAD_GL_FUNCTION(Func, FuncType)\
Func = (FuncType)eglGetProcAddress( #Func );\
    if( !Func )Func = Func##Stub;

#ifdef LOAD_GL_BIND_IMAGE_TEXTURE
    LOAD_GL_FUNCTION(glBindImageTexture, PFNGLBINDIMAGETEXTUREPROC)
#endif

#ifdef LOAD_GL_DISPATCH_COMPUTE
    LOAD_GL_FUNCTION(glDispatchCompute, PFNGLDISPATCHCOMPUTEPROC)
#endif

#ifdef LOAD_GEN_PROGRAM_PIPELINES
    LOAD_GL_FUNCTION(glGenProgramPipelines, PFNGLGENPROGRAMPIPELINESPROC)
#endif

#ifdef LOAD_GL_DELETE_PROGRAM_PIPELINES
    LOAD_GL_FUNCTION(glDeleteProgramPipelines, PFNGLDELETEPROGRAMPIPELINESPROC)
#endif

#ifdef LOAD_GL_BIND_PROGRAM_PIPELINE
    LOAD_GL_FUNCTION(glBindProgramPipeline, PFNGLBINDPROGRAMPIPELINEPROC)
#endif

#ifdef LOAD_GL_USE_PROGRAM_STAGES
    LOAD_GL_FUNCTION(glUseProgramStages, PFNGLUSEPROGRAMSTAGESPROC)
#endif

#ifdef LOAD_GL_PROGRAM_UNIFORM_1I
    LOAD_GL_FUNCTION(glProgramUniform1i, PFNGLPROGRAMUNIFORM1IPROC)
#endif

#ifdef LOAD_GL_MEMORY_BARRIER 
    LOAD_GL_FUNCTION(glMemoryBarrier,  PFNGLMEMORYBARRIERPROC)
#endif

#ifdef LOAD_DRAW_ELEMENTS_INDIRECT
    LOAD_GL_FUNCTION(glDrawElementsIndirect, PFNGLDRAWELEMENTSINDIRECTPROC)
#endif

#ifdef LOAD_DRAW_ARRAYS_INDIRECT
    LOAD_GL_FUNCTION(glDrawArraysIndirect, PFNGLDRAWARRAYSINDIRECTPROC)
#endif

#ifdef LOAD_GL_TEX_STORAGE_2D_MULTISAMPLE
    LOAD_GL_FUNCTION(glTexStorage2DMultisample, PFNGLTEXSTORAGE2DMULTISAMPLEPROC)
#endif

#ifdef LOAD_GL_GET_PROGRAM_INTERFACEIV
    LOAD_GL_FUNCTION(glGetProgramInterfaceiv, PFNGLGETPROGRAMINTERFACEIVPROC)
#endif

#ifdef LOAD_GL_GET_PROGRAM_RESOURCE_NAME
    LOAD_GL_FUNCTION(glGetProgramResourceName, PFNGLGETPROGRAMRESOURCENAMEPROC)
#endif

#ifdef LOAD_GL_GET_PROGRAM_RESOURCE_INDEX
    LOAD_GL_FUNCTION(glGetProgramResourceIndex, PFNGLGETPROGRAMRESOURCEINDEXPROC)
#endif

#ifdef LOAD_GL_GET_PROGRAM_RESOURCEIV
    LOAD_GL_FUNCTION(glGetProgramResourceiv, PFNGLGETPROGRAMRESOURCEIVPROC)
#endif

#ifdef LOAD_DISPATCH_COMPUTE_INDIRECT
    LOAD_GL_FUNCTION(glDispatchComputeIndirect, PFNGLDISPATCHCOMPUTEINDIRECTPROC)
#endif

#ifdef LOAD_GL_TEX_BUFFER
    LOAD_GL_FUNCTION(glTexBuffer, PFNGLTEXBUFFERPROC)
#endif

#ifdef LOAD_GL_POLYGON_MODE
    glPolygonMode = (PFNGLPOLYGONMODE)eglGetProcAddress("glPolygonMode");
#endif

#ifdef LOAD_GL_ENABLEI
    LOAD_GL_FUNCTION(glEnablei, PFNGLENABLEIPROC)
#endif

#ifdef LOAD_GL_BLEND_FUNC_SEPARATEI 
    LOAD_GL_FUNCTION(glBlendFuncSeparatei, PFNGLBLENDFUNCSEPARATEIPROC)
#endif

#ifdef LOAD_GL_BLEND_EQUATION_SEPARATEI
    LOAD_GL_FUNCTION(glBlendEquationSeparatei, PFNGLBLENDEQUATIONSEPARATEIPROC)
#endif

#ifdef LOAD_GL_DISABLEI
    LOAD_GL_FUNCTION(glDisablei, PFNGLDISABLEIPROC)
#endif

#ifdef LOAD_GL_COLOR_MASKI
    LOAD_GL_FUNCTION(glColorMaski, PFNGLCOLORMASKIPROC)
#endif

#ifdef LOAD_GL_VIEWPORT_INDEXEDF
    LOAD_GL_FUNCTION(glViewportIndexedf, PFNGLVIEWPORTINDEXEDFPROC)
#endif

#ifdef LOAD_GL_SCISSOR_INDEXED
    LOAD_GL_FUNCTION(glScissorIndexed, PFNGLSCISSORINDEXEDPROC)
#endif

#ifdef LOAD_GL_FRAMEBUFFER_TEXTURE
    LOAD_GL_FUNCTION(glFramebufferTexture, PFNGLFRAMEBUFFERTEXTUREPROC)
#endif

#ifdef LOAD_GL_FRAMEBUFFER_TEXTURE_1D
    LOAD_GL_FUNCTION(glFramebufferTexture1D, PFNGLFRAMEBUFFERTEXTURE1DPROC)
#endif

#ifdef LOAD_GL_FRAMEBUFFER_TEXTURE_3D
    LOAD_GL_FUNCTION(glFramebufferTexture3D, PFNGLFRAMEBUFFERTEXTURE3DPROC)
#endif

#ifdef LOAD_GL_COPY_IMAGE_SUB_DATA
    // Do not use proxy if function is not available!
    LOAD_GL_FUNCTION(glCopyImageSubData, PFNGLCOPYIMAGESUBDATAPROC)
#endif

#ifdef LOAD_GL_PATCH_PARAMTER_I
    LOAD_GL_FUNCTION(glPatchParameteri, PFNGLPATCHPARAMETERIPROC)
#endif

#ifdef LOAD_GET_TEX_LEVEL_PARAMETER_IV
    LOAD_GL_FUNCTION(glGetTexLevelParameteriv, PFNGLGETTEXLEVELPARAMETERIVPROC)
#endif

#ifdef LOAD_GL_SHADER_STORAGE_BLOCK_BINDING
    LOAD_GL_FUNCTION(glShaderStorageBlockBinding, PFNGLSHADERSTORAGEBLOCKBINDINGPROC)
#endif

#ifdef LOAD_GL_TEX_STORAGE_3D_MULTISAMPLE
    LOAD_GL_FUNCTION(glTexStorage3DMultisample, PFNGLTEXSTORAGE3DMULTISAMPLEPROC)
#endif

#ifdef LOAD_GL_TEXTURE_VIEW
    LOAD_GL_FUNCTION(glTextureView, PFNGLTEXTUREVIEWPROC)
#endif

#ifdef LOAD_GL_DRAW_ELEMENTS_INSTANCED_BASE_VERTEX_BASE_INSTANCE
    LOAD_GL_FUNCTION(glDrawElementsInstancedBaseVertexBaseInstance, PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCEPROC)
#endif

#ifdef LOAD_GL_DRAW_ELEMENTS_INSTANCED_BASE_VERTEX
    LOAD_GL_FUNCTION(glDrawElementsInstancedBaseVertex, PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC)
#endif

#ifdef LOAD_GL_DRAW_ELEMENTS_INSTANCED_BASE_INSTANCE
    LOAD_GL_FUNCTION(glDrawElementsInstancedBaseInstance, PFNGLDRAWELEMENTSINSTANCEDBASEINSTANCEPROC)
#endif

#ifdef LOAD_GL_DRAW_ARRAYS_INSTANCED_BASE_INSTANCE
    LOAD_GL_FUNCTION(glDrawArraysInstancedBaseInstance, PFNGLDRAWARRAYSINSTANCEDBASEINSTANCEPROC)
#endif

#ifdef LOAD_GL_DRAW_ELEMENTS_BASE_VERTEX
    LOAD_GL_FUNCTION(glDrawElementsBaseVertex, PFNGLDRAWELEMENTSBASEVERTEXPROC)
#endif

#ifdef LOAD_DEBUG_MESSAGE_CALLBACK
    LOAD_GL_FUNCTION(glDebugMessageCallback, PFNGLDEBUGMESSAGECALLBACKPROC)
#endif

#ifdef LOAD_DEBUG_MESSAGE_CONTROL
    LOAD_GL_FUNCTION(glDebugMessageControl, PFNGLDEBUGMESSAGECONTROLPROC);
#endif

#ifdef LOAD_GL_GET_QUERY_OBJECT_UI64V
    // Do not use stub
    glGetQueryObjectui64v = (PFNGLGETQUERYOBJECTUI64VPROC)eglGetProcAddress( "glGetQueryObjectui64vEXT" );
#endif

#ifdef LOAD_GL_QUERY_COUNTER
    // Do not use stub
    glQueryCounter = (PFNGLQUERYCOUNTERPROC)eglGetProcAddress( "glQueryCounterEXT" );
#endif
}
