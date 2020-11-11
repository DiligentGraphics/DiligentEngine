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

#pragma once

#include <memory>

#include "../../Basic/interface/BasicFileSystem.hpp"
#include "../../../Primitives/interface/DataBlob.h"

// Do not include windows headers here as they will mess up CreateDirectory()
// and DeleteFile() functions!
//#define NOMINMAX
//#include <wrl.h>

class WindowsStoreFile : public BasicFile
{
public:
    WindowsStoreFile(const FileOpenAttribs& OpenAttribs);
    ~WindowsStoreFile();

    void Read(Diligent::IDataBlob* pData);

    bool Read(void* Data, size_t BufferSize);

    void Write(Diligent::IDataBlob* pData);
    bool Write(const void* Data, size_t BufferSize);

    size_t GetSize();

    size_t GetPos();

    void SetPos(size_t Offset, FilePosOrigin Origin);

private:
    // We have to do such tricks, because we cannot #include <wrl.h>
    // to avoid name clashes.
    std::unique_ptr<class FileHandleWrapper> m_FileHandle;
};

struct WindowsStoreFileSystem : public BasicFileSystem
{
public:
    static WindowsStoreFile* OpenFile(const FileOpenAttribs& OpenAttribs);

    static inline Diligent::Char GetSlashSymbol() { return '\\'; }

    static bool FileExists(const Diligent::Char* strFilePath);
    static bool PathExists(const Diligent::Char* strPath);

    static bool CreateDirectory(const Diligent::Char* strPath);
    static void ClearDirectory(const Diligent::Char* strPath);
    static void DeleteFile(const Diligent::Char* strPath);

    static std::vector<std::unique_ptr<FindFileData>> Search(const Diligent::Char* SearchPattern);
};
