// Copyright 2014 Intel Corporation All Rights Reserved
//
// Intel makes no representations about the suitability of this software for any purpose.  
// THIS SOFTWARE IS PROVIDED ""AS IS."" INTEL SPECIFICALLY DISCLAIMS ALL WARRANTIES,
// EXPRESS OR IMPLIED, AND ALL LIABILITY, INCLUDING CONSEQUENTIAL AND OTHER INDIRECT DAMAGES,
// FOR THE USE OF THIS SOFTWARE, INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PROPRIETARY
// RIGHTS, AND INCLUDING THE WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
// Intel does not assume any responsibility for any errors which may appear in this software
// nor any responsibility to update it.

#include "mesh.h"
#include "noise.h"
#include <map>
#include <random>

using namespace DirectX;

void CreateIcosahedron(Mesh *outMesh)
{
    static const float a = std::sqrt(2.0f / (5.0f - std::sqrt(5.0f)));
    static const float b = std::sqrt(2.0f / (5.0f + std::sqrt(5.0f)));

    static const size_t num_vertices = 12;
    static const Vertex vertices[num_vertices] = // x, y, z
    {
        {-b,  a,  0},
        { b,  a,  0},
        {-b, -a,  0},
        { b, -a,  0},
        { 0, -b,  a},
        { 0,  b,  a},
        { 0, -b, -a},
        { 0,  b, -a},
        { a,  0, -b},
        { a,  0,  b},
        {-a,  0, -b},
        {-a,  0,  b},
    };

    static const size_t num_triangles = 20;
    static const IndexType indices[num_triangles*3] =
    {
         0,  5, 11,
         0,  1,  5,
         0,  7,  1,
         0, 10,  7,
         0, 11, 10,
         1,  9,  5,
         5,  4, 11,
        11,  2, 10,
        10,  6,  7,
         7,  8,  1,
         3,  4,  9,
         3,  2,  4,
         3,  6,  2,
         3,  8,  6,
         3,  9,  8,
         4,  5,  9,
         2, 11,  4,
         6, 10,  2,
         8,  7,  6,
         9,  1,  8,
    };

    outMesh->clear();
    outMesh->vertices.insert(outMesh->vertices.end(), vertices, vertices + num_vertices);
    outMesh->indices.insert(outMesh->indices.end(), indices, indices + num_triangles*3);
}


// Maps edge (lower index first!) to
struct Edge
{
    Edge(IndexType i0, IndexType i1)
        : v0(i0), v1(i1)
    {
        if (v0 > v1)
            std::swap(v0, v1);
    }
    IndexType v0;
    IndexType v1;

    bool operator<(const Edge &c) const
    {
        return v0 < c.v0 || (v0 == c.v0 && v1 < c.v1);
    }
};

typedef std::map<Edge, IndexType> MidpointMap;

inline IndexType EdgeMidpoint(Mesh *mesh, MidpointMap *midpoints, Edge e)
{
    auto index = midpoints->find(e);
    if (index == midpoints->end())
    {
        auto a = mesh->vertices[e.v0];
        auto b = mesh->vertices[e.v1];

        Vertex m;
        m.x = (a.x + b.x) * 0.5f;
        m.y = (a.y + b.y) * 0.5f;
        m.z = (a.z + b.z) * 0.5f;

        index = midpoints->insert(std::make_pair(e, static_cast<IndexType>(mesh->vertices.size()))).first;
        mesh->vertices.push_back(m);
    }
    return index->second;
}


void SubdivideInPlace(Mesh *outMesh)
{
    MidpointMap midpoints;

    std::vector<IndexType> newIndices;
    newIndices.reserve(outMesh->indices.size() * 4);
    outMesh->vertices.reserve(outMesh->vertices.size() * 2);

    assert(outMesh->indices.size() % 3 == 0); // trilist
    size_t triangles = outMesh->indices.size() / 3;
    for (size_t t = 0; t < triangles; ++t)
    {
        auto t0 = outMesh->indices[t*3+0];
        auto t1 = outMesh->indices[t*3+1];
        auto t2 = outMesh->indices[t*3+2];

        auto m0 = EdgeMidpoint(outMesh, &midpoints, Edge(t0, t1));
        auto m1 = EdgeMidpoint(outMesh, &midpoints, Edge(t1, t2));
        auto m2 = EdgeMidpoint(outMesh, &midpoints, Edge(t2, t0));

        IndexType indices[] = {
            t0, m0, m2,
            m0, t1, m1,
            m0, m1, m2,
            m2, m1, t2,
        };
        newIndices.insert(newIndices.end(), indices, indices + 4*3);
    }

    std::swap(outMesh->indices, newIndices); // Constant time
}


