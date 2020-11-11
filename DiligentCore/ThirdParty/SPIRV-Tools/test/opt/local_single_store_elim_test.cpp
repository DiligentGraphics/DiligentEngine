// Copyright (c) 2017 Valve Corporation
// Copyright (c) 2017 LunarG Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <string>

#include "test/opt/pass_fixture.h"
#include "test/opt/pass_utils.h"

namespace spvtools {
namespace opt {
namespace {

using LocalSingleStoreElimTest = PassTest<::testing::Test>;

TEST_F(LocalSingleStoreElimTest, PositiveAndNegative) {
  // Single store to v is optimized. Multiple store to
  // f is not optimized.
  //
  // #version 140
  //
  // in vec4 BaseColor;
  // in float fi;
  //
  // void main()
  // {
  //     vec4 v = BaseColor;
  //     float f = fi;
  //     if (f < 0)
  //         f = 0.0;
  //     gl_FragColor = v + f;
  // }

  const std::string predefs =
      R"(OpCapability Shader
%1 = OpExtInstImport "GLSL.std.450"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %BaseColor %fi %gl_FragColor
OpExecutionMode %main OriginUpperLeft
OpSource GLSL 140
OpName %main "main"
OpName %v "v"
OpName %BaseColor "BaseColor"
OpName %f "f"
OpName %fi "fi"
OpName %gl_FragColor "gl_FragColor"
%void = OpTypeVoid
%9 = OpTypeFunction %void
%float = OpTypeFloat 32
%v4float = OpTypeVector %float 4
%_ptr_Function_v4float = OpTypePointer Function %v4float
%_ptr_Input_v4float = OpTypePointer Input %v4float
%BaseColor = OpVariable %_ptr_Input_v4float Input
%_ptr_Function_float = OpTypePointer Function %float
%_ptr_Input_float = OpTypePointer Input %float
%fi = OpVariable %_ptr_Input_float Input
%float_0 = OpConstant %float 0
%bool = OpTypeBool
%_ptr_Output_v4float = OpTypePointer Output %v4float
%gl_FragColor = OpVariable %_ptr_Output_v4float Output
)";

  const std::string before =
      R"(%main = OpFunction %void None %9
%19 = OpLabel
%v = OpVariable %_ptr_Function_v4float Function
%f = OpVariable %_ptr_Function_float Function
%20 = OpLoad %v4float %BaseColor
OpStore %v %20
%21 = OpLoad %float %fi
OpStore %f %21
%22 = OpLoad %float %f
%23 = OpFOrdLessThan %bool %22 %float_0
OpSelectionMerge %24 None
OpBranchConditional %23 %25 %24
%25 = OpLabel
OpStore %f %float_0
OpBranch %24
%24 = OpLabel
%26 = OpLoad %v4float %v
%27 = OpLoad %float %f
%28 = OpCompositeConstruct %v4float %27 %27 %27 %27
%29 = OpFAdd %v4float %26 %28
OpStore %gl_FragColor %29
OpReturn
OpFunctionEnd
)";

  const std::string after =
      R"(%main = OpFunction %void None %9
%19 = OpLabel
%v = OpVariable %_ptr_Function_v4float Function
%f = OpVariable %_ptr_Function_float Function
%20 = OpLoad %v4float %BaseColor
OpStore %v %20
%21 = OpLoad %float %fi
OpStore %f %21
%22 = OpLoad %float %f
%23 = OpFOrdLessThan %bool %22 %float_0
OpSelectionMerge %24 None
OpBranchConditional %23 %25 %24
%25 = OpLabel
OpStore %f %float_0
OpBranch %24
%24 = OpLabel
%27 = OpLoad %float %f
%28 = OpCompositeConstruct %v4float %27 %27 %27 %27
%29 = OpFAdd %v4float %20 %28
OpStore %gl_FragColor %29
OpReturn
OpFunctionEnd
)";

  SinglePassRunAndCheck<LocalSingleStoreElimPass>(predefs + before,
                                                  predefs + after, true, true);
}

TEST_F(LocalSingleStoreElimTest, ThreeStores) {
  // Three stores to multiple loads of v is not optimized.

  const std::string predefs =
      R"(OpCapability Shader
%1 = OpExtInstImport "GLSL.std.450"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %BaseColor %fi %gl_FragColor
OpExecutionMode %main OriginUpperLeft
OpSource GLSL 140
OpName %main "main"
OpName %v "v"
OpName %BaseColor "BaseColor"
OpName %fi "fi"
OpName %r "r"
OpName %gl_FragColor "gl_FragColor"
%void = OpTypeVoid
%9 = OpTypeFunction %void
%float = OpTypeFloat 32
%v4float = OpTypeVector %float 4
%_ptr_Function_v4float = OpTypePointer Function %v4float
%_ptr_Input_v4float = OpTypePointer Input %v4float
%BaseColor = OpVariable %_ptr_Input_v4float Input
%_ptr_Input_float = OpTypePointer Input %float
%fi = OpVariable %_ptr_Input_float Input
%float_0 = OpConstant %float 0
%bool = OpTypeBool
%float_1 = OpConstant %float 1
%_ptr_Output_v4float = OpTypePointer Output %v4float
%gl_FragColor = OpVariable %_ptr_Output_v4float Output
)";

  const std::string before =
      R"(%main = OpFunction %void None %9
%19 = OpLabel
%v = OpVariable %_ptr_Function_v4float Function
%r = OpVariable %_ptr_Function_v4float Function
%20 = OpLoad %v4float %BaseColor
OpStore %v %20
%21 = OpLoad %float %fi
%22 = OpFOrdLessThan %bool %21 %float_0
OpSelectionMerge %23 None
OpBranchConditional %22 %24 %25
%24 = OpLabel
%26 = OpLoad %v4float %v
OpStore %v %26
OpStore %r %26
OpBranch %23
%25 = OpLabel
%27 = OpLoad %v4float %v
%28 = OpCompositeConstruct %v4float %float_1 %float_1 %float_1 %float_1
OpStore %v %28
%29 = OpFSub %v4float %28 %27
OpStore %r %29
OpBranch %23
%23 = OpLabel
%30 = OpLoad %v4float %r
OpStore %gl_FragColor %30
OpReturn
OpFunctionEnd
)";

  SinglePassRunAndCheck<LocalSingleStoreElimPass>(predefs + before,
                                                  predefs + before, true, true);
}

TEST_F(LocalSingleStoreElimTest, MultipleLoads) {
  // Single store to multiple loads of v is optimized.
  //
  // #version 140
  //
  // in vec4 BaseColor;
  // in float fi;
  //
  // void main()
  // {
  //     vec4 v = BaseColor;
  //     float f = fi;
  //     if (f < 0)
  //         f = 0.0;
  //     gl_FragColor = v + f;
  // }

  const std::string predefs =
      R"(OpCapability Shader
%1 = OpExtInstImport "GLSL.std.450"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %BaseColor %fi %gl_FragColor
OpExecutionMode %main OriginUpperLeft
OpSource GLSL 140
OpName %main "main"
OpName %v "v"
OpName %BaseColor "BaseColor"
OpName %fi "fi"
OpName %r "r"
OpName %gl_FragColor "gl_FragColor"
%void = OpTypeVoid
%9 = OpTypeFunction %void
%float = OpTypeFloat 32
%v4float = OpTypeVector %float 4
%_ptr_Function_v4float = OpTypePointer Function %v4float
%_ptr_Input_v4float = OpTypePointer Input %v4float
%BaseColor = OpVariable %_ptr_Input_v4float Input
%_ptr_Input_float = OpTypePointer Input %float
%fi = OpVariable %_ptr_Input_float Input
%float_0 = OpConstant %float 0
%bool = OpTypeBool
%float_1 = OpConstant %float 1
%_ptr_Output_v4float = OpTypePointer Output %v4float
%gl_FragColor = OpVariable %_ptr_Output_v4float Output
)";

  const std::string before =
      R"(%main = OpFunction %void None %9
