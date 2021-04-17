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

#include <assert.h>
#include <d3d12.h>
#include <d3dx12.h>

// Used for CPU-only descriptors; simple linear allocator.
class DescriptorArray
{
public:
    // device must live for the lifetime of this object (no explicit refcount increment)
    DescriptorArray(ID3D12Device* device, UINT maxDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type,
                      bool shaderVisible = false)
        : mDevice(device)
        , mMaxSize(maxDescriptors)
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = type;
        desc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.NumDescriptors = mMaxSize;

        ThrowIfFailed(mDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mHeap)));

        mCPUBegin = mHeap->GetCPUDescriptorHandleForHeapStart();
        mHandleIncrementSize = mDevice->GetDescriptorHandleIncrementSize(desc.Type);

        if (shaderVisible) {
            mGPUBegin = mHeap->GetGPUDescriptorHandleForHeapStart();
        }
    }

    ~DescriptorArray()
    {
        SafeRelease(&mHeap);
    }
    
    // Direct access to handles
    CD3DX12_CPU_DESCRIPTOR_HANDLE CPU(UINT index = 0) const
    {
        return CD3DX12_CPU_DESCRIPTOR_HANDLE(mCPUBegin, index, mHandleIncrementSize);
    }
    CD3DX12_GPU_DESCRIPTOR_HANDLE GPU(UINT index = 0) const
    {
        return CD3DX12_GPU_DESCRIPTOR_HANDLE(mGPUBegin, index, mHandleIncrementSize);
    }
    CD3DX12_GPU_DESCRIPTOR_HANDLE GPUEnd() const { return GPU(mCurrentSize); }
    INT HandleIncrementSize() const { return mHandleIncrementSize; }

    // NOTE: Caller can fill in data at new handle and/or derived classes provide
    // specialized methods to do it in one step.
    CD3DX12_CPU_DESCRIPTOR_HANDLE Append()
    {
        assert(mCurrentSize < mMaxSize);
        return CPU(mCurrentSize++);
    }
    
    // Invalidates contents of any previous handles
    void Clear() { mCurrentSize = 0; }
    void Resize(UINT newSize) { mCurrentSize = newSize; }
    UINT Size() const { return mCurrentSize; }
    
protected:
    ID3D12Device* mDevice = nullptr;
    ID3D12DescriptorHeap* mHeap = nullptr;
    CD3DX12_CPU_DESCRIPTOR_HANDLE mCPUBegin;
    CD3DX12_GPU_DESCRIPTOR_HANDLE mGPUBegin;
    UINT mHandleIncrementSize = 0;
    UINT mCurrentSize = 0;
    UINT mMaxSize = 0;
};


// Render target views
class RTVDescriptorList : public DescriptorArray
{
public:
    RTVDescriptorList(ID3D12Device* device, UINT maxDescriptors)
        : DescriptorArray( device, maxDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE_RTV )
    {}

    using DescriptorArray::Append;

    D3D12_CPU_DESCRIPTOR_HANDLE Append(ID3D12Resource* resource, D3D12_RENDER_TARGET_VIEW_DESC* desc = nullptr)
    {
        auto handle = Append();
        mDevice->CreateRenderTargetView(resource, desc, handle);
        return handle;
    }
};

// Depth stencil views
class DSVDescriptorList : public DescriptorArray
{
public:
    DSVDescriptorList(ID3D12Device* device, UINT descriptorCount)
        : DescriptorArray( device, descriptorCount, D3D12_DESCRIPTOR_HEAP_TYPE_DSV )
    {}

    using DescriptorArray::Append;

    D3D12_CPU_DESCRIPTOR_HANDLE Append(ID3D12Resource* resource, D3D12_DEPTH_STENCIL_VIEW_DESC* desc = nullptr)
    {
        auto handle = Append();
        mDevice->CreateDepthStencilView(resource, desc, handle);
        return handle;
    }
};

// SRVs, CBVs, UAVs, etc.
class SRVDescriptorList : public DescriptorArray
{
public:
    SRVDescriptorList(ID3D12Device* device, UINT descriptorCount)
        : DescriptorArray( device, descriptorCount, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true )
    {}
    
    ID3D12DescriptorHeap* Heap() { return mHeap; }

    // In this class it's more useful to have the GPU handle returned
    // In the long run it's useful to have both (for copying, etc) but this suits our purposes for now

    D3D12_GPU_DESCRIPTOR_HANDLE AppendSRV(ID3D12Resource* resource, D3D12_SHADER_RESOURCE_VIEW_DESC* desc = nullptr)
    {
        auto handle = GPU(mCurrentSize); 
        mDevice->CreateShaderResourceView(resource, desc, DescriptorArray::Append());
        return handle;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE AppendCBV(D3D12_CONSTANT_BUFFER_VIEW_DESC* desc = nullptr)
    {
        auto handle = GPU(mCurrentSize);
        mDevice->CreateConstantBufferView(desc, DescriptorArray::Append());
        return handle;
    }
};

// Samplers
class SMPDescriptorList : public DescriptorArray
{
public:
    SMPDescriptorList(ID3D12Device* device, UINT descriptorCount)
        : DescriptorArray( device, descriptorCount, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, true )
    {}

    ID3D12DescriptorHeap* Heap() { return mHeap; }
    
    D3D12_GPU_DESCRIPTOR_HANDLE Append(D3D12_SAMPLER_DESC* desc)
    {
        auto handle = GPU(mCurrentSize);
        mDevice->CreateSampler(desc, DescriptorArray::Append());
        return handle;        
    }
};