void SpherifyInPlace(Mesh *outMesh, float radius)
{
    for (auto &v : outMesh->vertices) {
        float n = radius / std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
        v.x *= n;
        v.y *= n;
        v.z *= n;
    }
}


void ComputeAvgNormalsInPlace(Mesh *outMesh)
{
    for (auto &v : outMesh->vertices) {
        v.nx = 0.0f;
        v.ny = 0.0f;
        v.nz = 0.0f;
    }

    assert(outMesh->indices.size() % 3 == 0); // trilist
    size_t triangles = outMesh->indices.size() / 3;
    for (size_t t = 0; t < triangles; ++t)
    {
        auto v1 = &outMesh->vertices[outMesh->indices[t*3+0]];
        auto v2 = &outMesh->vertices[outMesh->indices[t*3+1]];
        auto v3 = &outMesh->vertices[outMesh->indices[t*3+2]];

        // Two edge vectors u,v
        auto ux = v2->x - v1->x;
        auto uy = v2->y - v1->y;
        auto uz = v2->z - v1->z;
        auto vx = v3->x - v1->x;
        auto vy = v3->y - v1->y;
        auto vz = v3->z - v1->z;

        // cross(u,v)
        float nx = uy*vz - uz*vy;
        float ny = uz*vx - ux*vz;
        float nz = ux*vy - uy*vx;

        // Do not normalize... weight average by contributing face area
        v1->nx += nx; v1->ny += ny; v1->nz += nz;
        v2->nx += nx; v2->ny += ny; v2->nz += nz;
        v3->nx += nx; v3->ny += ny; v3->nz += nz;
    }

    // Normalize
    for (auto &v : outMesh->vertices) {
        float n = 1.0f / std::sqrt(v.nx*v.nx + v.ny*v.ny + v.nz*v.nz);
        v.nx *= n;
        v.ny *= n;
        v.nz *= n;
    }
}


void CreateGeospheres(Mesh *outMesh, unsigned int subdivLevelCount, unsigned int* outSubdivIndexOffsets)
{
    CreateIcosahedron(outMesh);
    outSubdivIndexOffsets[0] = 0;

    std::vector<Vertex> vertices(outMesh->vertices);
    std::vector<IndexType> indices(outMesh->indices);

    for (unsigned int i = 0; i < subdivLevelCount; ++i) {
        outSubdivIndexOffsets[i+1] = (unsigned int)indices.size();
        SubdivideInPlace(outMesh);

        // Ensure we add the proper offset to the indices from this subdiv level for the combined mesh
        // This avoids also needing to track a base vertex index for each subdiv level
        IndexType vertexOffset = (IndexType)vertices.size();
        vertices.insert(vertices.end(), outMesh->vertices.begin(), outMesh->vertices.end());

        for (auto newIndex : outMesh->indices) {
            indices.push_back(newIndex + vertexOffset);
        }
    }
    outSubdivIndexOffsets[subdivLevelCount+1] = (unsigned int)indices.size();

    SpherifyInPlace(outMesh);

    // Put the union of vertices/indices back into the mesh object
    std::swap(outMesh->indices, indices);
    std::swap(outMesh->vertices, vertices);
}


