/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_RUNTIME_API_AUTHORITY_DISCOVERY_API_HPP
#define KAGOME_RUNTIME_RUNTIME_API_AUTHORITY_DISCOVERY_API_HPP

#include "primitives/authority_discovery_id.hpp"
#include "primitives/block_id.hpp"

namespace kagome::runtime {

  class AuthorityDiscoveryApi {
   public:
    virtual ~AuthorityDiscoveryApi() = default;

    virtual outcome::result<std::vector<primitives::AuthorityDiscoveryId>>
    authorities(const primitives::BlockHash &block) = 0;
  };
}  // namespace kagome::runtime

#endif  // KAGOME_RUNTIME_RUNTIME_API_AUTHORITY_DISCOVERY_API_HPP
