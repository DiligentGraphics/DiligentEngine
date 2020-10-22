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

#include <vector>
#include <directxmath.h>

typedef unsigned short IndexType;

// NOTE: This data could be compressed, but it's not really the bottleneck at the moment
struct Vertex
{
    float x;
    float y;
    float z;
    float nx;
    float ny;
    float nz;
};

struct Mesh
{
    void clear()
    {
        vertices.clear();
        indices.clear();
    }

    std::vector<Vertex> vertices;
    std::vector<IndexType> indices;
};

void CreateIcosahedron(Mesh *outMesh);

// 1 face -> 4 faces
void SubdivideInPlace(Mesh *outMesh);

void SpherifyInPlace(Mesh *outMesh, float radius = 1.0f);

void ComputeAvgNormalsInPlace(Mesh *outMesh);

// subdivIndexOffset array should be [subdivLevels+2] in size
void CreateGeospheres(Mesh *outMesh, unsigned int subdivLevelCount, unsigned int* outSubdivIndexOffsets);

// Returns a combined "mesh" that includes:
// - A set of indices for each subdiv level (outSubdivIndexOffsets for offsets/counts)
// - A set of vertices for each mesh instance (base vertices per mesh computed from vertexCountPerMesh)
// - Indices already have the vertex offsets for the correct subdiv level "baked-in", so only need the mesh offset
void CreateAsteroidsFromGeospheres(Mesh *outMesh,
                                   unsigned int subdivLevelCount, unsigned int meshInstanceCount,
                                   unsigned int rngSeed,
                                   unsigned int* outSubdivIndexOffsets, unsigned int* vertexCountPerMesh);


struct SkyboxVertex
{
    float x;
    float y;
    float z;
    float u;
    float v;
    float face;
};

// Skybox mesh... vertices-only currently
void CreateSkyboxMesh(std::vector<SkyboxVertex>* outVertices);