void CreateAsteroidsFromGeospheres(Mesh *outMesh,
                                   unsigned int subdivLevelCount, unsigned int meshInstanceCount,
                                   unsigned int rngSeed,
                                   unsigned int* outSubdivIndexOffsets, unsigned int* vertexCountPerMesh)
{
    assert(subdivLevelCount <= meshInstanceCount);

    std::mt19937 rng(rngSeed);

    Mesh baseMesh;
    CreateGeospheres(&baseMesh, subdivLevelCount, outSubdivIndexOffsets);

    // Per unique mesh
    *vertexCountPerMesh = (unsigned int)baseMesh.vertices.size();
    std::vector<Vertex> vertices;
    vertices.reserve(meshInstanceCount * baseMesh.vertices.size());
    // Reuse indices for the different unique meshes

    auto randomNoise = std::uniform_real_distribution<float>(0.0f, 10000.0f);
    auto randomPersistence = std::normal_distribution<float>(0.95f, 0.04f);
    float noiseScale = 0.5f;
    float radiusScale = 0.9f;
    float radiusBias = 0.3f;

    // Create and randomize unique vertices for each mesh instance
    for (unsigned int m = 0; m < meshInstanceCount; ++m) {
        Mesh newMesh(baseMesh);
        NoiseOctaves<4> textureNoise(randomPersistence(rng));
        float noise = randomNoise(rng);

        for (auto &v : newMesh.vertices) {
            float radius = textureNoise(v.x*noiseScale, v.y*noiseScale, v.z*noiseScale, noise);
            radius = radius * radiusScale + radiusBias;
            v.x *= radius;
            v.y *= radius;
            v.z *= radius;
        }
        ComputeAvgNormalsInPlace(&newMesh);

        vertices.insert(vertices.end(), newMesh.vertices.begin(), newMesh.vertices.end());
    }

    // Copy to output
    std::swap(outMesh->indices, baseMesh.indices);
    std::swap(outMesh->vertices, vertices);
}


void CreateSkyboxMesh(std::vector<SkyboxVertex>* outVertices)
{
    // See http://msdn.microsoft.com/en-us/library/windows/desktop/bb204881(v=vs.85).aspx
    
    // Cube mesh centered at zero
    static const float c = 0.5f;
    static const DirectX::XMFLOAT3 vertexPos[] = { // x, y, z
        {-c,  c, -c}, // 0
        { c,  c, -c}, // 1
        { c,  c,  c}, // 2
        {-c,  c,  c}, // 3
        {-c, -c,  c}, // 4
        { c, -c,  c}, // 5
        { c, -c, -c}, // 6
        {-c, -c, -c}, // 7
    };

    static const size_t numIndices = 6*6;
    static const unsigned int indices[numIndices] = {
        2, 6, 1, 2, 5, 6, // right   (+c,  .,  .), face 0
        0, 4, 3, 0, 7, 4, // left    (-c,  .,  .), face 1
        0, 2, 1, 0, 3, 2, // top     ( ., +c,  .), face 2
        4, 6, 5, 4, 7, 6, // bottom  ( ., -c,  .), face 3
        3, 5, 2, 3, 4, 5, // back    ( .,  ., +c), face 4
        1, 6, 7, 1, 7, 0, // front   ( .,  ., -c), face 5
    };

    // Flatten out the indexed mesh and add the per-face parameters
    std::vector<SkyboxVertex> vertices(numIndices);
    for (size_t i = 0; i < numIndices; ++i) {
        auto &v = vertices[i];

        v.x = vertexPos[indices[i]].x;
        v.y = vertexPos[indices[i]].y;
        v.z = vertexPos[indices[i]].z;

        auto face = i / 6;
        v.face = (float)face;

        // Face uv's are fairly manual
        switch (face)
        {
        case 0:
            v.u = -v.z;
            v.v = -v.y;
            break;
        case 1:
            v.u =  v.z;
            v.v = -v.y;
            break;
        case 2:
            v.u =  v.x;
            v.v =  v.z;
            break;
        case 3:
            v.u =  v.x;
            v.v = -v.z;
            break;
        case 4:
            v.u =  v.x;
            v.v = -v.y;
            break;
        case 5:
            v.u = -v.x;
            v.v = -v.y;
            break;
        };
        // Move to 0..1
        v.u += 0.5f;
        v.v += 0.5f;
    }

    std::swap(*outVertices, vertices);
}
