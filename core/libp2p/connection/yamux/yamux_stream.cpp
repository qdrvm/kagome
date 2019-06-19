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
      return "received unconsumed data amount is greater than it can be";
    case E::IS_WRITING:
      return "there is already a pending write operation on this stream";
    case E::IS_READING:
      return "there is already a pending read operation on this stream";
    case E::INVALID_WINDOW_SIZE:
      return "either window size greater than the maximum one or less than "
             "current number of unconsumed bytes was tried to be set";
    case E::INTERNAL_ERROR:
      return "internal error happened";
  }
  return "unknown error";
}

namespace libp2p::connection {

  YamuxStream::YamuxStream(
      std::shared_ptr<YamuxedConnection> yamuxed_connection,
      YamuxedConnection::StreamId stream_id, uint32_t maximum_window_size)
      : yamuxed_connection_{std::move(yamuxed_connection)},
        stream_id_{stream_id},
        maximum_window_size_{maximum_window_size} {}

  void YamuxStream::read(gsl::span<uint8_t> out, size_t bytes,
                         ReadCallbackFunc cb) {
    if (bytes == 0 || out.empty() || static_cast<size_t>(out.size()) < bytes) {
      return cb(Error::INVALID_ARGUMENT);
    }
    if (!is_readable_) {
      return cb(Error::NOT_READABLE);
    }
    if (is_reading_) {
      return cb(Error::IS_READING);
    }

    is_reading_ = true;

    // if there is already enough data in our buffer, read it immediately and
    // send an acknowledgement
    if (read_buffer_.size() >= bytes) {
      if (boost::asio::buffer_copy(boost::asio::buffer(out.data(), bytes),
                                   read_buffer_.data(), bytes)
          != bytes) {
        return cb(Error::INTERNAL_ERROR);
      }
      return yamuxed_connection_->streamAckBytes(
          stream_id_, bytes,
          [self{shared_from_this()}, cb = std::move(cb), bytes](auto &&res) {
            self->is_reading_ = false;
            if (!res) {
              return cb(res.error());
            }
            self->read_buffer_.consume(bytes);
            cb(bytes);
          });
    }

    // else, set a callback, which is called each time a new data arrives
    yamuxed_connection_->streamAddDataNotifyee(
        stream_id_,
        [self{shared_from_this()}, cb = std::move(cb), out, bytes]() mutable {
          if (self->read_buffer_.size() < bytes) {
            // not yet
            return false;
          }

          if (boost::asio::buffer_copy(boost::asio::buffer(out.data(), bytes),
                                       self->read_buffer_.data(), bytes)
              != bytes) {
            cb(Error::INTERNAL_ERROR);
          } else {
            self->yamuxed_connection_->streamAckBytes(
                self->stream_id_, bytes,
                [self, cb = std::move(cb), bytes](auto &&res) {
                  self->is_reading_ = false;
                  if (!res) {
                    return cb(res.error());
                  }
                  self->read_buffer_.consume(bytes);
                  cb(bytes);
                });
          }
          return true;
        });
  }

  void YamuxStream::readSome(gsl::span<uint8_t> out, size_t bytes,
                             ReadCallbackFunc cb) {
    if (bytes == 0 || out.empty() || static_cast<size_t>(out.size()) < bytes) {
      return cb(Error::INVALID_ARGUMENT);
    }
    if (!is_readable_) {
      return cb(Error::NOT_READABLE);
    }
    if (is_reading_) {
      return cb(Error::IS_READING);
    }

    is_reading_ = true;

    // if there is already enough data in our buffer, read it immediately
    if (read_buffer_.size() != 0) {
      auto to_read = std::min(read_buffer_.size(), bytes);
      if (boost::asio::buffer_copy(boost::asio::buffer(out.data(), to_read),
                                   read_buffer_.data(), to_read)
          != to_read) {
        return cb(Error::INTERNAL_ERROR);
      }
      return yamuxed_connection_->streamAckBytes(
          stream_id_, to_read,
          [self{shared_from_this()}, cb = std::move(cb), to_read](auto &&res) {
            self->is_reading_ = false;
            if (!res) {
              return cb(res.error());
            }
            self->read_buffer_.consume(to_read);
            cb(to_read);
          });
    }

    // else, set a callback, which is called each time a new data arrives
    yamuxed_connection_->streamAddDataNotifyee(
        stream_id_,
        [self{shared_from_this()}, cb = std::move(cb), out, bytes]() mutable {
          if (self->read_buffer_.size() == 0) {
            // not yet
            return false;
          }

          auto to_read = std::min(self->read_buffer_.size(), bytes);
          if (boost::asio::buffer_copy(boost::asio::buffer(out.data(), to_read),
                                       self->read_buffer_.data(), to_read)
              != bytes) {
            cb(Error::INTERNAL_ERROR);
          } else {
            self->yamuxed_connection_->streamAckBytes(
                self->stream_id_, to_read,
                [self, cb = std::move(cb), to_read](auto &&res) {
                  self->is_reading_ = false;
                  if (!res) {
                    return cb(res.error());
                  }
                  self->read_buffer_.consume(to_read);
                  cb(to_read);
                });
          }
          return true;
        });
  }

