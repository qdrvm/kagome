/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/remote_sync_protocol_client.hpp"

#include "common/visitor.hpp"
#include "network/common.hpp"
#include "network/helpers/scale_message_read_writer.hpp"
#include "network/rpc.hpp"

namespace kagome::network {
  RemoteSyncProtocolClient::RemoteSyncProtocolClient(
      libp2p::Host &host, const libp2p::peer::PeerInfo &peer_info)
      : host_{host},
        peer_info_{peer_info},
        log_(common::createLogger("RemoteSyncProtocolClient")) {}

  void RemoteSyncProtocolClient::requestBlocks(
      const network::BlocksRequest &request,
      std::function<void(outcome::result<network::BlocksResponse>)> cb) {
    visit_in_place(
        request.from,
        [this](primitives::BlockNumber from) {
          log_->debug("Requesting blocks: from {}", from);
        },
        [this, &request](const primitives::BlockHash &from) {
          if (not request.to) {
            log_->debug("Requesting blocks: from {}", from.toHex());
          } else {
            log_->debug("Requesting blocks: from {}, to {}",
                        from.toHex(),
                        request.to->toHex());
          }
        });
    network::RPC<network::ScaleMessageReadWriter>::
        write<network::BlocksRequest, network::BlocksResponse>(
            host_, peer_info_, network::kSyncProtocol, request, std::move(cb));
  }
}  // namespace kagome::network
