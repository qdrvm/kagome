/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_SERVER_LIBP2P_HPP
#define KAGOME_CONSENSUS_SERVER_LIBP2P_HPP

#include "common/logger.hpp"
#include "libp2p/host/host.hpp"
#include "network/consensus_server.hpp"
#include "network/types/network_message.hpp"

namespace libp2p {
  namespace basic {
    class MessageReadWriter;
  }
  namespace connection {
    struct Stream;
  }
}  // namespace libp2p

namespace kagome::network {
  class ConsensusServerLibp2p
      : public ConsensusServer,
        public std::enable_shared_from_this<ConsensusServerLibp2p> {
   public:
    explicit ConsensusServerLibp2p(
        libp2p::Host &host,
        common::Logger log = common::createLogger("ConsensusServerLibp2p"));

    ~ConsensusServerLibp2p() override = default;

    void start() const override;

    void setBlocksRequestHandler(BlocksRequestHandler handler) override;

    enum class Error { UNEXPECTED_MESSAGE_TYPE = 1 };

   private:
    void handleSyncProto(
        std::shared_ptr<libp2p::connection::Stream> stream) const;

    outcome::result<NetworkMessage> handleBlocksRequest(
        const BlocksRequest &request) const;

    libp2p::Host &host_;
    common::Logger log_;

    BlocksRequestHandler blocks_request_handler_;
  };
}  // namespace kagome::network

OUTCOME_HPP_DECLARE_ERROR(kagome::network, ConsensusServerLibp2p::Error)

#endif  // KAGOME_CONSENSUS_SERVER_LIBP2P_HPP
