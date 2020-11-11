﻿#pragma once

#include <ppltasks.h>	// For create_task

namespace DX
{
    inline void ThrowIfFailed(HRESULT hr)
    {
        if (FAILED(hr))
        {
            // Set a breakpoint on this line to catch Win32 API errors.
            throw Platform::Exception::CreateException(hr);
        }
    }

    // Function that reads from a binary file asynchronously.
    inline Concurrency::task<std::vector<byte>> ReadDataAsync(const std::wstring& filename)
    {
        using namespace Windows::Storage;
        using namespace Concurrency;

        auto folder = Windows::ApplicationModel::Package::Current->InstalledLocation;

        return create_task(folder->GetFileAsync(Platform::StringReference(filename.c_str()))).then([](StorageFile^ file)
        {
            return FileIO::ReadBufferAsync(file);
        }).then([](Streams::IBuffer^ fileBuffer) -> std::vector<byte>
        {
            std::vector<byte> returnBuffer;
            returnBuffer.resize(fileBuffer->Length);
            Streams::DataReader::FromBuffer(fileBuffer)->ReadBytes(Platform::ArrayReference<byte>(returnBuffer.data(), fileBuffer->Length));
            return returnBuffer;
        });
    }

    // Converts a length in device-independent pixels (DIPs) to a length in physical pixels.
    inline float ConvertDipsToPixels(float dips, float dpi)
    {
        static const float dipsPerInch = 96.0f;
        return floorf(dips * dpi / dipsPerInch + 0.5f); // Round to nearest integer.
    }

    // Assign a name to the object to aid with debugging.
#if defined(_DEBUG)
    inline void SetName(ID3D12Object* pObject, LPCWSTR name)
    {
        pObject->SetName(name);
    }
#else
    inline void SetName(ID3D12Object*, LPCWSTR)
    {
    }
#endif
}

// Naming helper function for ComPtr<T>.
// Assigns the name of the variable as the name of the object.
#define NAME_D3D12_OBJECT(x) DX::SetName(x.Get(), L#x)
