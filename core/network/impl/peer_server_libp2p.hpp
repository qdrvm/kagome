/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PEER_SERVER_LIBP2P_HPP
#define KAGOME_PEER_SERVER_LIBP2P_HPP

#include <vector>

#include "common/logger.hpp"
#include "libp2p/basic/message_read_writer.hpp"
#include "libp2p/host/host.hpp"
#include "libp2p/peer/peer_info.hpp"
#include "network/network_state.hpp"
#include "network/peer_server.hpp"

namespace libp2p::connection {
  struct Stream;
}

namespace kagome::network {
  /**
   * Implementation of PeerServer on top of Libp2p
   */
  class PeerServerLibp2p
      : public PeerServer,
        public std::enable_shared_from_this<PeerServerLibp2p> {
   public:
    /**
     * Create an instance of PeerServer on top of Libp2p
     * @param network_state - current state of the Polkadot network
     * @param host - Libp2p host object
     * @param peer_info - this peer's information in Libp2p network
     * @param logger to write messages to
     */
    PeerServerLibp2p(
        std::shared_ptr<NetworkState> network_state,
        libp2p::Host &host,
        libp2p::peer::PeerInfo peer_info,
        common::Logger logger = common::createLogger("PeerServerLibp2p"));

    void onBlocksRequest(BlocksRequestHandler handler) const override;

    void onBlockAnnounce(BlockAnnounceHandler handler) const override;

   private:
    void handleSyncProto(
        std::shared_ptr<libp2p::connection::Stream> stream) const;

    bool handleBlocksRequest(
        BlocksRequest request,
        std::shared_ptr<libp2p::basic::MessageReadWriter> read_writer) const;

    void handleGossipProto(
        std::shared_ptr<libp2p::basic::MessageReadWriter> read_writer) const;

    void handleBlockAnnounce(
        BlockAnnounce announce,
        std::shared_ptr<libp2p::basic::MessageReadWriter> read_writer) const;

    std::shared_ptr<NetworkState> network_state_;
    libp2p::Host &host_;
    libp2p::peer::PeerInfo peer_info_;
    common::Logger log_;

    // functions to be called, when a corresponding message arrives
    BlocksRequestHandler blocks_request_handler_;
    BlockAnnounceHandler block_announce_handler_;
  };
}  // namespace kagome::network

#endif  // KAGOME_PEER_SERVER_LIBP2P_HPP
