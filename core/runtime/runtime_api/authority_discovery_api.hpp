/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

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
