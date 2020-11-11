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

#include <string>
#include <sstream>
#include <cfloat>

#include "DXSDKMeshLoader.hpp"
#include "DataBlobImpl.hpp"
#include "RefCntAutoPtr.hpp"
#include "FileWrapper.hpp"
#include "TextureUtilities.h"
#include "GraphicsAccessories.hpp"

namespace Diligent
{


//--------------------------------------------------------------------------------------
bool DXSDKMesh::CreateFromFile(const char* szFileName)
{
    FileWrapper File;
    File.Open(FileOpenAttribs{szFileName});
    if (!File)
    {
        LOG_ERROR("Failed to open SDK Mesh file ", szFileName);
        return false;
    }

    RefCntAutoPtr<IDataBlob> pFileData(MakeNewRCObj<DataBlobImpl>()(0));
    File->Read(pFileData);

    File.Close();

    auto res = CreateFromMemory(reinterpret_cast<Uint8*>(pFileData->GetDataPtr()),
                                static_cast<Uint32>(pFileData->GetSize()));

    return res;
}

void DXSDKMesh::ComputeBoundingBoxes()
{
    for (Uint32 i = 0; i < m_pMeshHeader->NumMeshes; i++)
    {
        auto& Mesh = m_pMeshArray[i];

        float3 Min{+FLT_MAX, +FLT_MAX, +FLT_MAX};
        float3 Max{-FLT_MAX, -FLT_MAX, -FLT_MAX};

        const auto& VertexData = m_pVertexBufferArray[Mesh.VertexBuffers[0]];
        auto*       PosDecl    = VertexData.Decl;
        while (PosDecl->Stream != 0xFF && PosDecl->Usage != DXSDKMESH_VERTEX_SEMANTIC_POSITION)
            ++PosDecl;
        VERIFY(PosDecl->Stream != 0xFF, "Position semantic not found in this buffer");
        VERIFY(PosDecl->Type == DXSDKMESH_VERTEX_DATA_TYPE_FLOAT3, "Vertex is expected to be a 3-component float vector");

        auto        IndexType = GetIndexType(i);
        const auto* Vertices  = GetRawVerticesAt(Mesh.VertexBuffers[0]);
        const auto* Indices   = GetRawIndicesAt(Mesh.IndexBuffer);
        auto        Stride    = GetVertexStride(Mesh.VertexBuffers[0]);
        for (Uint32 subsetIdx = 0; subsetIdx < Mesh.NumSubsets; ++subsetIdx)
        {
            auto& Subset = m_pSubsetArray[Mesh.pSubsets[subsetIdx]];

            for (Uint32 v = 0; v < Subset.IndexCount; ++v)
            {
                Uint32 Index = IndexType == IT_16BIT ?
                    reinterpret_cast<const Uint16*>(Indices)[Subset.IndexStart + v] :
                    reinterpret_cast<const Uint32*>(Indices)[Subset.IndexStart + v];
                const float3& Vertex =
                    reinterpret_cast<const float3&>(Vertices[Index * Stride + PosDecl->Offset]);
                Min = std::min(Min, Vertex);
                Max = std::max(Max, Vertex);
            }
        }
        Mesh.BoundingBoxCenter  = (Max + Min) * 0.5;
        Mesh.BoundingBoxExtents = (Max - Min);
    }
}

bool DXSDKMesh::CreateFromMemory(Uint8* pData,
                                 Uint32 DataUint8s)
{
    m_StaticMeshData.resize(DataUint8s);
    memcpy(m_StaticMeshData.data(), pData, DataUint8s);

    // Pointer fixup
    auto* pStaticMeshData = m_StaticMeshData.data();
    // clang-format off
    m_pMeshHeader        = reinterpret_cast<DXSDKMESH_HEADER*>              (pStaticMeshData);
    m_pVertexBufferArray = reinterpret_cast<DXSDKMESH_VERTEX_BUFFER_HEADER*>(pStaticMeshData + m_pMeshHeader->VertexStreamHeadersOffset);
    m_pIndexBufferArray  = reinterpret_cast<DXSDKMESH_INDEX_BUFFER_HEADER*> (pStaticMeshData + m_pMeshHeader->IndexStreamHeadersOffset);
    m_pMeshArray         = reinterpret_cast<DXSDKMESH_MESH*>                (pStaticMeshData + m_pMeshHeader->MeshDataOffset);
    m_pSubsetArray       = reinterpret_cast<DXSDKMESH_SUBSET*>              (pStaticMeshData + m_pMeshHeader->SubsetDataOffset);
    m_pFrameArray        = reinterpret_cast<DXSDKMESH_FRAME*>               (pStaticMeshData + m_pMeshHeader->FrameDataOffset);
    m_pMaterialArray     = reinterpret_cast<DXSDKMESH_MATERIAL*>            (pStaticMeshData + m_pMeshHeader->MaterialDataOffset);
    // clang-format on

    for (Uint32 i = 0; i < m_pMeshHeader->NumMaterials; i++)
    {
        auto& Mat            = m_pMaterialArray[i];
        Mat.pDiffuseTexture  = nullptr;
        Mat.pNormalTexture   = nullptr;
        Mat.pSpecularTexture = nullptr;
        Mat.pDiffuseRV       = nullptr;
        Mat.pNormalRV        = nullptr;
        Mat.pSpecularRV      = nullptr;
    }

    // Setup subsets
    for (Uint32 i = 0; i < m_pMeshHeader->NumMeshes; i++)
    {
        m_pMeshArray[i].pSubsets         = reinterpret_cast<Uint32*>(pStaticMeshData + m_pMeshArray[i].SubsetOffset);
        m_pMeshArray[i].pFrameInfluences = reinterpret_cast<Uint32*>(pStaticMeshData + m_pMeshArray[i].FrameInfluenceOffset);
    }

    // error condition
    if (m_pMeshHeader->Version != DXSDKMESH_FILE_VERSION)
    {
        LOG_ERROR("Unexpected SDK mesh file version");
        return false;
    }

    // Setup buffer data pointer
    Uint8* pBufferData = m_StaticMeshData.data() + m_pMeshHeader->HeaderSize + m_pMeshHeader->NonBufferDataSize;

    // Get the start of the buffer data
    Uint64 BufferDataStart = m_pMeshHeader->HeaderSize + m_pMeshHeader->NonBufferDataSize;

    // Create VBs
    m_ppVertices.resize(m_pMeshHeader->NumVertexBuffers);
    for (Uint32 i = 0; i < m_pMeshHeader->NumVertexBuffers; i++)
    {
        m_ppVertices[i] = reinterpret_cast<Uint8*>(pBufferData + (m_pVertexBufferArray[i].DataOffset - BufferDataStart));
    }

    // Create IBs
    m_ppIndices.resize(m_pMeshHeader->NumIndexBuffers);
    for (Uint32 i = 0; i < m_pMeshHeader->NumIndexBuffers; i++)
    {
        m_ppIndices[i] = reinterpret_cast<Uint8*>(pBufferData + (m_pIndexBufferArray[i].DataOffset - BufferDataStart));
    }

    ComputeBoundingBoxes();

    return true;
}

static void LoadTexture(IRenderDevice*                    pDevice,
                        const Char*                       ResourceDirectory,
                        const Char*                       Name,
                        bool                              IsSRGB,
                        ITexture**                        ppTexture,
                        ITextureView**                    ppSRV,
                        std::vector<StateTransitionDesc>& Barriers)
{
    std::string FullPath = ResourceDirectory;
    if (!FullPath.empty() && FullPath.back() != FileSystem::GetSlashSymbol())
        FullPath.push_back(FileSystem::GetSlashSymbol());
    FullPath.append(Name);
    if (FileSystem::FileExists(FullPath.c_str()))
    {
        TextureLoadInfo LoadInfo;
        LoadInfo.IsSRGB = IsSRGB;
        CreateTextureFromFile(FullPath.c_str(), LoadInfo, pDevice, ppTexture);
        if (*ppTexture != nullptr)
        {
            *ppSRV = (*ppTexture)->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
            (*ppSRV)->AddRef();
        }
        else
        {
            LOG_ERROR("Failed to load texture ", Name);
        }
        Barriers.emplace_back(*ppTexture, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_SHADER_RESOURCE, true);
    }
}

void DXSDKMesh::LoadGPUResources(const Char* ResourceDirectory, IRenderDevice* pDevice, IDeviceContext* pDeviceCtx)
{
    std::vector<StateTransitionDesc> Barriers;
    for (Uint32 i = 0; i < m_pMeshHeader->NumMaterials; i++)
    {
        auto& Mat = m_pMaterialArray[i];
        if (Mat.DiffuseTexture[0] != 0)
        {
            LoadTexture(pDevice, ResourceDirectory, Mat.DiffuseTexture, true, &Mat.pDiffuseTexture, &Mat.pDiffuseRV, Barriers);
        }

        if (Mat.NormalTexture[0] != 0)
        {
            LoadTexture(pDevice, ResourceDirectory, Mat.NormalTexture, false, &Mat.pNormalTexture, &Mat.pNormalRV, Barriers);
        }

        if (Mat.SpecularTexture[0] != 0)
        {
            LoadTexture(pDevice, ResourceDirectory, Mat.SpecularTexture, false, &Mat.pSpecularTexture, &Mat.pSpecularRV, Barriers);
        }
    }

    m_VertexBuffers.resize(m_pMeshHeader->NumVertexBuffers);
    for (Uint32 i = 0; i < m_pMeshHeader->NumVertexBuffers; i++)
    {
        const auto& VBArr = m_pVertexBufferArray[i];

        std::stringstream ss;
        ss << "DXSDK Mesh vertex buffer #" << i;
        std::string VBName = ss.str();
        BufferDesc  VBDesc;
        VBDesc.Name          = VBName.c_str();
        VBDesc.Usage         = USAGE_IMMUTABLE;
        VBDesc.uiSizeInBytes = static_cast<Uint32>(VBArr.NumVertices * VBArr.StrideUint8s);
        VBDesc.BindFlags     = BIND_VERTEX_BUFFER;

        BufferData InitData{GetRawVerticesAt(i), static_cast<Uint32>(VBArr.SizeUint8s)};
        pDevice->CreateBuffer(VBDesc, &InitData, &m_VertexBuffers[i]);

        Barriers.emplace_back(m_VertexBuffers[i], RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_VERTEX_BUFFER, true);
    }

    // Create IBs
    m_IndexBuffers.resize(m_pMeshHeader->NumIndexBuffers);
    for (Uint32 i = 0; i < m_pMeshHeader->NumIndexBuffers; i++)
    {
        const auto& IBArr = m_pIndexBufferArray[i];

        std::stringstream ss;
        ss << "DXSDK Mesh index buffer #" << i;
        std::string IBName = ss.str();

        BufferDesc IBDesc;
        IBDesc.Name          = IBName.c_str();
        IBDesc.Usage         = USAGE_IMMUTABLE;
        IBDesc.uiSizeInBytes = static_cast<Uint32>(IBArr.NumIndices * (IBArr.IndexType == IT_16BIT ? 2 : 4));
        IBDesc.BindFlags     = BIND_INDEX_BUFFER;

        BufferData InitData{GetRawIndicesAt(i), static_cast<Uint32>(IBArr.SizeUint8s)};
        pDevice->CreateBuffer(IBDesc, &InitData, &m_IndexBuffers[i]);

        Barriers.emplace_back(m_IndexBuffers[i], RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_INDEX_BUFFER, true);
    }

    pDeviceCtx->TransitionResourceStates(static_cast<Uint32>(Barriers.size()), Barriers.data());
}

//--------------------------------------------------------------------------------------
DXSDKMesh::~DXSDKMesh()
{
    Destroy();
}

//--------------------------------------------------------------------------------------
bool DXSDKMesh::Create(const Char* szFileName)
{
    return CreateFromFile(szFileName);
}

//--------------------------------------------------------------------------------------
bool DXSDKMesh::Create(Uint8* pData, Uint32 DataUint8s)
{
    return CreateFromMemory(pData, DataUint8s);
}

//--------------------------------------------------------------------------------------
void DXSDKMesh::Destroy()
{
    for (Uint32 i = 0; i < m_pMeshHeader->NumMaterials; i++)
    {
        auto& Mat = m_pMaterialArray[i];
        if (Mat.pDiffuseTexture)
            Mat.pDiffuseTexture->Release();

        if (Mat.pNormalTexture)
            Mat.pNormalTexture->Release();

        if (Mat.pSpecularTexture)
            Mat.pSpecularTexture->Release();

        if (Mat.pDiffuseRV)
            Mat.pDiffuseRV->Release();

        if (Mat.pNormalRV)
            Mat.pNormalRV->Release();

        if (Mat.pSpecularRV)
            Mat.pSpecularRV->Release();
    }

    m_VertexBuffers.clear();
    m_IndexBuffers.clear();

    m_StaticMeshData.clear();

    //delete[] m_pAdjacencyIndexBufferArray; m_pAdjacencyIndexBufferArray = nullptr;

    //delete[] m_pAnimationData;              m_pAnimationData            = nullptr;
    //delete[] m_pBindPoseFrameMatrices;      m_pBindPoseFrameMatrices    = nullptr;
    //delete[] m_pTransformedFrameMatrices;   m_pTransformedFrameMatrices = nullptr;
    //delete[] m_pWorldPoseFrameMatrices;     m_pWorldPoseFrameMatrices   = nullptr;

    m_ppVertices.clear();
    m_ppIndices.clear();

    m_pMeshHeader        = nullptr;
    m_pVertexBufferArray = nullptr;
    m_pIndexBufferArray  = nullptr;
    m_pMeshArray         = nullptr;
    m_pSubsetArray       = nullptr;
    m_pFrameArray        = nullptr;
    m_pMaterialArray     = nullptr;

    //m_pAnimationHeader    = nullptr;
    //m_pAnimationFrameData = nullptr;
}


//--------------------------------------------------------------------------------------
PRIMITIVE_TOPOLOGY DXSDKMesh::GetPrimitiveType(DXSDKMESH_PRIMITIVE_TYPE PrimType)
{
    switch (PrimType)
    {
        case PT_TRIANGLE_LIST:
            return PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        case PT_TRIANGLE_STRIP:
            return PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

        case PT_LINE_LIST:
            return PRIMITIVE_TOPOLOGY_LINE_LIST;

        case PT_LINE_STRIP:
            return PRIMITIVE_TOPOLOGY_LINE_STRIP;

        case PT_POINT_LIST:
            return PRIMITIVE_TOPOLOGY_POINT_LIST;

        case PT_TRIANGLE_LIST_ADJ:
            UNEXPECTED("Unsupported primitive topolgy type");
            return PRIMITIVE_TOPOLOGY_UNDEFINED; // PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ;

        case PT_TRIANGLE_STRIP_ADJ:
            UNEXPECTED("Unsupported primitive topolgy type");
            return PRIMITIVE_TOPOLOGY_UNDEFINED; // PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ;

        case PT_LINE_LIST_ADJ:
            UNEXPECTED("Unsupported primitive topolgy type");
            return PRIMITIVE_TOPOLOGY_UNDEFINED; // PRIMITIVE_TOPOLOGY_LINELIST_ADJ;

        case PT_LINE_STRIP_ADJ:
            UNEXPECTED("Unsupported primitive topolgy type");
            return PRIMITIVE_TOPOLOGY_UNDEFINED; // D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ;

        default:
            UNEXPECTED("Unknown primitive topolgy type");
            return PRIMITIVE_TOPOLOGY_UNDEFINED;
    }
}

//--------------------------------------------------------------------------------------
VALUE_TYPE DXSDKMesh::GetIBFormat(Uint32 iMesh) const
{
    switch (m_pIndexBufferArray[m_pMeshArray[iMesh].IndexBuffer].IndexType)
    {
        case IT_16BIT:
            return VT_UINT16;
        case IT_32BIT:
            return VT_UINT32;
        default:
            UNEXPECTED("Unexpected IB format");
            return VT_UINT32;
    }
}

} // namespace Diligent
