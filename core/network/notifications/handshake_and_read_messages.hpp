/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "network/notifications/handshake.hpp"
#include "network/notifications/read_messages.hpp"

namespace kagome::network::notifications {
  template <typename Message,
            typename Self,
            typename Handshake,
            typename OnHandshake,
            typename OnMessage>
  void handshakeAndReadMessages(std::weak_ptr<Self> weak,
                                std::shared_ptr<Stream> stream,
                                const Handshake &handshake,
                                OnHandshake on_handshake,
                                OnMessage on_message) {
    auto frame_stream = std::make_shared<MessageReadWriterUvarint>(stream);
    auto cb = [weak = std::move(weak),
               stream,
               frame_stream,
               on_handshake = std::move(on_handshake),
               on_message = std::move(on_message)](
                  outcome::result<Handshake> r) mutable {
      auto self = weak.lock();
      if (not self) {
        return;
      }
      if (not r) {
        return;
      }
      if (not on_handshake(self, stream, r.value())) {
        stream->reset();
        return;
      }
      auto cb = [weak = std::move(weak),
                 on_message = std::move(on_message)](Message message) mutable {
        auto self = weak.lock();
        if (not self) {
          return false;
        }
        return on_message(self, std::move(message));
      };
      notifications::readMessages<Message>(stream, frame_stream, std::move(cb));
    };
    notifications::handshake(stream, frame_stream, handshake, std::move(cb));
  }
}  // namespace kagome::network::notifications
