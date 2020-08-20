/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/remote_sync_protocol_client.hpp"

#include "common/visitor.hpp"
#include "network/common.hpp"
//#include "network/helpers/scale_message_read_writer.hpp"
#include "network/helpers/protobuf_message_read_writer.hpp"
#include "network/rpc.hpp"

namespace kagome::network {
  template<> struct ProtobufMessageAdapter<network::BlocksRequest> {
    static size_t size(const network::BlocksRequest &t) {
      //static_assert(false, "No impl!");
      return 0;
    }
    static std::vector<uint8_t>::iterator write(const network::BlocksRequest &/*t*/, std::vector<uint8_t> &out, std::vector<uint8_t>::iterator /*loaded*/) {
      //static_assert(false, "No impl!");
      return out.end();
    }
  };

  RemoteSyncProtocolClient::RemoteSyncProtocolClient(
      libp2p::Host &host, libp2p::peer::PeerInfo peer_info)
      : host_{host},
        peer_info_{std::move(peer_info)},
        log_(common::createLogger("RemoteSyncProtocolClient")) {
    assert(false);
  }

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
            host_, peer_info_, network::kSyncProtocol, request, std::move(cb));
  }
}  // namespace kagome::network
