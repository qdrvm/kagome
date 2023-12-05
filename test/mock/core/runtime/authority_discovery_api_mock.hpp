/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/runtime_api/authority_discovery_api.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime {
  struct AuthorityDiscoveryApiMock : AuthorityDiscoveryApi {
    MOCK_METHOD(outcome::result<std::vector<primitives::AuthorityDiscoveryId>>,
                authorities,
                (const primitives::BlockHash &),
                (override));
  };
}  // namespace kagome::runtime
