/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_REMOTE_SYNC_PROTOCOL_CLIENT
#define KAGOME_REMOTE_SYNC_PROTOCOL_CLIENT

#include "network/sync_protocol_client.hpp"

#include <libp2p/host/host.hpp>
#include <libp2p/peer/peer_info.hpp>

#include "common/logger.hpp"

namespace kagome::application {
  class ChainSpec;
}

namespace kagome::network {

  class RemoteSyncProtocolClient
      : public network::SyncProtocolClient,
        public std::enable_shared_from_this<RemoteSyncProtocolClient> {
   public:
    RemoteSyncProtocolClient(
        libp2p::Host &host,
        libp2p::peer::PeerInfo peer_info,
        std::shared_ptr<kagome::application::ChainSpec> config);

    void requestBlocks(
        const network::BlocksRequest &request,
        std::function<void(outcome::result<network::BlocksResponse>)> cb)
        override;

    boost::optional<std::reference_wrapper<const libp2p::peer::PeerId>> peerId()
        const override {
      return std::reference_wrapper<const libp2p::peer::PeerId>{peer_info_.id};
    }

   private:
    libp2p::Host &host_;
    const libp2p::peer::PeerInfo peer_info_;
    common::Logger log_;
    std::shared_ptr<kagome::application::ChainSpec> config_;
  };
}  // namespace kagome::network

#endif  // KAGOME_REMOTE_SYNC_PROTOCOL_CLIENT