%19 = OpLabel
%v = OpVariable %_ptr_Function_v4float Function
%r = OpVariable %_ptr_Function_v4float Function
%20 = OpLoad %v4float %BaseColor
OpStore %v %20
%21 = OpLoad %float %fi
%22 = OpFOrdLessThan %bool %21 %float_0
OpSelectionMerge %23 None
OpBranchConditional %22 %24 %25
%24 = OpLabel
%26 = OpLoad %v4float %v
OpStore %r %26
OpBranch %23
%25 = OpLabel
%27 = OpLoad %v4float %v
%28 = OpCompositeConstruct %v4float %float_1 %float_1 %float_1 %float_1
%29 = OpFSub %v4float %28 %27
OpStore %r %29
OpBranch %23
%23 = OpLabel
%30 = OpLoad %v4float %r
OpStore %gl_FragColor %30
OpReturn
OpFunctionEnd
)";

  const std::string after =
      R"(%main = OpFunction %void None %9
%19 = OpLabel
%v = OpVariable %_ptr_Function_v4float Function
%r = OpVariable %_ptr_Function_v4float Function
%20 = OpLoad %v4float %BaseColor
OpStore %v %20
%21 = OpLoad %float %fi
%22 = OpFOrdLessThan %bool %21 %float_0
OpSelectionMerge %23 None
OpBranchConditional %22 %24 %25
%24 = OpLabel
OpStore %r %20
OpBranch %23
%25 = OpLabel
%28 = OpCompositeConstruct %v4float %float_1 %float_1 %float_1 %float_1
%29 = OpFSub %v4float %28 %20
OpStore %r %29
OpBranch %23
%23 = OpLabel
%30 = OpLoad %v4float %r
OpStore %gl_FragColor %30
OpReturn
OpFunctionEnd
)";

  SinglePassRunAndCheck<LocalSingleStoreElimPass>(predefs + before,
                                                  predefs + after, true, true);
}

TEST_F(LocalSingleStoreElimTest, NoStoreElimWithInterveningAccessChainLoad) {
  // Last load of v is eliminated, but access chain load and store of v isn't
  //
  // #version 140
  //
  // in vec4 BaseColor;
  //
  // void main()
  // {
  //     vec4 v = BaseColor;
  //     float f = v[3];
  //     gl_FragColor = v * f;
  // }

  const std::string predefs =
      R"(OpCapability Shader
%1 = OpExtInstImport "GLSL.std.450"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %BaseColor %gl_FragColor
OpExecutionMode %main OriginUpperLeft
OpSource GLSL 140
OpName %main "main"
OpName %v "v"
OpName %BaseColor "BaseColor"
OpName %f "f"
OpName %gl_FragColor "gl_FragColor"
%void = OpTypeVoid
%8 = OpTypeFunction %void
%float = OpTypeFloat 32
%v4float = OpTypeVector %float 4
%_ptr_Function_v4float = OpTypePointer Function %v4float
%_ptr_Input_v4float = OpTypePointer Input %v4float
%BaseColor = OpVariable %_ptr_Input_v4float Input
%_ptr_Function_float = OpTypePointer Function %float
%uint = OpTypeInt 32 0
%uint_3 = OpConstant %uint 3
%_ptr_Output_v4float = OpTypePointer Output %v4float
%gl_FragColor = OpVariable %_ptr_Output_v4float Output
)";

  const std::string before =
      R"(%main = OpFunction %void None %8
%17 = OpLabel
%v = OpVariable %_ptr_Function_v4float Function
%f = OpVariable %_ptr_Function_float Function
%18 = OpLoad %v4float %BaseColor
OpStore %v %18
%19 = OpAccessChain %_ptr_Function_float %v %uint_3
%20 = OpLoad %float %19
OpStore %f %20
%21 = OpLoad %v4float %v
%22 = OpLoad %float %f
%23 = OpVectorTimesScalar %v4float %21 %22
OpStore %gl_FragColor %23
OpReturn
OpFunctionEnd
)";

  const std::string after =
      R"(%main = OpFunction %void None %8
%17 = OpLabel
%v = OpVariable %_ptr_Function_v4float Function
%f = OpVariable %_ptr_Function_float Function
%18 = OpLoad %v4float %BaseColor
OpStore %v %18
%19 = OpAccessChain %_ptr_Function_float %v %uint_3
%20 = OpLoad %float %19
OpStore %f %20
%23 = OpVectorTimesScalar %v4float %18 %20
OpStore %gl_FragColor %23
OpReturn
OpFunctionEnd
)";

  SinglePassRunAndCheck<LocalSingleStoreElimPass>(predefs + before,
                                                  predefs + after, true, true);
}

TEST_F(LocalSingleStoreElimTest, NoReplaceOfDominatingPartialStore) {
  // Note: SPIR-V hand edited to initialize v to vec4(0.0)
  //
  // #version 140
  //
  // in vec4 BaseColor;
  //
  // void main()
  // {
  //     vec4 v;
  //     float v[1] = 1.0;
  //     gl_FragColor = v;
  // }

  const std::string assembly =
      R"(OpCapability Shader
%1 = OpExtInstImport "GLSL.std.450"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %gl_FragColor %BaseColor
OpExecutionMode %main OriginUpperLeft
OpSource GLSL 140
OpName %main "main"
OpName %v "v"
OpName %gl_FragColor "gl_FragColor"
OpName %BaseColor "BaseColor"
%void = OpTypeVoid
%7 = OpTypeFunction %void
%float = OpTypeFloat 32
%v4float = OpTypeVector %float 4
%_ptr_Function_v4float = OpTypePointer Function %v4float
%float_0 = OpConstant %float 0
%12 = OpConstantComposite %v4float %float_0 %float_0 %float_0 %float_0
%float_1 = OpConstant %float 1
%uint = OpTypeInt 32 0
%uint_1 = OpConstant %uint 1
%_ptr_Function_float = OpTypePointer Function %float
%_ptr_Output_v4float = OpTypePointer Output %v4float
%gl_FragColor = OpVariable %_ptr_Output_v4float Output
%_ptr_Input_v4float = OpTypePointer Input %v4float
%BaseColor = OpVariable %_ptr_Input_v4float Input
%main = OpFunction %void None %7
%19 = OpLabel
%v = OpVariable %_ptr_Function_v4float Function %12
%20 = OpAccessChain %_ptr_Function_float %v %uint_1
OpStore %20 %float_1
%21 = OpLoad %v4float %v
OpStore %gl_FragColor %21
OpReturn
OpFunctionEnd
)";

  SinglePassRunAndCheck<LocalSingleStoreElimPass>(assembly, assembly, true,
                                                  true);
}

