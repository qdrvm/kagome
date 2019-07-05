/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MESSAGE_READ_WRITER_HPP
#define KAGOME_MESSAGE_READ_WRITER_HPP

#include <memory>
#include <vector>

#include <gsl/span>
#include "common/buffer.hpp"
#include "libp2p/basic/message_read_writer_error.hpp"
#include "libp2p/basic/readwriter.hpp"
#include "libp2p/basic/varint_reader.hpp"
#include "libp2p/multi/uvarint.hpp"

namespace libp2p::basic {
  /**
   * Allows to read and write messages of (\tparam Message) type
   * @tparam Message - type of the message; for now, works with Protobuf
   * messages only
   */
  class MessageReadWriter
      : public std::enable_shared_from_this<MessageReadWriter> {
   public:
    explicit MessageReadWriter(std::shared_ptr<ReadWriter> conn)
        : conn_{std::move(conn)} {}

    void read(gsl::span<uint8_t> buffer, Reader::ReadCallbackFunc cb) {
      if (buffer.empty()) {
        return cb(MessageReadWriterError::LITTLE_BUFFER);
      }
      VarintReader::readVarint(
          conn_,
          [self{shared_from_this()}, cb = std::move(cb),
           buffer](auto varint_opt) mutable {
            if (!varint_opt) {
              return cb(MessageReadWriterError::VARINT_EXPECTED);
            }

            auto msg_len = varint_opt->toUInt64();
            if (static_cast<uint64_t>(buffer.size()) < msg_len) {
              return cb(MessageReadWriterError::LITTLE_BUFFER);
            }

            self->conn_->read(buffer, msg_len,
                              [self, cb = std::move(cb)](auto &&res) mutable {
                                cb(std::forward<decltype(res)>(res));
                              });
          });
    }

    void write(gsl::span<const uint8_t> buffer, Writer::WriteCallbackFunc cb) {
      if (buffer.empty()) {
        return cb(outcome::success());
      }

      auto varint_len = multi::UVarint{static_cast<uint64_t>(buffer.size())};

      auto msg_bytes = std::make_shared<std::vector<uint8_t>>();
      msg_bytes->reserve(varint_len.size() + buffer.size());
      msg_bytes->insert(msg_bytes->end(),
                        std::make_move_iterator(varint_len.toVector().begin()),
                        std::make_move_iterator(varint_len.toVector().end()));
      msg_bytes->insert(msg_bytes->end(), buffer.begin(), buffer.end());

      conn_->write(*msg_bytes, msg_bytes->size(),
                   [cb = std::move(cb)](auto &&res) {
                     cb(std::forward<decltype(res)>(res));
                   });
    }

   private:
    std::shared_ptr<ReadWriter> conn_;
  };
}  // namespace libp2p::basic

#endif  // KAGOME_MESSAGE_READ_WRITER_HPP
