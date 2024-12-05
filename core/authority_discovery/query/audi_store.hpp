/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/peer/peer_info.hpp>

#include "authority_discovery/query/authority_peer_info.hpp"
#include "common/buffer.hpp"
#include "primitives/authority_discovery_id.hpp"
#include "storage/face/map_cursor.hpp"

namespace kagome::authority_discovery {

  /**
   * Interface for storing and retrieving authority discovery data
   */
  class AudiStore {
   public:
    virtual ~AudiStore() = default;

    /**
     * Store authority discovery data
     * @param authority the authority to store
     * @param data the data to store
     */
    virtual void store(const primitives::AuthorityDiscoveryId &authority,
                       const AuthorityPeerInfo &data) = 0;

    /**
     * Get authority discovery data
     * @param authority the authority to get
     * @return the data if it exists, otherwise std::nullopt
     */
    virtual std::optional<AuthorityPeerInfo> get(
        const primitives::AuthorityDiscoveryId &authority) const = 0;

    /**
     * Remove authority discovery data
     * @param authority the authority to remove
     */
    virtual outcome::result<void> remove(const primitives::AuthorityDiscoveryId &authority) = 0;

    /**
     * Check if the store contains the authority
     * @param authority the authority to check
     * @return true if the authority is in the store, false otherwise
     */
    virtual bool contains(
        const primitives::AuthorityDiscoveryId &authority) const = 0;

    virtual void forEach(
        std::function<void(const primitives::AuthorityDiscoveryId &,
                           const AuthorityPeerInfo &)> f) const = 0;

    virtual void retainIf(
        std::function<bool(const primitives::AuthorityDiscoveryId &,
                           const AuthorityPeerInfo &)> f) = 0;
  };

}  // namespace kagome::authority_discovery
