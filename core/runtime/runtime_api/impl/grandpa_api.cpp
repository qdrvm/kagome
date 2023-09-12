/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/grandpa_api.hpp"

#include "runtime/executor.hpp"

namespace kagome::runtime {

  GrandpaApiImpl::GrandpaApiImpl(std::shared_ptr<Executor> executor)
      : executor_{std::move(executor)} {
    BOOST_ASSERT(executor_);
  }

  outcome::result<GrandpaApi::AuthorityList> GrandpaApiImpl::authorities(
      const primitives::BlockHash &block_hash) {
    return executor_->callAt<AuthorityList>(block_hash,
                                            "GrandpaApi_grandpa_authorities");
  }

  outcome::result<primitives::AuthoritySetId> GrandpaApiImpl::current_set_id(
      const primitives::BlockHash &block_hash) {
    return executor_->callAt<primitives::AuthoritySetId>(
        block_hash, "GrandpaApi_current_set_id");
  }

}  // namespace kagome::runtime
