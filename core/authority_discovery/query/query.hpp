/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_AUTHORITY_DISCOVERY_QUERY_QUERY_HPP
#define KAGOME_AUTHORITY_DISCOVERY_QUERY_QUERY_HPP

#include <libp2p/peer/peer_info.hpp>

#include "primitives/authority_discovery_id.hpp"

namespace kagome::authority_discovery {
  /**
   * Query peer info from authority discovery public key.
   */
  class Query {
   public:
    virtual ~Query() = default;

    /**
     * Get cached peer info from authority discovery public key.
     */
    virtual std::optional<libp2p::peer::PeerInfo> get(
        const primitives::AuthorityDiscoveryId &authority) const = 0;

    /**
     * Get cached authority discovery public key by peer id.
     */
    virtual std::optional<primitives::AuthorityDiscoveryId> get(
        const libp2p::peer::PeerId &peer_id) const = 0;
  };
}  // namespace kagome::authority_discovery

#endif  // KAGOME_AUTHORITY_DISCOVERY_QUERY_QUERY_HPP
