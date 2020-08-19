/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MESSAGE_READ_WRITER_HPP
#define KAGOME_MESSAGE_READ_WRITER_HPP

#include <functional>
#include <memory>
#include <gsl/span>

namespace kagome::network {
  /**
   * Chain specific messages read-writer
   */
  template<typename Ancestor> struct MessageReadWriter final {
    using AncestorType = Ancestor;
    using BufferContainer = std::vector<uint8_t>;

   public:
    MessageReadWriter(std::shared_ptr<AncestorType> sink) : sink_(std::move(sink))
    { }

    MessageReadWriter(MessageReadWriter &&) = default;
    MessageReadWriter& operator=(MessageReadWriter &&) = default;

    MessageReadWriter(const MessageReadWriter&) = delete;
    MessageReadWriter& operator=(const MessageReadWriter&) = delete;

   public:
    template<typename T, typename Adapter>
    BufferContainer::iterator write(const T &t, BufferContainer &out, size_t reserved) {
      constexpr size_t r = Adapter::size(t) + reserved;
      out.resize(r);

      BufferContainer::iterator loaded = out.end();
      if (sink_)
        loaded = sink_->write(t, out, r);

      assert(std::distance(out.begin(), loaded) >= r);
      return Adapter::write(t, out, loaded);
    }

   private:
    std::shared_ptr<AncestorType> sink_;
  };

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
