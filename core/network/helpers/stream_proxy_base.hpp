/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/connection/stream.hpp>

namespace libp2p::connection {
  /**
   * Allows implementing only required subset of virtual methods.
   * Implements all virtual methods and forwards them to stream.
   */
  struct StreamProxyBase : Stream {
    std::shared_ptr<Stream> stream;

    explicit StreamProxyBase(std::shared_ptr<Stream> stream)
        : stream{std::move(stream)} {}

    void read(std::span<uint8_t> out,
              size_t bytes,
              ReadCallbackFunc cb) override {
      stream->read(out, bytes, std::move(cb));
    }
    void readSome(std::span<uint8_t> out,
                  size_t bytes,
                  ReadCallbackFunc cb) override {
      stream->readSome(out, bytes, std::move(cb));
    }
    void deferReadCallback(outcome::result<size_t> res,
                           ReadCallbackFunc cb) override {
      stream->deferReadCallback(res, std::move(cb));
    }

    void write(std::span<const uint8_t> in,
               size_t bytes,
               WriteCallbackFunc cb) override {
      stream->write(in, bytes, std::move(cb));
    }
    void writeSome(std::span<const uint8_t> in,
                   size_t bytes,
                   WriteCallbackFunc cb) override {
      stream->writeSome(in, bytes, std::move(cb));
    }
    void deferWriteCallback(std::error_code ec, WriteCallbackFunc cb) override {
      stream->deferWriteCallback(ec, std::move(cb));
    }

    bool isClosedForRead() const override {
      return stream->isClosedForRead();
    }
    bool isClosedForWrite() const override {
      return stream->isClosedForWrite();
    }
    bool isClosed() const override {
      return stream->isClosed();
    }
    void close(VoidResultHandlerFunc cb) override {
      stream->close(std::move(cb));
    }
    void reset() override {
      stream->reset();
    }
    void adjustWindowSize(uint32_t new_size,
                          VoidResultHandlerFunc cb) override {
      stream->adjustWindowSize(new_size, std::move(cb));
    }
    outcome::result<bool> isInitiator() const override {
      return stream->isInitiator();
    }
    outcome::result<peer::PeerId> remotePeerId() const override {
      return stream->remotePeerId();
    }
    outcome::result<multi::Multiaddress> localMultiaddr() const override {
      return stream->localMultiaddr();
    }
    outcome::result<multi::Multiaddress> remoteMultiaddr() const override {
      return stream->remoteMultiaddr();
    }
  };
}  // namespace libp2p::connection
