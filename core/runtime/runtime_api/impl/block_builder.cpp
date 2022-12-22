/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/block_builder.hpp"

#include "runtime/common/executor.hpp"

namespace kagome::runtime {

  BlockBuilderImpl::BlockBuilderImpl(std::shared_ptr<Executor> executor)
      : executor_{std::move(executor)} {
    BOOST_ASSERT(executor_);
  }

  outcome::result<primitives::ApplyExtrinsicResult>
  BlockBuilderImpl::apply_extrinsic(RuntimeEnvironment &env,
                                    const primitives::Extrinsic &extrinsic) {
    return executor_->call<primitives::ApplyExtrinsicResult>(
        env, "BlockBuilder_apply_extrinsic", extrinsic);
  }

  outcome::result<primitives::BlockHeader> BlockBuilderImpl::finalize_block(
      RuntimeEnvironment &env) {
    return executor_->call<primitives::BlockHeader>(
        env, "BlockBuilder_finalize_block");
  }

  outcome::result<std::vector<primitives::Extrinsic>>
  BlockBuilderImpl::inherent_extrinsics(RuntimeEnvironment &env,
                                        const primitives::InherentData &data) {
    return executor_->call<std::vector<primitives::Extrinsic>>(
        env, "BlockBuilder_inherent_extrinsics", data);
  }

  outcome::result<primitives::CheckInherentsResult>
  BlockBuilderImpl::check_inherents(const primitives::Block &block,
                                    const primitives::InherentData &data) {
    return executor_->callAt<primitives::CheckInherentsResult>(
        block.header.parent_hash, "BlockBuilder_check_inherents", block, data);
  }

  outcome::result<common::Hash256> BlockBuilderImpl::random_seed(
      const primitives::BlockHash &block) {
    return executor_->callAt<common::Hash256>(block,
                                              "BlockBuilder_random_seed");
  }

}  // namespace kagome::runtime
