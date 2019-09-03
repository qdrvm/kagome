/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SCALE_MESSAGE_READ_WRITER_HPP
#define KAGOME_SCALE_MESSAGE_READ_WRITER_HPP

#include <functional>
#include <memory>

#include <outcome/outcome.hpp>
#include "libp2p/basic/message_read_writer.hpp"
#include "scale/scale.hpp"

namespace libp2p::basic {
  /**
   * Read and write messages, encoded into SCALE with a prepended varint
   */
  class ScaleMessageReadWriter
      : public std::enable_shared_from_this<ScaleMessageReadWriter> {
    template <typename MsgType>
    using ReadCallback = std::function<void(outcome::result<MsgType>)>;

   public:
    explicit ScaleMessageReadWriter(
        std::shared_ptr<MessageReadWriter> read_writer);
    explicit ScaleMessageReadWriter(
        const std::shared_ptr<ReadWriter> &read_writer);

    /**
     * Read a SCALE-encoded message from the channel
     * @tparam MsgType - type of the message
     * @param cb to be called, when the message is read, or error happens
     */
    template <typename MsgType>
    void read(ReadCallback<MsgType> cb) const {
      read_writer_->read(
          [self{shared_from_this()}, cb = std::move(cb)](auto &&read_res) {
            if (!read_res) {
              return cb(read_res.error());
            }

            auto msg_res = kagome::scale::decode<MsgType>(*read_res.value());
            if (!msg_res) {
              return cb(msg_res.error());
            }

            return cb(std::move(msg_res.value()));
          });
    }

    /**
     * SCALE-encode a message and write it to the channel
     * @tparam MsgType - type of the message
     * @param msg to be written
     * @param cb to be called, when the message is written, or error happens
     */
    template <typename MsgType>
    void write(const MsgType &msg, Writer::WriteCallbackFunc cb) const {
      auto encoded_msg_res = kagome::scale::encode(msg);
      if (!encoded_msg_res) {
        return cb(encoded_msg_res.error());
      }
      auto msg_ptr = std::make_shared<std::vector<uint8_t>>(
          std::move(encoded_msg_res.value()));

      read_writer_->write(*msg_ptr,
                          [self{shared_from_this()},
                           msg_ptr,
                           cb = std::move(cb)](auto &&write_res) {
                            if (!write_res) {
                              return cb(write_res.error());
                            }
                          });
    }

   private:
    std::shared_ptr<MessageReadWriter> read_writer_;
  };
}  // namespace libp2p::basic

#endif  // KAGOME_SCALE_MESSAGE_READ_WRITER_HPP
