/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/grandpa_api.hpp"

#include "runtime/executor.hpp"

namespace kagome::runtime {

  GrandpaApiImpl::GrandpaApiImpl(std::shared_ptr<Executor> executor)
      : executor_{std::move(executor)} {
    BOOST_ASSERT(executor_);
  }

  outcome::result<GrandpaApi::Authorities> GrandpaApiImpl::authorities(
      const primitives::BlockHash &block_hash) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block_hash));
    return executor_->call<Authorities>(ctx, "GrandpaApi_grandpa_authorities");
  }

  outcome::result<GrandpaApi::AuthoritySetId> GrandpaApiImpl::current_set_id(
      const primitives::BlockHash &block_hash) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block_hash));
    return executor_->call<AuthoritySetId>(ctx, "GrandpaApi_current_set_id");
  }

}  // namespace kagome::runtime
