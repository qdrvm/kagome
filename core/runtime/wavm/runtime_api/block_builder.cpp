/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/runtime_api/block_builder.hpp"

#include "runtime/wavm/executor.hpp"

namespace kagome::runtime::wavm {

  WavmBlockBuilder::WavmBlockBuilder(std::shared_ptr<Executor> executor)
      : executor_{std::move(executor)} {
    BOOST_ASSERT(executor_);
  }

  outcome::result<primitives::ApplyResult> WavmBlockBuilder::apply_extrinsic(
      const primitives::Extrinsic &extrinsic) {
    return executor_->persistentCallAtLatest<primitives::ApplyResult>(
        "BlockBuilder_apply_extrinsic", extrinsic);
  }

  outcome::result<primitives::BlockHeader> WavmBlockBuilder::finalise_block() {
    return executor_->persistentCallAtLatest<primitives::BlockHeader>(
        "BlockBuilder_finalise_block");
  }

  outcome::result<std::vector<primitives::Extrinsic>>
  WavmBlockBuilder::inherent_extrinsics(const primitives::InherentData &data) {
    return executor_->callAtLatest<std::vector<primitives::Extrinsic>>(
        "BlockBuilder_inherent_extrinsics", data);
  }

  outcome::result<primitives::CheckInherentsResult>
  WavmBlockBuilder::check_inherents(const primitives::Block &block,
                                    const primitives::InherentData &data) {
    return executor_->callAtLatest<primitives::CheckInherentsResult>(
        "BlockBuilder_check_inherents", block, data);
  }

  outcome::result<common::Hash256> WavmBlockBuilder::random_seed() {
    return executor_->callAtLatest<common::Hash256>("BlockBuilder_random_seed");
  }

}  // namespace kagome::runtime::wavm
