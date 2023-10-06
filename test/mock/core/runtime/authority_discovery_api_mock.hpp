/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_AUTHORITY_DISCOVERY_API_MOCK
#define KAGOME_RUNTIME_AUTHORITY_DISCOVERY_API_MOCK

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

#endif  // KAGOME_RUNTIME_AUTHORITY_DISCOVERY_API_MOCK
