/*     Copyright 2015-2016 Egor Yusov
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

#include "UWPFileSystem.hpp"
#include "Errors.hpp"
#include "../../Common/interface/StringTools.hpp"
#include "DebugUtilities.hpp"


// Windows headers define CreateDirectory and DeleteFile as macros.
// So we need to do some tricks to avoid name mess.
bool CreateDirectoryImpl(const Diligent::Char* strPath);

bool WindowsStoreFileSystem::CreateDirectory(const Diligent::Char* strPath)
{
    return CreateDirectoryImpl(strPath);
}

void DeleteFileImpl(const Diligent::Char* strPath);

void WindowsStoreFileSystem::DeleteFile(const Diligent::Char* strPath)
{
    return DeleteFileImpl(strPath);
}


#define NOMINMAX
#include <wrl.h>

using namespace Diligent;
using namespace Microsoft::WRL;

class FileHandleWrapper
{
public:
    Wrappers::FileHandle FH;
};

WindowsStoreFile::WindowsStoreFile(const FileOpenAttribs& OpenAttribs) :
    BasicFile(OpenAttribs, WindowsStoreFileSystem::GetSlashSymbol()),
    m_FileHandle(new FileHandleWrapper)
{
    CREATEFILE2_EXTENDED_PARAMETERS extendedParams = {0};

    extendedParams.dwSize               = sizeof(CREATEFILE2_EXTENDED_PARAMETERS);
    extendedParams.dwFileAttributes     = FILE_ATTRIBUTE_NORMAL;
    extendedParams.dwFileFlags          = FILE_FLAG_SEQUENTIAL_SCAN;
    extendedParams.dwSecurityQosFlags   = SECURITY_ANONYMOUS;
    extendedParams.lpSecurityAttributes = nullptr;
    extendedParams.hTemplateFile        = nullptr;

    auto  wstrPath           = Diligent::WidenString(m_OpenAttribs.strFilePath);
    DWORD dwDesiredAccess    = 0;
    DWORD dwShareMode        = 0;
    DWORD dwCreateDeposition = 0;
    switch (OpenAttribs.AccessMode)
    {
        case EFileAccessMode::Read:
            dwDesiredAccess = GENERIC_READ;
            // In Windows 8.1, the file cannot be opened if it is not shared!
            dwShareMode        = FILE_SHARE_READ;
            dwCreateDeposition = OPEN_EXISTING;
            break;

        case EFileAccessMode::Overwrite:
            dwDesiredAccess    = GENERIC_WRITE;
            dwShareMode        = 0;
            dwCreateDeposition = CREATE_ALWAYS;
            break;

        case EFileAccessMode::Append:
            dwDesiredAccess    = GENERIC_WRITE;
            dwShareMode        = 0;
            dwCreateDeposition = OPEN_ALWAYS;
            break;

        default:
            UNEXPECTED("Unknown file access mode");
            break;
    }
    m_FileHandle->FH.Attach(CreateFile2(
        wstrPath.c_str(),
        dwDesiredAccess,
        dwShareMode,
        dwCreateDeposition,
        &extendedParams));

    if (m_FileHandle->FH.Get() == INVALID_HANDLE_VALUE)
    {
        LOG_ERROR_AND_THROW("Failed to open file ", m_OpenAttribs.strFilePath);
    }
}

WindowsStoreFile::~WindowsStoreFile()
{
}

bool WindowsStoreFile::Read(void* Data, size_t BufferSize)
{
    DWORD BytesRead = 0;
    if (!ReadFile(
            m_FileHandle->FH.Get(),
            Data,
            static_cast<DWORD>(BufferSize),
            &BytesRead,
            nullptr))
    {
        return false;
    }

    return BytesRead == BufferSize;
}

size_t WindowsStoreFile::GetSize()
{
    FILE_STANDARD_INFO fileInfo = {0};
    if (!GetFileInformationByHandleEx(
            m_FileHandle->FH.Get(),
            FileStandardInfo,
            &fileInfo,
            sizeof(fileInfo)))
    {
        LOG_ERROR_AND_THROW("Failed to get file info");
    }

    if (fileInfo.EndOfFile.HighPart != 0)
    {
        LOG_ERROR_AND_THROW("File is too large to be read");
    }

    return fileInfo.EndOfFile.LowPart;
}

void WindowsStoreFile::Read(Diligent::IDataBlob* pData)
{
    pData->Resize(GetSize());

    if (!Read(pData->GetDataPtr(), pData->GetSize()))
    {
        LOG_ERROR_AND_THROW("Failed to read data from file");
    }
}

void WindowsStoreFile::Write(Diligent::IDataBlob* pData)
{
    DWORD numBytesWritten;
    if (!WriteFile(
            m_FileHandle->FH.Get(),
            pData->GetDataPtr(),
            static_cast<DWORD>(pData->GetSize()),
            &numBytesWritten,
            nullptr) ||
        numBytesWritten != pData->GetSize())
    {
        LOG_ERROR_AND_THROW("Failed to write data to file");
    }
}

bool WindowsStoreFile::Write(const void* Data, size_t BufferSize)
{
    UNSUPPORTED("Not implemented");
    return false;
}

size_t WindowsStoreFile::GetPos()
{
    UNSUPPORTED("Not implemented");
    return 0;
}

void WindowsStoreFile::SetPos(size_t Offset, FilePosOrigin Origin)
{
    UNSUPPORTED("Not implemented");
}


WindowsStoreFile* WindowsStoreFileSystem::OpenFile(const FileOpenAttribs& OpenAttribs)
{
    WindowsStoreFile* pFile = nullptr;
    try
    {
        pFile = new WindowsStoreFile(OpenAttribs);
    }
    catch (const std::runtime_error& /*err*/)
    {
    }

    return pFile;
}

