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

#include <d3d11.h> // For D3D11_SUBRESOURCE_DATA
#include <DirectXMath.h>
#include <vector>
#include <algorithm>
#include <random>

#include "mesh.h"
#include "settings.h"

// We may want to ISPC-ify this down the road and just let it own the data structure in AoSoA format or similar
// For now we'll just do the dumb thing and see if it's fast enough
struct AsteroidDynamic
{
    DirectX::XMMATRIX world;
    // These depend on chosen subdiv level, hence are not constant
    unsigned int indexStart;
    unsigned int indexCount;
};

struct AsteroidStatic
{
    DirectX::XMFLOAT3 surfaceColor;
    DirectX::XMFLOAT3 deepColor;
    DirectX::XMVECTOR spinAxis;
    float scale;
    float spinVelocity;
    float orbitVelocity;
    unsigned int vertexStart;
    unsigned int textureIndex;
};

class AsteroidsSimulation
{
private:
    // NOTE: Memory could be optimized further for efficient cache traversal, etc.
    std::vector<AsteroidStatic> mAsteroidStatic;
    std::vector<AsteroidDynamic> mAsteroidDynamic;

    Mesh mMeshes;
    std::vector<unsigned int> mIndexOffsets;
    unsigned int mSubdivCount;
    unsigned int mVertexCountPerMesh;

    unsigned int mTextureDim;
    unsigned int mTextureCount;
    unsigned int mTextureArraySize;
    unsigned int mTextureMipLevels;
    std::vector<BYTE> mTextureDataBuffer;
    std::vector<D3D11_SUBRESOURCE_DATA> mTextureSubresources;

    unsigned int SubresourceIndex(unsigned int texture, unsigned int arrayElement = 0, unsigned int mip = 0)
    {
        return mip + mTextureMipLevels * (arrayElement + mTextureArraySize * texture);
    }

    void CreateTextures(unsigned int textureCount, unsigned int rngSeed);
    
public:
    AsteroidsSimulation(unsigned int rngSeed, unsigned int asteroidCount,
                        unsigned int meshInstanceCount, unsigned int subdivCount,
                        unsigned int textureCount);

    const Mesh* Meshes() { return &mMeshes; }
    const D3D11_SUBRESOURCE_DATA* TextureData(unsigned int textureIndex)
    {
        return mTextureSubresources.data() + SubresourceIndex(textureIndex);
    }

    unsigned int GetTextureMipLevels()const{return mTextureMipLevels;}

    const AsteroidStatic* StaticData() const { return mAsteroidStatic.data(); }
    const AsteroidDynamic* DynamicData() const { return mAsteroidDynamic.data(); }

    // Can optionally provide a range of asteroids to update; count = 0 => to the end
    // This is useful for multithreading
    void Update(float frameTime, DirectX::XMVECTOR cameraEye, const Settings& settings,
                size_t startIndex = 0, size_t count = 0);
};
