/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_GOSSIPER_BROADCAST_HPP
#define KAGOME_GOSSIPER_BROADCAST_HPP

#include <unordered_map>

#include <gsl/span>
#include "libp2p/connection/stream.hpp"
#include "libp2p/host/host.hpp"
#include "libp2p/peer/peer_info.hpp"
#include "network/gossiper.hpp"

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
    GossiperBroadcast(libp2p::Host &host,
                      gsl::span<const libp2p::peer::PeerInfo> peer_infos);

    ~GossiperBroadcast() override = default;

    void blockAnnounce(const BlockAnnounce &announce) override;

    void precommit(Precommit pc) override;

    void prevote(Prevote pv) override;

    void primaryPropose(PrimaryPropose pv) override;

   private:
    template <typename MsgType>
    void broadcast(MsgType &&msg);

    libp2p::Host &host_;
    std::unordered_map<libp2p::peer::PeerInfo,
                       std::shared_ptr<libp2p::connection::Stream>>
        streams_;
  };
}  // namespace kagome::network

#endif  // KAGOME_GOSSIPER_BROADCAST_HPP
