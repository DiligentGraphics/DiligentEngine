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
#include "descriptor.h"

#include <d3d12.h>

__declspec(align(64)) // Avoid false sharing
class SubsetD3D12
{
public:
    SubsetD3D12(ID3D12Device* mDevice, UINT srvCount, ID3D12PipelineState* pso)
    {
        ThrowIfFailed(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCmdAlloc)));
        ThrowIfFailed(mDevice->CreateCommandList(1, D3D12_COMMAND_LIST_TYPE_DIRECT, mCmdAlloc, pso, IID_PPV_ARGS(&mCmdLst)));
        ThrowIfFailed(mCmdLst->Close());
    }

    ~SubsetD3D12()
    {
        SafeRelease(&mCmdLst);
        SafeRelease(&mCmdAlloc);
    }
    
    ID3D12GraphicsCommandList* Begin(ID3D12PipelineState* pso)
    {
        ThrowIfFailed(mCmdAlloc->Reset());
        ThrowIfFailed(mCmdLst->Reset(mCmdAlloc, pso));
        return mCmdLst;
    }

    ID3D12GraphicsCommandList* End()
    {
        ThrowIfFailed(mCmdLst->Close());
        return mCmdLst;
    }

    ID3D12GraphicsCommandList* mCmdLst = nullptr;
    ID3D12CommandAllocator*    mCmdAlloc = nullptr;
};
