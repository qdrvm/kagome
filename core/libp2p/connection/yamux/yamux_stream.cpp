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
    case E::RECEIVE_OVERFLOW:
      return "received unacknowledged data amount is greater than it can be";
  }
  return "unknown error";
}

namespace libp2p::connection {

  YamuxStream::YamuxStream(std::shared_ptr<YamuxedConnection> conn,
                           YamuxedConnection::StreamId stream_id)
      : yamux_{std::move(conn)}, stream_id_{stream_id} {}

  outcome::result<size_t> YamuxStream::write(gsl::span<const uint8_t> in) {
    if (!is_writable_) {
      return Error::NOT_WRITABLE;
    }

    auto in_size = in.size();
    while (send_window_size_ - in_size < 0) {
      // we are processing frames, as window update, which can decrease increase
      // the window size, is to be among those frames
      OUTCOME_TRY(processNextFrame());
    }

    OUTCOME_TRY(written_bytes, yamux_->streamWrite(stream_id_, in, false));
    send_window_size_ -= in_size;
    return written_bytes;
  }

  outcome::result<size_t> YamuxStream::writeSome(gsl::span<const uint8_t> in) {
    if (!is_writable_) {
      return Error::NOT_WRITABLE;
    }

    auto in_size = in.size();
    while (send_window_size_ - in_size < 0) {
      OUTCOME_TRY(processNextFrame());
    }

    OUTCOME_TRY(written_bytes, yamux_->streamWrite(stream_id_, in, true));
    send_window_size_ -= written_bytes;
    return written_bytes;
  }

  outcome::result<std::vector<uint8_t>> YamuxStream::read(size_t bytes) {
    std::vector<uint8_t> result;
    result.reserve(bytes);
    OUTCOME_TRY(read(result));
    OUTCOME_TRY(yamux_->streamAckBytes(stream_id_, bytes));
    return result;
  }

  outcome::result<std::vector<uint8_t>> YamuxStream::readSome(size_t bytes) {
    std::vector<uint8_t> result;
    result.reserve(bytes);
    OUTCOME_TRY(read_bytes, readSome(result));
    OUTCOME_TRY(yamux_->streamAckBytes(stream_id_, read_bytes));
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

    while (read_buffer_.size() < static_cast<size_t>(buf.size())) {
      OUTCOME_TRY(processNextFrame());
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
      OUTCOME_TRY(processNextFrame());
    }

    auto to_read =
        std::min(read_buffer_.size(), static_cast<size_t>(buf.size()));
    boost::asio::buffer_copy(boost::asio::buffer(buf.data(), to_read),
                             read_buffer_.data(), to_read);
    read_buffer_.consume(to_read);
    return to_read;
  }

  void YamuxStream::reset() {
    yamux_->streamReset(stream_id_);
    resetStream();
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

  outcome::result<void> YamuxStream::adjustWindowSize(uint32_t new_size) {
    // delta is sent over the network, so it can be negative as well
    OUTCOME_TRY(
        yamux_->streamAckBytes(stream_id_, receive_window_size_ - new_size));
    receive_window_size_ = new_size;
    return outcome::success();
  }

  outcome::result<void> YamuxStream::processNextFrame() const {
    return yamux_->streamProcessNextFrame();
  }

  void YamuxStream::resetStream() {
    is_readable_ = false;
    is_writable_ = false;
  }

  outcome::result<void> YamuxStream::commitData(std::vector<uint8_t> &&data) {
    if (data.size() + read_buffer_.size() > receive_window_size_) {
      return Error::RECEIVE_OVERFLOW;
    }
    std::ostream s(&read_buffer_);
    std::move(data.begin(), data.end(), std::ostream_iterator<uint8_t>(s));
    return outcome::success();
  }

}  // namespace libp2p::connection
