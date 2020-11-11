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

#include <vector>
#include <memory>
#include <cmath>

#include "GLTFLoader.hpp"
#include "MapHelper.hpp"
#include "CommonlyUsedStates.h"
#include "DataBlobImpl.hpp"
#include "Image.h"
#include "FileSystem.hpp"
#include "FileWrapper.hpp"
#include "GraphicsAccessories.hpp"
#include "TextureLoader.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE

#include "../../ThirdParty/tinygltf/tiny_gltf.h"

namespace Diligent
{


namespace GLTF
{

RefCntAutoPtr<ITexture> TextureFromGLTFImage(IRenderDevice*         pDevice,
                                             IDeviceContext*        pCtx,
                                             const tinygltf::Image& gltfimage,
                                             ISampler*              pSampler,
                                             float                  AlphaCutoff)
{
    if (gltfimage.image.empty())
    {
        LOG_ERROR_AND_THROW("Failed to create texture for image ", gltfimage.uri, ": no data available.");
    }
    if (gltfimage.width <= 0 || gltfimage.height <= 0 || gltfimage.component <= 0)
    {
        LOG_ERROR_AND_THROW("Failed to create texture for image ", gltfimage.uri, ": invalid parameters.");
    }

    std::vector<Uint8> RGBA;

    const Uint8* pTextureData = nullptr;
    if (gltfimage.component == 3)
    {
        RGBA.resize(gltfimage.width * gltfimage.height * 4);
        // Due to depressing performance of iterators in debug MSVC we have to use raw pointers here
        const auto* rgb  = gltfimage.image.data();
        auto*       rgba = RGBA.data();
        for (int i = 0; i < gltfimage.width * gltfimage.height; ++i)
        {
            rgba[0] = rgb[0];
            rgba[1] = rgb[1];
            rgba[2] = rgb[2];
            rgba[3] = 255;

            rgba += 4;
            rgb += 3;
        }
        VERIFY_EXPR(rgb == gltfimage.image.data() + gltfimage.image.size());
        VERIFY_EXPR(rgba == RGBA.data() + RGBA.size());

        pTextureData = RGBA.data();
    }
    else if (gltfimage.component == 4)
    {
        pTextureData = gltfimage.image.data();
        if (AlphaCutoff > 0)
        {
            // Remap alpha channel using the following formula to improve mip maps:
            //
            //      A_new = max(A_old; 1/3 * A_old + 2/3 * CutoffThreshold)
            //
            // https://asawicki.info/articles/alpha_test.php5

            VERIFY_EXPR(AlphaCutoff > 0 && AlphaCutoff <= 1);
            AlphaCutoff *= 255.f;

            RGBA.resize(gltfimage.width * gltfimage.height * 4);
            // Due to depressing performance of iterators in debug MSVC we have to use raw pointers here
            const auto* src = gltfimage.image.data();
            auto*       dst = RGBA.data();
            for (int i = 0; i < gltfimage.width * gltfimage.height; ++i)
            {
                dst[0] = src[0];
                dst[1] = src[1];
                dst[2] = src[2];
                dst[3] = std::max(src[3], static_cast<Uint8>(std::min(1.f / 3.f * src[3] + 2.f / 3.f * AlphaCutoff, 255.f)));

                src += 4;
                dst += 4;
            }
            VERIFY_EXPR(src == gltfimage.image.data() + gltfimage.image.size());
            VERIFY_EXPR(dst == RGBA.data() + RGBA.size());

            pTextureData = RGBA.data();
        }
    }
    else
    {
        UNEXPECTED("Unexpected number of color components in gltf image: ", gltfimage.component);
    }

    TextureDesc TexDesc;
    TexDesc.Name      = "GLTF Texture";
    TexDesc.Type      = RESOURCE_DIM_TEX_2D;
    TexDesc.Usage     = USAGE_DEFAULT;
    TexDesc.BindFlags = BIND_SHADER_RESOURCE;
    TexDesc.Width     = gltfimage.width;
    TexDesc.Height    = gltfimage.height;
    TexDesc.Format    = TEX_FORMAT_RGBA8_UNORM;
    TexDesc.MipLevels = 0;
    TexDesc.MiscFlags = MISC_TEXTURE_FLAG_GENERATE_MIPS;
    RefCntAutoPtr<ITexture> pTexture;

    pDevice->CreateTexture(TexDesc, nullptr, &pTexture);
    Box UpdateBox;
    UpdateBox.MaxX = TexDesc.Width;
    UpdateBox.MaxY = TexDesc.Height;
    TextureSubResData Level0Data(pTextureData, gltfimage.width * 4);
    pCtx->UpdateTexture(pTexture, 0, 0, UpdateBox, Level0Data, RESOURCE_STATE_TRANSITION_MODE_NONE, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    pTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE)->SetSampler(pSampler);

    return pTexture;
}



Mesh::Mesh(IRenderDevice* pDevice, const float4x4& matrix)
{
    Transforms.matrix = matrix;
}

void Mesh::SetBoundingBox(const float3& min, const float3& max)
{
    BB.Min    = min;
    BB.Max    = max;
    IsValidBB = true;
}





float4x4 Node::LocalMatrix() const
{
    return Matrix * float4x4::Scale(Scale) * Rotation.ToMatrix() * float4x4::Translation(Translation);
}

float4x4 Node::GetMatrix() const
{
    auto mat = LocalMatrix();

    for (auto* p = Parent; p != nullptr; p = p->Parent)
    {
        mat = mat * p->LocalMatrix();
    }
    return mat;
}

void Node::Update()
{
    if (_Mesh)
    {
        _Mesh->Transforms.matrix = GetMatrix();
        if (_Skin != nullptr)
        {
            // Update join matrices
            auto   InverseTransform = _Mesh->Transforms.matrix.Inverse(); // TODO: do not use inverse tranform here
            size_t numJoints        = std::min((uint32_t)_Skin->Joints.size(), Uint32{Mesh::TransformData::MaxNumJoints});
            for (size_t i = 0; i < numJoints; i++)
            {
                auto* JointNode = _Skin->Joints[i];
                auto  JointMat  = _Skin->InverseBindMatrices[i] * JointNode->GetMatrix() * InverseTransform;

                _Mesh->Transforms.jointMatrix[i] = JointMat;
            }
            _Mesh->Transforms.jointcount = static_cast<int>(numJoints);
        }
    }

    for (auto& child : Children)
    {
        child->Update();
    }
}




Model::Model(IRenderDevice*     pDevice,
             IDeviceContext*    pContext,
             const std::string& filename,
             TextureCacheType*  pTextureCache)
{
    LoadFromFile(pDevice, pContext, filename, pTextureCache);
}

void Model::LoadNode(IRenderDevice*               pDevice,
                     Node*                        parent,
                     const tinygltf::Node&        gltf_node,
                     uint32_t                     nodeIndex,
                     const tinygltf::Model&       gltf_model,
                     std::vector<uint32_t>&       indexBuffer,
                     std::vector<VertexAttribs0>& vertexData0,
                     std::vector<VertexAttribs1>& vertexData1)
{
    std::unique_ptr<Node> NewNode(new Node{});
    NewNode->Index     = nodeIndex;
    NewNode->Parent    = parent;
    NewNode->Name      = gltf_node.name;
    NewNode->SkinIndex = gltf_node.skin;
    NewNode->Matrix    = float4x4::Identity();

    // Generate local node matrix
    //float3 Translation;
    if (gltf_node.translation.size() == 3)
    {
        NewNode->Translation = float3::MakeVector(gltf_node.translation.data());
    }

    if (gltf_node.rotation.size() == 4)
    {
        NewNode->Rotation.q = float4::MakeVector(gltf_node.rotation.data());
        //NewNode->rotation = glm::mat4(q);
    }

    if (gltf_node.scale.size() == 3)
    {
        NewNode->Scale = float3::MakeVector(gltf_node.scale.data());
    }

    if (gltf_node.matrix.size() == 16)
    {
        NewNode->Matrix = float4x4::MakeMatrix(gltf_node.matrix.data());
    }

    // Node with children
    if (gltf_node.children.size() > 0)
    {
        for (size_t i = 0; i < gltf_node.children.size(); i++)
        {
            LoadNode(pDevice, NewNode.get(), gltf_model.nodes[gltf_node.children[i]], gltf_node.children[i], gltf_model, indexBuffer, vertexData0, vertexData1);
        }
    }

    // Node contains mesh data
    if (gltf_node.mesh > -1)
    {
        const tinygltf::Mesh& gltf_mesh = gltf_model.meshes[gltf_node.mesh];
        std::unique_ptr<Mesh> NewMesh(new Mesh(pDevice, NewNode->Matrix));
        for (size_t j = 0; j < gltf_mesh.primitives.size(); j++)
        {
            const tinygltf::Primitive& primitive = gltf_mesh.primitives[j];

            uint32_t indexStart  = static_cast<uint32_t>(indexBuffer.size());
            uint32_t vertexStart = static_cast<uint32_t>(vertexData0.size());
            VERIFY_EXPR(vertexData1.empty() || vertexData0.size() == vertexData1.size());

            uint32_t indexCount  = 0;
            uint32_t vertexCount = 0;
            float3   PosMin;
            float3   PosMax;
            bool     hasSkin    = false;
            bool     hasIndices = primitive.indices > -1;

            // Vertices
            {
                const float*    bufferPos          = nullptr;
                const float*    bufferNormals      = nullptr;
                const float*    bufferTexCoordSet0 = nullptr;
                const float*    bufferTexCoordSet1 = nullptr;
                const uint8_t*  bufferJoints8      = nullptr;
                const uint16_t* bufferJoints16     = nullptr;
                const float*    bufferWeights      = nullptr;

                int posStride          = -1;
                int normalsStride      = -1;
                int texCoordSet0Stride = -1;
                int texCoordSet1Stride = -1;
                int jointsStride       = -1;
                int weightsStride      = -1;

                {
                    auto position_it = primitive.attributes.find("POSITION");
                    VERIFY(position_it != primitive.attributes.end(), "Position attribute is required");

                    const tinygltf::Accessor&   posAccessor = gltf_model.accessors[position_it->second];
                    const tinygltf::BufferView& posView     = gltf_model.bufferViews[posAccessor.bufferView];
                    VERIFY(posAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT, "Position component type is expected to be float");
                    VERIFY(posAccessor.type == TINYGLTF_TYPE_VEC3, "Position type is expected to be vec3");

                    bufferPos = reinterpret_cast<const float*>(&(gltf_model.buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]));
                    PosMin =
                        float3 //
                        {
                            static_cast<float>(posAccessor.minValues[0]),
                            static_cast<float>(posAccessor.minValues[1]),
                            static_cast<float>(posAccessor.minValues[2]) //
                        };
                    PosMax =
                        float3 //
                        {
                            static_cast<float>(posAccessor.maxValues[0]),
                            static_cast<float>(posAccessor.maxValues[1]),
                            static_cast<float>(posAccessor.maxValues[2]) //
                        };
                    posStride = posAccessor.ByteStride(posView) / tinygltf::GetComponentSizeInBytes(posAccessor.componentType);
                    VERIFY(posStride > 0, "Position stride is invalid");

                    vertexCount = static_cast<uint32_t>(posAccessor.count);
                }

                if (primitive.attributes.find("NORMAL") != primitive.attributes.end())
                {
                    const tinygltf::Accessor&   normAccessor = gltf_model.accessors[primitive.attributes.find("NORMAL")->second];
                    const tinygltf::BufferView& normView     = gltf_model.bufferViews[normAccessor.bufferView];
                    VERIFY(normAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT, "Normal component type is expected to be float");
                    VERIFY(normAccessor.type == TINYGLTF_TYPE_VEC3, "Normal type is expected to be vec3");

                    bufferNormals = reinterpret_cast<const float*>(&(gltf_model.buffers[normView.buffer].data[normAccessor.byteOffset + normView.byteOffset]));
                    normalsStride = normAccessor.ByteStride(normView) / tinygltf::GetComponentSizeInBytes(normAccessor.componentType);
                    VERIFY(normalsStride > 0, "Normal stride is invalid");
                }

                if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
                {
                    const tinygltf::Accessor&   uvAccessor = gltf_model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
                    const tinygltf::BufferView& uvView     = gltf_model.bufferViews[uvAccessor.bufferView];
                    VERIFY(uvAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT, "UV0 component type is expected to be float");
                    VERIFY(uvAccessor.type == TINYGLTF_TYPE_VEC2, "UV0 type is expected to be vec2");

                    bufferTexCoordSet0 = reinterpret_cast<const float*>(&(gltf_model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
                    texCoordSet0Stride = uvAccessor.ByteStride(uvView) / tinygltf::GetComponentSizeInBytes(uvAccessor.componentType);
                    VERIFY(texCoordSet0Stride > 0, "Texcoord0 stride is invalid");
                }
                if (primitive.attributes.find("TEXCOORD_1") != primitive.attributes.end())
                {
                    const tinygltf::Accessor&   uvAccessor = gltf_model.accessors[primitive.attributes.find("TEXCOORD_1")->second];
                    const tinygltf::BufferView& uvView     = gltf_model.bufferViews[uvAccessor.bufferView];
                    VERIFY(uvAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT, "UV1 component type is expected to be float");
                    VERIFY(uvAccessor.type == TINYGLTF_TYPE_VEC2, "UV1 type is expected to be vec2");

                    bufferTexCoordSet1 = reinterpret_cast<const float*>(&(gltf_model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
                    texCoordSet1Stride = uvAccessor.ByteStride(uvView) / tinygltf::GetComponentSizeInBytes(uvAccessor.componentType);
                    VERIFY(texCoordSet1Stride > 0, "Texcoord1 stride is invalid");
                }

                // Skinning
                // Joints
                if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end())
                {
                    const tinygltf::Accessor&   jointAccessor = gltf_model.accessors[primitive.attributes.find("JOINTS_0")->second];
                    const tinygltf::BufferView& jointView     = gltf_model.bufferViews[jointAccessor.bufferView];
                    VERIFY(jointAccessor.type == TINYGLTF_TYPE_VEC4, "Joint type is expected to be vec4");

                    const auto* bufferJoints = &(gltf_model.buffers[jointView.buffer].data[jointAccessor.byteOffset + jointView.byteOffset]);
                    switch (jointAccessor.componentType)
                    {
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                            bufferJoints16 = reinterpret_cast<const uint16_t*>(bufferJoints);
                            break;

                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                            bufferJoints8 = reinterpret_cast<const uint8_t*>(bufferJoints);
                            break;

                        default:
                            UNEXPECTED("Joint component type is expected to be unsigned short or byte");
                    }

                    jointsStride = jointAccessor.ByteStride(jointView) / tinygltf::GetComponentSizeInBytes(jointAccessor.componentType);
                    VERIFY(jointsStride > 0, "Joints stride is invalid");
                }

                if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end())
                {
                    const tinygltf::Accessor&   weightsAccessor = gltf_model.accessors[primitive.attributes.find("WEIGHTS_0")->second];
                    const tinygltf::BufferView& weightsView     = gltf_model.bufferViews[weightsAccessor.bufferView];
                    VERIFY(weightsAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT, "Weights component type is expected to be float");
                    VERIFY(weightsAccessor.type == TINYGLTF_TYPE_VEC4, "Weights type is expected to be vec4");

                    bufferWeights = reinterpret_cast<const float*>(&(gltf_model.buffers[weightsView.buffer].data[weightsAccessor.byteOffset + weightsView.byteOffset]));
                    weightsStride = weightsAccessor.ByteStride(weightsView) / tinygltf::GetComponentSizeInBytes(weightsAccessor.componentType);
                    VERIFY(weightsStride > 0, "Weights stride is invalid");
                }

                hasSkin = bufferWeights != nullptr && (bufferJoints8 != nullptr || bufferJoints16 != nullptr);

                for (uint32_t v = 0; v < vertexCount; v++)
                {
                    VertexAttribs0 vert0{};
                    vert0.pos = float4(float3::MakeVector(bufferPos + v * posStride), 1.0f);
                    // clang-format off
                    vert0.normal = bufferNormals      != nullptr ? normalize(float3::MakeVector(bufferNormals + v * normalsStride)) : float3{};
                    vert0.uv0    = bufferTexCoordSet0 != nullptr ? float2::MakeVector(bufferTexCoordSet0 + v * texCoordSet0Stride)  : float2{};
                    vert0.uv1    = bufferTexCoordSet1 != nullptr ? float2::MakeVector(bufferTexCoordSet1 + v * texCoordSet1Stride)  : float2{};
                    // clang-format on
                    vertexData0.push_back(vert0);

                    VertexAttribs1 vert1{};
                    if (hasSkin)
                    {
                        vert1.joint0 = bufferJoints8 != nullptr ?
                            float4::MakeVector(bufferJoints8 + v * jointsStride) :
                            float4::MakeVector(bufferJoints16 + v * jointsStride);
                        vert1.weight0 = float4::MakeVector(bufferWeights + v * weightsStride);
                    }
                    vertexData1.push_back(vert1);
                }
            }

            // Indices
            if (hasIndices)
            {
                const tinygltf::Accessor&   accessor   = gltf_model.accessors[primitive.indices > -1 ? primitive.indices : 0];
                const tinygltf::BufferView& bufferView = gltf_model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer&     buffer     = gltf_model.buffers[bufferView.buffer];

                indexCount = static_cast<uint32_t>(accessor.count);

                const void* dataPtr = &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

                switch (accessor.componentType)
                {
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
                    {
                        const uint32_t* buf = static_cast<const uint32_t*>(dataPtr);
                        for (size_t index = 0; index < accessor.count; index++)
                        {
                            indexBuffer.push_back(buf[index] + vertexStart);
                        }
                        break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
                    {
                        const uint16_t* buf = static_cast<const uint16_t*>(dataPtr);
                        for (size_t index = 0; index < accessor.count; index++)
                        {
                            indexBuffer.push_back(buf[index] + vertexStart);
                        }
                        break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
                    {
                        const uint8_t* buf = static_cast<const uint8_t*>(dataPtr);
                        for (size_t index = 0; index < accessor.count; index++)
                        {
                            indexBuffer.push_back(buf[index] + vertexStart);
                        }
                        break;
                    }
                    default:
                        std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
                        return;
                }
            }
            std::unique_ptr<Primitive> newPrimitive(
                new Primitive //
                {
                    indexStart,
                    indexCount,
                    vertexCount,
                    primitive.material > -1 ? Materials[primitive.material] : Materials.back() //
                }                                                                              //
            );

            newPrimitive->SetBoundingBox(PosMin, PosMax);
            NewMesh->Primitives.push_back(std::move(newPrimitive));
        }

        // Mesh BB from BBs of primitives
        for (const auto& prim : NewMesh->Primitives)
        {
            if (prim->IsValidBB && !NewMesh->IsValidBB)
            {
                NewMesh->BB        = prim->BB;
                NewMesh->IsValidBB = true;
            }
            float3 bb_min = std::min(NewMesh->BB.Min, prim->BB.Min);
            float3 bb_max = std::max(NewMesh->BB.Max, prim->BB.Max);
            NewMesh->SetBoundingBox(bb_min, bb_max);
        }
        NewNode->_Mesh = std::move(NewMesh);
    }

    LinearNodes.push_back(NewNode.get());
    if (parent)
    {
        parent->Children.push_back(std::move(NewNode));
    }
    else
    {
        Nodes.push_back(std::move(NewNode));
    }
}


void Model::LoadSkins(const tinygltf::Model& gltf_model)
{
    for (const auto& source : gltf_model.skins)
    {
        std::unique_ptr<Skin> NewSkin(new Skin{});
        NewSkin->Name = source.name;

        // Find skeleton root node
        if (source.skeleton > -1)
        {
            NewSkin->pSkeletonRoot = NodeFromIndex(source.skeleton);
        }

        // Find joint nodes
        for (int jointIndex : source.joints)
        {
            Node* node = NodeFromIndex(jointIndex);
            if (node)
            {
                NewSkin->Joints.push_back(NodeFromIndex(jointIndex));
            }
        }

        // Get inverse bind matrices from buffer
        if (source.inverseBindMatrices > -1)
        {
            const tinygltf::Accessor&   accessor   = gltf_model.accessors[source.inverseBindMatrices];
            const tinygltf::BufferView& bufferView = gltf_model.bufferViews[accessor.bufferView];
            const tinygltf::Buffer&     buffer     = gltf_model.buffers[bufferView.buffer];
            NewSkin->InverseBindMatrices.resize(accessor.count);
            memcpy(NewSkin->InverseBindMatrices.data(), &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(float4x4));
        }

        Skins.push_back(std::move(NewSkin));
    }
}

static float GetTextureAlphaCutoffValue(const tinygltf::Model& gltf_model, int TextureIndex)
{
    float AlphaCutoff = -1.f;

    for (const auto& gltf_mat : gltf_model.materials)
    {
        auto base_color_tex_it = gltf_mat.values.find("baseColorTexture");
        if (base_color_tex_it == gltf_mat.values.end())
        {
            // The material has no base texture
            continue;
        }

        if (base_color_tex_it->second.TextureIndex() != TextureIndex)
        {
            // The material does not use this texture
            continue;
        }

        auto alpha_mode_it = gltf_mat.additionalValues.find("alphaMode");
        if (alpha_mode_it == gltf_mat.additionalValues.end())
        {
            // The material uses this texture, but it is not an alpha-blended or an alpha-cut material
            AlphaCutoff = 0.f;
            continue;
        }

        const tinygltf::Parameter& param = alpha_mode_it->second;
        if (param.string_value == "MASK")
        {
            auto MaterialAlphaCutoff = 0.5f;
            auto alpha_cutoff_it     = gltf_mat.additionalValues.find("alphaCutoff");
            if (alpha_cutoff_it != gltf_mat.additionalValues.end())
            {
                MaterialAlphaCutoff = static_cast<float>(alpha_cutoff_it->second.Factor());
            }

            if (AlphaCutoff < 0)
            {
                AlphaCutoff = MaterialAlphaCutoff;
            }
            else if (AlphaCutoff != MaterialAlphaCutoff)
            {
                if (AlphaCutoff == 0)
                {
                    LOG_WARNING_MESSAGE("Texture ", TextureIndex,
                                        " is used in an alpha-cut material with threshold ", MaterialAlphaCutoff,
                                        " as well as in a non-alpha-cut material."
                                        " Alpha remapping to improve mipmap generation will be disabled.");
                }
                else
                {
                    LOG_WARNING_MESSAGE("Texture ", TextureIndex,
                                        " is used in alpha-cut materials with different cutoff thresholds (", AlphaCutoff, ", ", MaterialAlphaCutoff,
                                        "). Alpha remapping to improve mipmap generation will use ",
                                        AlphaCutoff, '.');
                }
            }
        }
        else
        {
            // The material is not an alpha-cut material
            if (AlphaCutoff > 0)
            {
                LOG_WARNING_MESSAGE("Texture ", TextureIndex,
                                    " is used in an alpha-cut material as well as in a non-alpha-cut material."
                                    " Alpha remapping to improve mipmap generation will be disabled.");
            }
            AlphaCutoff = 0.f;
        }
    }

    return std::max(AlphaCutoff, 0.f);
}

void Model::LoadTextures(IRenderDevice*         pDevice,
                         IDeviceContext*        pCtx,
                         const tinygltf::Model& gltf_model,
                         const std::string&     BaseDir,
                         TextureCacheType*      pTextureCache)
{
    std::vector<ITexture*> NewTextures;
    for (const tinygltf::Texture& gltf_tex : gltf_model.textures)
    {
        const tinygltf::Image& gltf_image = gltf_model.images[gltf_tex.source];

        RefCntAutoPtr<ITexture> pTexture;
        if (pTextureCache != nullptr)
        {
            auto it = pTextureCache->find(BaseDir + gltf_image.uri);
            if (it != pTextureCache->end())
            {
                pTexture = it->second.Lock();
                if (!pTexture)
                {
                    // Image width and height (or pixel_type for dds/ktx) are initialized by LoadImageData()
                    // if the texture is found in the cache.
                    if ((gltf_image.width > 0 && gltf_image.height > 0) ||
                        (gltf_image.pixel_type == IMAGE_FILE_FORMAT_DDS || gltf_image.pixel_type == IMAGE_FILE_FORMAT_KTX))
                    {
                        UNEXPECTED("Stale textures should not be found in the texture cache because we hold strong references. "
                                   "This must be an unexpected effect of loading resources from multiple threads or a bug.");
                    }
                    else
                    {
                        pTextureCache->erase(it);
                    }
                }
            }
        }

        if (!pTexture)
        {
            RefCntAutoPtr<ISampler> pSampler;
            if (gltf_tex.sampler == -1)
            {
                // No sampler specified, use a default one
                pDevice->CreateSampler(Sam_LinearWrap, &pSampler);
            }
            else
            {
                pSampler = TextureSamplers[gltf_tex.sampler];
            }

            // Check if the texture is used in an alpha-cut material
            float AlphaCutoff = GetTextureAlphaCutoffValue(gltf_model, static_cast<int>(Textures.size()));

            if (gltf_image.width > 0 && gltf_image.height > 0)
            {
                pTexture = TextureFromGLTFImage(pDevice, pCtx, gltf_image, pSampler, AlphaCutoff);
                pCtx->GenerateMips(pTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
            }
            else if (gltf_image.pixel_type == IMAGE_FILE_FORMAT_DDS || gltf_image.pixel_type == IMAGE_FILE_FORMAT_KTX)
            {
                // Create the texture from raw bits
                RefCntAutoPtr<DataBlobImpl> pRawData(MakeNewRCObj<DataBlobImpl>()(gltf_image.image.size()));
                memcpy(pRawData->GetDataPtr(), gltf_image.image.data(), gltf_image.image.size());
                switch (gltf_image.pixel_type)
                {
                    case IMAGE_FILE_FORMAT_DDS:
                        CreateTextureFromDDS(pRawData, TextureLoadInfo{}, pDevice, &pTexture);
                        break;

                    case IMAGE_FILE_FORMAT_KTX:
                        CreateTextureFromKTX(pRawData, TextureLoadInfo{}, pDevice, &pTexture);
                        break;

                    default:
                        UNEXPECTED("Unknown raw image format");
                }
            }

            VERIFY_EXPR(pTexture);
            NewTextures.emplace_back(pTexture);

            if (pTextureCache != nullptr)
            {
                pTextureCache->emplace(BaseDir + gltf_image.uri, pTexture);
            }
        }

        Textures.push_back(std::move(pTexture));
    }

    if (!NewTextures.empty())
    {
        std::vector<StateTransitionDesc> Barriers;
        Barriers.reserve(NewTextures.size());
        for (auto& Tex : NewTextures)
        {
            if (Tex)
            {
                StateTransitionDesc Barrier{Tex, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_SHADER_RESOURCE};
                Barrier.UpdateResourceState = true;
                Barriers.emplace_back(Barrier);
            }
        }
        if (!Barriers.empty())
            pCtx->TransitionResourceStates(static_cast<Uint32>(Barriers.size()), Barriers.data());
    }
}

namespace
{

TEXTURE_ADDRESS_MODE GetWrapMode(int32_t wrapMode)
{
    switch (wrapMode)
    {
        case 10497:
            return TEXTURE_ADDRESS_WRAP;
        case 33071:
            return TEXTURE_ADDRESS_CLAMP;
        case 33648:
            return TEXTURE_ADDRESS_MIRROR;
        default:
            LOG_WARNING_MESSAGE("Unknown gltf address wrap mode: ", wrapMode, ". Defaulting to WRAP.");
            return TEXTURE_ADDRESS_WRAP;
    }
}

std::pair<FILTER_TYPE, FILTER_TYPE> GetFilterMode(int32_t filterMode)
{
    switch (filterMode)
    {
        case 9728: // NEAREST
            return {FILTER_TYPE_POINT, FILTER_TYPE_POINT};
        case 9729: // LINEAR
            return {FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR};
        case 9984: // NEAREST_MIPMAP_NEAREST
            return {FILTER_TYPE_POINT, FILTER_TYPE_POINT};
        case 9985: // LINEAR_MIPMAP_NEAREST
            return {FILTER_TYPE_LINEAR, FILTER_TYPE_POINT};
        case 9986:                                           // NEAREST_MIPMAP_LINEAR
            return {FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR}; // use linear min filter instead as point makes no sesne
        case 9987:                                           // LINEAR_MIPMAP_LINEAR
            return {FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR};
        default:
            LOG_WARNING_MESSAGE("Unknown gltf filter mode: ", filterMode, ". Defaulting to linear.");
            return {FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR};
    }
}

} // namespace

void Model::LoadTextureSamplers(IRenderDevice* pDevice, const tinygltf::Model& gltf_model)
{
    for (const tinygltf::Sampler& smpl : gltf_model.samplers)
    {
        SamplerDesc SamDesc;
        SamDesc.MagFilter = GetFilterMode(smpl.magFilter).first;
        auto MinMipFilter = GetFilterMode(smpl.minFilter);
        SamDesc.MinFilter = MinMipFilter.first;
        SamDesc.MipFilter = MinMipFilter.second;
        SamDesc.AddressU  = GetWrapMode(smpl.wrapS);
        SamDesc.AddressV  = GetWrapMode(smpl.wrapT);
        SamDesc.AddressW  = SamDesc.AddressV;
        RefCntAutoPtr<ISampler> pSampler;
        pDevice->CreateSampler(SamDesc, &pSampler);
        TextureSamplers.push_back(std::move(pSampler));
    }
}


void Model::LoadMaterials(const tinygltf::Model& gltf_model)
{
    for (const tinygltf::Material& gltf_mat : gltf_model.materials)
    {
        Material Mat;

        {
            auto base_color_tex_it = gltf_mat.values.find("baseColorTexture");
            if (base_color_tex_it != gltf_mat.values.end())
            {
                Mat.pBaseColorTexture      = Textures[base_color_tex_it->second.TextureIndex()];
                Mat.TexCoordSets.BaseColor = static_cast<Uint8>(base_color_tex_it->second.TextureTexCoord());
            }
        }

        {
            auto metal_rough_tex_it = gltf_mat.values.find("metallicRoughnessTexture");
            if (metal_rough_tex_it != gltf_mat.values.end())
            {
                Mat.pMetallicRoughnessTexture      = Textures[metal_rough_tex_it->second.TextureIndex()];
                Mat.TexCoordSets.MetallicRoughness = static_cast<Uint8>(metal_rough_tex_it->second.TextureTexCoord());
            }
        }

        {
            auto rough_factor_it = gltf_mat.values.find("roughnessFactor");
            if (rough_factor_it != gltf_mat.values.end())
            {
                Mat.RoughnessFactor = static_cast<float>(rough_factor_it->second.Factor());
            }
        }

        {
            auto metal_factor_it = gltf_mat.values.find("metallicFactor");
            if (metal_factor_it != gltf_mat.values.end())
            {
                Mat.MetallicFactor = static_cast<float>(metal_factor_it->second.Factor());
            }
        }

        {
            auto base_col_factor_it = gltf_mat.values.find("baseColorFactor");
            if (base_col_factor_it != gltf_mat.values.end())
            {
                Mat.BaseColorFactor = float4::MakeVector(base_col_factor_it->second.ColorFactor().data());
            }
        }

        {
            auto normal_tex_it = gltf_mat.additionalValues.find("normalTexture");
            if (normal_tex_it != gltf_mat.additionalValues.end())
            {
                Mat.pNormalTexture      = Textures[normal_tex_it->second.TextureIndex()];
                Mat.TexCoordSets.Normal = static_cast<Uint8>(normal_tex_it->second.TextureTexCoord());
            }
        }

        {
            auto emssive_tex_it = gltf_mat.additionalValues.find("emissiveTexture");
            if (emssive_tex_it != gltf_mat.additionalValues.end())
            {
                Mat.pEmissiveTexture      = Textures[emssive_tex_it->second.TextureIndex()];
                Mat.TexCoordSets.Emissive = static_cast<Uint8>(emssive_tex_it->second.TextureTexCoord());
            }
        }

        {
            auto occlusion_tex_it = gltf_mat.additionalValues.find("occlusionTexture");
            if (occlusion_tex_it != gltf_mat.additionalValues.end())
            {
                Mat.pOcclusionTexture      = Textures[occlusion_tex_it->second.TextureIndex()];
                Mat.TexCoordSets.Occlusion = static_cast<Uint8>(occlusion_tex_it->second.TextureTexCoord());
            }
        }

        {
            auto alpha_mode_it = gltf_mat.additionalValues.find("alphaMode");
            if (alpha_mode_it != gltf_mat.additionalValues.end())
            {
                const tinygltf::Parameter& param = alpha_mode_it->second;
                if (param.string_value == "BLEND")
                {
                    Mat.AlphaMode = Material::ALPHAMODE_BLEND;
                }
                if (param.string_value == "MASK")
                {
                    Mat.AlphaCutoff = 0.5f;
                    Mat.AlphaMode   = Material::ALPHAMODE_MASK;
                }
            }
        }

        {
            auto alpha_cutoff_it = gltf_mat.additionalValues.find("alphaCutoff");
            if (alpha_cutoff_it != gltf_mat.additionalValues.end())
            {
                Mat.AlphaCutoff = static_cast<float>(alpha_cutoff_it->second.Factor());
            }
        }

        {
            auto emissive_fctr_it = gltf_mat.additionalValues.find("emissiveFactor");
            if (emissive_fctr_it != gltf_mat.additionalValues.end())
            {
                Mat.EmissiveFactor = float4(float3::MakeVector(emissive_fctr_it->second.ColorFactor().data()), 1.0);
                //Mat.EmissiveFactor = float4(0.0f);
            }
        }

        {
            auto double_sided_it = gltf_mat.additionalValues.find("doubleSided");
            if (double_sided_it != gltf_mat.additionalValues.end())
            {
                Mat.DoubleSided = double_sided_it->second.bool_value;
            }
        }

        // Extensions
        // @TODO: Find out if there is a nicer way of reading these properties with recent tinygltf headers
        {
            auto ext_it = gltf_mat.extensions.find("KHR_materials_pbrSpecularGlossiness");
            if (ext_it != gltf_mat.extensions.end())
            {
                if (ext_it->second.Has("specularGlossinessTexture"))
                {
                    auto index                               = ext_it->second.Get("specularGlossinessTexture").Get("index");
                    Mat.extension.pSpecularGlossinessTexture = Textures[index.Get<int>()];
                    auto texCoordSet                         = ext_it->second.Get("specularGlossinessTexture").Get("texCoord");
                    Mat.TexCoordSets.SpecularGlossiness      = static_cast<Uint8>(texCoordSet.Get<int>());
                    Mat.workflow                             = Material::PbrWorkflow::SpecularGlossiness;
                }

                if (ext_it->second.Has("diffuseTexture"))
                {
                    auto index                    = ext_it->second.Get("diffuseTexture").Get("index");
                    Mat.extension.pDiffuseTexture = Textures[index.Get<int>()];
                }

                if (ext_it->second.Has("diffuseFactor"))
                {
                    auto factor = ext_it->second.Get("diffuseFactor");
                    for (uint32_t i = 0; i < factor.ArrayLen(); i++)
                    {
                        auto val                       = factor.Get(i);
                        Mat.extension.DiffuseFactor[i] = val.IsNumber() ? (float)val.Get<double>() : (float)val.Get<int>();
                    }
                }

                if (ext_it->second.Has("specularFactor"))
                {
                    auto factor = ext_it->second.Get("specularFactor");
                    for (uint32_t i = 0; i < factor.ArrayLen(); i++)
                    {
                        auto val                        = factor.Get(i);
                        Mat.extension.SpecularFactor[i] = val.IsNumber() ? (float)val.Get<double>() : (float)val.Get<int>();
                    }
                }
            }
        }

        Materials.push_back(Mat);
    }

    // Push a default material at the end of the list for meshes with no material assigned
    Materials.push_back(Material{});
}


void Model::LoadAnimations(const tinygltf::Model& gltf_model)
{
    for (const tinygltf::Animation& gltf_anim : gltf_model.animations)
    {
        Animation animation{};
        animation.Name = gltf_anim.name;
        if (gltf_anim.name.empty())
        {
            animation.Name = std::to_string(Animations.size());
        }

        // Samplers
        for (auto& samp : gltf_anim.samplers)
        {
            AnimationSampler AnimSampler{};

            if (samp.interpolation == "LINEAR")
            {
                AnimSampler.Interpolation = AnimationSampler::INTERPOLATION_TYPE::LINEAR;
            }
            else if (samp.interpolation == "STEP")
            {
                AnimSampler.Interpolation = AnimationSampler::INTERPOLATION_TYPE::STEP;
            }
            else if (samp.interpolation == "CUBICSPLINE")
            {
                AnimSampler.Interpolation = AnimationSampler::INTERPOLATION_TYPE::CUBICSPLINE;
            }

            // Read sampler input time values
            {
                const tinygltf::Accessor&   accessor   = gltf_model.accessors[samp.input];
                const tinygltf::BufferView& bufferView = gltf_model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer&     buffer     = gltf_model.buffers[bufferView.buffer];

                VERIFY_EXPR(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

                const void*  dataPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];
                const float* buf     = static_cast<const float*>(dataPtr);
                for (size_t index = 0; index < accessor.count; index++)
                {
                    AnimSampler.Inputs.push_back(buf[index]);
                }

                for (auto input : AnimSampler.Inputs)
                {
                    if (input < animation.Start)
                    {
                        animation.Start = input;
                    }
                    if (input > animation.End)
                    {
                        animation.End = input;
                    }
                }
            }

            // Read sampler output T/R/S values
            {
                const tinygltf::Accessor&   accessor   = gltf_model.accessors[samp.output];
                const tinygltf::BufferView& bufferView = gltf_model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer&     buffer     = gltf_model.buffers[bufferView.buffer];

                VERIFY_EXPR(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

                const void* dataPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];

                switch (accessor.type)
                {
                    case TINYGLTF_TYPE_VEC3:
                    {
                        const float3* buf = static_cast<const float3*>(dataPtr);
                        for (size_t index = 0; index < accessor.count; index++)
                        {
                            AnimSampler.OutputsVec4.push_back(float4(buf[index], 0.0f));
                        }
                        break;
                    }

                    case TINYGLTF_TYPE_VEC4:
                    {
                        const float4* buf = static_cast<const float4*>(dataPtr);
                        for (size_t index = 0; index < accessor.count; index++)
                        {
                            AnimSampler.OutputsVec4.push_back(buf[index]);
                        }
                        break;
                    }

                    default:
                    {
                        LOG_WARNING_MESSAGE("Unknown type", accessor.type);
                        break;
                    }
                }
            }

            animation.Samplers.push_back(AnimSampler);
        }


        for (auto& source : gltf_anim.channels)
        {
            AnimationChannel channel{};

            if (source.target_path == "rotation")
            {
                channel.PathType = AnimationChannel::PATH_TYPE::ROTATION;
            }
            else if (source.target_path == "translation")
            {
                channel.PathType = AnimationChannel::PATH_TYPE::TRANSLATION;
            }
            else if (source.target_path == "scale")
            {
                channel.PathType = AnimationChannel::PATH_TYPE::SCALE;
            }
            else if (source.target_path == "weights")
            {
                LOG_WARNING_MESSAGE("Weights not yet supported, skipping channel");
                continue;
            }

            channel.SamplerIndex = source.sampler;
            channel.node         = NodeFromIndex(source.target_node);
            if (!channel.node)
            {
                continue;
            }

            animation.Channels.push_back(channel);
        }

        Animations.push_back(animation);
    }
}

namespace Callbacks
{

namespace
{

struct ImageLoaderData
{
    Model::TextureCacheType*              pTextureCache;
    std::vector<RefCntAutoPtr<ITexture>>* pTextureHold;
    std::string                           BaseDir;
};


bool LoadImageData(tinygltf::Image*     gltf_image,
                   const int            gltf_image_idx,
                   std::string*         error,
                   std::string*         warning,
                   int                  req_width,
                   int                  req_height,
                   const unsigned char* image_data,
                   int                  size,
                   void*                user_data)
{
    (void)warning;

    auto* pLoaderData = reinterpret_cast<ImageLoaderData*>(user_data);
    if (pLoaderData != nullptr && pLoaderData->pTextureCache != nullptr)
    {
        auto it = pLoaderData->pTextureCache->find(pLoaderData->BaseDir + gltf_image->uri);
        if (it != pLoaderData->pTextureCache->end())
        {
            if (auto pTexture = it->second.Lock())
            {
                const auto& TexDesc    = pTexture->GetDesc();
                const auto& FmtAttribs = GetTextureFormatAttribs(TexDesc.Format);

                gltf_image->width      = TexDesc.Width;
                gltf_image->height     = TexDesc.Height;
                gltf_image->component  = FmtAttribs.NumComponents;
                gltf_image->bits       = FmtAttribs.ComponentSize * 8;
                gltf_image->pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;

                // Keep strong reference to ensure the texture is alive.
                pLoaderData->pTextureHold->emplace_back(std::move(pTexture));

                return true;
            }
            else
            {
                // Texture is stale - remove it from the cache
                pLoaderData->pTextureCache->erase(it);
            }
        }
    }

    ImageLoadInfo LoadInfo;
    LoadInfo.Format = Image::GetFileFormat(image_data, size);
    if (LoadInfo.Format == IMAGE_FILE_FORMAT_UNKNOWN)
    {
        if (error != nullptr)
        {
            *error += FormatString("Unknown format for image[", gltf_image_idx, "] name = '", gltf_image->name, "'");
        }
        return false;
    }

    if (LoadInfo.Format == IMAGE_FILE_FORMAT_DDS || LoadInfo.Format == IMAGE_FILE_FORMAT_KTX)
    {
        // Store binary data directly
        gltf_image->image.resize(size);
        memcpy(gltf_image->image.data(), image_data, size);
        // Use pixel_type field to indicate the file format
        gltf_image->pixel_type = LoadInfo.Format;
    }
    else
    {
        RefCntAutoPtr<DataBlobImpl> pImageData(MakeNewRCObj<DataBlobImpl>()(size));
        memcpy(pImageData->GetDataPtr(), image_data, size);
        RefCntAutoPtr<Image> pImage;
        Image::CreateFromDataBlob(pImageData, LoadInfo, &pImage);
        if (!pImage)
        {
            if (error != nullptr)
            {
                *error += FormatString("Failed to load image[", gltf_image_idx, "] name = '", gltf_image->name, "'");
            }
            return false;
        }
        const auto& ImgDesc = pImage->GetDesc();

        if (req_width > 0)
        {
            if (static_cast<Uint32>(req_width) != ImgDesc.Width)
            {
                if (error != nullptr)
                {
                    (*error) += FormatString("Image width mismatch for image[",
                                             gltf_image_idx, "] name = '", gltf_image->name,
                                             "': requested width: ",
                                             req_width, ", actual width: ",
                                             ImgDesc.Width);
                }
                return false;
            }
        }

        if (req_height > 0)
        {
            if (static_cast<Uint32>(req_height) != ImgDesc.Height)
            {
                if (error != nullptr)
                {
                    (*error) += FormatString("Image height mismatch for image[",
                                             gltf_image_idx, "] name = '", gltf_image->name,
                                             "': requested height: ",
                                             req_height, ", actual height: ",
                                             ImgDesc.Height);
                }
                return false;
            }
        }

        gltf_image->width      = ImgDesc.Width;
        gltf_image->height     = ImgDesc.Height;
        gltf_image->component  = 4;
        gltf_image->bits       = GetValueSize(ImgDesc.ComponentType) * 8;
        gltf_image->pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;
        auto DstRowSize        = gltf_image->width * gltf_image->component * (gltf_image->bits / 8);
        gltf_image->image.resize(static_cast<size_t>(gltf_image->height * DstRowSize));
        auto*        pPixelsBlob = pImage->GetData();
        const Uint8* pSrcPixels  = reinterpret_cast<const Uint8*>(pPixelsBlob->GetDataPtr());
        if (ImgDesc.NumComponents == 3)
        {
            for (Uint32 row = 0; row < ImgDesc.Height; ++row)
            {
                for (Uint32 col = 0; col < ImgDesc.Width; ++col)
                {
                    Uint8*       DstPixel = gltf_image->image.data() + DstRowSize * row + col * gltf_image->component;
                    const Uint8* SrcPixel = pSrcPixels + ImgDesc.RowStride * row + col * ImgDesc.NumComponents;

                    DstPixel[0] = SrcPixel[0];
                    DstPixel[1] = SrcPixel[1];
                    DstPixel[2] = SrcPixel[2];
                    DstPixel[3] = 1;
                }
            }
        }
        else if (gltf_image->component == 4)
        {
            for (Uint32 row = 0; row < ImgDesc.Height; ++row)
            {
                memcpy(gltf_image->image.data() + DstRowSize * row, pSrcPixels + ImgDesc.RowStride * row, DstRowSize);
            }
        }
        else
        {
            *error += FormatString("Unexpected number of image comonents (", ImgDesc.NumComponents, ")");
            return false;
        }
    }

    return true;
}

bool FileExists(const std::string& abs_filename, void*)
{
    return FileSystem::FileExists(abs_filename.c_str());
}

bool ReadWholeFile(std::vector<unsigned char>* out,
                   std::string*                err,
                   const std::string&          filepath,
                   void*)
{
    FileWrapper pFile(filepath.c_str(), EFileAccessMode::Read);
    if (!pFile)
    {
        if (err)
        {
            (*err) += FormatString("Unable to open file ", filepath, "\n");
        }
        return false;
    }

    auto size = pFile->GetSize();
    if (size == 0)
    {
        if (err)
        {
            (*err) += FormatString("File is empty: ", filepath, "\n");
        }
        return false;
    }

    out->resize(size);
    pFile->Read(out->data(), size);

    return true;
}

} // namespace

} // namespace Callbacks

void Model::LoadFromFile(IRenderDevice*     pDevice,
                         IDeviceContext*    pContext,
                         const std::string& filename,
                         TextureCacheType*  pTextureCache)
{
    tinygltf::Model    gltf_model;
    tinygltf::TinyGLTF gltf_context;

    std::vector<RefCntAutoPtr<ITexture>> TextureHold;

    Callbacks::ImageLoaderData LoaderData //
        {
            pTextureCache,
            &TextureHold //
        };

    if (filename.find_last_of("/\\") != std::string::npos)
        LoaderData.BaseDir = filename.substr(0, filename.find_last_of("/\\"));
    LoaderData.BaseDir += '/';

    gltf_context.SetImageLoader(Callbacks::LoadImageData, &LoaderData);
    tinygltf::FsCallbacks fsCallbacks = {};
    fsCallbacks.ExpandFilePath        = tinygltf::ExpandFilePath;
    fsCallbacks.FileExists            = Callbacks::FileExists;
    fsCallbacks.ReadWholeFile         = Callbacks::ReadWholeFile;
    fsCallbacks.WriteWholeFile        = tinygltf::WriteWholeFile;
    fsCallbacks.user_data             = this;
    gltf_context.SetFsCallbacks(fsCallbacks);

    bool   binary = false;
    size_t extpos = filename.rfind('.', filename.length());
    if (extpos != std::string::npos)
    {
        binary = (filename.substr(extpos + 1, filename.length() - extpos) == "glb");
    }

    std::string error;
    std::string warning;

    bool fileLoaded;
    if (binary)
        fileLoaded = gltf_context.LoadBinaryFromFile(&gltf_model, &error, &warning, filename.c_str());
    else
        fileLoaded = gltf_context.LoadASCIIFromFile(&gltf_model, &error, &warning, filename.c_str());
    if (!fileLoaded)
    {
        LOG_ERROR_AND_THROW("Failed to load gltf file ", filename, ": ", error);
    }
    if (!warning.empty())
    {
        LOG_WARNING_MESSAGE("Loaded gltf file ", filename, " with the following warning:", warning);
    }

    std::vector<Uint32>         IndexBuffer;
    std::vector<VertexAttribs0> VertexData0;
    std::vector<VertexAttribs1> VertexData1;

    LoadTextureSamplers(pDevice, gltf_model);
    LoadTextures(pDevice, pContext, gltf_model, LoaderData.BaseDir, pTextureCache);
    LoadMaterials(gltf_model);

    // TODO: scene handling with no default scene
    const tinygltf::Scene& scene = gltf_model.scenes[gltf_model.defaultScene > -1 ? gltf_model.defaultScene : 0];
    for (size_t i = 0; i < scene.nodes.size(); i++)
    {
        const tinygltf::Node node = gltf_model.nodes[scene.nodes[i]];
        LoadNode(pDevice, nullptr, node, scene.nodes[i], gltf_model, IndexBuffer, VertexData0, VertexData1);
    }

    if (gltf_model.animations.size() > 0)
    {
        LoadAnimations(gltf_model);
    }
    LoadSkins(gltf_model);

    for (auto* node : LinearNodes)
    {
        // Assign skins
        if (node->SkinIndex >= 0)
        {
            node->_Skin = Skins[node->SkinIndex].get();
        }

        // Initial pose
        if (node->_Mesh)
        {
            node->Update();
        }
    }


    Extensions = gltf_model.extensionsUsed;

    {
        VERIFY_EXPR(!VertexData0.empty());
        BufferDesc VBDesc;
        VBDesc.Name          = "GLTF vertex attribs 0 buffer";
        VBDesc.uiSizeInBytes = static_cast<Uint32>(VertexData0.size() * sizeof(VertexData0[0]));
        VBDesc.BindFlags     = BIND_VERTEX_BUFFER;
        VBDesc.Usage         = USAGE_IMMUTABLE;

        BufferData BuffData(VertexData0.data(), VBDesc.uiSizeInBytes);
        pDevice->CreateBuffer(VBDesc, &BuffData, &pVertexBuffer[0]);
    }

    {
        VERIFY_EXPR(!VertexData1.empty());
        BufferDesc VBDesc;
        VBDesc.Name          = "GLTF vertex attribs 1 buffer";
        VBDesc.uiSizeInBytes = static_cast<Uint32>(VertexData1.size() * sizeof(VertexData1[0]));
        VBDesc.BindFlags     = BIND_VERTEX_BUFFER;
        VBDesc.Usage         = USAGE_IMMUTABLE;

        BufferData BuffData(VertexData1.data(), VBDesc.uiSizeInBytes);
        pDevice->CreateBuffer(VBDesc, &BuffData, &pVertexBuffer[1]);
    }


    if (!IndexBuffer.empty())
    {
        BufferDesc IBDesc;
        IBDesc.Name          = "GLTF inde buffer";
        IBDesc.uiSizeInBytes = static_cast<Uint32>(IndexBuffer.size() * sizeof(IndexBuffer[0]));
        IBDesc.BindFlags     = BIND_INDEX_BUFFER;
        IBDesc.Usage         = USAGE_IMMUTABLE;

        BufferData BuffData(IndexBuffer.data(), IBDesc.uiSizeInBytes);
        pDevice->CreateBuffer(IBDesc, &BuffData, &pIndexBuffer);
    }

    GetSceneDimensions();
}

void Model::CalculateBoundingBox(Node* node, const Node* parent)
{
    BoundBox parentBvh = parent ? parent->BVH : BoundBox{dimensions.min, dimensions.max};

    if (node->_Mesh)
    {
        if (node->_Mesh->IsValidBB)
        {
            node->AABB = node->_Mesh->BB.Transform(node->GetMatrix());
            if (node->Children.empty())
            {
                node->BVH.Min    = node->AABB.Min;
                node->BVH.Max    = node->AABB.Max;
                node->IsValidBVH = true;
            }
        }
    }

    parentBvh.Min = std::min(parentBvh.Min, node->BVH.Min);
    parentBvh.Max = std::max(parentBvh.Max, node->BVH.Max);

    for (auto& child : node->Children)
    {
        CalculateBoundingBox(child.get(), node);
    }
}

void Model::GetSceneDimensions()
{
    // Calculate binary volume hierarchy for all nodes in the scene
    for (auto* node : LinearNodes)
    {
        CalculateBoundingBox(node, nullptr);
    }

    dimensions.min = float3{+FLT_MAX, +FLT_MAX, +FLT_MAX};
    dimensions.max = float3{-FLT_MAX, -FLT_MAX, -FLT_MAX};

    for (const auto* node : LinearNodes)
    {
        if (node->IsValidBVH)
        {
            dimensions.min = std::min(dimensions.min, node->BVH.Min);
            dimensions.max = std::max(dimensions.max, node->BVH.Max);
        }
    }

    // Calculate scene AABBTransform
    AABBTransform       = float4x4::Scale(dimensions.max[0] - dimensions.min[0], dimensions.max[1] - dimensions.min[1], dimensions.max[2] - dimensions.min[2]);
    AABBTransform[3][0] = dimensions.min[0];
    AABBTransform[3][1] = dimensions.min[1];
    AABBTransform[3][2] = dimensions.min[2];
}

void Model::UpdateAnimation(Uint32 index, float time)
{
    if (index > static_cast<Uint32>(Animations.size()) - 1)
    {
        LOG_WARNING_MESSAGE("No animation with index ", index);
        return;
    }
    Animation& animation = Animations[index];

    bool updated = false;
    for (auto& channel : animation.Channels)
    {
        AnimationSampler& sampler = animation.Samplers[channel.SamplerIndex];
        if (sampler.Inputs.size() > sampler.OutputsVec4.size())
        {
            continue;
        }

        for (size_t i = 0; i < sampler.Inputs.size() - 1; i++)
        {
            if ((time >= sampler.Inputs[i]) && (time <= sampler.Inputs[i + 1]))
            {
                float u = std::max(0.0f, time - sampler.Inputs[i]) / (sampler.Inputs[i + 1] - sampler.Inputs[i]);
                if (u <= 1.0f)
                {
                    switch (channel.PathType)
                    {
                        case AnimationChannel::PATH_TYPE::TRANSLATION:
                        {
                            float4 trans              = lerp(sampler.OutputsVec4[i], sampler.OutputsVec4[i + 1], u);
                            channel.node->Translation = float3(trans);
                            break;
                        }

                        case AnimationChannel::PATH_TYPE::SCALE:
                        {
                            float4 scale        = lerp(sampler.OutputsVec4[i], sampler.OutputsVec4[i + 1], u);
                            channel.node->Scale = float3(scale);
                            break;
                        }

                        case AnimationChannel::PATH_TYPE::ROTATION:
                        {
                            Quaternion q1;
                            q1.q.x = sampler.OutputsVec4[i].x;
                            q1.q.y = sampler.OutputsVec4[i].y;
                            q1.q.z = sampler.OutputsVec4[i].z;
                            q1.q.w = sampler.OutputsVec4[i].w;

                            Quaternion q2;
                            q2.q.x = sampler.OutputsVec4[i + 1].x;
                            q2.q.y = sampler.OutputsVec4[i + 1].y;
                            q2.q.z = sampler.OutputsVec4[i + 1].z;
                            q2.q.w = sampler.OutputsVec4[i + 1].w;

                            channel.node->Rotation = normalize(slerp(q1, q2, u));
                            break;
                        }
                    }
                    updated = true;
                }
            }
        }
    }

    if (updated)
    {
        for (auto& node : Nodes)
        {
            node->Update();
        }
    }
}


Node* Model::FindNode(Node* parent, Uint32 index)
{
    Node* nodeFound = nullptr;
    if (parent->Index == index)
    {
        return parent;
    }
    for (auto& child : parent->Children)
    {
        nodeFound = FindNode(child.get(), index);
        if (nodeFound)
        {
            break;
        }
    }
    return nodeFound;
}


Node* Model::NodeFromIndex(uint32_t index)
{
    Node* nodeFound = nullptr;
    for (auto& node : Nodes)
    {
        nodeFound = FindNode(node.get(), index);
        if (nodeFound)
        {
            break;
        }
    }
    return nodeFound;
}


} // namespace GLTF

} // namespace Diligent
