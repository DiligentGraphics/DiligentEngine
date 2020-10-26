// Copyright 2014 Intel Corporation All Rights Reserved
//
// Intel makes no representations about the suitability of this software for any purpose.  
// THIS SOFTWARE IS PROVIDED ""AS IS."" INTEL SPECIFICALLY DISCLAIMS ALL WARRANTIES,
// EXPRESS OR IMPLIED, AND ALL LIABILITY, INCLUDING CONSEQUENTIAL AND OTHER INDIRECT DAMAGES,
// FOR THE USE OF THIS SOFTWARE, INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PROPRIETARY
// RIGHTS, AND INCLUDING THE WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
// Intel does not assume any responsibility for any errors which may appear in this software
// nor any responsibility to update it.

#include "texture.h"
#include "util.h"
#include "noise.h"
#include "DDSTextureLoader.h"

#include <stdint.h>
#include <sstream>


static void WaitForAll(ID3D12Device* device, ID3D12CommandQueue* queue)
{
    // Kind of ugly, but yeah...
    ID3D12Fence* fence = nullptr;
    ThrowIfFailed(device->CreateFence( 0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
    auto eventHandle = CreateEvent(NULL, FALSE, FALSE, NULL);

    queue->Signal(fence, 1);
    fence->SetEventOnCompletion(1, eventHandle);
    WaitForSingleObject(eventHandle, INFINITE);

    CloseHandle(eventHandle);
    fence->Release();
}


void GenerateMips2D_XXXX8(D3D11_SUBRESOURCE_DATA* subresources, size_t widthLevel0, size_t heightLevel0, size_t mipLevels)
{
    for (size_t m = 1; m < mipLevels; ++m) {
        auto rowPitchSrc = subresources[m - 1].SysMemPitch;
        const BYTE* dataSrc = (BYTE*)subresources[m - 1].pSysMem;

        auto rowPitchDst = subresources[m].SysMemPitch;
        BYTE* dataDst = (BYTE*)subresources[m].pSysMem;
        
        auto width = widthLevel0 >> m;
        auto height = heightLevel0 >> m;

        // Iterating byte-wise is simpler in this case (pulls apart color nicely)
        // Not optimized at all, obviously...
        for (size_t y = 0; y < height; ++y) {
            auto rowSrc0 = (dataSrc + (y*2+0)*rowPitchSrc);
            auto rowSrc1 = (dataSrc + (y*2+1)*rowPitchSrc);
            auto rowDst  = (dataDst + (y    )*rowPitchDst);
            for (size_t x = 0; x < width; ++x) {
                for (size_t comp = 0; comp < 4; ++comp) {
                    uint32_t c = rowSrc0[x*8+comp+0];
                    c +=         rowSrc0[x*8+comp+4];
                    c +=         rowSrc1[x*8+comp+0];
                    c +=         rowSrc1[x*8+comp+4];
                    c = c / 4;
                    assert(c < 256);
                    rowDst[4*x+comp] = (byte)c;
                }
            }
        }
    }
}


void FillNoise2D_RGBA8(D3D11_SUBRESOURCE_DATA* subresources, size_t width, size_t height, size_t mipLevels,
                       float seed, float persistence, float noiseScale, float noiseStrength,
					   float redScale, float greenScale, float blueScale)
{
    NoiseOctaves<4> textureNoise(persistence);
    
    // Level 0
    for (size_t y = 0; y < height; ++y) {
        uint32_t* row = (uint32_t*)((BYTE*)subresources[0].pSysMem + y*subresources[0].SysMemPitch);
        for (size_t x = 0; x < width; ++x) {
            auto c = textureNoise((float)x*noiseScale, (float)y*noiseScale, seed);
            c = std::max(0.0f, std::min(1.0f, (c - 0.5f) * noiseStrength + 0.5f));

            int32_t cr = (int32_t)(c * redScale);
			int32_t cg = (int32_t)(c * greenScale);
			int32_t cb = (int32_t)(c * blueScale);
			assert(cr >= 0 && cr < 256);
			assert(cg >= 0 && cg < 256);
            assert(cb >= 0 && cb < 256);

            row[x] = (cr) << 16 | (cg) <<  8 | (cb) << 0;
        }
    }

    if (mipLevels > 1)
        GenerateMips2D_XXXX8(subresources, width, height, mipLevels);
}


void InitializeTexture2D(
    ID3D12Device* device, ID3D12CommandQueue* cmdQueue,
    ID3D12Resource* texture, const D3D12_RESOURCE_DESC* desc, UINT bytesPerPixel,
    const D3D11_SUBRESOURCE_DATA* initialData,
    D3D12_RESOURCE_STATES stateAfter)
{
    // Pull some data
    auto format = desc->Format;
    UINT width = (UINT)desc->Width;
    UINT height = desc->Height;
    UINT arraySize = desc->DepthOrArraySize;
    UINT mipLevels = desc->MipLevels;
 
    // Pow2 mip chain!
    assert(mipLevels == 1 || ((width & (width-1)) == 0 && (height & (height-1)) == 0));
        
    std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> placedUpload;
    UINT64 totalSize = 0;
    for (UINT a = 0; a < arraySize; ++a) {
        for (UINT m = 0; m < mipLevels; ++m) {
            D3D12_PLACED_SUBRESOURCE_FOOTPRINT placed = {};
            placed.Footprint.Format = format;
            placed.Footprint.Width = width >> m;   // TODO: Handle mip sizes properly!
            placed.Footprint.Height = height >> m; // TODO: Handle mip sizes properly!
            placed.Footprint.Depth = 1;
            placed.Footprint.RowPitch = Align<UINT>(placed.Footprint.Width * bytesPerPixel, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
            placed.Offset = totalSize;
        
            totalSize = Align<UINT64>(placed.Offset + (size_t{placed.Footprint.RowPitch} * size_t{placed.Footprint.Height}),
                                      D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
            placedUpload.push_back(placed);
        }
    }

    ID3D12Resource* uploadBuffer = nullptr;
    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(totalSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&uploadBuffer)
    ));

    BYTE *baseData = nullptr;
    ThrowIfFailed(uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&baseData)));    
     
    // Fill in data (RGBA8)
    for (UINT a = 0; a < arraySize; ++a) {
        for (UINT m = 0; m < mipLevels; ++m) {
            auto subresource = a * mipLevels + m;
            
            BYTE* dataSrc = (BYTE*)initialData[subresource].pSysMem;
            auto rowPitchSrc = initialData[subresource].SysMemPitch;

            auto placed = &placedUpload[subresource];
            BYTE* dataDst = baseData + placed->Offset;
            auto rowPitchDst = placed->Footprint.RowPitch;
            UINT width_mip = placed->Footprint.Width;
            UINT height_mip = placed->Footprint.Height;

            for (UINT y = 0; y < height_mip; ++y) {
                memcpy(dataDst + y*size_t{rowPitchDst}, dataSrc + y*size_t{rowPitchSrc}, size_t{bytesPerPixel} * width_mip);
            }
        }
    }

    // Create some new resources for initialization
    ID3D12GraphicsCommandList* cmdLst = nullptr;
    ID3D12CommandAllocator* cmdAlloc = nullptr;
    ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAlloc)));
    ThrowIfFailed(device->CreateCommandList(1, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc, nullptr, IID_PPV_ARGS(&cmdLst)));

    {
        ResourceBarrier rb;
        rb.AddTransition(texture, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
        rb.Submit(cmdLst);
    }

    // Copy data from each subresource into texture
    for (size_t s = 0; s < placedUpload.size(); ++s) {
        CD3DX12_TEXTURE_COPY_LOCATION dest(texture, static_cast<UINT>(s));
        CD3DX12_TEXTURE_COPY_LOCATION src(uploadBuffer, placedUpload[s]);

        cmdLst->CopyTextureRegion(
            &dest, 0, 0, 0,
            &src, nullptr);
    }

    {
        ResourceBarrier rb;
        rb.AddTransition(texture, D3D12_RESOURCE_STATE_COPY_DEST, stateAfter);
        rb.Submit(cmdLst);
    }

    ThrowIfFailed(cmdLst->Close());
    cmdQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList*const*>(&cmdLst));

    // Kind of ugly... but yeah
    WaitForAll(device, cmdQueue);

    SafeRelease(&uploadBuffer);
    SafeRelease(&cmdLst);
    SafeRelease(&cmdAlloc);
}


