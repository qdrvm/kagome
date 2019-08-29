/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_SERVER_LIBP2P_HPP
#define KAGOME_CONSENSUS_SERVER_LIBP2P_HPP

#include "libp2p/host/host.hpp"
#include "network/consensus_server.hpp"

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
    explicit ConsensusServerLibp2p(libp2p::Host &host);

    ~ConsensusServerLibp2p() override = default;

    void start() const override;

    void setBlocksRequestHandler(BlocksRequestHandler handler) override;

   private:
    void handleSyncProto(
        std::shared_ptr<libp2p::connection::Stream> stream) const;

    bool handleBlocksRequest(
        const BlocksRequest &request,
        const std::shared_ptr<libp2p::basic::MessageReadWriter> &read_writer)
        const;

    libp2p::Host &host_;

    BlocksRequestHandler blocks_request_handler_;
  };
}  // namespace kagome::network

#endif  // KAGOME_CONSENSUS_SERVER_LIBP2P_HPP
