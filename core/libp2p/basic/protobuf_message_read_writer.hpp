/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PROTOBUF_MESSAGE_READ_WRITER_HPP
#define KAGOME_PROTOBUF_MESSAGE_READ_WRITER_HPP

#include "libp2p/basic/message_read_writer.hpp"

namespace libp2p::basic {
  /**
   * Reader/writer for Protobuf messages; user of this class MUST ENSURE that no
   * two parallel reads or writes happens - this will lead to UB
   */
  class ProtobufMessageReadWriter
      : public std::enable_shared_from_this<ProtobufMessageReadWriter> {
    template <typename ProtoMsgType>
    using ReadCallbackFunc = std::function<void(outcome::result<ProtoMsgType>)>;

   public:
    /**
     * Create an instance of read/writer
     * @param read_writer, with the help of which messages are read & written
     */
    explicit ProtobufMessageReadWriter(
        std::shared_ptr<MessageReadWriter> read_writer);

    /**
     * Read a message from the connection
     * @tparam ProtoMsgType - type of the message to be read
     * @param cb to be called, when the message is read, or error happens
     */
    template <
        typename ProtoMsgType,
        typename = typename std::is_default_constructible<ProtoMsgType>::value>
    void read(ReadCallbackFunc<ProtoMsgType> cb) {
      read_writer_->read(
          [self{shared_from_this()}, cb = std::move(cb)](auto &&res) mutable {
            if (!res) {
              return cb(res.error());
            }

            auto &&buf = res.value();
            ProtoMsgType msg;
            msg.ParseFromArray(buf->data(), buf->size());
            cb(std::move(msg));
          });
    }

    /**
     * Write a message to the connection
     * @tparam ProtoMsgType - type of the message to be written
     * @param msg to be written
     * @param cb to be called, when the message is written, or error happens
     */
    template <
        typename ProtoMsgType,
        typename = typename std::is_default_constructible<ProtoMsgType>::value>
    void write(const ProtoMsgType &msg, Writer::WriteCallbackFunc cb) {
      auto msg_bytes = std::make_shared<std::vector<uint8_t>>();
      msg_bytes->reserve(msg.ByteSize());
      msg.SerializeToArray(msg_bytes->data(), msg.ByteSize());
      read_writer_->write(*msg, [msg, cb = std::move(cb)](auto &&res) {
        cb(std::forward<decltype(res)>(res));
      });
    }

   private:
    std::shared_ptr<MessageReadWriter> read_writer_;
  };
}  // namespace libp2p::basic

#endif  // KAGOME_PROTOBUF_MESSAGE_READ_WRITER_HPP
