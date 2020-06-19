/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_NETWORK_GOSSIPER_HPP
#define KAGOME_CORE_NETWORK_GOSSIPER_HPP

#include "consensus/babe/babe_gossiper.hpp"
#include "consensus/grandpa/gossiper.hpp"
#include "network/extrinsic_gossiper.hpp"

#include <libp2p/connection/stream.hpp>
#include <libp2p/peer/peer_info.hpp>

namespace kagome::network {
  /**
   * Joins all available gossipers
   */
  struct Gossiper : public ExtrinsicGossiper,
                    public consensus::BabeGossiper,
                    public consensus::grandpa::Gossiper {
    virtual void reserveStream(
        const libp2p::peer::PeerInfo &info,
        std::shared_ptr<libp2p::connection::Stream> stream) = 0;

    // Add new stream to gossip
    virtual void addStream(
        std::shared_ptr<libp2p::connection::Stream> stream) = 0;
  };
}  // namespace kagome::network

#endif  // KAGOME_CORE_NETWORK_GOSSIPER_HPP
