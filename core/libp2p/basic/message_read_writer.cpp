/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/basic/message_read_writer.hpp"

#include <vector>

#include "libp2p/basic/message_read_writer_error.hpp"
#include "libp2p/basic/varint_reader.hpp"
#include "libp2p/multi/uvarint.hpp"

namespace libp2p::basic {
  MessageReadWriter::MessageReadWriter(std::shared_ptr<ReadWriter> conn)
      : conn_{std::move(conn)} {}

  void MessageReadWriter::read(
      gsl::span<uint8_t> buffer,
      std::function<void(outcome::result<size_t>, size_t)> cb) {
    if (buffer.empty()) {
      return cb(MessageReadWriterError::BUFFER_EMPTY, 0);
    }
    VarintReader::readVarint(
        conn_,
        [self{shared_from_this()}, cb = std::move(cb),
         buffer](auto varint_opt) mutable {
          if (!varint_opt) {
            return cb(MessageReadWriterError::VARINT_EXPECTED, 0);
          }

          auto msg_len = varint_opt->toUInt64();
          if (static_cast<uint64_t>(buffer.size()) < msg_len) {
            return cb(MessageReadWriterError::LITTLE_BUFFER, msg_len);
          }

          self->conn_->read(buffer, msg_len,
                            [self, cb = std::move(cb)](auto &&res) mutable {
                              cb(std::forward<decltype(res)>(res), 0);
                            });
        });
  }

  void MessageReadWriter::write(gsl::span<const uint8_t> buffer,
                                Writer::WriteCallbackFunc cb) {
    if (buffer.empty()) {
      return cb(MessageReadWriterError::BUFFER_EMPTY);
    }

    auto varint_len = multi::UVarint{static_cast<uint64_t>(buffer.size())};

    auto msg_bytes = std::make_shared<std::vector<uint8_t>>();
    msg_bytes->reserve(varint_len.size() + buffer.size());
    msg_bytes->insert(msg_bytes->end(),
                      std::make_move_iterator(varint_len.toVector().begin()),
                      std::make_move_iterator(varint_len.toVector().end()));
    msg_bytes->insert(msg_bytes->end(), buffer.begin(), buffer.end());

    conn_->write(
        *msg_bytes, msg_bytes->size(),
        [cb = std::move(cb), varint_size = varint_len.size()](auto &&res) {
          if (!res) {
            return cb(res.error());
          }
          // hide a written varint from the user of the method
          cb(res.value() - varint_size);
        });
  }
}  // namespace libp2p::basic