TEST_F(LocalSingleStoreElimTest, ElimIfCopyObjectInFunction) {
  // Note: hand edited to insert OpCopyObject
  //
  // #version 140
  //
  // in vec4 BaseColor;
  // in float fi;
  //
  // void main()
  // {
  //     vec4 v = BaseColor;
  //     float f = fi;
  //     if (f < 0)
  //         f = 0.0;
  //     gl_FragColor = v + f;
  // }

  const std::string predefs =
      R"(OpCapability Shader
%1 = OpExtInstImport "GLSL.std.450"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %BaseColor %fi %gl_FragColor
OpExecutionMode %main OriginUpperLeft
OpSource GLSL 140
OpName %main "main"
OpName %v "v"
OpName %BaseColor "BaseColor"
OpName %f "f"
OpName %fi "fi"
OpName %gl_FragColor "gl_FragColor"
%void = OpTypeVoid
%9 = OpTypeFunction %void
%float = OpTypeFloat 32
%v4float = OpTypeVector %float 4
%_ptr_Function_v4float = OpTypePointer Function %v4float
%_ptr_Input_v4float = OpTypePointer Input %v4float
%BaseColor = OpVariable %_ptr_Input_v4float Input
%_ptr_Function_float = OpTypePointer Function %float
%_ptr_Input_float = OpTypePointer Input %float
%fi = OpVariable %_ptr_Input_float Input
%float_0 = OpConstant %float 0
%bool = OpTypeBool
%_ptr_Output_v4float = OpTypePointer Output %v4float
%gl_FragColor = OpVariable %_ptr_Output_v4float Output
)";

  const std::string before =
      R"(%main = OpFunction %void None %9
%19 = OpLabel
%v = OpVariable %_ptr_Function_v4float Function
%f = OpVariable %_ptr_Function_float Function
%20 = OpLoad %v4float %BaseColor
OpStore %v %20
%21 = OpLoad %float %fi
OpStore %f %21
%22 = OpLoad %float %f
%23 = OpFOrdLessThan %bool %22 %float_0
OpSelectionMerge %24 None
OpBranchConditional %23 %25 %24
%25 = OpLabel
OpStore %f %float_0
OpBranch %24
%24 = OpLabel
%26 = OpCopyObject %_ptr_Function_v4float %v
%27 = OpLoad %v4float %26
%28 = OpLoad %float %f
%29 = OpCompositeConstruct %v4float %28 %28 %28 %28
%30 = OpFAdd %v4float %27 %29
OpStore %gl_FragColor %30
OpReturn
OpFunctionEnd
)";

  const std::string after =
      R"(%main = OpFunction %void None %9
%19 = OpLabel
%v = OpVariable %_ptr_Function_v4float Function
%f = OpVariable %_ptr_Function_float Function
%20 = OpLoad %v4float %BaseColor
OpStore %v %20
%21 = OpLoad %float %fi
OpStore %f %21
%22 = OpLoad %float %f
%23 = OpFOrdLessThan %bool %22 %float_0
OpSelectionMerge %24 None
OpBranchConditional %23 %25 %24
%25 = OpLabel
OpStore %f %float_0
OpBranch %24
%24 = OpLabel
%26 = OpCopyObject %_ptr_Function_v4float %v
%28 = OpLoad %float %f
%29 = OpCompositeConstruct %v4float %28 %28 %28 %28
%30 = OpFAdd %v4float %20 %29
OpStore %gl_FragColor %30
OpReturn
OpFunctionEnd
)";

  SinglePassRunAndCheck<LocalSingleStoreElimPass>(predefs + before,
                                                  predefs + after, true, true);
}

TEST_F(LocalSingleStoreElimTest, NoOptIfStoreNotDominating) {
  // Single store to f not optimized because it does not dominate
  // the load.
  //
  // #version 140
  //
  // in vec4 BaseColor;
  // in float fi;
  //
  // void main()
  // {
  //     float f;
  //     if (fi < 0)
  //         f = 0.5;
  //     if (fi < 0)
  //         gl_FragColor = BaseColor * f;
  //     else
  //         gl_FragColor = BaseColor;
  // }

  const std::string assembly =
      R"(OpCapability Shader
%1 = OpExtInstImport "GLSL.std.450"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %fi %gl_FragColor %BaseColor
OpExecutionMode %main OriginUpperLeft
OpSource GLSL 140
OpName %main "main"
OpName %fi "fi"
OpName %f "f"
OpName %gl_FragColor "gl_FragColor"
OpName %BaseColor "BaseColor"
%void = OpTypeVoid
%8 = OpTypeFunction %void
%float = OpTypeFloat 32
%_ptr_Input_float = OpTypePointer Input %float
%fi = OpVariable %_ptr_Input_float Input
%float_0 = OpConstant %float 0
%bool = OpTypeBool
%_ptr_Function_float = OpTypePointer Function %float
%float_0_5 = OpConstant %float 0.5
%v4float = OpTypeVector %float 4
%_ptr_Output_v4float = OpTypePointer Output %v4float
%gl_FragColor = OpVariable %_ptr_Output_v4float Output
%_ptr_Input_v4float = OpTypePointer Input %v4float
%BaseColor = OpVariable %_ptr_Input_v4float Input
%main = OpFunction %void None %8
%18 = OpLabel
%f = OpVariable %_ptr_Function_float Function
%19 = OpLoad %float %fi
%20 = OpFOrdLessThan %bool %19 %float_0
OpSelectionMerge %21 None
OpBranchConditional %20 %22 %21
%22 = OpLabel
OpStore %f %float_0_5
OpBranch %21
%21 = OpLabel
%23 = OpLoad %float %fi
%24 = OpFOrdLessThan %bool %23 %float_0
OpSelectionMerge %25 None
OpBranchConditional %24 %26 %27
%26 = OpLabel
%28 = OpLoad %v4float %BaseColor
%29 = OpLoad %float %f
%30 = OpVectorTimesScalar %v4float %28 %29
OpStore %gl_FragColor %30
OpBranch %25
%27 = OpLabel
%31 = OpLoad %v4float %BaseColor
OpStore %gl_FragColor %31
OpBranch %25
%25 = OpLabel
OpReturn
OpFunctionEnd
)";

  SinglePassRunAndCheck<LocalSingleStoreElimPass>(assembly, assembly, true,
                                                  true);
}

TEST_F(LocalSingleStoreElimTest, OptInitializedVariableLikeStore) {
  // Initialized variable f is optimized like it was a store.
  // Note: The SPIR-V was edited to turn the store to f to an
  // an initialization.
  //
  // #version 140
  //
  // void main()
  // {
  //     float f = 0.0;
  //     gl_FragColor = vec4(f);
  // }

  const std::string predefs =
      R"(OpCapability Shader
%1 = OpExtInstImport "GLSL.std.450"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %gl_FragColor
OpExecutionMode %main OriginUpperLeft
OpSource GLSL 140
OpName %main "main"
OpName %f "f"
OpName %gl_FragColor "gl_FragColor"
OpDecorate %gl_FragColor Location 0
%void = OpTypeVoid
%6 = OpTypeFunction %void
%float = OpTypeFloat 32
%_ptr_Function_float = OpTypePointer Function %float
%float_0 = OpConstant %float 0
%v4float = OpTypeVector %float 4
%_ptr_Output_v4float = OpTypePointer Output %v4float
%gl_FragColor = OpVariable %_ptr_Output_v4float Output
)";

  const std::string before =
      R"(%main = OpFunction %void None %6
