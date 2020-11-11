/*
 *  Copyright 2019-2020 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *  
 *      http://www.apache.org/licenses/LICENSE-2.0
 *  
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
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

#include <vector>

#include "../../../DiligentCore/Primitives/interface/BasicTypes.h"
#include "../../../DiligentCore/Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "../../../DiligentCore/Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "../../../DiligentCore/Graphics/GraphicsEngine/interface/Buffer.h"
#include "../../../DiligentCore/Graphics/GraphicsEngine/interface/Texture.h"
#include "../../../DiligentCore/Graphics/GraphicsEngine/interface/TextureView.h"
#include "../../../DiligentCore/Common/interface/BasicMath.hpp"
#include "../../../DiligentCore/Common/interface/RefCntAutoPtr.hpp"

namespace Diligent
{

//--------------------------------------------------------------------------------------
// Hard Defines for the various structures
//--------------------------------------------------------------------------------------
#define DXSDKMESH_FILE_VERSION 101

#define INVALID_FRAME          ((Uint32)-1)
#define INVALID_MESH           ((Uint32)-1)
#define INVALID_MATERIAL       ((Uint32)-1)
#define INVALID_SUBSET         ((Uint32)-1)
#define INVALID_ANIMATION_DATA ((Uint32)-1)
#define INVALID_SAMPLER_SLOT   ((Uint32)-1)
#define ERROR_RESOURCE_VALUE   1

// Enumerated Types.
enum DXSDKMESH_PRIMITIVE_TYPE
{
    PT_TRIANGLE_LIST = 0,
    PT_TRIANGLE_STRIP,
    PT_LINE_LIST,
    PT_LINE_STRIP,
    PT_POINT_LIST,
    PT_TRIANGLE_LIST_ADJ,
    PT_TRIANGLE_STRIP_ADJ,
    PT_LINE_LIST_ADJ,
    PT_LINE_STRIP_ADJ,
    PT_QUAD_PATCH_LIST,
    PT_TRIANGLE_PATCH_LIST,
};

enum DXSDKMESH_INDEX_TYPE
{
    IT_16BIT = 0,
    IT_32BIT,
};

enum FRAME_TRANSFORM_TYPE
{
    FTT_RELATIVE = 0,
    FTT_ABSOLUTE, //This is not currently used but is here to support absolute transformations in the future
};

// Structures.  Unions with pointers are forced to 64bit.
struct DXSDKMESH_HEADER
{
    //Basic Info and sizes
    Uint32 Version;
    Uint8  IsBigEndian;
    Uint64 HeaderSize;
    Uint64 NonBufferDataSize;
    Uint64 BufferDataSize;

    //Stats
    Uint32 NumVertexBuffers;
    Uint32 NumIndexBuffers;
    Uint32 NumMeshes;
    Uint32 NumTotalSubsets;
    Uint32 NumFrames;
    Uint32 NumMaterials;

    //Offsets to Data
    Uint64 VertexStreamHeadersOffset;
    Uint64 IndexStreamHeadersOffset;
    Uint64 MeshDataOffset;
    Uint64 SubsetDataOffset;
    Uint64 FrameDataOffset;
    Uint64 MaterialDataOffset;
};

enum DXSDKMESH_VERTEX_SEMANTIC
{
    DXSDKMESH_VERTEX_SEMANTIC_POSITION = 0,
    DXSDKMESH_VERTEX_SEMANTIC_BLENDWEIGHT,  // 1
    DXSDKMESH_VERTEX_SEMANTIC_BLENDINDICES, // 2
    DXSDKMESH_VERTEX_SEMANTIC_NORMAL,       // 3
    DXSDKMESH_VERTEX_SEMANTIC_PSIZE,        // 4
    DXSDKMESH_VERTEX_SEMANTIC_TEXCOORD,     // 5
    DXSDKMESH_VERTEX_SEMANTIC_TANGENT,      // 6
    DXSDKMESH_VERTEX_SEMANTIC_BINORMAL,     // 7
    DXSDKMESH_VERTEX_SEMANTIC_TESSFACTOR,   // 8
    DXSDKMESH_VERTEX_SEMANTIC_POSITIONT,    // 9
    DXSDKMESH_VERTEX_SEMANTIC_COLOR,        // 10
    DXSDKMESH_VERTEX_SEMANTIC_FOG,          // 11
    DXSDKMESH_VERTEX_SEMANTIC_DEPTH,        // 12
    DXSDKMESH_VERTEX_SEMANTIC_SAMPLE        // 13
};

enum DXSDKMESH_VERTEX_DATA_TYPE
{
    DXSDKMESH_VERTEX_DATA_TYPE_FLOAT1   = 0, // 1D float expanded to (value, 0., 0., 1.)
    DXSDKMESH_VERTEX_DATA_TYPE_FLOAT2   = 1, // 2D float expanded to (value, value, 0., 1.)
    DXSDKMESH_VERTEX_DATA_TYPE_FLOAT3   = 2, // 3D float expanded to (value, value, value, 1.)
    DXSDKMESH_VERTEX_DATA_TYPE_FLOAT4   = 3, // 4D float
    DXSDKMESH_VERTEX_DATA_TYPE_D3DCOLOR = 4, // 4D packed unsigned bytes mapped to 0. to 1. range
                                             // Input is in D3DCOLOR format (ARGB) expanded to (R, G, B, A)
    DXSDKMESH_VERTEX_DATA_TYPE_UBYTE4 = 5,   // 4D unsigned byte
    DXSDKMESH_VERTEX_DATA_TYPE_SHORT2 = 6,   // 2D signed short expanded to (value, value, 0., 1.)
    DXSDKMESH_VERTEX_DATA_TYPE_SHORT4 = 7,   // 4D signed short

    DXSDKMESH_VERTEX_DATA_TYPE_UBYTE4N   = 8,  // Each of 4 bytes is normalized by dividing to 255.0
    DXSDKMESH_VERTEX_DATA_TYPE_SHORT2N   = 9,  // 2D signed short normalized (v[0]/32767.0,v[1]/32767.0,0,1)
    DXSDKMESH_VERTEX_DATA_TYPE_SHORT4N   = 10, // 4D signed short normalized (v[0]/32767.0,v[1]/32767.0,v[2]/32767.0,v[3]/32767.0)
    DXSDKMESH_VERTEX_DATA_TYPE_USHORT2N  = 11, // 2D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,0,1)
    DXSDKMESH_VERTEX_DATA_TYPE_USHORT4N  = 12, // 4D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,v[2]/65535.0,v[3]/65535.0)
    DXSDKMESH_VERTEX_DATA_TYPE_UDEC3     = 13, // 3D unsigned 10 10 10 format expanded to (value, value, value, 1)
    DXSDKMESH_VERTEX_DATA_TYPE_DEC3N     = 14, // 3D signed 10 10 10 format normalized and expanded to (v[0]/511.0, v[1]/511.0, v[2]/511.0, 1)
    DXSDKMESH_VERTEX_DATA_TYPE_FLOAT16_2 = 15, // Two 16-bit floating point values, expanded to (value, value, 0, 1)
    DXSDKMESH_VERTEX_DATA_TYPE_FLOAT16_4 = 16, // Four 16-bit floating point values
    DXSDKMESH_VERTEX_DATA_TYPE_UNUSED    = 17, // When the type field in a decl is unused.
};

struct DXSDKMESH_VERTEX_ELEMENT
{
    Uint16 Stream;     // Stream index
    Uint16 Offset;     // Offset in the stream in Uint8s
    Uint8  Type;       // Data type
    Uint8  Method;     // Processing method
    Uint8  Usage;      // Semantics
    Uint8  UsageIndex; // Semantic index
};


struct DXSDKMESH_VERTEX_BUFFER_HEADER
{
    Uint64 NumVertices;
    Uint64 SizeUint8s;
    Uint64 StrideUint8s;

    static constexpr size_t  MaxVertexElements = 32;
    DXSDKMESH_VERTEX_ELEMENT Decl[MaxVertexElements];
    union
    {
        Uint64   DataOffset; //(This also forces the union to 64bits)
        IBuffer* pVB;
    };
};

struct DXSDKMESH_INDEX_BUFFER_HEADER
{
    Uint64 NumIndices;
    Uint64 SizeUint8s;
    Uint32 IndexType;
    union
    {
        Uint64   DataOffset; //(This also forces the union to 64bits)
        IBuffer* pIB;
    };
};

struct DXSDKMESH_MESH
{
    static constexpr size_t MaxMeshName = 100;
    char                    Name[MaxMeshName];
    Uint8                   NumVertexBuffers;
    static constexpr size_t MaxVertexStreams = 16;
    Uint32                  VertexBuffers[MaxVertexStreams];
    Uint32                  IndexBuffer;
    Uint32                  NumSubsets;
    Uint32                  NumFrameInfluences; //aka bones

    float3 BoundingBoxCenter;
    float3 BoundingBoxExtents;

    union
    {
        Uint64  SubsetOffset; //Offset to list of subsets (This also forces the union to 64bits)
        Uint32* pSubsets;     //Pointer to list of subsets
    };
    union
    {
        Uint64  FrameInfluenceOffset; //Offset to list of frame influences (This also forces the union to 64bits)
        Uint32* pFrameInfluences;     //Pointer to list of frame influences
    };
};

struct DXSDKMESH_SUBSET
{
    static constexpr size_t MaxSubsetName = 100;
    char                    Name[MaxSubsetName];
    Uint32                  MaterialID;
    Uint32                  PrimitiveType;
    Uint64                  IndexStart;
    Uint64                  IndexCount;
    Uint64                  VertexStart;
    Uint64                  VertexCount;
};

struct DXSDKMESH_FRAME
{
    static constexpr size_t MaxFrameName = 100;
    char                    Name[MaxFrameName];
    Uint32                  Mesh;
    Uint32                  ParentFrame;
    Uint32                  ChildFrame;
    Uint32                  SiblingFrame;
    float4x4                Matrix;
    Uint32                  AnimationDataIndex; //Used to index which set of keyframes transforms this frame
};

struct DXSDKMESH_MATERIAL
{
    static constexpr size_t MaxMaterialName = 100;
    char                    Name[MaxMaterialName];

    // Use MaterialInstancePath
    static constexpr size_t MaxMaterialPath = 260;
    char                    MaterialInstancePath[MaxMaterialPath];

    // Or fall back to d3d8-type materials
    static constexpr size_t MaxTextureName = 260;
    char                    DiffuseTexture[MaxTextureName];
    char                    NormalTexture[MaxTextureName];
    char                    SpecularTexture[MaxTextureName];

    float4 Diffuse;
    float4 Ambient;
    float4 Specular;
    float4 Emissive;
    float  Power;

    union
    {
        Uint64    Force64_1; //Force the union to 64bits
        ITexture* pDiffuseTexture = nullptr;
    };
    union
    {
        Uint64    Force64_2; //Force the union to 64bits
        ITexture* pNormalTexture = nullptr;
    };
    union
    {
        Uint64    Force64_3; //Force the union to 64bits
        ITexture* pSpecularTexture = nullptr;
    };

    union
    {
        Uint64        Force64_4; //Force the union to 64bits
        ITextureView* pDiffuseRV = nullptr;
    };
    union
    {
        Uint64        Force64_5; //Force the union to 64bits
        ITextureView* pNormalRV = nullptr;
    };
    union
    {
        Uint64        Force64_6; //Force the union to 64bits
        ITextureView* pSpecularRV = nullptr;
    };
};

struct SDKANIMATION_FILE_HEADER
{
    Uint32 Version;
    Uint8  IsBigEndian;
    Uint32 FrameTransformType;
    Uint32 NumFrames;
    Uint32 NumAnimationKeys;
    Uint32 AnimationFPS;
    Uint64 AnimationDataSize;
    Uint64 AnimationDataOffset;
};

struct SDKANIMATION_DATA
{
    float3 Translation;
    float4 Orientation;
    float3 Scaling;
};

struct SDKANIMATION_FRAME_DATA
{
    static constexpr size_t MaxFrameName = 100;
    char                    FrameName[MaxFrameName];
    union
    {
        Uint64             DataOffset;
        SDKANIMATION_DATA* pAnimationData;
    };
};


// This class reads the DXSDKMesh file formats
class DXSDKMesh
{
public:
    virtual ~DXSDKMesh();

    bool Create(const Char* szFileName);
    bool Create(Uint8* pData, Uint32 DataUint8s);
    void LoadGPUResources(const Char* ResourceDirectory, IRenderDevice* pDevice, IDeviceContext* pDeviceCtx);
    void Destroy();

    //Helpers
    static PRIMITIVE_TOPOLOGY GetPrimitiveType(DXSDKMESH_PRIMITIVE_TYPE PrimType);
    VALUE_TYPE                GetIBFormat(Uint32 iMesh) const;
    DXSDKMESH_INDEX_TYPE      GetIndexType(Uint32 iMesh) const
    {
        return (DXSDKMESH_INDEX_TYPE)m_pIndexBufferArray[m_pMeshArray[iMesh].IndexBuffer].IndexType;
    }

    // clang-format off
    Uint32 GetNumMeshes()    const { return m_pMeshHeader ? m_pMeshHeader->NumMeshes : 0; }
    Uint32 GetNumMaterials() const { return m_pMeshHeader ? m_pMeshHeader->NumMaterials : 0; }
    Uint32 GetNumVBs()       const { return m_pMeshHeader ? m_pMeshHeader->NumVertexBuffers : 0; }
    Uint32 GetNumIBs()       const { return m_pMeshHeader ? m_pMeshHeader->NumIndexBuffers : 0; }
    // clang-format on

    const Uint8*              GetRawVerticesAt(Uint32 iVB) const { return m_ppVertices[iVB]; }
    const Uint8*              GetRawIndicesAt(Uint32 iIB) const { return m_ppIndices[iIB]; }
    const DXSDKMESH_MATERIAL& GetMaterial(Uint32 iMaterial) const { return m_pMaterialArray[iMaterial]; }

    const DXSDKMESH_MESH&   GetMesh(Uint32 iMesh) const { return m_pMeshArray[iMesh]; }
    Uint32                  GetNumSubsets(Uint32 iMesh) const { return m_pMeshArray[iMesh].NumSubsets; }
    const DXSDKMESH_SUBSET& GetSubset(Uint32 iMesh, Uint32 iSubset) const
    {
        return m_pSubsetArray[m_pMeshArray[iMesh].pSubsets[iSubset]];
    }

    Uint32 GetMeshVertexStride(Uint32 iMesh, Uint32 iVB) const
    {
        return (Uint32)m_pVertexBufferArray[m_pMeshArray[iMesh].VertexBuffers[iVB]].StrideUint8s;
    }

    Uint32 GetVertexStride(Uint32 iVB) const
    {
        return (Uint32)m_pVertexBufferArray[iVB].StrideUint8s;
    }

    Uint64 GetNumMeshVertices(Uint32 iMesh, Uint32 iVB) const
    {
        return m_pVertexBufferArray[m_pMeshArray[iMesh].VertexBuffers[iVB]].NumVertices;
    }

    Uint64 GetNumMeshIndices(Uint32 iMesh) const
    {
        return m_pIndexBufferArray[m_pMeshArray[iMesh].IndexBuffer].NumIndices;
    }

    IBuffer* GetMeshVertexBuffer(Uint32 iMesh, Uint32 iVB)
    {
        return m_VertexBuffers[m_pMeshArray[iMesh].VertexBuffers[iVB]];
    }

    IBuffer* GetMeshIndexBuffer(Uint32 iMesh)
    {
        return m_IndexBuffers[m_pMeshArray[iMesh].IndexBuffer];
    }

    const DXSDKMESH_VERTEX_ELEMENT* VBElements(Uint32 iVB) const { return m_pVertexBufferArray[0].Decl; }

    //Uint32                          GetNumFrames();
    //DXSDKMESH_FRAME*                GetFrame( Uint32 iFrame );
    //DXSDKMESH_FRAME*                FindFrame( char* pszName );

protected:
    bool CreateFromFile(const char* szFileName);

    bool CreateFromMemory(Uint8* pData,
                          Uint32 DataUint8s);

    void ComputeBoundingBoxes();

    //These are the pointers to the two chunks of data loaded in from the mesh file
    std::vector<Uint8> m_StaticMeshData;
    //Uint8*  m_pAnimationData    = nullptr;
    std::vector<Uint8*> m_ppVertices;
    std::vector<Uint8*> m_ppIndices;

    std::vector<RefCntAutoPtr<IBuffer>> m_VertexBuffers;
    std::vector<RefCntAutoPtr<IBuffer>> m_IndexBuffers;

    //General mesh info
    DXSDKMESH_HEADER*               m_pMeshHeader        = nullptr;
    DXSDKMESH_VERTEX_BUFFER_HEADER* m_pVertexBufferArray = nullptr;
    DXSDKMESH_INDEX_BUFFER_HEADER*  m_pIndexBufferArray  = nullptr;
    DXSDKMESH_MESH*                 m_pMeshArray         = nullptr;
    DXSDKMESH_SUBSET*               m_pSubsetArray       = nullptr;
    DXSDKMESH_FRAME*                m_pFrameArray        = nullptr;
    DXSDKMESH_MATERIAL*             m_pMaterialArray     = nullptr;

    // Adjacency information (not part of the m_pStaticMeshData, so it must be created and destroyed separately )
    //DXSDKMESH_INDEX_BUFFER_HEADER* m_pAdjacencyIndexBufferArray = nullptr;

    //Animation (TODO: Add ability to load/track multiple animation sets)
    //SDKANIMATION_FILE_HEADER*   m_pAnimationHeader          = nullptr;
    //SDKANIMATION_FRAME_DATA*    m_pAnimationFrameData       = nullptr;
    //float4x4*                   m_pBindPoseFrameMatrices    = nullptr;
    //float4x4*                   m_pTransformedFrameMatrices = nullptr;
    //float4x4*                   m_pWorldPoseFrameMatrices   = nullptr;
};

} // namespace Diligent
