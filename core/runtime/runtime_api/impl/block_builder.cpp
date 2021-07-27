/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/block_builder.hpp"

#include "runtime/executor.hpp"

namespace kagome::runtime {

  BlockBuilderImpl::BlockBuilderImpl(std::shared_ptr<Executor> executor)
      : executor_{std::move(executor)} {
    BOOST_ASSERT(executor_);
  }

  outcome::result<primitives::ApplyResult> BlockBuilderImpl::apply_extrinsic(
      const primitives::Extrinsic &extrinsic) {
    return executor_->persistentCallAtLatest<primitives::ApplyResult>(
        "BlockBuilder_apply_extrinsic", extrinsic);
  }

  outcome::result<primitives::BlockHeader> BlockBuilderImpl::finalise_block() {
    return executor_->persistentCallAtLatest<primitives::BlockHeader>(
        "BlockBuilder_finalise_block");
  }

  outcome::result<std::vector<primitives::Extrinsic>>
  BlockBuilderImpl::inherent_extrinsics(const primitives::InherentData &data) {
    return executor_->callAtLatest<std::vector<primitives::Extrinsic>>(
        "BlockBuilder_inherent_extrinsics", data);
  }

  outcome::result<primitives::CheckInherentsResult>
  BlockBuilderImpl::check_inherents(const primitives::Block &block,
                                    const primitives::InherentData &data) {
    return executor_->callAtLatest<primitives::CheckInherentsResult>(
        "BlockBuilder_check_inherents", block, data);
  }

  outcome::result<common::Hash256> BlockBuilderImpl::random_seed() {
    return executor_->callAtLatest<common::Hash256>("BlockBuilder_random_seed");
  }

}  // namespace kagome::runtime::wavm
