/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PEER_CLIENT_LIBP2P_HPP
#define KAGOME_PEER_CLIENT_LIBP2P_HPP

#include <memory>

#include "common/logger.hpp"
#include "libp2p/host/host.hpp"
#include "libp2p/peer/peer_info.hpp"
#include "network/impl/rpc_sender_libp2p.hpp"
#include "network/peer_client.hpp"

namespace kagome::network {
  /**
   * Implementation of PeerClient on top of Libp2p
   * @tparam ReadWriter - type of reader/writer, which will be used to write
   * messages to the stream; for instance, it can be
   * libp2p::basic::MessageReadWriter
   */
  class PeerClientLibp2p
      : public PeerClient,
        public std::enable_shared_from_this<PeerClientLibp2p> {
   public:
    /**
     * Create an instance of PeerClient on top of Libp2p
     * @param host - Libp2p host object
     * @param peer_info - this peer's information in Libp2p network
     * @param rpc_sender to send and accept messages
     * @param logger to write messages to
     */
    PeerClientLibp2p(
        libp2p::Host &host,
        libp2p::peer::PeerInfo peer_info,
        std::shared_ptr<RPCSender<RPCInfoLibp2p>> rpc_sender,
        common::Logger logger = common::createLogger("PeerClientLibp2p"));

    ~PeerClientLibp2p() override = default;

    void blocksRequest(BlocksRequest request,
                       BlocksResponseHandler handler) const override;

    void blockAnnounce(BlockAnnounce block_announce,
                       std::function<void(const outcome::result<void> &)>
                           handler) const override;

   private:
    void onBlocksResponseReceived(
        outcome::result<std::shared_ptr<common::Buffer>> encoded_response_res,
        const BlocksResponseHandler &cb) const;

    libp2p::Host &host_;
    libp2p::peer::PeerInfo peer_info_;
    std::shared_ptr<RPCSender<RPCInfoLibp2p>> rpc_sender_;
    common::Logger log_;
  };
}  // namespace kagome::network

#endif  // KAGOME_PEER_CLIENT_LIBP2P_HPP