%12 = OpLabel
%f = OpVariable %_ptr_Function_float Function %float_0
%13 = OpLoad %float %f
%14 = OpCompositeConstruct %v4float %13 %13 %13 %13
OpStore %gl_FragColor %14
OpReturn
OpFunctionEnd
)";

  const std::string after =
      R"(%main = OpFunction %void None %6
%12 = OpLabel
%f = OpVariable %_ptr_Function_float Function %float_0
%14 = OpCompositeConstruct %v4float %float_0 %float_0 %float_0 %float_0
OpStore %gl_FragColor %14
OpReturn
OpFunctionEnd
)";

  SinglePassRunAndCheck<LocalSingleStoreElimPass>(predefs + before,
                                                  predefs + after, true, true);
}

TEST_F(LocalSingleStoreElimTest, PointerVariable) {
  // Test that checks if a pointer variable is removed.

  const std::string before =
      R"(OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %1 "main" %2
OpExecutionMode %1 OriginUpperLeft
OpMemberDecorate %_struct_3 0 Offset 0
OpDecorate %_runtimearr__struct_3 ArrayStride 16
OpMemberDecorate %_struct_5 0 Offset 0
OpDecorate %_struct_5 BufferBlock
OpMemberDecorate %_struct_6 0 Offset 0
OpDecorate %_struct_6 BufferBlock
OpDecorate %2 Location 0
OpDecorate %7 DescriptorSet 0
OpDecorate %7 Binding 0
%void = OpTypeVoid
%10 = OpTypeFunction %void
%int = OpTypeInt 32 1
%uint = OpTypeInt 32 0
%float = OpTypeFloat 32
%v4float = OpTypeVector %float 4
%_ptr_Output_v4float = OpTypePointer Output %v4float
%_ptr_Uniform_v4float = OpTypePointer Uniform %v4float
%_struct_3 = OpTypeStruct %v4float
%_runtimearr__struct_3 = OpTypeRuntimeArray %_struct_3
%_struct_5 = OpTypeStruct %_runtimearr__struct_3
%_ptr_Uniform__struct_5 = OpTypePointer Uniform %_struct_5
%_struct_6 = OpTypeStruct %int
%_ptr_Uniform__struct_6 = OpTypePointer Uniform %_struct_6
%_ptr_Function__ptr_Uniform__struct_5 = OpTypePointer Function %_ptr_Uniform__struct_5
%_ptr_Function__ptr_Uniform__struct_6 = OpTypePointer Function %_ptr_Uniform__struct_6
%int_0 = OpConstant %int 0
%uint_0 = OpConstant %uint 0
%2 = OpVariable %_ptr_Output_v4float Output
%7 = OpVariable %_ptr_Uniform__struct_5 Uniform
%1 = OpFunction %void None %10
%23 = OpLabel
%24 = OpVariable %_ptr_Function__ptr_Uniform__struct_5 Function
OpStore %24 %7
%26 = OpLoad %_ptr_Uniform__struct_5 %24
%27 = OpAccessChain %_ptr_Uniform_v4float %26 %int_0 %uint_0 %int_0
%28 = OpLoad %v4float %27
%29 = OpCopyObject %v4float %28
OpStore %2 %28
OpReturn
OpFunctionEnd
)";

  const std::string after =
      R"(OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %1 "main" %2
OpExecutionMode %1 OriginUpperLeft
OpMemberDecorate %_struct_3 0 Offset 0
OpDecorate %_runtimearr__struct_3 ArrayStride 16
OpMemberDecorate %_struct_5 0 Offset 0
OpDecorate %_struct_5 BufferBlock
OpMemberDecorate %_struct_6 0 Offset 0
OpDecorate %_struct_6 BufferBlock
OpDecorate %2 Location 0
OpDecorate %7 DescriptorSet 0
OpDecorate %7 Binding 0
%void = OpTypeVoid
%10 = OpTypeFunction %void
%int = OpTypeInt 32 1
%uint = OpTypeInt 32 0
%float = OpTypeFloat 32
%v4float = OpTypeVector %float 4
%_ptr_Output_v4float = OpTypePointer Output %v4float
%_ptr_Uniform_v4float = OpTypePointer Uniform %v4float
%_struct_3 = OpTypeStruct %v4float
%_runtimearr__struct_3 = OpTypeRuntimeArray %_struct_3
%_struct_5 = OpTypeStruct %_runtimearr__struct_3
%_ptr_Uniform__struct_5 = OpTypePointer Uniform %_struct_5
%_struct_6 = OpTypeStruct %int
%_ptr_Uniform__struct_6 = OpTypePointer Uniform %_struct_6
%_ptr_Function__ptr_Uniform__struct_5 = OpTypePointer Function %_ptr_Uniform__struct_5
%_ptr_Function__ptr_Uniform__struct_6 = OpTypePointer Function %_ptr_Uniform__struct_6
%int_0 = OpConstant %int 0
%uint_0 = OpConstant %uint 0
%2 = OpVariable %_ptr_Output_v4float Output
%7 = OpVariable %_ptr_Uniform__struct_5 Uniform
%1 = OpFunction %void None %10
%23 = OpLabel
%24 = OpVariable %_ptr_Function__ptr_Uniform__struct_5 Function
OpStore %24 %7
%27 = OpAccessChain %_ptr_Uniform_v4float %7 %int_0 %uint_0 %int_0
%28 = OpLoad %v4float %27
%29 = OpCopyObject %v4float %28
OpStore %2 %28
OpReturn
OpFunctionEnd
)";

  // Relax logical pointers to allow pointer allocations.
  SetAssembleOptions(SPV_TEXT_TO_BINARY_OPTION_PRESERVE_NUMERIC_IDS);
  ValidatorOptions()->relax_logical_pointer = true;
  SinglePassRunAndCheck<LocalSingleStoreElimPass>(before, after, true, true);
}

// Test that that an unused OpAccessChain between a store and a use does does
// not hinders the replacement of the use.  We need to check this because
// local-access-chain-convert does always remove the OpAccessChain instructions
// that become dead.

