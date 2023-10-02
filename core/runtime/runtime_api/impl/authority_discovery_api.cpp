/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/authority_discovery_api.hpp"

#include "runtime/executor.hpp"

namespace kagome::runtime {
  AuthorityDiscoveryApiImpl::AuthorityDiscoveryApiImpl(
      std::shared_ptr<Executor> executor)
      : executor_{std::move(executor)} {
    BOOST_ASSERT(executor_);
  }

  outcome::result<std::vector<primitives::AuthorityDiscoveryId>>
  AuthorityDiscoveryApiImpl::authorities(const primitives::BlockHash &block) {
    OUTCOME_TRY(ref,
                cache_.get_else(
                    block,
                    [&]() -> outcome::result<
                              std::vector<primitives::AuthorityDiscoveryId>> {
                      OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
                      return executor_
                          ->call<std::vector<primitives::AuthorityDiscoveryId>>(
                              ctx, "AuthorityDiscoveryApi_authorities");
                    }));
    return *ref;
  }
}  // namespace kagome::runtime
