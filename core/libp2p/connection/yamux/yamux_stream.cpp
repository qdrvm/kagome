/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/connection/yamux/yamux_stream.hpp"

#include <future>

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
    if (send_window_size_ - in_size >= 0) {
      // we can write immediately - window size on the other sides allows us
      OUTCOME_TRY(written_bytes, yamux_->streamWrite(stream_id_, in, false));
      send_window_size_ -= in_size;
      return written_bytes;
    }

    // when a window size gets updated, that lambda will be called, and if that
    // time window is wide enough, write to the connection
    std::promise<outcome::result<size_t>> write_promise;
    auto fut = write_promise.get_future();
    yamux_->streamAddWindowUpdateHandler(
        stream_id_,
        std::make_shared<YamuxedConnection::ReadWriteCompletionHandler>(
            [this, p = std::move(write_promise), in]() mutable {
              if (send_window_size_ - in.size() >= 0) {
                auto written_bytes_res =
                    yamux_->streamWrite(stream_id_, in, false);
                if (!written_bytes_res) {
                  p.set_value(written_bytes_res);
                } else {
                  send_window_size_ -= in.size();
                  p.set_value(written_bytes_res.value());
                }
                return true;
              }
              return false;
            }));

    return fut.get();
  }

  outcome::result<size_t> YamuxStream::writeSome(gsl::span<const uint8_t> in) {
    if (!is_writable_) {
      return Error::NOT_WRITABLE;
    }

    auto in_size = in.size();
    if (send_window_size_ - in_size >= 0) {
      OUTCOME_TRY(written_bytes, yamux_->streamWrite(stream_id_, in, true));
      send_window_size_ -= in_size;
      return written_bytes;
    }

    std::promise<outcome::result<size_t>> write_promise;
    auto fut = write_promise.get_future();
    yamux_->streamAddWindowUpdateHandler(
        stream_id_,
        std::make_shared<YamuxedConnection::ReadWriteCompletionHandler>(
            [this, p = std::move(write_promise), in]() mutable {
              if (send_window_size_ - in.size() >= 0) {
                auto written_bytes_res =
                    yamux_->streamWrite(stream_id_, in, true);
                if (!written_bytes_res) {
                  p.set_value(written_bytes_res);
                } else {
                  send_window_size_ -= in.size();
                  p.set_value(written_bytes_res.value());
                }
                return true;
              }
              return false;
            }));

    return fut.get();
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

    // if there is already enough data in our buffer, read it immediately
    if (read_buffer_.size() >= static_cast<size_t>(buf.size())) {
      boost::asio::buffer_copy(boost::asio::buffer(buf.data(), buf.size()),
                               read_buffer_.data(), buf.size());
      read_buffer_.consume(buf.size());
      return buf.size();
    }

    // else, create a promise for that data and pass it to the connection
    std::promise<size_t> read_promise;
    auto fut = read_promise.get_future();
    yamux_->streamRead(
        stream_id_,
        std::make_shared<YamuxedConnection::ReadWriteCompletionHandler>(
            [this, p = std::move(read_promise), buf]() mutable {
              if (read_buffer_.size() >= static_cast<size_t>(buf.size())) {
                boost::asio::buffer_copy(
                    boost::asio::buffer(buf.data(), buf.size()),
                    read_buffer_.data(), buf.size());
                read_buffer_.consume(buf.size());
                p.set_value(buf.size());
                return true;
              }
              return false;
            }));

    return fut.get();
  }

  outcome::result<size_t> YamuxStream::readSome(gsl::span<uint8_t> buf) {
    if (buf.empty()) {
      return Error::INVALID_ARGUMENT;
    }
    if (!is_readable_) {
      return Error::NOT_READABLE;
    }

    if (read_buffer_.size() != 0) {
      auto to_read =
          std::min(read_buffer_.size(), static_cast<size_t>(buf.size()));
      boost::asio::buffer_copy(boost::asio::buffer(buf.data(), to_read),
                               read_buffer_.data(), to_read);
      read_buffer_.consume(to_read);
      return to_read;
    }

    std::promise<size_t> read_promise;
    auto fut = read_promise.get_future();
    yamux_->streamRead(
        stream_id_,
        std::make_shared<YamuxedConnection::ReadWriteCompletionHandler>(
            [this, p = std::move(read_promise), buf]() mutable {
              if (read_buffer_.size() != 0) {
                auto to_read = std::min(read_buffer_.size(),
                                        static_cast<size_t>(buf.size()));
                boost::asio::buffer_copy(
                    boost::asio::buffer(buf.data(), to_read),
                    read_buffer_.data(), to_read);
                read_buffer_.consume(to_read);
                p.set_value(to_read);
                return true;
              }
              return false;
            }));

    return fut.get();
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
