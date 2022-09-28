/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_HELPERS_STREAM_READ_BUFFER_HPP
#define KAGOME_NETWORK_HELPERS_STREAM_READ_BUFFER_HPP

#include "network/helpers/stream_proxy.hpp"

namespace libp2p::connection {
  struct StreamReadBuffer : StreamProxy,
                            std::enable_shared_from_this<StreamReadBuffer> {
    std::shared_ptr<std::vector<uint8_t>> buffer;
    size_t begin{};
    size_t end{};

    StreamReadBuffer(std::shared_ptr<Stream> stream, size_t capacity)
        : StreamProxy{std::move(stream)},
          buffer{std::make_shared<std::vector<uint8_t>>(capacity)} {}

    size_t size() const {
      return end - begin;
    }

    void readFull(gsl::span<uint8_t> out, size_t n, ReadCallbackFunc cb) {
      readSome(out,
               out.size(),
               [weak{weak_from_this()}, out, n, cb{std::move(cb)}](
                   outcome::result<size_t> _r) mutable {
                 if (auto self{weak.lock()}) {
                   if (_r.has_error()) {
                     return cb(_r.error());
                   }
                   const auto &r = _r.value();
                   const auto _r{gsl::narrow<ptrdiff_t>(r)};
                   BOOST_ASSERT(_r <= out.size());
                   if (_r == out.size()) {
                     return cb(n);
                   }
                   self->readFull(out.subspan(r), n, std::move(cb));
                 }
               });
    }

    void read(gsl::span<uint8_t> out, size_t n, ReadCallbackFunc cb) override {
      BOOST_ASSERT(out.size() >= gsl::narrow<ptrdiff_t>(n));
      readFull(out.first(n), n, std::move(cb));
    }

    void readSome(gsl::span<uint8_t> out,
                  size_t n,
                  ReadCallbackFunc cb) override {
      BOOST_ASSERT(out.size() >= gsl::narrow<ptrdiff_t>(n));
      if (n == 0) {
        return cb(n);
      }
      if (size() != 0) {
        n = std::min(n, size());
        std::copy_n(buffer->begin() + begin, n, out.begin());
        begin += n;
        return cb(n);
      }
      stream->readSome(
          *buffer,
          buffer->size(),
          [weak{weak_from_this()}, out, n, cb{std::move(cb)}, buffer{buffer}](
              outcome::result<size_t> _r) mutable {
            if (auto self{weak.lock()}) {
              if (_r.has_error()) {
                return cb(_r.error());
              }
              const auto &r = _r.value();
              self->begin = 0;
              self->end = r;
              self->readSome(out, n, std::move(cb));
            }
          });
    }
  };
}  // namespace libp2p::connection

namespace kagome::network {
  inline void streamReadBuffer(libp2p::StreamAndProtocol &result) {
    constexpr size_t kBuffer{1 << 16};
    result.stream = std::make_shared<libp2p::connection::StreamReadBuffer>(
        std::move(result.stream), kBuffer);
  }

  inline void streamReadBuffer(libp2p::StreamAndProtocolOrError &result) {
    if (result.has_value()) {
      streamReadBuffer(result.value());
    }
  }
}  // namespace kagome::network

#endif  // KAGOME_NETWORK_HELPERS_STREAM_READ_BUFFER_HPP
