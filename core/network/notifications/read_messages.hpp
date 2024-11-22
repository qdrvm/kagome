/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/basic/message_read_writer_uvarint.hpp>
#include <libp2p/connection/stream.hpp>
#include <scale/scale.hpp>

namespace kagome::network::notifications {
  using libp2p::basic::MessageReadWriterUvarint;
  using libp2p::connection::Stream;

  template <typename OnMessage>
  void readMessagesRaw(std::shared_ptr<Stream> stream,
                       std::shared_ptr<MessageReadWriterUvarint> frame_stream,
                       OnMessage on_message) {
    auto cb = [stream = std::move(stream),
               frame_stream,
               on_message = std::move(on_message)](
                  libp2p::basic::MessageReadWriter::ReadCallback r) mutable {
      if (!r) {
        stream->reset();
        return;
      }
      auto &message = r.value();
      // TODO(turuslan): `MessageReadWriterUvarint` reuse buffer
      if (not message) {
        static auto empty = std::make_shared<std::vector<uint8_t>>();
        message = empty;
      }
      if (not on_message(message)) {
        stream->reset();
        return;
      }
      readMessagesRaw(
          std::move(stream), std::move(frame_stream), std::move(on_message));
    };
    frame_stream->read(std::move(cb));
  }

  template <typename Message, typename OnMessage>
  void readMessages(std::shared_ptr<Stream> stream,
                    std::shared_ptr<MessageReadWriterUvarint> frame_stream,
                    OnMessage on_message) {
    auto cb = [stream, on_message = std::move(on_message)](
                  MessageReadWriterUvarint::ResultType raw) mutable {
      auto r = scale::decode<Message>(*raw);
      if (not r) {
        stream->reset();
        return false;
      }
      return on_message(std::move(r.value()));
    };
    readMessagesRaw(std::move(stream), std::move(frame_stream), std::move(cb));
  }
}  // namespace kagome::network::notifications
