/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <functional>
#include <memory>

#include <libp2p/basic/message_read_writer_uvarint.hpp>
#include <libp2p/basic/protobuf_message_read_writer.hpp>
#include <outcome/outcome.hpp>

#include "network/adapters/protobuf.hpp"
#include "network/adapters/uvar.hpp"
#include "network/helpers/message_read_writer.hpp"
#include "scale/scale.hpp"

namespace kagome::network {
  /**
   * Read and write messages, encoded into protobuf with a prepended varint
   */
  class ProtobufMessageReadWriter
      : public std::enable_shared_from_this<ProtobufMessageReadWriter> {
    template <typename MsgType>
    using ReadCallback = std::function<void(outcome::result<MsgType>)>;

    std::shared_ptr<libp2p::basic::MessageReadWriter> read_writer_;

   public:
    explicit ProtobufMessageReadWriter(
        const std::shared_ptr<libp2p::basic::ReadWriter> &read_writer)
        : read_writer_(
            std::make_shared<libp2p::basic::MessageReadWriterUvarint>(
                read_writer)) {}

    /**
     * Read a Protobuf message from the channel
     * @tparam MsgType - type of the message
     * @param cb to be called, when the message is read, or error happens
     */
    template <typename MsgType>
    void read(ReadCallback<MsgType> &&cb) const {
      read_writer_->read(
          [self{shared_from_this()}, cb = std::move(cb)](auto &&read_res) {
            if (!read_res) {
              return cb(read_res.error());
            }

            using ProtobufRW =
                MessageReadWriter<ProtobufMessageAdapter<MsgType>, NoSink>;

            MsgType msg;
            if (read_res.value()) {
              if (auto msg_res = ProtobufRW::read(
                      msg, *read_res.value(), read_res.value()->begin());
                  !msg_res) {
                return cb(msg_res.error());
              }
            }
            return cb(std::move(msg));
          });
    }

    /**
     * Serialize to protobuf message and write it to the channel
     * @tparam MsgType - type of the message
     * @param msg to be written
     * @param cb to be called, when the message is written, or error happens
     */
    template <typename MsgType>
    void write(const MsgType &msg,
               libp2p::basic::Writer::WriteCallbackFunc &&cb) const {
      using ProtobufRW =
          MessageReadWriter<ProtobufMessageAdapter<MsgType>, NoSink>;

      // TODO(iceseer): PRE-523 cache this vector
      std::vector<uint8_t> out;
      auto it = ProtobufRW::write(msg, out);

      std::span<uint8_t> data(it.base(),
                              out.size() - std::distance(out.begin(), it));

      read_writer_->write(data,
                          [self{shared_from_this()},
                           out{std::move(out)},
                           cb = std::move(cb)](auto &&write_res) {
                            if (!write_res) {
                              return cb(write_res.error());
                            }
                            cb(outcome::success());
                          });
    }
  };

}  // namespace kagome::network
