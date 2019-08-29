/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_GOSSIPER_CLIENT_LIBP2P_HPP
#define KAGOME_GOSSIPER_CLIENT_LIBP2P_HPP

#include "common/logger.hpp"
#include "libp2p/peer/peer_info.hpp"
#include "network/gossiper_client.hpp"
#include "network/impl/rpc_sender_libp2p.hpp"

namespace kagome::network {
  class GossiperClientLibp2p
      : public GossiperClient,
        public std::enable_shared_from_this<GossiperClientLibp2p> {
   public:
    GossiperClientLibp2p(
        libp2p::peer::PeerInfo peer_info,
        std::shared_ptr<RPCSender<RPCInfoLibp2p>> rpc_sender,
        common::Logger log = common::createLogger("PeerClientLibp2p"));

    void blockAnnounce(
        BlockAnnounce block_announce,
        std::function<void(const outcome::result<void> &)> cb) const override;

   private:
    libp2p::peer::PeerInfo peer_info_;
    std::shared_ptr<RPCSender<RPCInfoLibp2p>> rpc_sender_;
    common::Logger log_;
  };
}  // namespace kagome::network

#endif  // KAGOME_GOSSIPER_CLIENT_LIBP2P_HPP
