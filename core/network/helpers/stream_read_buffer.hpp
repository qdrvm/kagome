/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "network/helpers/stream_proxy_base.hpp"

#include <libp2p/basic/read_return_size.hpp>
#include <libp2p/common/ambigous_size.hpp>
#include <libp2p/connection/stream_and_protocol.hpp>
#include <thread>

#include "log/logger.hpp"

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

    void read(BytesOut out, size_t n, ReadCallbackFunc cb) override {
      ambigousSize(out, n);
      readReturnSize(shared_from_this(), out, std::move(cb));
    }

    /**
     * Read from buffer.
     */
    size_t readFromBuffer(BytesOut out) {
      // can't read more bytes than available
      auto n = std::min(out.size(), size());
      BOOST_ASSERT(n != 0);
      // copy bytes from buffer
      std::copy_n(buffer->begin() + begin, n, out.begin());
      // consume buffer bytes
      begin += n;
      return n;
    }

    void readSome(BytesOut out, size_t n, ReadCallbackFunc cb) override {
      ambigousSize(out, n);
      if (out.empty()) {
        return deferReadCallback(out.size(), std::move(cb));
      }
      if (size() != 0) {
        // read available buffer bytes
        return deferReadCallback(readFromBuffer(out), std::move(cb));
      }
      // read to fill buffer
      stream->readSome(
          *buffer,
          buffer->size(),
          [weak{weak_from_this()}, out, cb{std::move(cb)}, buffer{buffer}](
              outcome::result<size_t> _r) mutable {
            if (auto self{weak.lock()}) {
              if (_r.has_error()) {
                return self->deferReadCallback(_r.error(), std::move(cb));
              }
              const auto &r = _r.value();
              // fill buffer
              self->begin = 0;
              self->end = r;
              // read available buffer bytes
              self->deferReadCallback(self->readFromBuffer(out), std::move(cb));
            }
          });
    }
  };
}  // namespace libp2p::connection

namespace kagome::network {
  struct StreamWrapper final : libp2p::connection::Stream {
    std::shared_ptr<libp2p::connection::StreamReadBuffer> stream_;
    log::Logger logger_ = log::createLogger("Stream", "network");
    const std::thread::id this_id_{std::this_thread::get_id()};

    void check() const {
      // BOOST_ASSERT(this_id_ == std::this_thread::get_id());
    }

    StreamWrapper(std::shared_ptr<libp2p::connection::StreamReadBuffer> stream)
        : stream_{std::move(stream)} {}

    bool isClosedForRead() const override {
      return stream_->isClosedForRead();
    }

    bool isClosedForWrite() const override {
      return stream_->isClosedForWrite();
    }

    bool isClosed() const override {
      return stream_->isClosed();
    }

    void close(VoidResultHandlerFunc cb) override {
      check();
      stream_->close(std::move(cb));
    }

    void reset() override {
      check();
      stream_->reset();
    }

    void adjustWindowSize(uint32_t new_size,
                          VoidResultHandlerFunc cb) override {
      stream_->adjustWindowSize(new_size, std::move(cb));
    }

    outcome::result<bool> isInitiator() const override {
      return stream_->isInitiator();
    }

    outcome::result<libp2p::peer::PeerId> remotePeerId() const override {
      return stream_->remotePeerId();
    }

    outcome::result<libp2p::multi::Multiaddress> localMultiaddr()
        const override {
      return stream_->localMultiaddr();
    }

    outcome::result<libp2p::multi::Multiaddress> remoteMultiaddr()
        const override {
      return stream_->remoteMultiaddr();
    }

    void read(libp2p::BytesOut out,
              size_t bytes,
              ReadCallbackFunc cb) override {
      check();
      stream_->read(out, bytes, std::move(cb));
    }

    void readSome(libp2p::BytesOut out,
                  size_t bytes,
                  ReadCallbackFunc cb) override {
      check();
      stream_->readSome(out, bytes, std::move(cb));
    }

    void deferReadCallback(outcome::result<size_t> res,
                           ReadCallbackFunc cb) override {
      stream_->deferReadCallback(std::move(res), std::move(cb));
    }

    void writeSome(libp2p::BytesIn in,
                   size_t bytes,
                   WriteCallbackFunc cb) override {
      check();
      stream_->writeSome(in, bytes, std::move(cb));
    }

    void deferWriteCallback(std::error_code ec, WriteCallbackFunc cb) override {
      stream_->deferWriteCallback(ec, std::move(cb));
    }
  };

  /**
   * Wrap stream from `setProtocolHandler`.
   * Makes reading from stream buffered.
   */
  inline void streamReadBuffer(libp2p::StreamAndProtocol &result) {
    constexpr size_t kBuffer{1 << 16};
    result.stream = std::make_shared<StreamWrapper>(
        std::make_shared<libp2p::connection::StreamReadBuffer>(
            std::move(result.stream), kBuffer));
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
