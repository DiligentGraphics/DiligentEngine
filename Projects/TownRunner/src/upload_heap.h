// Copyright 2014 Intel Corporation All Rights Reserved
//
// Intel makes no representations about the suitability of this software for any purpose.  
// THIS SOFTWARE IS PROVIDED ""AS IS."" INTEL SPECIFICALLY DISCLAIMS ALL WARRANTIES,
// EXPRESS OR IMPLIED, AND ALL LIABILITY, INCLUDING CONSEQUENTIAL AND OTHER INDIRECT DAMAGES,
// FOR THE USE OF THIS SOFTWARE, INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PROPRIETARY
// RIGHTS, AND INCLUDING THE WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
// Intel does not assume any responsibility for any errors which may appear in this software
// nor any responsibility to update it.

#pragma once

#include "util.h"
#include <d3d12.h>

// Untyped version
class UploadHeap
{
public:
    UploadHeap(ID3D12Device* device, UINT64 size)
    {
        ThrowIfFailed(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_UPLOAD ),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer( size ),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&mHeap)
        ));

        ThrowIfFailed(mHeap->Map(0, nullptr, reinterpret_cast<void**>(&mHeapWO)));
    }

    ~UploadHeap()
    {
        SafeRelease(&mHeap);
    }

    ID3D12Resource* Heap() { return mHeap; }
    
    // Write-only!
    void* DataWO() { return mHeapWO; }

private:
    ID3D12Resource* mHeap = nullptr;
    void* mHeapWO = nullptr;
};


// Structure/typed version
template <typename T>
class UploadHeapT : public UploadHeap
{
public:
    UploadHeapT(ID3D12Device* device)
        : UploadHeap(device, sizeof(T))
    {}
    
    T* DataWO() { return (T*)UploadHeap::DataWO(); }
};
