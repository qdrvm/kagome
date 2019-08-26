/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PEER_CLIENT_LIBP2P_HPP
#define KAGOME_PEER_CLIENT_LIBP2P_HPP

#include <memory>
#include <string_view>

#include "common/logger.hpp"
#include "libp2p/host/host.hpp"
#include "libp2p/peer/peer_info.hpp"
#include "network/network_state.hpp"
#include "network/peer_client.hpp"

namespace libp2p::basic {
  class MessageReadWriter;
}

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
     * @param logger to write messages to
     */
    PeerClientLibp2p(
        libp2p::Host &host,
        libp2p::peer::PeerInfo peer_info,
        common::Logger logger = common::createLogger("PeerClientLibp2p"));

    enum class Error { SUCCESS = 0, LIBP2P_ERROR = 1 };

    void blocksRequest(BlocksRequest request,
                       BlockResponseHandler handler) const override;

    void blockAnnounce(BlockAnnounce block_announce,
                       std::function<void(const outcome::result<void> &)>
                           handler) const override;

   private:
    void onBlocksRequestWritten(
        outcome::result<size_t> write_res,
        const std::shared_ptr<libp2p::basic::MessageReadWriter> &read_writer,
        BlockResponseHandler cb) const;

    libp2p::Host &host_;
    libp2p::peer::PeerInfo peer_info_;
    common::Logger log_;
  };
}  // namespace kagome::network

#endif  // KAGOME_PEER_CLIENT_LIBP2P_HPP
