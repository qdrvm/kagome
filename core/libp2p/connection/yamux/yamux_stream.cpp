/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "yamux_stream.hpp"

namespace libp2p::connection {

  YamuxStream::YamuxStream(std::shared_ptr<YamuxedConnection> conn,
                           YamuxedConnection::StreamId stream_id)
      : yamux_{std::move(conn)}, stream_id_{stream_id} {}

  YamuxStream::~YamuxStream() {
    this->resetStream();
  }

  outcome::result<size_t> YamuxStream::write(gsl::span<const uint8_t> in) {}

  outcome::result<size_t> YamuxStream::writeSome(gsl::span<const uint8_t> in) {}

  outcome::result<std::vector<uint8_t>> YamuxStream::read(size_t bytes) {}

  outcome::result<std::vector<uint8_t>> YamuxStream::readSome(size_t bytes) {}

  outcome::result<size_t> YamuxStream::read(gsl::span<uint8_t> buf) {}

  outcome::result<size_t> YamuxStream::readSome(gsl::span<uint8_t> buf) {}

  void YamuxStream::reset() {}

  bool YamuxStream::isClosedForRead() const {}

  bool YamuxStream::isClosedForWrite() const {}

  bool YamuxStream::isClosed() const {}

  outcome::result<void> YamuxStream::close() {}

  void YamuxStream::resetStream() {}

}  // namespace libp2p::connection
