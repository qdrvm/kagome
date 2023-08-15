/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_RUNTIME_API_IMPL_AUTHORITY_DISCOVERY_API_HPP
#define KAGOME_RUNTIME_RUNTIME_API_IMPL_AUTHORITY_DISCOVERY_API_HPP

#include "common/lru_cache.hpp"
#include "runtime/runtime_api/authority_discovery_api.hpp"

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
    LruCache<primitives::BlockHash, Auths> cache_{10};
  };
}  // namespace kagome::runtime

#endif  // KAGOME_RUNTIME_RUNTIME_API_IMPL_AUTHORITY_DISCOVERY_API_HPP
