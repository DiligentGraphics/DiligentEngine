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

#include <assert.h>
#include <d3d12.h>

#include <algorithm>
#include <vector>

#define CBUFFER_ALIGN __declspec(align(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT))

#define WIDE_HELPER_2(x) L##x
#define WIDE_HELPER_1(x) WIDE_HELPER_2(x)
#define W__FILE__ WIDE_HELPER_1(__FILE__)

template<typename T>
inline void SafeRelease(T** p)
{
    auto q = *p;
    if (q != nullptr) {
        q->Release();
        *p = nullptr;
    }
}

inline HRESULT ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr)) throw;
    return hr;
}

template <typename T>
inline T AlignUp(T v, T align)
{
    return (v + (align-1)) & ~(align-1);
}

struct ResourceBarrier {
    std::vector<D3D12_RESOURCE_BARRIER> mDescs;

    void AddTransition(
        ID3D12Resource* resource,
        D3D12_RESOURCE_STATES stateBefore,
        D3D12_RESOURCE_STATES stateAfter,
        UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
    {
        D3D12_RESOURCE_BARRIER desc = {};
        desc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        desc.Transition.pResource = resource;
        desc.Transition.StateBefore = stateBefore;
        desc.Transition.StateAfter = stateAfter;
        desc.Transition.Subresource = subresource;
        mDescs.push_back(desc);
    }

    void ReverseTransitions()
    {
        for (auto ii = mDescs.begin(), ie = mDescs.end(); ii != ie; ++ii) {
            if (ii->Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION) {
                std::swap(ii->Transition.StateBefore, ii->Transition.StateAfter);
            }
        }
    }

    void Submit(ID3D12GraphicsCommandList* commandList) const
    {
        assert(mDescs.size() <= UINT_MAX);
        commandList->ResourceBarrier((UINT)mDescs.size(), mDescs.data());
    }
};
