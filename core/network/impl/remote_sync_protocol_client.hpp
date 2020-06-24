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

namespace kagome::network {

  class RemoteSyncProtocolClient
      : public network::SyncProtocolClient,
        public std::enable_shared_from_this<RemoteSyncProtocolClient> {
   public:
    RemoteSyncProtocolClient(libp2p::Host &host,
                             const libp2p::peer::PeerInfo &peer_info);

    void requestBlocks(
        const network::BlocksRequest &request,
        std::function<void(outcome::result<network::BlocksResponse>)> cb)
        override;

   private:
    libp2p::Host &host_;
    const libp2p::peer::PeerInfo &peer_info_;
    common::Logger log_;
  };
}  // namespace kagome::network

#endif  // KAGOME_REMOTE_SYNC_PROTOCOL_CLIENT
