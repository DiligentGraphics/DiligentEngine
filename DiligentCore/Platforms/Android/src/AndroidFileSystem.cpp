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

#include <string>
#include <android/native_activity.h>

#include "AndroidFileSystem.hpp"
#include "Errors.hpp"
#include "DebugUtilities.hpp"


namespace
{

class JNIMiniHelper
{
public:
    static void Init(ANativeActivity* activity, std::string activity_class_name, AAssetManager* asset_manager)
    {
        VERIFY(activity != nullptr || asset_manager != nullptr, "Activity and asset manager can't both be null");

        auto& TheHelper                = GetInstance();
        TheHelper.activity_            = activity;
        TheHelper.activity_class_name_ = std::move(activity_class_name);
        TheHelper.asset_manager_       = asset_manager;
        if (TheHelper.asset_manager_ == nullptr && TheHelper.activity_ != nullptr)
        {
            TheHelper.asset_manager_ = TheHelper.activity_->assetManager;
        }
    }

    static JNIMiniHelper& GetInstance()
    {
        static JNIMiniHelper helper;
        return helper;
    }


    bool OpenFile(const char* fileName, std::ifstream& IFS, AAsset*& AssetFile, size_t& FileSize)
    {
        if (activity_ == nullptr && asset_manager_ == nullptr)
        {
            LOG_ERROR_MESSAGE("JNIMiniHelper has not been initialized. Call init() to initialize the helper");
            return false;
        }

        // Lock mutex
        std::lock_guard<std::mutex> lock(mutex_);

        if (activity_ != nullptr)
        {
            // First, try reading from externalFileDir;
            std::string ExternalFilesPath;
            {
                JNIEnv* env          = nullptr;
                bool    DetachThread = AttachCurrentThread(env);
                if (jstring jstr_path = GetExternalFilesDirJString(env))
                {
                    const char* path  = env->GetStringUTFChars(jstr_path, nullptr);
                    ExternalFilesPath = std::string(path);
                    if (fileName[0] != '/')
                    {
                        ExternalFilesPath.append("/");
                    }
                    ExternalFilesPath.append(fileName);
                    env->ReleaseStringUTFChars(jstr_path, path);
                    env->DeleteLocalRef(jstr_path);
                }
                if (DetachThread)
                    DetachCurrentThread();
            }

            IFS.open(ExternalFilesPath.c_str(), std::ios::binary);
        }

        if (IFS && IFS.is_open())
        {
            IFS.seekg(0, std::ifstream::end);
            FileSize = IFS.tellg();
            IFS.seekg(0, std::ifstream::beg);
            return true;
        }
        else if (asset_manager_ != nullptr)
        {
            // Fallback to assetManager
            AssetFile = AAssetManager_open(asset_manager_, fileName, AASSET_MODE_BUFFER);
            if (!AssetFile)
            {
                return false;
            }
            uint8_t* data = (uint8_t*)AAsset_getBuffer(AssetFile);
            if (data == nullptr)
            {
                AAsset_close(AssetFile);

                LOG_ERROR_MESSAGE("Failed to open: ", fileName);
                return false;
            }
            FileSize = AAsset_getLength(AssetFile);
            return true;
        }
        else
        {
            return false;
        }
    }

    /*
     * Attach current thread
     * In Android, the thread doesn't have to be 'Detach' current thread
     * as application process is only killed and VM does not shut down
     */
    bool AttachCurrentThread(JNIEnv*& env)
    {
        env = nullptr;
        if (activity_->vm->GetEnv((void**)&env, JNI_VERSION_1_4) == JNI_OK)
            return false; // Already attached
        activity_->vm->AttachCurrentThread(&env, nullptr);
        pthread_key_create((int32_t*)activity_, DetachCurrentThreadDtor);
        return true;
    }

    /*
     * Unregister this thread from the VM
     */
    static void DetachCurrentThreadDtor(void* p)
    {
        LOG_INFO_MESSAGE("detached current thread");
        auto* activity = reinterpret_cast<ANativeActivity*>(p);
        activity->vm->DetachCurrentThread();
    }

private:
    JNIMiniHelper()
    {
    }

    ~JNIMiniHelper()
    {
    }

    // clang-format off
    JNIMiniHelper           (const JNIMiniHelper&) = delete;
    JNIMiniHelper& operator=(const JNIMiniHelper&) = delete;
    JNIMiniHelper           (JNIMiniHelper&&)      = delete;
    JNIMiniHelper& operator=(JNIMiniHelper&&)      = delete;
    // clang-format on

    jstring GetExternalFilesDirJString(JNIEnv* env)
    {
        if (activity_ == nullptr)
        {
            LOG_ERROR_MESSAGE("JNIHelper has not been initialized. Call init() to initialize the helper");
            return NULL;
        }

        jstring obj_Path = nullptr;
        // Invoking getExternalFilesDir() java API
        jclass    cls_Env  = env->FindClass(activity_class_name_.c_str());
        jmethodID mid      = env->GetMethodID(cls_Env, "getExternalFilesDir", "(Ljava/lang/String;)Ljava/io/File;");
        jobject   obj_File = env->CallObjectMethod(activity_->clazz, mid, NULL);
        if (obj_File)
        {
            jclass    cls_File    = env->FindClass("java/io/File");
            jmethodID mid_getPath = env->GetMethodID(cls_File, "getPath", "()Ljava/lang/String;");
            obj_Path              = (jstring)env->CallObjectMethod(obj_File, mid_getPath);
            env->DeleteLocalRef(cls_File);
            env->DeleteLocalRef(obj_File);
        }
        env->DeleteLocalRef(cls_Env);
        return obj_Path;
    }

