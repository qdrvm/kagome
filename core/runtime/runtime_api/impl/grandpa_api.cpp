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

  outcome::result<boost::optional<GrandpaApi::ScheduledChange>>
  GrandpaApiImpl::pending_change(primitives::BlockHash const &block,
                                 const Digest &digest) {
    return executor_->callAt<boost::optional<ScheduledChange>>(
        block, "GrandpaApi_pending_change", digest);
  }

  outcome::result<boost::optional<GrandpaApi::ForcedChange>>
  GrandpaApiImpl::forced_change(primitives::BlockHash const &block,
                                const Digest &digest) {
    return executor_->callAt<boost::optional<ForcedChange>>(
        block, "GrandpaApi_forced_change", digest);
  }

  outcome::result<GrandpaApi::AuthorityList> GrandpaApiImpl::authorities(
      const primitives::BlockId &block_id) {
    OUTCOME_TRY(block_hash, block_header_repo_->getHashById(block_id));
    return executor_->callAt<AuthorityList>(block_hash,
                                            "GrandpaApi_grandpa_authorities");
  }

}  // namespace kagome::runtime
