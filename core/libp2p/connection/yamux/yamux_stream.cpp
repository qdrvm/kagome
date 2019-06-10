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
    case E::IS_WRITING:
      return "there is already a pending write operation on this stream";
    case E::IS_READING:
      return "there is already a pending read operation on this stream";
  }
  return "unknown error";
}

namespace libp2p::connection {

  YamuxStream::YamuxStream(std::shared_ptr<YamuxedConnection> conn,
                           YamuxedConnection::StreamId stream_id)
      : yamux_{std::move(conn)}, stream_id_{stream_id} {}

  void YamuxStream::write(gsl::span<const uint8_t> in,
                          std::function<WriteCallback> cb) {
    write(in, std::move(cb), false);
  }

  void YamuxStream::writeSome(gsl::span<const uint8_t> in,
                              std::function<WriteCallback> cb) {
    write(in, std::move(cb), true);
  }

  void YamuxStream::write(gsl::span<const uint8_t> in,
                          std::function<WriteCallback> cb, bool some) {
    if (!is_writable_) {
      return cb(Error::NOT_WRITABLE);
    }
    if (is_writing_) {
      return cb(Error::IS_WRITING);
    }
    is_writing_ = true;

    auto in_size = in.size();
    if (send_window_size_ - in_size >= 0) {
      // we can write immediately - window size on the other sides allows us
      auto written_bytes_res = yamux_->streamWrite(stream_id_, in, some);
      if (written_bytes_res) {
        send_window_size_ -= in_size;
      }
      is_writing_ = false;
      return cb(written_bytes_res);
    }

    // each time a window size gets updated, that lambda will be called, and if
    // that time the window is wide enough, finally write to the connection
    yamux_->streamAddWindowUpdateHandler(
        stream_id_, [this, some, in, cb = std::move(cb)]() mutable {
          if (send_window_size_ - in.size() >= 0) {
            auto written_bytes_res = yamux_->streamWrite(stream_id_, in, some);
            if (written_bytes_res) {
              send_window_size_ -= in.size();
            }
            is_writing_ = false;
            cb(written_bytes_res);
            return true;
          }
          return false;
        });
  }

  void YamuxStream::read(size_t bytes, std::function<ReadCallback> cb) {
    if (bytes == 0) {
      return cb(Error::INVALID_ARGUMENT);
    }
    if (!is_readable_) {
      return cb(Error::NOT_READABLE);
    }
    if (is_reading_) {
      return cb(Error::IS_READING);
    }

    std::vector<uint8_t> result;
    result.reserve(bytes);

    // if there is already enough data in our buffer, read it immediately
    if (read_buffer_.size() >= bytes) {
      boost::asio::buffer_copy(boost::asio::buffer(result), read_buffer_.data(),
                               bytes);
      read_buffer_.consume(bytes);
      return cb(std::move(result));
    }

    // else, set a callback, which is called each time a new data arrives
    is_reading_ = true;
    yamux_->streamRead(stream_id_,
                       [this, bytes, result = std::move(result),
                        cb = std::move(cb)]() mutable {
                         if (read_buffer_.size() >= bytes) {
                           boost::asio::buffer_copy(boost::asio::buffer(result),
                                                    read_buffer_.data(), bytes);
                           read_buffer_.consume(bytes);
                           cb(std::move(result));
                           is_reading_ = false;
                           return true;
                         }
                         return false;
                       });
  }

  void YamuxStream::readSome(size_t bytes, std::function<ReadCallback> cb) {
    if (bytes == 0) {
      return cb(Error::INVALID_ARGUMENT);
    }
    if (!is_readable_) {
      return cb(Error::NOT_READABLE);
    }
    if (is_reading_) {
      return cb(Error::IS_READING);
    }

    std::vector<uint8_t> result;
    result.reserve(bytes);

    // if there is already enough data in our buffer, read it immediately
    if (read_buffer_.size() != 0) {
      auto to_read = std::min(read_buffer_.size(), bytes);
      boost::asio::buffer_copy(boost::asio::buffer(result), read_buffer_.data(),
                               to_read);
      result.resize(to_read);
      read_buffer_.consume(to_read);
      return cb(std::move(result));
    }

    // else, set a callback, which is called each time a new data arrives
    is_reading_ = true;
    yamux_->streamRead(stream_id_,
                       [this, bytes, result = std::move(result),
                        cb = std::move(cb)]() mutable {
                         if (read_buffer_.size() != 0) {
                           auto to_read = std::min(read_buffer_.size(), bytes);
                           boost::asio::buffer_copy(boost::asio::buffer(result),
                                                    read_buffer_.data(),
                                                    to_read);
                           result.resize(to_read);
                           read_buffer_.consume(to_read);
                           cb(std::move(result));
                           is_reading_ = false;
                           return true;
                         }
                         return false;
                       });
  }

  outcome::result<void> YamuxStream::reset() {
    if (is_writing_) {
      return Error::IS_WRITING;
    }
    is_writing_ = true;
    yamux_->streamReset(stream_id_);
    is_writing_ = false;
    resetStream();
    return outcome::success();
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

  void YamuxStream::close(
      const std::function<void(outcome::result<void>)> &cb) {
    if (is_writing_) {
      return cb(Error::IS_WRITING);
    }

    is_writing_ = true;
    auto close_res = yamux_->streamClose(stream_id_);
    is_writing_ = false;
    cb(close_res);
  }

  void YamuxStream::adjustWindowSize(
      uint32_t new_size, const std::function<void(outcome::result<void>)> &cb) {
    if (is_writing_) {
      return cb(Error::IS_WRITING);
    }

    // delta is sent over the network, so it can be negative as well
    is_writing_ = true;
    auto ack_res =
        yamux_->streamAckBytes(stream_id_, receive_window_size_ - new_size);
    is_writing_ = false;
    if (ack_res) {
      receive_window_size_ = new_size;
    }
    cb(ack_res);
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