    void DetachCurrentThread()
    {
        activity_->vm->DetachCurrentThread();
    }

    ANativeActivity* activity_ = nullptr;
    std::string      activity_class_name_;
    AAssetManager*   asset_manager_ = nullptr;

    // mutex for synchronization
    // This class uses singleton pattern and can be invoked from multiple threads,
    // each methods locks the mutex for a thread safety
    mutable std::mutex mutex_;
};


} // namespace


bool AndroidFile::Open(const char* FileName, std::ifstream& IFS, AAsset*& AssetFile, size_t& Size)
{
    return JNIMiniHelper::GetInstance().OpenFile(FileName, IFS, AssetFile, Size);
}

AndroidFile::AndroidFile(const FileOpenAttribs& OpenAttribs) :
    BasicFile(OpenAttribs, AndroidFileSystem::GetSlashSymbol())
{
    auto FullPath = m_OpenAttribs.strFilePath;
    if (!Open(FullPath, m_IFS, m_AssetFile, m_Size))
    {
        LOG_ERROR_AND_THROW("Failed to open file ", FullPath);
    }
}

AndroidFile::~AndroidFile()
{
    if (m_IFS && m_IFS.is_open())
        m_IFS.close();

    if (m_AssetFile != nullptr)
        AAsset_close(m_AssetFile);
}

void AndroidFile::Read(Diligent::IDataBlob* pData)
{
    pData->Resize(GetSize());
    Read(pData->GetDataPtr(), pData->GetSize());
}

bool AndroidFile::Read(void* Data, size_t BufferSize)
{
    VERIFY(BufferSize == m_Size, "Only whole file reads are currently supported");

    if (m_IFS && m_IFS.is_open())
    {
        m_IFS.read((char*)Data, BufferSize);
        return true;
    }
    else if (m_AssetFile != nullptr)
    {
        const uint8_t* src_data = (uint8_t*)AAsset_getBuffer(m_AssetFile);
        auto           FileSize = AAsset_getLength(m_AssetFile);
        if (FileSize > BufferSize)
        {
            LOG_WARNING_MESSAGE("Requested buffer size (", BufferSize, ") exceeds file size (", FileSize, ")");
            BufferSize = FileSize;
        }
        memcpy(Data, src_data, BufferSize);
        return true;
    }
    else
    {
        return false;
    }
}

bool AndroidFile::Write(const void* Data, size_t BufferSize)
{
    UNSUPPORTED("Not implemented");

    return false;
}

size_t AndroidFile::GetPos()
{
    UNSUPPORTED("Not implemented");

    return 0;
}

void AndroidFile::SetPos(size_t Offset, FilePosOrigin Origin)
{
    UNSUPPORTED("Not implemented");
}


void AndroidFileSystem::Init(ANativeActivity* NativeActivity, const char* NativeActivityClassName, AAssetManager* AssetManager)
{
    JNIMiniHelper::Init(NativeActivity, NativeActivityClassName != nullptr ? NativeActivityClassName : "", AssetManager);
}

AndroidFile* AndroidFileSystem::OpenFile(const FileOpenAttribs& OpenAttribs)
{
    AndroidFile* pFile = nullptr;
    try
    {
        pFile = new AndroidFile(OpenAttribs);
    }
    catch (const std::runtime_error& err)
    {
    }

    return pFile;
}


bool AndroidFileSystem::FileExists(const Diligent::Char* strFilePath)
{
    std::ifstream   IFS;
    AAsset*         AssetFile = nullptr;
    size_t          Size      = 0;
    FileOpenAttribs OpenAttribs;
    OpenAttribs.strFilePath = strFilePath;
    BasicFile   DummyFile(OpenAttribs, AndroidFileSystem::GetSlashSymbol());
    const auto& Path   = DummyFile.GetPath(); // This is necessary to correct slashes
    bool        Exists = AndroidFile::Open(Path.c_str(), IFS, AssetFile, Size);

    if (IFS && IFS.is_open())
        IFS.close();
    if (AssetFile != nullptr)
        AAsset_close(AssetFile);

    return Exists;
}

bool AndroidFileSystem::PathExists(const Diligent::Char* strPath)
{
    UNSUPPORTED("Not implemented");
    return false;
}

bool AndroidFileSystem::CreateDirectory(const Diligent::Char* strPath)
{
    UNSUPPORTED("Not implemented");
    return false;
}

void AndroidFileSystem::ClearDirectory(const Diligent::Char* strPath)
{
    UNSUPPORTED("Not implemented");
}

void AndroidFileSystem::DeleteFile(const Diligent::Char* strPath)
{
    UNSUPPORTED("Not implemented");
}

std::vector<std::unique_ptr<FindFileData>> AndroidFileSystem::Search(const Diligent::Char* SearchPattern)
{
    UNSUPPORTED("Not implemented");
    return std::vector<std::unique_ptr<FindFileData>>();
}
