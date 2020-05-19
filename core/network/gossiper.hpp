/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_NETWORK_GOSSIPER_HPP
#define KAGOME_CORE_NETWORK_GOSSIPER_HPP

#include "network/extrinsic_gossiper.hpp"
#include "consensus/babe/babe_gossiper.hpp"
#include "consensus/grandpa/gossiper.hpp"

#include <libp2p/connection/stream.hpp>

namespace kagome::network {
  /**
   * Joins all available gossipers
   */
  struct Gossiper : public ExtrinsicGossiper,
                    public consensus::BabeGossiper,
                    public consensus::grandpa::Gossiper {
    // Add new stream to gossip
    virtual void addStream(
        std::shared_ptr<libp2p::connection::Stream> stream) = 0;
  };
}  // namespace kagome::network

#endif  // KAGOME_CORE_NETWORK_GOSSIPER_HPP