TEST_F(LocalSingleStoreElimTest,
       StoreElimWithUnusedInterveningAccessChainLoad) {
  // Last load of v is eliminated, but access chain load and store of v isn't
  //
  // #version 140
  //
  // in vec4 BaseColor;
  //
  // void main()
  // {
  //     vec4 v = BaseColor;
  //     float f = v[3];
  //     gl_FragColor = v * f;
  // }

  const std::string predefs =
      R"(OpCapability Shader
%1 = OpExtInstImport "GLSL.std.450"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %BaseColor %gl_FragColor
OpExecutionMode %main OriginUpperLeft
OpSource GLSL 140
OpName %main "main"
OpName %v "v"
OpName %BaseColor "BaseColor"
OpName %gl_FragColor "gl_FragColor"
%void = OpTypeVoid
%8 = OpTypeFunction %void
%float = OpTypeFloat 32
%v4float = OpTypeVector %float 4
%_ptr_Function_v4float = OpTypePointer Function %v4float
%_ptr_Input_v4float = OpTypePointer Input %v4float
%BaseColor = OpVariable %_ptr_Input_v4float Input
%_ptr_Function_float = OpTypePointer Function %float
%uint = OpTypeInt 32 0
%uint_3 = OpConstant %uint 3
%_ptr_Output_v4float = OpTypePointer Output %v4float
%gl_FragColor = OpVariable %_ptr_Output_v4float Output
)";

  const std::string before =
      R"(%main = OpFunction %void None %8
%17 = OpLabel
%v = OpVariable %_ptr_Function_v4float Function
%18 = OpLoad %v4float %BaseColor
OpStore %v %18
%19 = OpAccessChain %_ptr_Function_float %v %uint_3
%21 = OpLoad %v4float %v
OpStore %gl_FragColor %21
OpReturn
OpFunctionEnd
)";

  const std::string after =
      R"(%main = OpFunction %void None %8
%17 = OpLabel
%v = OpVariable %_ptr_Function_v4float Function
%18 = OpLoad %v4float %BaseColor
OpStore %v %18
%19 = OpAccessChain %_ptr_Function_float %v %uint_3
OpStore %gl_FragColor %18
OpReturn
OpFunctionEnd
)";

  SetAssembleOptions(SPV_TEXT_TO_BINARY_OPTION_PRESERVE_NUMERIC_IDS);
  SinglePassRunAndCheck<LocalSingleStoreElimPass>(predefs + before,
                                                  predefs + after, true, true);
}

TEST_F(LocalSingleStoreElimTest, VariablePointerTest) {
  // Check that the load of the first variable is still used and that the load
  // of the third variable is propagated.  The first load has to remain because
  // of the store to the variable pointer.
  const std::string text = R"(
; CHECK: [[v1:%\w+]] = OpVariable
; CHECK: [[v2:%\w+]] = OpVariable
; CHECK: [[v3:%\w+]] = OpVariable
; CHECK: [[ld1:%\w+]] = OpLoad %int [[v1]]
; CHECK: OpIAdd %int [[ld1]] %int_0
               OpCapability Shader
               OpCapability VariablePointers
          %1 = OpExtInstImport "GLSL.std.450"
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %2 "main"
               OpExecutionMode %2 LocalSize 1 1 1
               OpSource GLSL 450
               OpMemberDecorate %_struct_3 0 Offset 0
               OpMemberDecorate %_struct_3 1 Offset 4
       %void = OpTypeVoid
          %5 = OpTypeFunction %void
        %int = OpTypeInt 32 1
       %bool = OpTypeBool
  %_struct_3 = OpTypeStruct %int %int
%_ptr_Function__struct_3 = OpTypePointer Function %_struct_3
%_ptr_Function_int = OpTypePointer Function %int
       %true = OpConstantTrue %bool
      %int_0 = OpConstant %int 0
      %int_1 = OpConstant %int 1
         %13 = OpConstantNull %_struct_3
          %2 = OpFunction %void None %5
         %14 = OpLabel
         %15 = OpVariable %_ptr_Function_int Function
         %16 = OpVariable %_ptr_Function_int Function
         %17 = OpVariable %_ptr_Function_int Function
               OpStore %15 %int_1
               OpStore %17 %int_0
               OpSelectionMerge %18 None
               OpBranchConditional %true %19 %20
         %19 = OpLabel
               OpBranch %18
         %20 = OpLabel
               OpBranch %18
         %18 = OpLabel
         %21 = OpPhi %_ptr_Function_int %15 %19 %16 %20
               OpStore %21 %int_0
         %22 = OpLoad %int %15
         %23 = OpLoad %int %17
         %24 = OpIAdd %int %22 %23
               OpReturn
               OpFunctionEnd
  )";
  SinglePassRunAndMatch<LocalSingleStoreElimPass>(text, false);
}

