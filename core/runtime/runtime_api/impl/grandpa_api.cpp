/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/grandpa_api.hpp"

#include "runtime/executor.hpp"

namespace kagome::runtime {

  GrandpaApiImpl::GrandpaApiImpl(
      std::shared_ptr<blockchain::BlockHeaderRepository> block_header_repo,
      std::shared_ptr<Executor> executor)
      : block_header_repo_{std::move(block_header_repo)},
        executor_{std::move(executor)} {
    BOOST_ASSERT(block_header_repo_);
    BOOST_ASSERT(executor_);
  }

  outcome::result<std::optional<GrandpaApi::ScheduledChange>>
  GrandpaApiImpl::pending_change(const primitives::BlockHash &block,
                                 const Digest &digest) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    return executor_->call<std::optional<ScheduledChange>>(
        ctx, "GrandpaApi_pending_change", digest);
  }

  outcome::result<std::optional<GrandpaApi::ForcedChange>>
  GrandpaApiImpl::forced_change(const primitives::BlockHash &block,
                                const Digest &digest) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    return executor_->call<std::optional<ForcedChange>>(
        ctx, "GrandpaApi_forced_change", digest);
  }

  outcome::result<GrandpaApi::AuthorityList> GrandpaApiImpl::authorities(
      const primitives::BlockId &block_id) {
    OUTCOME_TRY(block_hash, block_header_repo_->getHashById(block_id));

    OUTCOME_TRY(
        ref,
        authorities_.get_else(
            block_hash, [&]() -> outcome::result<primitives::AuthorityList> {
              OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block_hash));
              return executor_->call<AuthorityList>(
                  ctx, "GrandpaApi_grandpa_authorities");
            }));
    return *ref;
  }

  outcome::result<primitives::AuthoritySetId> GrandpaApiImpl::current_set_id(
      const primitives::BlockHash &block_hash) {
    OUTCOME_TRY(
        ref,
        set_id_.get_else(
            block_hash, [&]() -> outcome::result<primitives::AuthoritySetId> {
              OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block_hash));
              return executor_->call<primitives::AuthoritySetId>(
                  ctx, "GrandpaApi_current_set_id");
            }));
    return *ref;
  }

}  // namespace kagome::runtime
