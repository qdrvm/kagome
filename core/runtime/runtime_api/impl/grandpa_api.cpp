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
    return executor_->callAt<std::optional<ScheduledChange>>(
        block, "GrandpaApi_pending_change", digest);
  }

  outcome::result<std::optional<GrandpaApi::ForcedChange>>
  GrandpaApiImpl::forced_change(const primitives::BlockHash &block,
                                const Digest &digest) {
    return executor_->callAt<std::optional<ForcedChange>>(
        block, "GrandpaApi_forced_change", digest);
  }

  outcome::result<GrandpaApi::AuthorityList> GrandpaApiImpl::authorities(
      const primitives::BlockId &block_id) {
    OUTCOME_TRY(block_hash, block_header_repo_->getHashById(block_id));

    OUTCOME_TRY(ref, authorities_.get_else(block_hash, [&] {
      return executor_->callAt<AuthorityList>(block_hash,
                                              "GrandpaApi_grandpa_authorities");
    }));
    return *ref;
  }

  outcome::result<primitives::AuthoritySetId> GrandpaApiImpl::current_set_id(
      const primitives::BlockHash &block_hash) {
    OUTCOME_TRY(ref, set_id_.get_else(block_hash, [&] {
      return executor_->callAt<primitives::AuthoritySetId>(
          block_hash, "GrandpaApi_current_set_id");
    }));
    return *ref;
  }

}  // namespace kagome::runtime
