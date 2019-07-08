/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PROTOBUF_MESSAGE_READ_WRITER_HPP
#define KAGOME_PROTOBUF_MESSAGE_READ_WRITER_HPP

#include "libp2p/basic/message_read_writer.hpp"
#include "libp2p/basic/message_read_writer_error.hpp"

namespace libp2p::basic {
  /**
   * Reader/writer for Protobuf messages; user of this class MUST ENSURE that no
   * two parallel reads or writes happens - this will lead to UB
   */
  class ProtobufMessageReadWriter
      : public std::enable_shared_from_this<ProtobufMessageReadWriter> {
    template <typename ProtoMsgType>
    using ReadCallbackFunc =
        std::function<void(outcome::result<std::shared_ptr<ProtoMsgType>>)>;

   public:
    /**
     * Create an instance of read/writer
     * @param conn, from/to which the messages are to be read/written
     */
    explicit ProtobufMessageReadWriter(std::shared_ptr<ReadWriter> conn)
        : conn_{conn},
          read_writer_{std::make_shared<MessageReadWriter>(std::move(conn))} {}

    /**
     * Read a message from the connection
     * @tparam ProtoMsgType - type of the message to be read
     * @param cb to be called, when the message is read, or error happens
     */
    template <typename ProtoMsgType>
    void read(ReadCallbackFunc<ProtoMsgType> cb) {
      // we don't know size of the message in advance, so firstly allocate 1KB,
      // maybe that'll be enough
      static constexpr size_t kInitialSize = 1024;
      auto buf = std::make_shared<std::vector<uint8_t>>(kInitialSize, 0);

      read_writer_->read(
          *buf,
          [self{shared_from_this()}, cb = std::move(cb), buf](
              auto &&res, auto &&msg_len) mutable {
            if (res) {
              return self->readCompleted(buf, std::move(cb));
            }
            if (res.error() != MessageReadWriterError::LITTLE_BUFFER) {
              return cb(std::forward<decltype(res)>(res));
            }

            // the buffer was not big enough; increase its size and this time
            // read from a raw connection
            if (msg_len == 0) {
              // should never happen
              return cb(MessageReadWriterError::INTERNAL_ERROR);
            }
            buf->insert(buf->end(), msg_len - buf->size(), 0);
            self->conn_->read(
                *buf, msg_len,
                [self, buf, cb = std::move(cb)](auto &&res) mutable {
                  if (!res) {
                    return cb(std::forward<decltype(res)>(res));
                  }
                  self->readCompleted(buf, std::move(cb));
                });
          });
    }

    /**
     * Write a message to the connection
     * @tparam ProtoMsgType - type of the message to be written
     * @param msg to be written
     * @param cb to be called, when the message is written, or error happens
     */
    template <typename ProtoMsgType>
    void write(const ProtoMsgType &msg, Writer::WriteCallbackFunc cb) {
      auto msg_bytes = std::make_shared<std::vector<uint8_t>>();
      msg_bytes->reserve(msg.ByteSize());
      msg.SerializeToArray(msg_bytes->data(), msg.ByteSize());
      read_writer_->write(*msg, [msg, cb = std::move(cb)](auto &&res) {
        cb(std::forward<decltype(res)>(res));
      });
    }

   private:
    template <typename ProtoMsgType>
    void readCompleted(const std::shared_ptr<std::vector<uint8_t>> &read_buf,
                       ReadCallbackFunc<ProtoMsgType> cb) {
      auto msg = std::make_shared<ProtoMsgType>();
      msg->ParseFromArray(read_buf->data(), read_buf->size());
      cb(std::move(msg));
    }

    std::shared_ptr<ReadWriter> conn_;
    std::shared_ptr<MessageReadWriter> read_writer_;
  };
}  // namespace libp2p::basic

#endif  // KAGOME_PROTOBUF_MESSAGE_READ_WRITER_HPP
