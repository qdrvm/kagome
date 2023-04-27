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
    // https://github.com/paritytech/substrate/blob/943c520aa78fcfaf3509790009ad062e8d4c6990/client/block-builder/src/lib.rs#L204-L237
    OUTCOME_TRY(env.storage_provider->startTransaction());
    auto should_rollback = true;
    auto rollback = gsl::finally([&] {
      if (should_rollback) {
        std::ignore = env.storage_provider->rollbackTransaction();
      }
    });
    OUTCOME_TRY(result,
                executor_->call<primitives::ApplyExtrinsicResult>(
                    env, "BlockBuilder_apply_extrinsic", extrinsic));
    if (auto ok = boost::get<primitives::DispatchOutcome>(&result);
        ok and boost::get<primitives::DispatchSuccess>(ok)) {
      should_rollback = false;
      OUTCOME_TRY(env.storage_provider->commitTransaction());
    }
    return std::move(result);
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
    // Can't use `EphemeralTrieBatch`, because we must `call` in context of
    // `env.storage_provider`s `PersistentTrieBatch`.
    OUTCOME_TRY(env.storage_provider->startTransaction());
    auto rollback = gsl::finally(
        [&] { std::ignore = env.storage_provider->rollbackTransaction(); });
    OUTCOME_TRY(result,
                executor_->call<std::vector<primitives::Extrinsic>>(
                    env, "BlockBuilder_inherent_extrinsics", data));
    return std::move(result);
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
