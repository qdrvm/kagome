/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/remote_sync_protocol_client.hpp"

#include "application/genesis_config.hpp"
#include "common/visitor.hpp"
#include "network/adapters/protobuf_block_request.hpp"
#include "network/adapters/protobuf_block_response.hpp"
#include "network/common.hpp"
#include "network/helpers/protobuf_message_read_writer.hpp"
#include "network/rpc.hpp"

namespace kagome::network {

  RemoteSyncProtocolClient::RemoteSyncProtocolClient(
      libp2p::Host &host,
      libp2p::peer::PeerInfo peer_info,
      std::shared_ptr<kagome::application::GenesisConfig> config)
      : host_{host},
        peer_info_{std::move(peer_info)},
        log_(common::createLogger("RemoteSyncProtocolClient")),
        config_(std::move(config)) {}

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
    network::RPC<network::ProtobufMessageReadWriter>::
        write<network::BlocksRequest, network::BlocksResponse>(
            host_,
            peer_info_,
            fmt::format(network::kSyncProtocol.data(), config_->protocolId()),
            request,
            std::move(cb));
  }
}  // namespace kagome::network
