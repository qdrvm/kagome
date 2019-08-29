/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_CLIENT_LIBP2P_HPP
#define KAGOME_CONSENSUS_CLIENT_LIBP2P_HPP

#include <memory>

#include "common/logger.hpp"
#include "libp2p/peer/peer_info.hpp"
#include "network/consensus_client.hpp"
#include "network/impl/rpc_sender_libp2p.hpp"

namespace kagome::network {
  class ConsensusClientLibp2p
      : public ConsensusClient,
        public std::enable_shared_from_this<ConsensusClientLibp2p> {
   public:
    ConsensusClientLibp2p(
        libp2p::peer::PeerInfo peer_info,
        std::shared_ptr<RPCSender<RPCInfoLibp2p>> rpc_sender,
        common::Logger log = common::createLogger("PeerClientLibp2p"));

    ~ConsensusClientLibp2p() override = default;

    void blocksRequest(BlocksRequest request,
                       BlocksResponseHandler cb) const override;

   private:
    void onBlocksResponseReceived(
        outcome::result<std::shared_ptr<std::vector<uint8_t>>>
            encoded_response_res,
        const BlocksResponseHandler &cb) const;

    libp2p::peer::PeerInfo peer_info_;
    std::shared_ptr<RPCSender<RPCInfoLibp2p>> rpc_sender_;
    common::Logger log_;
  };
}  // namespace kagome::network

#endif  // KAGOME_CONSENSUS_CLIENT_LIBP2P_HPP
