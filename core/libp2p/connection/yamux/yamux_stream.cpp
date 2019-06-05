/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/connection/yamux/yamux_stream.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::connection, YamuxStream::Error, e) {
  using E = libp2p::connection::YamuxStream::Error;
  switch (e) {
    case E::NOT_READABLE:
      return "the stream is closed for reads";
    case E::NOT_WRITABLE:
      return "the stream is closed for writes";
    case E::INVALID_ARGUMENT:
      return "provided argument is invalid";
  }
  return "unknown error";
}

namespace libp2p::connection {

  YamuxStream::YamuxStream(std::shared_ptr<YamuxedConnection> conn,
                           YamuxedConnection::StreamId stream_id)
      : yamux_{std::move(conn)}, stream_id_{stream_id} {}

  YamuxStream::~YamuxStream() {
    this->resetStream();
  }

  outcome::result<size_t> YamuxStream::write(gsl::span<const uint8_t> in) {
    if (!is_writable_) {
      return Error::NOT_WRITABLE;
    }
    return yamux_->streamWrite(stream_id_, in);
  }

  outcome::result<size_t> YamuxStream::writeSome(gsl::span<const uint8_t> in) {
    if (!is_writable_) {
      return Error::NOT_WRITABLE;
    }
    return yamux_->streamWrite(stream_id_, in);
  }

  outcome::result<std::vector<uint8_t>> YamuxStream::read(size_t bytes) {
    std::vector<uint8_t> result;
    result.reserve(bytes);
    OUTCOME_TRY(read(result));
    return result;
  }

  outcome::result<std::vector<uint8_t>> YamuxStream::readSome(size_t bytes) {
    std::vector<uint8_t> result;
    result.reserve(bytes);
    OUTCOME_TRY(read_bytes, readSome(result));
    result.resize(read_bytes);
    return result;
  }

  outcome::result<size_t> YamuxStream::read(gsl::span<uint8_t> buf) {
    if (buf.empty()) {
      return Error::INVALID_ARGUMENT;
    }
    if (!is_readable_) {
      return Error::NOT_READABLE;
    }

    while (read_buffer_.size() < static_cast<unsigned long>(buf.size())) {
      OUTCOME_TRY(yamux_->streamReadFrame());
    }

    boost::asio::buffer_copy(boost::asio::buffer(buf.data(), buf.size()),
                             read_buffer_.data(), buf.size());
    read_buffer_.consume(buf.size());
    return buf.size();
  }

  outcome::result<size_t> YamuxStream::readSome(gsl::span<uint8_t> buf) {
    if (buf.empty()) {
      return Error::INVALID_ARGUMENT;
    }
    if (!is_readable_) {
      return Error::NOT_READABLE;
    }

    while (read_buffer_.size() == 0) {
      OUTCOME_TRY(yamux_->streamReadFrame());
    }

    auto to_read =
        std::min(read_buffer_.size(), static_cast<unsigned long>(buf.size()));
    boost::asio::buffer_copy(boost::asio::buffer(buf.data(), to_read),
                             read_buffer_.data(), to_read);
    read_buffer_.consume(to_read);
    return to_read;
  }

  void YamuxStream::reset() {
    yamux_->streamReset(stream_id_);
  }

  bool YamuxStream::isClosedForRead() const noexcept {
    return !is_readable_;
  }

  bool YamuxStream::isClosedForWrite() const noexcept {
    return !is_writable_;
  }

  bool YamuxStream::isClosed() const noexcept {
    return !is_readable_ && !is_writable_;
  }

  outcome::result<void> YamuxStream::close() {
    return yamux_->streamClose(stream_id_);
  }

  void YamuxStream::resetStream() {
    is_readable_ = false;
    is_writable_ = false;
  }

  void YamuxStream::commitData(std::vector<uint8_t> &&data) {
    std::ostream s(&read_buffer_);
    std::move(data.begin(), data.end(), std::ostream_iterator<uint8_t>(s));
  }

}  // namespace libp2p::connection
