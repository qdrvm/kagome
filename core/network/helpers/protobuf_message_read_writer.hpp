/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PROTOBUF_MESSAGE_READ_WRITER_HPP
#define KAGOME_PROTOBUF_MESSAGE_READ_WRITER_HPP

#include <functional>
#include <memory>

#include <libp2p/basic/message_read_writer_uvarint.hpp>
#include <libp2p/basic/protobuf_message_read_writer.hpp>
#include <outcome/outcome.hpp>

#include "network/helpers/message_read_writer.hpp"
#include "network/helpers/adapters.hpp"
#include "scale/scale.hpp"

namespace kagome::network {
  template<typename T> struct UVarMessageAdapter {
    static size_t size(const T &t) {
      return 16; // LE128
    }

    static std::vector<uint8_t>::iterator write(const network::BlocksRequest &t, std::vector<uint8_t> &out, std::vector<uint8_t>::iterator loaded) {
      assert(std::distance(out.begin(), loaded) >= UVarMessageAdapter<T>::size(t));
      constexpr size_t kContinuationBitMask{0x80ull};
      constexpr size_t kSignificantBitsMask{0x7Full};

      auto data_sz = out.size();
      auto sz_start = --loaded;
      do {
        assert(std::distance(out.begin(), loaded) >= 1);
        *(loaded--) = (data_sz & kSignificantBitsMask) | kContinuationBitMask;
      } while(data_sz >>= size_t(7ull));
      *sz_start &= uint8_t(kSignificantBitsMask);

      return ++loaded;
    }
  };
  /**
   * Read and write messages, encoded into SCALE with a prepended varint
   */
  class ProtobufMessageReadWriter : public std::enable_shared_from_this<ProtobufMessageReadWriter> {
    template <typename MsgType>
    using ReadCallback = std::function<void(outcome::result<MsgType>)>;

   public:
    explicit ProtobufMessageReadWriter(std::shared_ptr<libp2p::basic::MessageReadWriter> read_writer)
        : read_writer_{std::move(read_writer)} {}
    explicit ProtobufMessageReadWriter(const std::shared_ptr<libp2p::basic::ReadWriter> &read_writer)
        : read_writer_{std::make_shared<libp2p::basic::MessageReadWriterUvarint>(
        read_writer)} {}

    /**
     * Read a Protobuf message from the channel
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
     * Serialize to protobuf message and write it to the channel
     * @tparam MsgType - type of the message
     * @param msg to be written
     * @param cb to be called, when the message is written, or error happens
     */
    template <typename MsgType>
    void write(const MsgType &msg, libp2p::basic::Writer::WriteCallbackFunc cb) const {
      using ProtobufRW = MessageReadWriter<ProtobufMessageAdapter<MsgType>, NoSink>;
      using UVarRW = MessageReadWriter<UVarMessageAdapter<MsgType>, ProtobufRW>;

      std::vector<uint8_t> out;
      auto it = UVarRW::write(msg, out);

      gsl::span<uint8_t> data(&*it, out.size() - std::distance(out.begin(), it));
      assert(!data.empty());

      read_writer_->write(data,
                          [self{shared_from_this()}, out{std::move(out)}, cb = std::move(cb)](auto &&write_res) {
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

#endif  // KAGOME_PROTOBUF_MESSAGE_READ_WRITER_HPP
