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

  template <typename OnHandhake>
  void handshakeRaw(std::shared_ptr<Stream> stream,
                    std::shared_ptr<MessageReadWriterUvarint> frame_stream,
                    common::BufferView handshake,
                    OnHandhake on_handshake) {
    auto cb = [stream = std::move(stream),
               frame_stream,
               on_handshake =
                   std::move(on_handshake)](outcome::result<size_t> r) mutable {
      if (not r) {
        stream->reset();
        return on_handshake(r.error());
      }
      auto cb = [stream = std::move(stream),
                 frame_stream,
                 on_handshake = std::move(on_handshake)](
                    libp2p::basic::MessageReadWriter::ReadCallback r) mutable {
        if (!r) {
          stream->reset();
          return on_handshake(r.error());
        }
        auto &handshake = r.value();
        // TODO(turuslan): `MessageReadWriterUvarint` reuse buffer
        if (not handshake) {
          static auto empty = std::make_shared<std::vector<uint8_t>>();
          handshake = empty;
        }
        on_handshake(handshake);
      };
      frame_stream->read(std::move(cb));
    };
    frame_stream->write(handshake, std::move(cb));
  }

  template <typename Handshake, typename OnHandshake>
  void handshake(std::shared_ptr<Stream> stream,
                 std::shared_ptr<MessageReadWriterUvarint> frame_stream,
                 const Handshake &handshake,
                 OnHandshake on_handhake) {
    auto cb = [stream, frame_stream, on_handhake = std::move(on_handhake)](
                  MessageReadWriterUvarint::ReadCallback r) mutable {
      if (not r) {
        return on_handhake(r.error());
      }
      auto r2 = scale::decode<Handshake>(*r.value());
      if (not r2) {
        stream->reset();
      }
      on_handhake(std::move(r2));
    };
    handshakeRaw(std::move(stream),
                 std::move(frame_stream),
                 scale::encode(handshake).value(),
                 std::move(cb));
  }
}  // namespace kagome::network::notifications