TEST_F(LocalSingleStoreElimTest, DebugDeclareTest) {
  // If OpenCL.DebugInfo.100 enabled, check that store/load is still
  // optimized, DebugValue placed after the store and the associated
  // DebugDeclare is removed.
  const std::string text = R"(
               OpCapability Shader
          %1 = OpExtInstImport "OpenCL.DebugInfo.100"
               OpMemoryModel Logical GLSL450
               OpEntryPoint Fragment %MainPs "MainPs" %g_tColor %g_sAniso %in_var_TEXCOORD2 %out_var_SV_Target0
               OpExecutionMode %MainPs OriginUpperLeft
         %20 = OpString "foo.frag"
         %24 = OpString "PS_OUTPUT"
         %28 = OpString "float"
         %31 = OpString "vColor"
         %33 = OpString "PS_INPUT"
         %38 = OpString "vTextureCoords"
         %40 = OpString "@type.2d.image"
         %41 = OpString "type.2d.image"
         %43 = OpString "Texture2D.TemplateParam"
         %47 = OpString "src.MainPs"
         %51 = OpString "tc"
         %53 = OpString "ps_output"
         %56 = OpString "i"
         %58 = OpString "@type.sampler"
         %59 = OpString "type.sampler"
         %61 = OpString "g_sAniso"
         %63 = OpString "g_tColor"
               OpName %type_2d_image "type.2d.image"
               OpName %g_tColor "g_tColor"
               OpName %type_sampler "type.sampler"
               OpName %g_sAniso "g_sAniso"
               OpName %in_var_TEXCOORD2 "in.var.TEXCOORD2"
               OpName %out_var_SV_Target0 "out.var.SV_Target0"
               OpName %MainPs "MainPs"
               OpName %PS_INPUT "PS_INPUT"
               OpMemberName %PS_INPUT 0 "vTextureCoords"
               OpName %param_var_i "param.var.i"
               OpName %PS_OUTPUT "PS_OUTPUT"
               OpMemberName %PS_OUTPUT 0 "vColor"
               OpName %type_sampled_image "type.sampled.image"
               OpDecorate %in_var_TEXCOORD2 Location 0
               OpDecorate %out_var_SV_Target0 Location 0
               OpDecorate %g_tColor DescriptorSet 0
               OpDecorate %g_tColor Binding 0
               OpDecorate %g_sAniso DescriptorSet 0
               OpDecorate %g_sAniso Binding 1
        %int = OpTypeInt 32 1
      %int_0 = OpConstant %int 0
       %uint = OpTypeInt 32 0
    %uint_32 = OpConstant %uint 32
      %float = OpTypeFloat 32
%type_2d_image = OpTypeImage %float 2D 2 0 0 1 Unknown
%_ptr_UniformConstant_type_2d_image = OpTypePointer UniformConstant %type_2d_image
%type_sampler = OpTypeSampler
%_ptr_UniformConstant_type_sampler = OpTypePointer UniformConstant %type_sampler
    %v2float = OpTypeVector %float 2
%_ptr_Input_v2float = OpTypePointer Input %v2float
    %v4float = OpTypeVector %float 4
%_ptr_Output_v4float = OpTypePointer Output %v4float
       %void = OpTypeVoid
   %uint_128 = OpConstant %uint 128
     %uint_0 = OpConstant %uint 0
    %uint_64 = OpConstant %uint 64
         %65 = OpTypeFunction %void
   %PS_INPUT = OpTypeStruct %v2float
%_ptr_Function_PS_INPUT = OpTypePointer Function %PS_INPUT
  %PS_OUTPUT = OpTypeStruct %v4float
         %75 = OpTypeFunction %PS_OUTPUT %_ptr_Function_PS_INPUT
%_ptr_Function_PS_OUTPUT = OpTypePointer Function %PS_OUTPUT
%_ptr_Function_v2float = OpTypePointer Function %v2float
%type_sampled_image = OpTypeSampledImage %type_2d_image
%_ptr_Function_v4float = OpTypePointer Function %v4float
   %g_tColor = OpVariable %_ptr_UniformConstant_type_2d_image UniformConstant
   %g_sAniso = OpVariable %_ptr_UniformConstant_type_sampler UniformConstant
%in_var_TEXCOORD2 = OpVariable %_ptr_Input_v2float Input
%out_var_SV_Target0 = OpVariable %_ptr_Output_v4float Output
         %39 = OpExtInst %void %1 DebugInfoNone
         %55 = OpExtInst %void %1 DebugExpression
         %22 = OpExtInst %void %1 DebugSource %20
         %23 = OpExtInst %void %1 DebugCompilationUnit 1 4 %22 HLSL
         %26 = OpExtInst %void %1 DebugTypeComposite %24 Structure %22 10 1 %23 %24 %uint_128 FlagIsProtected|FlagIsPrivate %27
         %29 = OpExtInst %void %1 DebugTypeBasic %28 %uint_32 Float
         %30 = OpExtInst %void %1 DebugTypeVector %29 4
         %27 = OpExtInst %void %1 DebugTypeMember %31 %30 %22 12 5 %26 %uint_0 %uint_128 FlagIsProtected|FlagIsPrivate
         %35 = OpExtInst %void %1 DebugTypeComposite %33 Structure %22 5 1 %23 %33 %uint_64 FlagIsProtected|FlagIsPrivate %36
         %37 = OpExtInst %void %1 DebugTypeVector %29 2
         %36 = OpExtInst %void %1 DebugTypeMember %38 %37 %22 7 5 %35 %uint_0 %uint_64 FlagIsProtected|FlagIsPrivate
         %42 = OpExtInst %void %1 DebugTypeComposite %40 Class %22 0 0 %23 %41 %39 FlagIsProtected|FlagIsPrivate
         %44 = OpExtInst %void %1 DebugTypeTemplateParameter %43 %29 %39 %22 0 0
         %45 = OpExtInst %void %1 DebugTypeTemplate %42 %44
         %46 = OpExtInst %void %1 DebugTypeFunction FlagIsProtected|FlagIsPrivate %26 %35
         %48 = OpExtInst %void %1 DebugFunction %47 %46 %22 15 1 %23 %47 FlagIsProtected|FlagIsPrivate 16 %39
         %50 = OpExtInst %void %1 DebugLexicalBlock %22 16 1 %48
         %52 = OpExtInst %void %1 DebugLocalVariable %51 %37 %22 19 12 %50 FlagIsLocal
         %54 = OpExtInst %void %1 DebugLocalVariable %53 %26 %22 17 15 %50 FlagIsLocal
         %57 = OpExtInst %void %1 DebugLocalVariable %56 %35 %22 15 29 %48 FlagIsLocal 1
         %60 = OpExtInst %void %1 DebugTypeComposite %58 Structure %22 0 0 %23 %59 %39 FlagIsProtected|FlagIsPrivate
         %62 = OpExtInst %void %1 DebugGlobalVariable %61 %60 %22 3 14 %23 %61 %g_sAniso FlagIsDefinition
         %64 = OpExtInst %void %1 DebugGlobalVariable %63 %42 %22 1 11 %23 %63 %g_tColor FlagIsDefinition
     %MainPs = OpFunction %void None %65
         %66 = OpLabel
        %114 = OpExtInst %void %1 DebugScope %50
         %98 = OpVariable %_ptr_Function_PS_OUTPUT Function
         %99 = OpVariable %_ptr_Function_v2float Function
        %115 = OpExtInst %void %1 DebugNoScope
        %100 = OpVariable %_ptr_Function_PS_OUTPUT Function
%param_var_i = OpVariable %_ptr_Function_PS_INPUT Function
         %70 = OpLoad %v2float %in_var_TEXCOORD2
         %71 = OpCompositeConstruct %PS_INPUT %70
               OpStore %param_var_i %71
        %116 = OpExtInst %void %1 DebugScope %48
        %102 = OpExtInst %void %1 DebugDeclare %57 %param_var_i %55
        %117 = OpExtInst %void %1 DebugScope %50
        %103 = OpExtInst %void %1 DebugDeclare %54 %98 %55
               OpLine %20 19 17
        %104 = OpAccessChain %_ptr_Function_v2float %param_var_i %int_0
        %105 = OpLoad %v2float %104
               OpLine %20 19 12
               OpStore %99 %105
        %106 = OpExtInst %void %1 DebugDeclare %52 %99 %55
;CHECK-NOT: %106 = OpExtInst %void %1 DebugDeclare %52 %99 %55
;CHECK:     %119 = OpExtInst %void %1 DebugValue %52 %105 %55
               OpLine %20 20 26
        %107 = OpLoad %type_2d_image %g_tColor
               OpLine %20 20 46
        %108 = OpLoad %type_sampler %g_sAniso
               OpLine %20 20 57
        %109 = OpLoad %v2float %99
;CHECK-NOT: %109 = OpLoad %v2float %99
               OpLine %20 20 26
        %110 = OpSampledImage %type_sampled_image %107 %108
        %111 = OpImageSampleImplicitLod %v4float %110 %109 None
;CHECK-NOT: %111 = OpImageSampleImplicitLod %v4float %110 %109 None
;CHECK:     %111 = OpImageSampleImplicitLod %v4float %110 %105 None
               OpLine %20 20 5
        %112 = OpAccessChain %_ptr_Function_v4float %98 %int_0
               OpStore %112 %111
               OpLine %20 21 12
        %113 = OpLoad %PS_OUTPUT %98
               OpLine %20 21 5
               OpStore %100 %113
        %118 = OpExtInst %void %1 DebugNoScope
         %73 = OpLoad %PS_OUTPUT %100
         %74 = OpCompositeExtract %v4float %73 0
               OpStore %out_var_SV_Target0 %74
               OpReturn
               OpFunctionEnd
  )";

  SetAssembleOptions(SPV_TEXT_TO_BINARY_OPTION_PRESERVE_NUMERIC_IDS);
  SinglePassRunAndMatch<LocalSingleStoreElimPass>(text, false);
}

