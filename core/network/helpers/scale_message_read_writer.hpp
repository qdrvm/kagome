/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KAGOME_SCALE_MESSAGE_READ_WRITER_HPP
#define KAGOME_SCALE_MESSAGE_READ_WRITER_HPP

#include <functional>
#include <memory>

#include <libp2p/basic/message_read_writer_uvarint.hpp>
#include <outcome/outcome.hpp>

#include "scale/scale.hpp"

namespace kagome::network {
  /**
   * Read and write messages, encoded into SCALE with a prepended varint
   */
  class ScaleMessageReadWriter
      : public std::enable_shared_from_this<ScaleMessageReadWriter> {
    template <typename MsgType>
    using ReadCallback = std::function<void(outcome::result<MsgType>)>;

   public:
    explicit ScaleMessageReadWriter(
        std::shared_ptr<libp2p::basic::MessageReadWriter> read_writer);
    explicit ScaleMessageReadWriter(
        const std::shared_ptr<libp2p::basic::ReadWriter> &read_writer);

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

            auto msg_res = scale::decode<MsgType>(*read_res.value());
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
    void write(const MsgType &msg,
               libp2p::basic::Writer::WriteCallbackFunc cb) const {
      auto encoded_msg_res = scale::encode(msg);
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
                            cb(outcome::success());
                          });
    }

   private:
    std::shared_ptr<libp2p::basic::MessageReadWriter> read_writer_;
  };
}  // namespace kagome::network

#endif  // KAGOME_SCALE_MESSAGE_READ_WRITER_HPP
