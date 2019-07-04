/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/basic/message_read_writer.hpp"

#include <vector>

#include "common/buffer.hpp"
#include "libp2p/basic/varint_reader.hpp"
#include "libp2p/multi/uvarint.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::basic, MessageReadWriterError, e) {
  using E = libp2p::basic::MessageReadWriterError;
  switch (e) {
    case E::VARINT_EXPECTED:
      return "varint expected at the beginning of Protobuf message";
  }
  return "unknown error";
}

namespace libp2p::basic {
  using kagome::common::Buffer;

  template <typename Message>
  void MessageReadWriter<Message>::read(
      std::shared_ptr<ReadWriter> conn,
      std::function<void(outcome::result<Message>)> cb) {
    // each Protobuf message in Libp2p is prepended with a varint, showing its
    // length
    VarintReader::readVarint(
        conn,
        [self{this->shared_from_this()}, conn,
         cb = std::move(cb)](auto varint_opt) mutable {
          if (!varint_opt) {
            return cb(MessageReadWriterError::VARINT_EXPECTED);
          }

          auto msg_len = varint_opt->toUint64();
          auto msg_buf = std::make_shared<Buffer>(msg_len, 0);
          conn->read(*msg_buf, msg_len,
                     [self{std::move(self)}, conn, cb = std::move(cb),
                      msg_buf](auto &&res) {
                       if (!res) {
                         return cb(res.error());
                       }

                       Message msg;
                       msg.ParseFromArray(msg_buf->data(), msg_buf->size());
                       cb(std::move(msg));
                     });
        });
  }

  template <typename Message>
  void MessageReadWriter<Message>::write(
      const std::shared_ptr<ReadWriter> &conn, Message &&msg,
      Writer::WriteCallbackFunc cb) {
    auto varint_len = multi::UVarint{msg.ByteSize()};

    auto msg_bytes =
        std::make_shared<Buffer>(varint_len.size() + msg.ByteSize(), 0);
    msg_bytes->put(varint_len.toBytes());
    msg.SerializeToArray(msg_bytes->data() + varint_len.size(), msg.ByteSize());
    conn->write(*msg_bytes, msg_bytes->size(),
                [conn, msg_bytes, cb = std::move(cb)](auto &&res) {
                  cb(std::forward<decltype(res)>(res));
                });
  }
}  // namespace libp2p::basic
