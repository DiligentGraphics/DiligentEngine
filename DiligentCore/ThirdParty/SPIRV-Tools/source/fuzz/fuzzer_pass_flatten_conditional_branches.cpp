// Copyright (c) 2020 Google LLC
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

#include "source/fuzz/fuzzer_pass_flatten_conditional_branches.h"

#include "source/fuzz/comparator_deep_blocks_first.h"
#include "source/fuzz/instruction_descriptor.h"
#include "source/fuzz/transformation_flatten_conditional_branch.h"

namespace spvtools {
namespace fuzz {

// A fuzzer pass that randomly selects conditional branches to flatten and
// flattens them, if possible.
FuzzerPassFlattenConditionalBranches::FuzzerPassFlattenConditionalBranches(
    opt::IRContext* ir_context, TransformationContext* transformation_context,
    FuzzerContext* fuzzer_context,
    protobufs::TransformationSequence* transformations)
    : FuzzerPass(ir_context, transformation_context, fuzzer_context,
                 transformations) {}

FuzzerPassFlattenConditionalBranches::~FuzzerPassFlattenConditionalBranches() =
    default;

void FuzzerPassFlattenConditionalBranches::Apply() {
  for (auto& function : *GetIRContext()->module()) {
    // Get all the selection headers that we want to flatten. We need to collect
    // all of them first, because, since we are changing the structure of the
    // module, it's not safe to modify them while iterating.
    std::vector<opt::BasicBlock*> selection_headers;

    for (auto& block : function) {
      // Randomly decide whether to consider this block.
      if (!GetFuzzerContext()->ChoosePercentage(
              GetFuzzerContext()->GetChanceOfFlatteningConditionalBranch())) {
        continue;
      }

      // Only consider this block if it is the header of a conditional.
      if (block.GetMergeInst() &&
          block.GetMergeInst()->opcode() == SpvOpSelectionMerge &&
          block.terminator()->opcode() == SpvOpBranchConditional) {
        selection_headers.emplace_back(&block);
      }
    }

    // Sort the headers so that those that are more deeply nested are considered
    // first, possibly enabling outer conditionals to be flattened.
    std::sort(selection_headers.begin(), selection_headers.end(),
              ComparatorDeepBlocksFirst(GetIRContext()));

    // Apply the transformation to the headers which can be flattened.
    for (auto header : selection_headers) {
      // Make a set to keep track of the instructions that need fresh ids.
      std::set<opt::Instruction*> instructions_that_need_ids;

      // Do not consider this header if the conditional cannot be flattened.
      if (!TransformationFlattenConditionalBranch::
              GetProblematicInstructionsIfConditionalCanBeFlattened(
                  GetIRContext(), header, &instructions_that_need_ids)) {
        continue;
      }

      // Some instructions will require to be enclosed inside conditionals
      // because they have side effects (for example, loads and stores). Some of
      // this have no result id, so we require instruction descriptors to
      // identify them. Each of them is associated with the necessary ids for it
      // via a SideEffectWrapperInfo message.
      std::vector<protobufs::SideEffectWrapperInfo> wrappers_info;

      for (auto instruction : instructions_that_need_ids) {
        protobufs::SideEffectWrapperInfo wrapper_info;
        *wrapper_info.mutable_instruction() =
            MakeInstructionDescriptor(GetIRContext(), instruction);
        wrapper_info.set_merge_block_id(GetFuzzerContext()->GetFreshId());
        wrapper_info.set_execute_block_id(GetFuzzerContext()->GetFreshId());

        // If the instruction has a non-void result id, we need to define more
        // fresh ids and provide an id of the suitable type whose value can be
        // copied in order to create a placeholder id.
        if (TransformationFlattenConditionalBranch::InstructionNeedsPlaceholder(
                GetIRContext(), *instruction)) {
          wrapper_info.set_actual_result_id(GetFuzzerContext()->GetFreshId());
          wrapper_info.set_alternative_block_id(
              GetFuzzerContext()->GetFreshId());
          wrapper_info.set_placeholder_result_id(
              GetFuzzerContext()->GetFreshId());

          // The id will be a zero constant if the type allows it, and an
          // OpUndef otherwise. We want to avoid using OpUndef, if possible, to
          // avoid undefined behaviour in the module as much as possible.
          if (fuzzerutil::CanCreateConstant(GetIRContext(),
                                            instruction->type_id())) {
            wrapper_info.set_value_to_copy_id(
                FindOrCreateZeroConstant(instruction->type_id(), true));
          } else {
            wrapper_info.set_value_to_copy_id(
                FindOrCreateGlobalUndef(instruction->type_id()));
          }
        }

        wrappers_info.emplace_back(wrapper_info);
      }

      // Apply the transformation, evenly choosing whether to lay out the true
      // branch or the false branch first.
      ApplyTransformation(TransformationFlattenConditionalBranch(
          header->id(), GetFuzzerContext()->ChooseEven(), wrappers_info));
    }
  }
}

}  // namespace fuzz
}  // namespace spvtools
