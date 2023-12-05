/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/runtime_api/authority_discovery_api.hpp"
#include "runtime/runtime_api/impl/lru.hpp"

namespace kagome::runtime {
  class Executor;

  class AuthorityDiscoveryApiImpl final : public AuthorityDiscoveryApi {
   public:
    explicit AuthorityDiscoveryApiImpl(std::shared_ptr<Executor> executor);

    outcome::result<std::vector<primitives::AuthorityDiscoveryId>> authorities(
        const primitives::BlockHash &block) override;

   private:
    std::shared_ptr<Executor> executor_;

    using Auths = std::vector<primitives::AuthorityDiscoveryId>;
    RuntimeApiLruBlock<Auths> cache_{10};
  };
}  // namespace kagome::runtime
