/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "network/helpers/stream_read_buffer.hpp"
#include "network/impl/protocols/protocol_base_impl.hpp"
#include "network/notifications/handshake.hpp"
#include "network/notifications/wait_read_close.hpp"

namespace kagome::network::notifications {
  template <typename Handshake>
  using ConnectAndHandshake =
      std::tuple<std::shared_ptr<Stream>,
                 std::shared_ptr<MessageReadWriterUvarint>,
                 Handshake>;

  template <typename Self, typename Handshake, typename OnHandshake>
  void connectAndHandshake(std::weak_ptr<Self> weak,
                           ProtocolBaseImpl &base,
                           const PeerId &peer_id,
                           Handshake handshake,
                           OnHandshake on_handshake) {
    auto cb = [weak = std::move(weak),
               handshake = std::move(handshake),
               on_handshake = std::move(on_handshake)](
                  libp2p::StreamAndProtocolOrError r) mutable {
      auto self = weak.lock();
      if (not self) {
        return;
      }
      streamReadBuffer(r);
      if (not r) {
        on_handshake(self, r.error());
        return;
      }
      auto &stream = r.value().stream;
      auto frame_stream = std::make_shared<MessageReadWriterUvarint>(stream);
      auto cb = [weak = std::move(weak),
                 stream,
                 frame_stream,
                 on_handshake = std::move(on_handshake)](
                    outcome::result<Handshake> r) mutable {
        auto self = weak.lock();
        if (not self) {
          return;
        }
        if (not r) {
          on_handshake(self, r.error());
          return;
        }
        waitReadClose(stream);
        on_handshake(self,
                     ConnectAndHandshake<Handshake>{
                         std::move(stream),
                         std::move(frame_stream),
                         std::move(r.value()),
                     });
      };
      notifications::handshake(
          std::move(stream), std::move(frame_stream), handshake, std::move(cb));
    };

    // auto addresses_res =
    //     base.host().getPeerRepository().getAddressRepository().getAddresses(
    //         peer_id);
    // if (not addresses_res.has_value()) {
    //   cb(addresses_res.as_failure());
    //   return;
    // }

    base.host().newStream(peer_id, base.protocolIds(), std::move(cb));

    // base.host().newStream(PeerInfo{peer_id, std::move(addresses_res.value())},
    //                       base.protocolIds(),
    //                       std::move(cb));
  }
}  // namespace kagome::network::notifications
