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

#include "UniqueIdentifier.hpp"

namespace GLObjectWrappers
{

template <class CreateReleaseHelperType>
class GLObjWrapper
{
public:
    explicit GLObjWrapper(bool CreateObject, CreateReleaseHelperType CreateReleaseHelper = CreateReleaseHelperType()) :
        // clang-format off
        m_uiHandle           {0                  },
        m_CreateReleaseHelper{CreateReleaseHelper}
    // clang-format on
    {
        if (CreateObject)
        {
            Create();
            if (!m_uiHandle)
            {
                LOG_ERROR_AND_THROW("Failed to create ", m_CreateReleaseHelper.Name, " GL object");
            }
        }
    }

    ~GLObjWrapper()
    {
        Release();
    }

    // clang-format off
    GLObjWrapper           (const GLObjWrapper&) = delete;
    GLObjWrapper& operator=(const GLObjWrapper&) = delete;
    // clang-format on

    GLObjWrapper(GLObjWrapper&& Wrapper) :
        // clang-format off
        m_uiHandle           {Wrapper.m_uiHandle                      },
        m_CreateReleaseHelper{std::move(Wrapper.m_CreateReleaseHelper)},
        m_UniqueId           {std::move(Wrapper.m_UniqueId)           }
    // clang-format on
    {
        Wrapper.m_uiHandle = 0;
    }

    GLObjWrapper& operator=(GLObjWrapper&& Wrapper)
    {
        Release();

        m_uiHandle            = Wrapper.m_uiHandle;
        m_CreateReleaseHelper = std::move(Wrapper.m_CreateReleaseHelper);
        m_UniqueId            = std::move(Wrapper.m_UniqueId);

        Wrapper.m_uiHandle = 0;

        return *this;
    }

    void Create()
    {
        VERIFY(m_uiHandle == 0, "GL object is already initialized");
        Release();
        m_CreateReleaseHelper.Create(m_uiHandle);
        VERIFY(m_uiHandle, "Failed to initialize GL object");
    }

    void Release()
    {
        if (m_uiHandle)
        {
            m_CreateReleaseHelper.Release(m_uiHandle);
            m_uiHandle = 0;
        }
    }

    Diligent::UniqueIdentifier GetUniqueID() const
    {
        // This unique ID is used to unambiguously identify the object for
        // tracking purposes.
        // Niether GL handle nor pointer could be safely used for this purpose
        // as both GL reuses released handles and the OS reuses released pointers
        return m_UniqueId.GetID();
    }

    operator GLuint() const { return m_uiHandle; }

    static GLObjWrapper Null()
    {
        return GLObjWrapper{false};
    }

private:
    GLuint                  m_uiHandle;
    CreateReleaseHelperType m_CreateReleaseHelper;

    // Have separate counter for every type of wrappers
    Diligent::UniqueIdHelper<CreateReleaseHelperType> m_UniqueId;
};

class GLBufferObjCreateReleaseHelper
{
public:
    explicit GLBufferObjCreateReleaseHelper(GLuint ExternalGLBufferHandle = 0) :
        m_ExternalGLBufferHandle{ExternalGLBufferHandle}
    {}

    void Create(GLuint& BuffObj)
    {
        if (m_ExternalGLBufferHandle != 0)
            BuffObj = m_ExternalGLBufferHandle; // Attach to external GL buffer handle
        else
            glGenBuffers(1, &BuffObj);
    }

    void Release(GLuint BuffObj)
    {
        if (m_ExternalGLBufferHandle != 0)
            m_ExternalGLBufferHandle = 0; // Detach from external GL buffer. DO NOT delete the buffer
        else
            glDeleteBuffers(1, &BuffObj);
    }
    static const char* Name;

private:
    GLuint m_ExternalGLBufferHandle;
};
typedef GLObjWrapper<GLBufferObjCreateReleaseHelper> GLBufferObj;


class GLProgramObjCreateReleaseHelper
{
public:
    static void Create(GLuint& ProgObj) { ProgObj = glCreateProgram(); }
    static void Release(GLuint ProgObj) { glDeleteProgram(ProgObj); }

    static const char* Name;
};
typedef GLObjWrapper<GLProgramObjCreateReleaseHelper> GLProgramObj;


class GLShaderObjCreateReleaseHelper
{
public:
    GLShaderObjCreateReleaseHelper(GLenum ShaderType) :
        m_ShaderType(ShaderType) {}
    void Create(GLuint& ShaderObj) { ShaderObj = glCreateShader(m_ShaderType); }
    void Release(GLuint ShaderObj) { glDeleteShader(ShaderObj); }