  void YamuxStream::write(gsl::span<const uint8_t> in, size_t bytes,
                          WriteCallbackFunc cb) {
    return write(in, bytes, std::move(cb), false);
  }

  void YamuxStream::writeSome(gsl::span<const uint8_t> in, size_t bytes,
                              WriteCallbackFunc cb) {
    return write(in, bytes, std::move(cb), true);
  }

  void YamuxStream::write(gsl::span<const uint8_t> in, size_t bytes,
                          WriteCallbackFunc cb, bool some) {
    if (!is_writable_) {
      return cb(Error::NOT_WRITABLE);
    }
    if (is_writing_) {
      return cb(Error::IS_WRITING);
    }

    is_writing_ = true;
    if (send_window_size_ - bytes >= 0) {
      // we can write immediately - window size on the other side allows us
      return yamuxed_connection_->streamWrite(
          stream_id_, in, bytes, some,
          [self{shared_from_this()}, cb = std::move(cb)](auto &&res) {
            self->is_writing_ = false;
            if (res) {
              self->send_window_size_ -= res.value();
            }
            return cb(std::forward<decltype(res)>(res));
          });
    }

    // each time a window size gets updated, that lambda will be called, and if
    // that time the window is wide enough, finally write to the connection
    return yamuxed_connection_->streamAddWindowUpdateNotifyee(
        stream_id_,
        [self{shared_from_this()}, in, bytes, some,
         cb = std::move(cb)]() mutable {
          if (self->send_window_size_ - bytes < 0) {
            // not yet
            return false;
          }
          self->yamuxed_connection_->streamWrite(
              self->stream_id_, in, bytes, some,
              [self, cb = std::move(cb)](auto &&res) {
                self->is_writing_ = false;
                if (res) {
                  self->send_window_size_ -= res.value();
                }
                return cb(std::forward<decltype(res)>(res));
              });
          return true;
        });
  }

  bool YamuxStream::isClosed() const noexcept {
    return !is_readable_ && !is_writable_;
  }

  void YamuxStream::close(std::function<void(outcome::result<void>)> cb) {
    if (is_writing_) {
      return cb(Error::IS_WRITING);
    }

    is_writing_ = true;
    return yamuxed_connection_->streamClose(
        stream_id_, [self{shared_from_this()}, cb = std::move(cb)](auto &&res) {
          self->is_writing_ = false;
          cb(std::forward<decltype(res)>(res));
        });
  }

  bool YamuxStream::isClosedForRead() const noexcept {
    return !is_readable_;
  }

  bool YamuxStream::isClosedForWrite() const noexcept {
    return !is_writable_;
  }

  void YamuxStream::reset(std::function<void(outcome::result<void>)> cb) {
    if (is_writing_) {
      return cb(Error::IS_WRITING);
    }

    is_writing_ = true;
    return yamuxed_connection_->streamReset(
        stream_id_, [self{shared_from_this()}, cb = std::move(cb)](auto &&res) {
          self->is_writing_ = false;
          self->resetStream();
          cb(std::forward<decltype(res)>(res));
        });
  }

  void YamuxStream::adjustWindowSize(
      uint32_t new_size, std::function<void(outcome::result<void>)> cb) {
    if (is_writing_) {
      return cb(Error::IS_WRITING);
    }

    if (new_size > maximum_window_size_ || new_size < read_buffer_.size()) {
      return cb(Error::INVALID_WINDOW_SIZE);
    }

    is_writing_ = true;
    return yamuxed_connection_->streamAckBytes(
        stream_id_, new_size - receive_window_size_,
        [self{shared_from_this()}, cb = std::move(cb), new_size](auto &&res) {
          self->is_writing_ = false;
          if (!res) {
            return cb(res.error());
          }
          self->receive_window_size_ = new_size;
          cb(outcome::success());
        });
  }

  void YamuxStream::resetStream() {
    is_readable_ = false;
    is_writable_ = false;
  }

  outcome::result<void> YamuxStream::commitData(gsl::span<const uint8_t> data,
                                                size_t data_size) {
    if (data_size + read_buffer_.size() > receive_window_size_) {
      return Error::RECEIVE_OVERFLOW;
    }

    std::ostream s(&read_buffer_);
    s << data.data();
    return outcome::success();
  }

}  // namespace libp2p::connection
