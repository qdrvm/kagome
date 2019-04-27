/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PROTOCOL_REPOSITORY_HPP
#define KAGOME_PROTOCOL_REPOSITORY_HPP

#include <list>

#include <gsl/span>
#include <outcome/outcome.hpp>
#include "libp2p/basic/garbage_collectable.hpp"
#include "libp2p/peer/peer_id.hpp"
#include "libp2p/peer/protocol.hpp"

namespace libp2p::peer {

  /**
   * @brief Storage for mapping between peer and its known protocols.
   */
  class ProtocolRepository : public basic::GarbageCollectable {
   public:
    ~ProtocolRepository() override = default;

    /**
     * @brief Add protocols to a peer.
     * param p peer
     * @param ms list of protocols
     * @return peer error, if no peer {@param p} found
     */
    virtual outcome::result<void> addProtocols(
        const PeerId &p, gsl::span<const Protocol> ms) = 0;

    /**
     * @brief Removes protocols from a peer.
     * @param p peer
     * @param ms list of protocols
     * @return peer error, if no peer {@param p} found
     */
    virtual outcome::result<void> removeProtocols(
        const PeerId &p, gsl::span<const Protocol> ms) = 0;

    /**
     * @brief Get all supported protocols by given peer {@param p}
     * @param p peer
     * @return list of protocols (may be empty) or peer error, if no peer
     * @param p} found
     */
    virtual outcome::result<std::list<Protocol>> getProtocols(
        const PeerId &p) const = 0;

    /**
     * @brief Calculates set intersection between {@param protocols} and stored
     * rotocols.
     * @param p peer
     * @param protocols check if given protocols are supported by a peer

     * eturn list of supported protocols (may be empty) or peer error, if no p
     * er {@param p} found
     */
    virtual outcome::result<std::list<Protocol>> supportsProtocols(
        const PeerId &p, gsl::span<const Protocol> protocols) const = 0;

    /**
     * @brief Remove all associated protocols for given peer
     * @param p peer
     *
     * @note does not remove peer from a list of known peers. So, peer can
     * contain "0 protocols".
     */
    virtual void clear(const PeerId &p) = 0;
  };

}  // namespace libp2p::peer

#endif  // KAGOME_PROTOCOL_REPOSITORY_HPP
