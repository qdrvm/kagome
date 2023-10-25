/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/block_builder.hpp"

#include "common/final_action.hpp"
#include "runtime/executor.hpp"
#include "runtime/trie_storage_provider.hpp"

namespace kagome::runtime {

  BlockBuilderImpl::BlockBuilderImpl(std::shared_ptr<Executor> executor)
      : executor_{std::move(executor)} {
    BOOST_ASSERT(executor_);
  }

  outcome::result<primitives::ApplyExtrinsicResult>
  BlockBuilderImpl::apply_extrinsic(RuntimeContext &ctx,
                                    const primitives::Extrinsic &extrinsic) {
    // https://github.com/paritytech/substrate/blob/943c520aa78fcfaf3509790009ad062e8d4c6990/client/block-builder/src/lib.rs#L204-L237
    OUTCOME_TRY(ctx.module_instance->getEnvironment()
                    .storage_provider->startTransaction());
    auto should_rollback = true;
    common::FinalAction rollback([&] {
      if (should_rollback) {
        std::ignore = ctx.module_instance->getEnvironment()
                          .storage_provider->rollbackTransaction();
      }
    });
    OUTCOME_TRY(result,
                executor_->decodedCallWithCtx<primitives::ApplyExtrinsicResult>(
                    ctx, "BlockBuilder_apply_extrinsic", extrinsic));
    if (auto ok = boost::get<primitives::DispatchOutcome>(&result);
        ok and boost::get<primitives::DispatchSuccess>(ok)) {
      should_rollback = false;
      OUTCOME_TRY(ctx.module_instance->getEnvironment()
                      .storage_provider->commitTransaction());
    }
    return result;
  }

  outcome::result<primitives::BlockHeader> BlockBuilderImpl::finalize_block(
      RuntimeContext &ctx) {
    return executor_->decodedCallWithCtx<primitives::BlockHeader>(
        ctx, "BlockBuilder_finalize_block");
  }

  outcome::result<std::vector<primitives::Extrinsic>>
  BlockBuilderImpl::inherent_extrinsics(RuntimeContext &ctx,
                                        const primitives::InherentData &data) {
    // https://github.com/paritytech/substrate/blob/ea4fbcb84cf3883123d1341068e1e70310ab2049/client/block-builder/src/lib.rs#L285
    // `create_inherents` should not change any state, to ensure this we always
    // rollback the transaction.
    //
    // Can't use `EphemeralTrieBatch`, because we must `call` in context of
    // `env.storage_provider`s `PersistentTrieBatch`.
    OUTCOME_TRY(ctx.module_instance->getEnvironment()
                    .storage_provider->startTransaction());
    common::FinalAction rollback([&] {
      std::ignore = ctx.module_instance->getEnvironment()
                        .storage_provider->rollbackTransaction();
    });
    OUTCOME_TRY(
        result,
        executor_->decodedCallWithCtx<std::vector<primitives::Extrinsic>>(
            ctx, "BlockBuilder_inherent_extrinsics", data));
    return result;
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
