/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/remote_sync_protocol_client.hpp"

#include "common/visitor.hpp"
#include "network/common.hpp"
#include "network/protobuf/api.v1.pb.h"
#include "network/helpers/protobuf_message_read_writer.hpp"
#include "network/rpc.hpp"

#ifdef _MSC_VER
# define LE_BE_SWAP32 _byteswap_ulong
# define LE_BE_SWAP64 _byteswap_uint64
#else//_MSC_VER
# define LE_BE_SWAP32 __builtin_bswap32
# define LE_BE_SWAP64 __builtin_bswap64
#endif//_MSC_VER

namespace kagome::network {
  template<> struct ProtobufMessageAdapter<network::BlocksRequest> {
    static size_t size(const network::BlocksRequest &t) {
      return 0;
    }
    static std::vector<uint8_t>::iterator write(const network::BlocksRequest &t, std::vector<uint8_t> &out, std::vector<uint8_t>::iterator loaded) {
      api::v1::BlockRequest msg;
      msg.set_fields(LE_BE_SWAP32(t.fields.attributes.to_ulong()));

      if (t.max) msg.set_max_blocks(*t.max);
      if (t.to) msg.set_to_block(t.to->toHex());

      msg.set_direction(static_cast<api::v1::Direction>(t.direction));
      kagome::visit_in_place(
          t.from,
          [&](const primitives::BlockHash &v) {
            msg.set_hash(v.toHex());
          },
          [&](const primitives::BlockNumber &v) {
            msg.set_number(std::to_string(v));
          });

      const size_t distance_was = std::distance(out.begin(), loaded);
      const size_t was_size = out.size();

      out.resize(was_size + msg.ByteSizeLong());
      msg.SerializeToArray(&out[was_size], msg.ByteSizeLong());

      auto res_it = out.begin();
      std::advance(res_it, std::min(distance_was, was_size));
      return std::move(res_it);
    }
  };

  RemoteSyncProtocolClient::RemoteSyncProtocolClient(
      libp2p::Host &host, libp2p::peer::PeerInfo peer_info)
      : host_{host},
        peer_info_{std::move(peer_info)},
        log_(common::createLogger("RemoteSyncProtocolClient")) {
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
