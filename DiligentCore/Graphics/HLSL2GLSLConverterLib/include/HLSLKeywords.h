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

// clang-format off

#define LIST_VECTOR_AND_MATRIX_EXPANSIONS(KEYWORD_HANDLER, type)\
KEYWORD_HANDLER(type)\
KEYWORD_HANDLER(type##1)\
KEYWORD_HANDLER(type##2)\
KEYWORD_HANDLER(type##3)\
KEYWORD_HANDLER(type##4)\
KEYWORD_HANDLER(type##1x1)\
KEYWORD_HANDLER(type##1x2)\
KEYWORD_HANDLER(type##1x3)\
KEYWORD_HANDLER(type##1x4)\
KEYWORD_HANDLER(type##2x1)\
KEYWORD_HANDLER(type##2x2)\
KEYWORD_HANDLER(type##2x3)\
KEYWORD_HANDLER(type##2x4)\
KEYWORD_HANDLER(type##3x1)\
KEYWORD_HANDLER(type##3x2)\
KEYWORD_HANDLER(type##3x3)\
KEYWORD_HANDLER(type##3x4)\
KEYWORD_HANDLER(type##4x1)\
KEYWORD_HANDLER(type##4x2)\
KEYWORD_HANDLER(type##4x3)\
KEYWORD_HANDLER(type##4x4)

#define ITERATE_KEYWORDS(KEYWORD_HANDLER)\
/*All built-in types must be defined consecutively between bool and void*/\
LIST_VECTOR_AND_MATRIX_EXPANSIONS(KEYWORD_HANDLER, bool)\
LIST_VECTOR_AND_MATRIX_EXPANSIONS(KEYWORD_HANDLER, float)\
LIST_VECTOR_AND_MATRIX_EXPANSIONS(KEYWORD_HANDLER, int)\
LIST_VECTOR_AND_MATRIX_EXPANSIONS(KEYWORD_HANDLER, uint)\
LIST_VECTOR_AND_MATRIX_EXPANSIONS(KEYWORD_HANDLER, min16float)\
LIST_VECTOR_AND_MATRIX_EXPANSIONS(KEYWORD_HANDLER, min10float)\
LIST_VECTOR_AND_MATRIX_EXPANSIONS(KEYWORD_HANDLER, min16int)\
LIST_VECTOR_AND_MATRIX_EXPANSIONS(KEYWORD_HANDLER, min12int)\
LIST_VECTOR_AND_MATRIX_EXPANSIONS(KEYWORD_HANDLER, min16uint)\
KEYWORD_HANDLER(matrix)\
KEYWORD_HANDLER(void)\
/*All flow control keywords must be defined consecutively between break and while*/\
KEYWORD_HANDLER(break)\
KEYWORD_HANDLER(case)\
KEYWORD_HANDLER(continue)\
KEYWORD_HANDLER(default)\
KEYWORD_HANDLER(do)\
KEYWORD_HANDLER(else)\
KEYWORD_HANDLER(for)\
KEYWORD_HANDLER(if)\
KEYWORD_HANDLER(return)\
KEYWORD_HANDLER(switch)\
KEYWORD_HANDLER(while)\
KEYWORD_HANDLER(AppendStructuredBuffer)\
KEYWORD_HANDLER(asm)\
KEYWORD_HANDLER(asm_fragment)\
KEYWORD_HANDLER(BlendState)\
KEYWORD_HANDLER(Buffer)\
KEYWORD_HANDLER(ByteAddressBuffer)\
KEYWORD_HANDLER(cbuffer)\
KEYWORD_HANDLER(centroid)\
KEYWORD_HANDLER(class)\
KEYWORD_HANDLER(column_major)\
KEYWORD_HANDLER(compile)\
KEYWORD_HANDLER(compile_fragment)\
KEYWORD_HANDLER(CompileShader)\
KEYWORD_HANDLER(const)\
KEYWORD_HANDLER(ComputeShader)\
KEYWORD_HANDLER(ConsumeStructuredBuffer)\
KEYWORD_HANDLER(DepthStencilState)\
KEYWORD_HANDLER(DepthStencilView)\
KEYWORD_HANDLER(discard)\
KEYWORD_HANDLER(double)\
KEYWORD_HANDLER(DomainShader)\
KEYWORD_HANDLER(dword)\
KEYWORD_HANDLER(export)\
KEYWORD_HANDLER(extern)\
KEYWORD_HANDLER(false)\
KEYWORD_HANDLER(fxgroup)\
KEYWORD_HANDLER(GeometryShader)\
KEYWORD_HANDLER(groupshared)\
KEYWORD_HANDLER(half)\
KEYWORD_HANDLER(Hullshader)\
KEYWORD_HANDLER(in)\
KEYWORD_HANDLER(inline)\
KEYWORD_HANDLER(inout)\
KEYWORD_HANDLER(InputPatch)\
KEYWORD_HANDLER(interface)\
KEYWORD_HANDLER(line)\
KEYWORD_HANDLER(lineadj)\
KEYWORD_HANDLER(linear)\
KEYWORD_HANDLER(LineStream)\
KEYWORD_HANDLER(namespace)\
KEYWORD_HANDLER(nointerpolation)\
KEYWORD_HANDLER(noperspective)\
KEYWORD_HANDLER(NULL)\
KEYWORD_HANDLER(out)\
KEYWORD_HANDLER(OutputPatch)\
KEYWORD_HANDLER(packoffset)\
KEYWORD_HANDLER(pass)\
KEYWORD_HANDLER(pixelfragment)\
KEYWORD_HANDLER(PixelShader)\
KEYWORD_HANDLER(point)\
KEYWORD_HANDLER(PointStream)\
KEYWORD_HANDLER(precise)\
KEYWORD_HANDLER(RasterizerState)\
KEYWORD_HANDLER(RenderTargetView)\
KEYWORD_HANDLER(register)\
KEYWORD_HANDLER(row_major)\
KEYWORD_HANDLER(RWBuffer)\
KEYWORD_HANDLER(RWByteAddressBuffer)\
KEYWORD_HANDLER(RWStructuredBuffer)\
KEYWORD_HANDLER(RWTexture1D)\
KEYWORD_HANDLER(RWTexture1DArray)\
KEYWORD_HANDLER(RWTexture2D)\
KEYWORD_HANDLER(RWTexture2DArray)\
KEYWORD_HANDLER(RWTexture3D)\
KEYWORD_HANDLER(sample)\
KEYWORD_HANDLER(sampler)\
KEYWORD_HANDLER(SamplerState)\
KEYWORD_HANDLER(SamplerComparisonState)\
KEYWORD_HANDLER(shared)\
KEYWORD_HANDLER(snorm)\
KEYWORD_HANDLER(stateblock)\
KEYWORD_HANDLER(stateblock_state)\
KEYWORD_HANDLER(static)\
KEYWORD_HANDLER(string)\
KEYWORD_HANDLER(struct)\
KEYWORD_HANDLER(StructuredBuffer)\
KEYWORD_HANDLER(tbuffer)\
KEYWORD_HANDLER(technique)\
KEYWORD_HANDLER(technique10)\
KEYWORD_HANDLER(technique11)\
KEYWORD_HANDLER(texture)\
KEYWORD_HANDLER(Texture1D)\
KEYWORD_HANDLER(Texture1DArray)\
KEYWORD_HANDLER(Texture2D)\
KEYWORD_HANDLER(Texture2DArray)\
KEYWORD_HANDLER(Texture2DMS)\
KEYWORD_HANDLER(Texture2DMSArray)\
KEYWORD_HANDLER(Texture3D)\
KEYWORD_HANDLER(TextureCube)\
KEYWORD_HANDLER(TextureCubeArray)\
KEYWORD_HANDLER(true)\
KEYWORD_HANDLER(typedef)\
KEYWORD_HANDLER(triangle)\
KEYWORD_HANDLER(triangleadj)\
KEYWORD_HANDLER(TriangleStream)\
KEYWORD_HANDLER(uniform)\
KEYWORD_HANDLER(unorm)\
KEYWORD_HANDLER(unsigned)\
KEYWORD_HANDLER(vector)\
KEYWORD_HANDLER(vertexfragment)\
KEYWORD_HANDLER(VertexShader)\
KEYWORD_HANDLER(volatile)
