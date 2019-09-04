/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_GOSSIPER_BROADCAST_HPP
#define KAGOME_GOSSIPER_BROADCAST_HPP

#include <unordered_map>
#include <unordered_set>

#include "libp2p/connection/stream.hpp"
#include "libp2p/peer/peer_info.hpp"
#include "libp2p/peer/protocol.hpp"
#include "network/gossiper.hpp"
#include "network/stream_manager.hpp"

namespace kagome::network {
  /**
   * Sends gossip messages using broadcast strategy
   */
  class GossiperBroadcast : public Gossiper {
    using Precommit = consensus::grandpa::Precommit;
    using Prevote = consensus::grandpa::Prevote;
    using PrimaryPropose = consensus::grandpa::PrimaryPropose;

    using Libp2pStreamManager = StreamManager<libp2p::peer::PeerInfo,
                                              libp2p::peer::Protocol,
                                              libp2p::connection::Stream>;

   public:
    GossiperBroadcast(std::shared_ptr<Libp2pStreamManager> stream_manager,
                      std::unordered_set<libp2p::peer::PeerInfo> peer_infos);

    ~GossiperBroadcast() override = default;

    void blockAnnounce(const BlockAnnounce &announce) const override;

    void precommit(Precommit pc) const override;

    void prevote(Prevote pv) const override;

    void primaryPropose(PrimaryPropose pv) const override;

   private:
    template <typename MsgType>
    void broadcast(MsgType &&msg) const;

    std::shared_ptr<Libp2pStreamManager> stream_manager_;
    std::unordered_set<libp2p::peer::PeerInfo> peer_infos_;
  };
}  // namespace kagome::network

#endif  // KAGOME_GOSSIPER_BROADCAST_HPP
