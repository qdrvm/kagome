/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_GOSSIPER_SERVER_LIBP2P_HPP
#define KAGOME_GOSSIPER_SERVER_LIBP2P_HPP

#include "common/logger.hpp"
#include "libp2p/host/host.hpp"
#include "network/gossiper_server.hpp"

namespace libp2p {
  namespace basic {
    class MessageReadWriter;
  }
  namespace connection {
    struct Stream;
  }
}  // namespace libp2p

namespace kagome::network {
  class GossiperServerLibp2p
      : public GossiperServer,
        public std::enable_shared_from_this<GossiperServerLibp2p> {
   public:
    explicit GossiperServerLibp2p(
        libp2p::Host &host,
        common::Logger log = common::createLogger("GossiperServerLibp2p"));

    ~GossiperServerLibp2p() override = default;

    void start() override;

    void setBlockAnnounceHandler(BlockAnnounceHandler handler) override;

   private:
    void handleGossipProtocol(
        std::shared_ptr<libp2p::connection::Stream> stream) const;

    void handleBlockAnnounce(
        const BlockAnnounce &announce,
        std::shared_ptr<libp2p::connection::Stream> stream) const;

    libp2p::Host &host_;
    common::Logger log_;

    BlockAnnounceHandler block_announce_handler_;
  };
}  // namespace kagome::network

#endif  // KAGOME_GOSSIPER_SERVER_LIBP2P_HPP