bool WindowsStoreFileSystem::FileExists(const Diligent::Char* strFilePath)
{
    FileOpenAttribs OpenAttribs;
    OpenAttribs.AccessMode  = EFileAccessMode::Read;
    OpenAttribs.strFilePath = strFilePath;
    BasicFile   DummyFile(OpenAttribs, WindowsStoreFileSystem::GetSlashSymbol());
    const auto& Path = DummyFile.GetPath();

    CREATEFILE2_EXTENDED_PARAMETERS extendedParams = {0};

    extendedParams.dwSize               = sizeof(CREATEFILE2_EXTENDED_PARAMETERS);
    extendedParams.dwFileAttributes     = FILE_ATTRIBUTE_NORMAL;
    extendedParams.dwFileFlags          = FILE_FLAG_SEQUENTIAL_SCAN;
    extendedParams.dwSecurityQosFlags   = SECURITY_ANONYMOUS;
    extendedParams.lpSecurityAttributes = nullptr;
    extendedParams.hTemplateFile        = nullptr;

    auto wstrPath = Diligent::WidenString(Path);

    auto Handle = CreateFile2(
        wstrPath.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        OPEN_EXISTING,
        &extendedParams);
    bool Exists = false;
    if (Handle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(Handle);
        Exists = true;
    }
    return Exists;
}


bool WindowsStoreFileSystem::PathExists(const Diligent::Char* strPath)
{
    UNSUPPORTED("Not implemented");
    return false;
}

void WindowsStoreFileSystem::ClearDirectory(const Diligent::Char* strPath)
{
    UNSUPPORTED("Not implemented");
}


bool CreateDirectoryImpl(const Diligent::Char* strPath)
{
    UNSUPPORTED("Not implemented");
    return false;
}

void DeleteFileImpl(const Diligent::Char* strPath)
{
    UNSUPPORTED("Not implemented");
}

std::vector<std::unique_ptr<FindFileData>> WindowsStoreFileSystem::Search(const Diligent::Char* SearchPattern)
{
    UNSUPPORTED("Not implemented");
    return std::vector<std::unique_ptr<FindFileData>>();
}
