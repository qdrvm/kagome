/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "authority_discovery/query/query.hpp"

#include <gmock/gmock.h>

namespace kagome::authority_discovery {

  class QueryMock : public Query {
   public:
    MOCK_METHOD(std::optional<libp2p::peer::PeerInfo>,
                get,
                (const primitives::AuthorityDiscoveryId &),
                (const, override));

    MOCK_METHOD(std::optional<primitives::AuthorityDiscoveryId>,
                get,
                (const libp2p::peer::PeerId &),
                (const, override));
  };

}  // namespace kagome::authority_discovery