TEST_F(LocalSingleStoreElimTest, DebugValueTest) {
  // If OpenCL.DebugInfo.100 enabled, check that store/load is still
  // optimized, DebugValue placed after the store and the associated
  // DebugValue Deref is removed.
  const std::string text = R"(
                 OpCapability Shader
          %1 = OpExtInstImport "OpenCL.DebugInfo.100"
               OpMemoryModel Logical GLSL450
               OpEntryPoint Fragment %MainPs "MainPs" %g_tColor %g_sAniso %in_var_TEXCOORD2 %out_var_SV_Target0
               OpExecutionMode %MainPs OriginUpperLeft
          %7 = OpString "foo.frag"
          %8 = OpString "PS_OUTPUT"
          %9 = OpString "float"
         %10 = OpString "vColor"
         %11 = OpString "PS_INPUT"
         %12 = OpString "vTextureCoords"
         %13 = OpString "@type.2d.image"
         %14 = OpString "type.2d.image"
         %15 = OpString "Texture2D.TemplateParam"
         %16 = OpString "src.MainPs"
         %17 = OpString "tc"
         %18 = OpString "ps_output"
         %19 = OpString "i"
         %20 = OpString "@type.sampler"
         %21 = OpString "type.sampler"
         %22 = OpString "g_sAniso"
         %23 = OpString "g_tColor"
               OpName %type_2d_image "type.2d.image"
               OpName %g_tColor "g_tColor"
               OpName %type_sampler "type.sampler"
               OpName %g_sAniso "g_sAniso"
               OpName %in_var_TEXCOORD2 "in.var.TEXCOORD2"
               OpName %out_var_SV_Target0 "out.var.SV_Target0"
               OpName %MainPs "MainPs"
               OpName %PS_INPUT "PS_INPUT"
               OpMemberName %PS_INPUT 0 "vTextureCoords"
               OpName %param_var_i "param.var.i"
               OpName %PS_OUTPUT "PS_OUTPUT"
               OpMemberName %PS_OUTPUT 0 "vColor"
               OpName %type_sampled_image "type.sampled.image"
               OpDecorate %in_var_TEXCOORD2 Location 0
               OpDecorate %out_var_SV_Target0 Location 0
               OpDecorate %g_tColor DescriptorSet 0
               OpDecorate %g_tColor Binding 0
               OpDecorate %g_sAniso DescriptorSet 0
               OpDecorate %g_sAniso Binding 1
        %int = OpTypeInt 32 1
      %int_0 = OpConstant %int 0
       %uint = OpTypeInt 32 0
    %uint_32 = OpConstant %uint 32
      %float = OpTypeFloat 32
%type_2d_image = OpTypeImage %float 2D 2 0 0 1 Unknown
%_ptr_UniformConstant_type_2d_image = OpTypePointer UniformConstant %type_2d_image
%type_sampler = OpTypeSampler
%_ptr_UniformConstant_type_sampler = OpTypePointer UniformConstant %type_sampler
    %v2float = OpTypeVector %float 2
%_ptr_Input_v2float = OpTypePointer Input %v2float
    %v4float = OpTypeVector %float 4
%_ptr_Output_v4float = OpTypePointer Output %v4float
       %void = OpTypeVoid
   %uint_128 = OpConstant %uint 128
     %uint_0 = OpConstant %uint 0
    %uint_64 = OpConstant %uint 64
         %45 = OpTypeFunction %void
   %PS_INPUT = OpTypeStruct %v2float
%_ptr_Function_PS_INPUT = OpTypePointer Function %PS_INPUT
  %PS_OUTPUT = OpTypeStruct %v4float
         %47 = OpTypeFunction %PS_OUTPUT %_ptr_Function_PS_INPUT
%_ptr_Function_PS_OUTPUT = OpTypePointer Function %PS_OUTPUT
%_ptr_Function_v2float = OpTypePointer Function %v2float
%type_sampled_image = OpTypeSampledImage %type_2d_image
%_ptr_Function_v4float = OpTypePointer Function %v4float
   %g_tColor = OpVariable %_ptr_UniformConstant_type_2d_image UniformConstant
   %g_sAniso = OpVariable %_ptr_UniformConstant_type_sampler UniformConstant
%in_var_TEXCOORD2 = OpVariable %_ptr_Input_v2float Input
%out_var_SV_Target0 = OpVariable %_ptr_Output_v4float Output
         %51 = OpExtInst %void %1 DebugInfoNone
         %52 = OpExtInst %void %1 DebugExpression
         %53 = OpExtInst %void %1 DebugOperation Deref
         %54 = OpExtInst %void %1 DebugExpression %53
         %55 = OpExtInst %void %1 DebugSource %7
         %56 = OpExtInst %void %1 DebugCompilationUnit 1 4 %55 HLSL
         %57 = OpExtInst %void %1 DebugTypeComposite %8 Structure %55 10 1 %56 %8 %uint_128 FlagIsProtected|FlagIsPrivate %58
         %59 = OpExtInst %void %1 DebugTypeBasic %9 %uint_32 Float
         %60 = OpExtInst %void %1 DebugTypeVector %59 4
         %58 = OpExtInst %void %1 DebugTypeMember %10 %60 %55 12 5 %57 %uint_0 %uint_128 FlagIsProtected|FlagIsPrivate
         %61 = OpExtInst %void %1 DebugTypeComposite %11 Structure %55 5 1 %56 %11 %uint_64 FlagIsProtected|FlagIsPrivate %62
         %63 = OpExtInst %void %1 DebugTypeVector %59 2
         %62 = OpExtInst %void %1 DebugTypeMember %12 %63 %55 7 5 %61 %uint_0 %uint_64 FlagIsProtected|FlagIsPrivate
         %64 = OpExtInst %void %1 DebugTypeComposite %13 Class %55 0 0 %56 %14 %51 FlagIsProtected|FlagIsPrivate
         %65 = OpExtInst %void %1 DebugTypeTemplateParameter %15 %59 %51 %55 0 0
         %66 = OpExtInst %void %1 DebugTypeTemplate %64 %65
         %67 = OpExtInst %void %1 DebugTypeFunction FlagIsProtected|FlagIsPrivate %57 %61
         %68 = OpExtInst %void %1 DebugFunction %16 %67 %55 15 1 %56 %16 FlagIsProtected|FlagIsPrivate 16 %51
         %69 = OpExtInst %void %1 DebugLexicalBlock %55 16 1 %68
         %70 = OpExtInst %void %1 DebugLocalVariable %17 %63 %55 19 12 %69 FlagIsLocal
         %71 = OpExtInst %void %1 DebugLocalVariable %18 %57 %55 17 15 %69 FlagIsLocal
         %72 = OpExtInst %void %1 DebugLocalVariable %19 %61 %55 15 29 %68 FlagIsLocal 1
         %73 = OpExtInst %void %1 DebugTypeComposite %20 Structure %55 0 0 %56 %21 %51 FlagIsProtected|FlagIsPrivate
         %74 = OpExtInst %void %1 DebugGlobalVariable %22 %73 %55 3 14 %56 %22 %g_sAniso FlagIsDefinition
         %75 = OpExtInst %void %1 DebugGlobalVariable %23 %64 %55 1 11 %56 %23 %g_tColor FlagIsDefinition
     %MainPs = OpFunction %void None %45
         %76 = OpLabel
        %101 = OpExtInst %void %1 DebugScope %69
         %78 = OpVariable %_ptr_Function_PS_OUTPUT Function
         %79 = OpVariable %_ptr_Function_v2float Function
        %102 = OpExtInst %void %1 DebugNoScope
         %81 = OpVariable %_ptr_Function_PS_OUTPUT Function
%param_var_i = OpVariable %_ptr_Function_PS_INPUT Function
         %82 = OpLoad %v2float %in_var_TEXCOORD2
         %83 = OpCompositeConstruct %PS_INPUT %82
               OpStore %param_var_i %83
        %103 = OpExtInst %void %1 DebugScope %68
         %85 = OpExtInst %void %1 DebugDeclare %72 %param_var_i %52
        %104 = OpExtInst %void %1 DebugScope %69
         %87 = OpExtInst %void %1 DebugDeclare %71 %78 %52
               OpLine %7 19 17
         %88 = OpAccessChain %_ptr_Function_v2float %param_var_i %int_0
         %89 = OpLoad %v2float %88
               OpLine %7 19 12
               OpStore %79 %89
         %90 = OpExtInst %void %1 DebugValue %70 %79 %54
;CHECK-NOT: %90 = OpExtInst %void %1 DebugValue %70 %79 %54
;CHECK:    %106 = OpExtInst %void %1 DebugValue %70 %89 %52
               OpLine %7 20 26
         %91 = OpLoad %type_2d_image %g_tColor
               OpLine %7 20 46
         %92 = OpLoad %type_sampler %g_sAniso
               OpLine %7 20 57
         %93 = OpLoad %v2float %79
;CHECK-NOT: %93 = OpLoad %v2float %79
               OpLine %7 20 26
         %94 = OpSampledImage %type_sampled_image %91 %92
         %95 = OpImageSampleImplicitLod %v4float %94 %93 None
;CHECK-NOT: %95 = OpImageSampleImplicitLod %v4float %94 %93 None
;CHECK:     %95 = OpImageSampleImplicitLod %v4float %94 %89 None
               OpLine %7 20 5
         %96 = OpAccessChain %_ptr_Function_v4float %78 %int_0
               OpStore %96 %95
               OpLine %7 21 12
         %97 = OpLoad %PS_OUTPUT %78
               OpLine %7 21 5
               OpStore %81 %97
        %105 = OpExtInst %void %1 DebugNoScope
         %99 = OpLoad %PS_OUTPUT %81
        %100 = OpCompositeExtract %v4float %99 0
               OpStore %out_var_SV_Target0 %100
               OpReturn
               OpFunctionEnd
  )";

  SetAssembleOptions(SPV_TEXT_TO_BINARY_OPTION_PRESERVE_NUMERIC_IDS);
  SinglePassRunAndMatch<LocalSingleStoreElimPass>(text, false);
}