    static const char* Name;

private:
    GLenum m_ShaderType;
};
typedef GLObjWrapper<GLShaderObjCreateReleaseHelper> GLShaderObj;


class GLPipelineObjCreateReleaseHelper
{
public:
    void Create(GLuint& Pipeline) { glGenProgramPipelines(1, &Pipeline); }
    void Release(GLuint Pipeline) { glDeleteProgramPipelines(1, &Pipeline); }

    static const char* Name;
};
typedef GLObjWrapper<GLPipelineObjCreateReleaseHelper> GLPipelineObj;


class GLVAOCreateReleaseHelper
{
public:
    void Create(GLuint& VAO) { glGenVertexArrays(1, &VAO); }
    void Release(GLuint VAO) { glDeleteVertexArrays(1, &VAO); }

    static const char* Name;
};
typedef GLObjWrapper<GLVAOCreateReleaseHelper> GLVertexArrayObj;


class GLTextureCreateReleaseHelper
{
public:
    explicit GLTextureCreateReleaseHelper(GLuint ExternalGLTextureHandle = 0) :
        m_ExternalGLTextureHandle(ExternalGLTextureHandle)
    {}

    void Create(GLuint& Tex)
    {
        if (m_ExternalGLTextureHandle != 0)
            Tex = m_ExternalGLTextureHandle; // Attach to the external texture
        else
            glGenTextures(1, &Tex);
    }

    void Release(GLuint Tex)
    {
        if (m_ExternalGLTextureHandle != 0)
            m_ExternalGLTextureHandle = 0; // Detach from the external texture. DO NOT delete it!
        else
            glDeleteTextures(1, &Tex);
    }

    static const char* Name;

private:
    GLuint m_ExternalGLTextureHandle;
};
typedef GLObjWrapper<GLTextureCreateReleaseHelper> GLTextureObj;

class GLSamplerCreateReleaseHelper
{
public:
    void Create(GLuint& Sampler) { glGenSamplers(1, &Sampler); }
    void Release(GLuint Sampler) { glDeleteSamplers(1, &Sampler); }

    static const char* Name;
};
typedef GLObjWrapper<GLSamplerCreateReleaseHelper> GLSamplerObj;


class GLFBOCreateReleaseHelper
{
public:
    explicit GLFBOCreateReleaseHelper(GLuint ExternalFBOHandle = 0) :
        m_ExternalFBOHandle(ExternalFBOHandle)
    {}

    void Create(GLuint& FBO)
    {
        if (m_ExternalFBOHandle != 0)
            FBO = m_ExternalFBOHandle; // Attach to external FBO handle
        else
            glGenFramebuffers(1, &FBO);
    }

    void Release(GLuint FBO)
    {
        if (m_ExternalFBOHandle != 0)
            m_ExternalFBOHandle = 0; // DO NOT delete the FBO
        else
            glDeleteFramebuffers(1, &FBO);
    }

    static const char* Name;

private:
    GLuint m_ExternalFBOHandle;
};
typedef GLObjWrapper<GLFBOCreateReleaseHelper> GLFrameBufferObj;


class GLRBOCreateReleaseHelper
{
public:
    void Create(GLuint& RBO) { glGenRenderbuffers(1, &RBO); }
    void Release(GLuint RBO) { glDeleteRenderbuffers(1, &RBO); }

    static const char* Name;
};
typedef GLObjWrapper<GLRBOCreateReleaseHelper> GLRenderBufferObj;

struct GLSyncObj
{
    GLSyncObj() {}
    GLSyncObj(GLsync _SyncHandle) :
        SyncHandle{_SyncHandle} {}

    // clang-format off
    GLSyncObj             (const GLSyncObj&) = delete;
    GLSyncObj& operator = (const GLSyncObj&) = delete;
    // clang-format on

    GLSyncObj(GLSyncObj&& rhs)
    {
        SyncHandle     = rhs.SyncHandle;
        rhs.SyncHandle = GLsync{};
    }

    GLSyncObj& operator=(GLSyncObj&& rhs)
    {
        Release();
        SyncHandle     = rhs.SyncHandle;
        rhs.SyncHandle = GLsync{};
        return *this;
    }

    void Release()
    {
        if (SyncHandle != GLsync{})
            glDeleteSync(SyncHandle);
        SyncHandle = GLsync{};
    }

    ~GLSyncObj()
    {
        Release();
    }

    operator GLsync() const { return SyncHandle; }

private:
    GLsync SyncHandle = {};
};

class GLQueryCreateReleaseHelper
{
public:
    void Create(GLuint& Query) { glGenQueries(1, &Query); }
    void Release(GLuint Query) { glDeleteQueries(1, &Query); }

    static const char* Name;
};
typedef GLObjWrapper<GLQueryCreateReleaseHelper> GLQueryObj;

} // namespace GLObjectWrappers
