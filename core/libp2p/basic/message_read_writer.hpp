/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MESSAGE_READ_WRITER_HPP
#define KAGOME_MESSAGE_READ_WRITER_HPP

#include <memory>
#include <vector>

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
  template <typename Message>
  class MessageReadWriter {
   public:
    /**
     * Read a message from the provided connection
     * @param conn to read message from
     * @param msg - reference to (\tparam Message) type, to which put the read
     * message
     * @param cb to be called after a successful read or error
     */
    void read(std::shared_ptr<ReadWriter> conn, Message &msg,
              std::function<void(outcome::result<void>)> cb) {
      // each Protobuf message in Libp2p is prepended with a varint, showing its
      // length
      VarintReader::readVarint(
          conn, [conn, cb = std::move(cb), &msg](auto varint_opt) mutable {
            if (!varint_opt) {
              return cb(MessageReadWriterError::VARINT_EXPECTED);
            }

            auto msg_len = varint_opt->toUInt64();
            auto msg_buf = std::make_shared<kagome::common::Buffer>(msg_len, 0);
            conn->read(
                *msg_buf, msg_len,
                [conn, cb = std::move(cb), msg_buf, &msg](auto &&res) mutable {
                  if (!res) {
                    return cb(res.error());
                  }

                  msg.ParseFromArray(msg_buf->data(), msg_buf->size());
                  cb(outcome::success());
                });
          });
    }

    /**
     * Write a message to the provided connection
     * @param conn to write message to
     * @param msg to be written
     * @param cb to be called after a successful right or error
     */
    void write(const std::shared_ptr<ReadWriter> &conn, Message &&msg,
               Writer::WriteCallbackFunc cb) {
      auto varint_len = multi::UVarint{msg.ByteSize()};

      auto msg_bytes = std::make_shared<kagome::common::Buffer>(
          varint_len.size() + msg.ByteSize(), 0);
      msg_bytes->put(varint_len.toBytes());
      msg.SerializeToArray(msg_bytes->data() + varint_len.size(),
                           msg.ByteSize());
      conn->write(*msg_bytes, msg_bytes->size(),
                  [conn, msg_bytes, cb = std::move(cb)](auto &&res) {
                    cb(std::forward<decltype(res)>(res));
                  });
    }
  };
}  // namespace libp2p::basic

#endif  // KAGOME_MESSAGE_READ_WRITER_HPP