HRESULT CreateTexture2DFromDDS_XXXX8(
    ID3D12Device* device, ID3D12CommandQueue* cmdQueue,
    ID3D12Resource** texture, 
    const char* fileName, 
    DXGI_FORMAT format, 
    D3D12_RESOURCE_STATES stateAfter )
{
    BYTE* heapData = nullptr;
    DDS_HEADER* header = nullptr;
    BYTE* bitData = nullptr;
    UINT bitSize = 0;

    std::wostringstream wfileName;
    wfileName << fileName;

    HRESULT hr = LoadTextureDataFromFile(wfileName.str().c_str(), &heapData, &header, &bitData, &bitSize);
    if (FAILED(hr)) {
        delete[] heapData;
        return hr;
    }

    unsigned int arraySize = 1;

    if (header->dwCaps2 & DDS_CUBEMAP) {
        assert((header->dwCaps2 & DDS_CUBEMAP_ALLFACES ) == DDS_CUBEMAP_ALLFACES);
        arraySize = 6;
    }

    // We only support XXXX8_UNORM[_SRGB] atm...
    if (format != DXGI_FORMAT_B8G8R8A8_UNORM && format != DXGI_FORMAT_R8G8B8A8_UNORM &&
        format != DXGI_FORMAT_B8G8R8A8_UNORM_SRGB && format != DXGI_FORMAT_R8G8B8A8_UNORM_SRGB) {
        return E_NOTIMPL;
    }

    unsigned int mipLevels = header->dwMipMapCount;
    if (mipLevels == 0) mipLevels = 1; // Hacky... but good enough for now
    
    // TODO: lots of stuff is not checked... just don't be stupid, this is hacky sample code after all :)

    auto desc = CD3DX12_RESOURCE_DESC::Tex2D(
        format, header->dwWidth, header->dwHeight,
        (UINT16)arraySize, (UINT16)mipLevels );

    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(texture)
    ));

    std::vector<D3D11_SUBRESOURCE_DATA> initialData(size_t{desc.MipLevels} * size_t{arraySize});
    
    BYTE* srcBits = bitData;
    const BYTE *endBits = bitData + bitSize;

    for (UINT a = 0; a < arraySize; ++a) {
        for (UINT m = 0; m < desc.MipLevels; ++m) {
            auto width  = (UINT)desc.Width >> m;
            auto height = desc.Height >> m;
            auto subresource = a * desc.MipLevels + m;
            auto rowPitch = 4 * width;
            auto bytes = rowPitch * height;

            assert(srcBits + bytes <= endBits);

            initialData[subresource].pSysMem = (void*)srcBits;
            initialData[subresource].SysMemPitch = rowPitch;
            initialData[subresource].SysMemSlicePitch = bytes;
                
            srcBits += bytes;
        }
    }

    InitializeTexture2D(device, cmdQueue, *texture, &desc, 4, initialData.data(), stateAfter);
    
    delete[] heapData;
    return S_OK;
}
