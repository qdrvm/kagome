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
    static constexpr size_t kContinuationBitMask{0x80ull};
    static constexpr size_t kSignificantBitsMask{0x7Full};
    static constexpr size_t kSignificantBitsMaskMSB{kSignificantBitsMask << 56};

    static size_t size(const T &t) {
      return sizeof(uint64_t);
    }

    static std::vector<uint8_t>::iterator write(const T &t, std::vector<uint8_t> &out, std::vector<uint8_t>::iterator loaded) {
      const auto remains = std::distance(out.begin(), loaded);
      assert(remains >= UVarMessageAdapter<T>::size(t));

      auto data_sz = out.size() - remains;
      while ((data_sz & kSignificantBitsMaskMSB) == 0 && data_sz != 0)
        data_sz <<= size_t(7ull);

      auto sz_start = --loaded;
      do {
        assert(std::distance(out.begin(), loaded) >= 1);
        *loaded-- = ((data_sz & kSignificantBitsMaskMSB) >> 55ull) | kContinuationBitMask;
      } while(data_sz <<= size_t(7ull));

      *sz_start &= uint8_t(kSignificantBitsMask);
      return ++loaded;
    }

    static libp2p::outcome::result<std::vector<uint8_t>::const_iterator> read(const std::vector<uint8_t> &src, std::vector<uint8_t>::const_iterator from) {
      if (from == src.end())
        return outcome::failure(boost::system::error_code{});

      uint64_t sz = 0;
      constexpr size_t kPayloadSize = ((UVarMessageAdapter<T>::size() << size_t(3)) + size_t(6)) / size_t(7);
      const auto loaded = std::distance(src.begin(), from);

      auto const * const beg = &*from;
      auto const * const end = &src[std::min(loaded + kPayloadSize, src.size())];
      auto const * ptr = beg;
      size_t counter = 0;

      do {
        sz |= (static_cast<uint64_t>(*ptr & kSignificantBitsMask) << (size_t(7) * counter++));
      } while(uint8_t(0) != (*ptr++ & uint8_t(kContinuationBitMask)) && ptr != end);

      if (sz != src.size() - (loaded + counter))
        return outcome::failure(boost::system::error_code{});

      std::advance(from, counter);
      return from;
    }
  };
  
  /**
   * Read and write messages, encoded into protobuf with a prepended varint
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

            using ProtobufRW = MessageReadWriter<ProtobufMessageAdapter<MsgType>, NoSink>;
            using UVarRW = MessageReadWriter<UVarMessageAdapter<MsgType>, ProtobufRW>;

            //auto msg_res = UVarRW::read(*read_res.value());
            //if (!msg_res) {
              //return cb(msg_res.error());
            //}

            //return cb(std::move(msg_res.value()));
            return cb(read_res.error());
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

      // TODO(iceseer) : try to cache this vector
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
