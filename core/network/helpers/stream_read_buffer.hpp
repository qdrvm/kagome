/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_HELPERS_STREAM_READ_BUFFER_HPP
#define KAGOME_NETWORK_HELPERS_STREAM_READ_BUFFER_HPP

#include "network/helpers/stream_proxy_base.hpp"

namespace libp2p::connection {
  /**
   * Stream with buffered reader.
   *
   * Works around problem when reading varint prefix of big message (bigger than
   * yamux window) causes yamux to send/receive each byte of message in separate
   * packet.
   */
  struct StreamReadBuffer : StreamProxyBase,
                            std::enable_shared_from_this<StreamReadBuffer> {
    std::shared_ptr<std::vector<uint8_t>> buffer;
    size_t begin{};
    size_t end{};

    StreamReadBuffer(std::shared_ptr<Stream> stream, size_t capacity)
        : StreamProxyBase{std::move(stream)},
          buffer{std::make_shared<std::vector<uint8_t>>(capacity)} {}

    /**
     * Returns count of available bytes in buffer.
     */
    size_t size() const {
      return end - begin;
    }

    /**
     * Async loop iteration.
     * Reads `out.size()` remaining bytes of `total` bytes.
     */
    void readFull(gsl::span<uint8_t> out, size_t total, ReadCallbackFunc cb) {
      // read some bytes
      readSome(out,
               out.size(),
               [weak{weak_from_this()}, out, total, cb{std::move(cb)}](
                   outcome::result<size_t> bytes_num_res) mutable {
                 if (auto self{weak.lock()}) {
                   if (bytes_num_res.has_error()) {
                     return cb(bytes_num_res.error());
                   }
                   const auto &bytes_num = bytes_num_res.value();
                   BOOST_ASSERT(bytes_num != 0);
                   const auto bytes_num_ptrdiff{gsl::narrow<ptrdiff_t>(bytes_num)};
                   BOOST_ASSERT(bytes_num_ptrdiff <= out.size());
                   if (bytes_num_ptrdiff == out.size()) {
                     // successfully read last bytes
                     return cb(total);
                   }
                   // read remaining bytes
                   self->readFull(out.subspan(bytes_num_ptrdiff), total, std::move(cb));
                 }
               });
    }

    void read(gsl::span<uint8_t> out, size_t n, ReadCallbackFunc cb) override {
      BOOST_ASSERT(out.size() >= gsl::narrow<ptrdiff_t>(n));
      out = out.first(n);
      readFull(out, n, std::move(cb));
    }

    /**
     * Read from buffer.
     */
    size_t readFromBuffer(gsl::span<uint8_t> out) {
      // can't read more bytes than available
      auto n = std::min(gsl::narrow<size_t>(out.size()), size());
      BOOST_ASSERT(n != 0);
      // copy bytes from buffer
      std::copy_n(buffer->begin() + begin, n, out.begin());
      // consume buffer bytes
      begin += n;
      return n;
    }

    void readSome(gsl::span<uint8_t> out,
                  size_t n,
                  ReadCallbackFunc cb) override {
      BOOST_ASSERT(out.size() >= gsl::narrow<ptrdiff_t>(n));
      out = out.first(n);
      if (out.empty()) {
        return cb(out.size());
      }
      if (size() != 0) {
        // read available buffer bytes
        return cb(readFromBuffer(out));
      }
      // read to fill buffer
      stream->readSome(
          *buffer,
          buffer->size(),
          [weak{weak_from_this()}, out, cb{std::move(cb)}, buffer{buffer}](
              outcome::result<size_t> _r) mutable {
            if (auto self{weak.lock()}) {
              if (_r.has_error()) {
                return cb(_r.error());
              }
              const auto &r = _r.value();
              // fill buffer
              self->begin = 0;
              self->end = r;
              // read available buffer bytes
              cb(self->readFromBuffer(out));
            }
          });
    }
  };
}  // namespace libp2p::connection

namespace kagome::network {
  /**
   * Wrap stream from `setProtocolHandler`.
   * Makes reading from stream buffered.
   */
  inline void streamReadBuffer(libp2p::StreamAndProtocol &result) {
    constexpr size_t kBuffer{1 << 16};
    result.stream = std::make_shared<libp2p::connection::StreamReadBuffer>(
        std::move(result.stream), kBuffer);
  }

  /**
   * Wrap stream from `newStream`.
   * Makes reading from stream buffered.
   */
  inline void streamReadBuffer(libp2p::StreamAndProtocolOrError &result) {
    if (result.has_value()) {
      streamReadBuffer(result.value());
    }
  }
}  // namespace kagome::network

#endif  // KAGOME_NETWORK_HELPERS_STREAM_READ_BUFFER_HPP
