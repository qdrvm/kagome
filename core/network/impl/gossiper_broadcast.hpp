/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_GOSSIPER_BROADCAST_HPP
#define KAGOME_GOSSIPER_BROADCAST_HPP

#include <unordered_map>

#include <gsl/span>
#include "common/logger.hpp"
#include "libp2p/connection/stream.hpp"
#include "libp2p/host/host.hpp"
#include "libp2p/peer/peer_info.hpp"
#include "network/gossiper.hpp"
#include "network/types/gossip_message.hpp"
#include "network/types/peer_list.hpp"

namespace kagome::network {
  /**
   * Sends gossip messages using broadcast strategy
   */
  class GossiperBroadcast
      : public Gossiper,
        public std::enable_shared_from_this<GossiperBroadcast> {
    using Precommit = consensus::grandpa::Precommit;
    using Prevote = consensus::grandpa::Prevote;
    using PrimaryPropose = consensus::grandpa::PrimaryPropose;

   public:
    GossiperBroadcast(libp2p::Host &host, const PeerList &peer_infos);

    ~GossiperBroadcast() override = default;

    void blockAnnounce(const BlockAnnounce &announce) override;

    void vote(const consensus::grandpa::VoteMessage &msg) override;

    void finalize(const consensus::grandpa::Fin &fin) override;

   private:
    void broadcast(GossipMessage &&msg);

    libp2p::Host &host_;
    std::unordered_map<libp2p::peer::PeerInfo,
                       std::shared_ptr<libp2p::connection::Stream>>
        streams_;
    common::Logger logger_;
  };
}  // namespace kagome::network

#endif  // KAGOME_GOSSIPER_BROADCAST_HPP
