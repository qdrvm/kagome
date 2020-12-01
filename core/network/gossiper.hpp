/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_NETWORK_GOSSIPER_HPP
#define KAGOME_CORE_NETWORK_GOSSIPER_HPP

#include "consensus/babe/babe_gossiper.hpp"
#include "consensus/grandpa/gossiper.hpp"
#include "network/extrinsic_gossiper.hpp"

#include <libp2p/peer/peer_info.hpp>

namespace kagome::network {
  /**
   * Joins all available gossipers
   */
  struct Gossiper : public ExtrinsicGossiper,
                    public consensus::BabeGossiper,
                    public consensus::grandpa::Gossiper {
    /**
     * @brief Need to store self peer info
     * @param peer_info is the peer info of the peer
     */
    virtual void storeSelfPeerInfo(const libp2p::peer::PeerInfo &self_info) = 0;

    /**
     * @returns number of active (opened) streams
     */
    virtual uint32_t getActiveStreamNumber() = 0;
  };
}  // namespace kagome::network

#endif  // KAGOME_CORE_NETWORK_GOSSIPER_HPP