TEST_F(LocalSingleStoreElimTest, DebugDeclareForFunctionParameter) {
  // We have to add DebugValue for DebugDeclare regardless of the scope
  // if it declares a function parameter.
  const std::string text = R"(
               OpCapability Shader
          %1 = OpExtInstImport "OpenCL.DebugInfo.100"
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
          %3 = OpString "fn_call.hlsl"
          %4 = OpString "int"
          %5 = OpString ""
          %6 = OpString "x"
          %7 = OpString "A"
          %8 = OpString "main"
               OpName %type_StructuredBuffer_int "type.StructuredBuffer.int"
               OpName %data "data"
               OpName %main "main"
               OpDecorate %data DescriptorSet 0
               OpDecorate %data Binding 0
               OpDecorate %_runtimearr_int ArrayStride 4
               OpMemberDecorate %type_StructuredBuffer_int 0 Offset 0
               OpMemberDecorate %type_StructuredBuffer_int 0 NonWritable
               OpDecorate %type_StructuredBuffer_int BufferBlock
        %int = OpTypeInt 32 1
       %uint = OpTypeInt 32 0
     %uint_0 = OpConstant %uint 0
    %uint_32 = OpConstant %uint 32
%_runtimearr_int = OpTypeRuntimeArray %int
%type_StructuredBuffer_int = OpTypeStruct %_runtimearr_int
%_ptr_Uniform_type_StructuredBuffer_int = OpTypePointer Uniform %type_StructuredBuffer_int
       %void = OpTypeVoid
         %18 = OpTypeFunction %void
%_ptr_Function_int = OpTypePointer Function %int
%_ptr_Uniform_int = OpTypePointer Uniform %int
       %data = OpVariable %_ptr_Uniform_type_StructuredBuffer_int Uniform
         %21 = OpExtInst %void %1 DebugInfoNone
         %22 = OpExtInst %void %1 DebugExpression
         %23 = OpExtInst %void %1 DebugTypeBasic %4 %uint_32 Signed
         %24 = OpExtInst %void %1 DebugTypeFunction FlagIsProtected|FlagIsPrivate %23 %23
         %25 = OpExtInst %void %1 DebugSource %3
         %26 = OpExtInst %void %1 DebugCompilationUnit 1 4 %25 HLSL
         %27 = OpExtInst %void %1 DebugFunction %7 %24 %25 17 1 %26 %5 FlagIsProtected|FlagIsPrivate 18 %21
         %28 = OpExtInst %void %1 DebugLocalVariable %6 %23 %25 17 11 %27 FlagIsLocal 1
         %29 = OpExtInst %void %1 DebugTypeFunction FlagIsProtected|FlagIsPrivate %void
         %30 = OpExtInst %void %1 DebugFunction %8 %29 %25 25 1 %26 %5 FlagIsProtected|FlagIsPrivate 25 %21
         %31 = OpExtInst %void %1 DebugLexicalBlock %25 25 13 %30
         %32 = OpExtInst %void %1 DebugInlinedAt 26 %31
;CHECK: [[fn_param:%\w+]] = OpExtInst %void [[ext:%\w+]] DebugLocalVariable
;CHECK: [[bb:%\w+]] = OpExtInst %void [[ext]] DebugLexicalBlock
;CHECK: [[inlined_at:%\w+]] = OpExtInst %void [[ext]] DebugInlinedAt 26 [[bb]]
       %main = OpFunction %void None %18
         %bb = OpLabel
         %33 = OpExtInst %void %1 DebugScope %31
         %34 = OpVariable %_ptr_Function_int Function
               OpLine %3 26 15
         %35 = OpAccessChain %_ptr_Uniform_int %data %uint_0 %uint_0
         %36 = OpLoad %int %35
;CHECK: DebugScope [[bb]]
;CHECK: OpLine {{%\w+}} 26 15
;CHECK: OpStore {{%\w+}} [[val:%\w+]]
               OpStore %34 %36
         %37 = OpExtInst %void %1 DebugScope %27 %32
         %38 = OpExtInst %void %1 DebugDeclare %28 %34 %22
;CHECK: DebugScope {{%\w+}} [[inlined_at:%\w+]]
;CHECK: DebugValue [[fn_param]] [[val]]
               OpReturn
               OpFunctionEnd
  )";

  SetAssembleOptions(SPV_TEXT_TO_BINARY_OPTION_PRESERVE_NUMERIC_IDS);
  SinglePassRunAndMatch<LocalSingleStoreElimPass>(text, false);
}

// TODO(greg-lunarg): Add tests to verify handling of these cases:
//
//    Other types
//    Others?

}  // namespace
}  // namespace opt
}  // namespace spvtools
