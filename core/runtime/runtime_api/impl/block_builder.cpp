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
    // https://github.com/paritytech/substrate/blob/ea4fbcb84cf3883123d1341068e1e70310ab2049/client/block-builder/src/lib.rs#L285
    // `create_inherents` should not change any state, to ensure this we always
    // rollback the transaction.
    //
    // `rollbackTransaction` is called by `Executor::call`.
    //
    // Can't use `EphemeralTrieBatch`, because we must `call` in context of
    // `env.storage_provider`s `PersistentTrieBatch`.
    OUTCOME_TRY(env.storage_provider->startTransaction());
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
