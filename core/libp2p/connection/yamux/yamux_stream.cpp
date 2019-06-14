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
    case E::INTERNAL_ERROR:
      return "internal error happened";
  }
  return "unknown error";
}

#define SHP_FROM_PROMISE(p) \
  std::make_shared<std::decay_t<decltype(p)>>(std::forward<decltype(p)>(p))

namespace libp2p::connection {

  YamuxStream::YamuxStream(std::shared_ptr<YamuxedConnection> conn,
                           YamuxedConnection::StreamId stream_id)
      : yamux_{std::move(conn)}, stream_id_{stream_id} {}

  cti::continuable<size_t> YamuxStream::write(gsl::span<const uint8_t> in) {
    return write(in, false);
  }

  cti::continuable<size_t> YamuxStream::writeSome(gsl::span<const uint8_t> in) {
    return write(in, true);
  }

  cti::continuable<size_t> YamuxStream::write(gsl::span<const uint8_t> in,
                                              bool some) {
    if (!is_writable_) {
      return ERROR_CONTINUABLE(Error::NOT_WRITABLE);
    }
    if (is_writing_) {
      return ERROR_CONTINUABLE(Error::IS_WRITING);
    }
    is_writing_ = true;

    if (send_window_size_ - in.size() >= 0) {
      // we can write immediately - window size on the other side allows us
      return cti::make_continuable<size_t>(
          [t = shared_from_this(), in, some](auto &&promise) {
            auto p = SHP_FROM_PROMISE(promise);
            t->yamux_->streamWrite(t->stream_id_, in, some,
                                   [t, p = std::move(p), in](auto &&res) {
                                     t->is_writing_ = false;
                                     if (!res) {
                                       return p->set_exception(res.error());
                                     }
                                     t->send_window_size_ -= in.size();
                                     return p->set_value(res.value());
                                   });
          });
    }

    // each time a window size gets updated, that lambda will be called, and if
    // that time the window is wide enough, finally write to the connection
    return cti::make_continuable<size_t>(
        [t = shared_from_this(), some, in](auto &&promise) {
          auto p = SHP_FROM_PROMISE(promise);
          t->yamux_->streamAddWindowUpdateHandler(
              t->stream_id_, [t, p = std::move(p), some, in] {
                if (t->send_window_size_ - in.size() < 0) {
                  // not yet
                  return false;
                }
                t->yamux_->streamWrite(t->stream_id_, in, some,
                                       [t, p = std::move(p), in](auto &&res) {
                                         t->is_writing_ = false;
                                         if (!res) {
                                           return p->set_exception(res.error());
                                         }
                                         t->send_window_size_ -= in.size();
                                         return p->set_value(res.value());
                                       });
                return true;
              });
        });
  }

  cti::continuable<std::vector<uint8_t>> YamuxStream::read(size_t bytes) {
    if (bytes == 0) {
      return ERROR_CONTINUABLE(Error::INVALID_ARGUMENT);
    }
    if (!is_readable_) {
      return ERROR_CONTINUABLE(Error::NOT_READABLE);
    }
    if (is_reading_) {
      return ERROR_CONTINUABLE(Error::IS_READING);
    }

    // if there is already enough data in our buffer, read it immediately
    if (read_buffer_.size() >= bytes) {
      std::vector<uint8_t> result(bytes);
      if (boost::asio::buffer_copy(boost::asio::buffer(result),
                                   read_buffer_.data(), bytes)
          != bytes) {
        return ERROR_CONTINUABLE(Error::INTERNAL_ERROR);
      }
      read_buffer_.consume(bytes);
      return cti::make_ready_continuable(std::move(result));
    }

    // else, set a callback, which is called each time a new data arrives
    is_reading_ = true;
    return cti::make_continuable<std::vector<uint8_t>>(
        [t = shared_from_this(), bytes](auto &&promise) {
          auto p = SHP_FROM_PROMISE(promise);
          t->yamux_->streamRead(t->stream_id_, [t, p = std::move(p), bytes] {
            if (t->read_buffer_.size() < bytes) {
              // not yet
              return false;
            }

            std::vector<uint8_t> result(bytes);
            if (boost::asio::buffer_copy(boost::asio::buffer(result),
                                         t->read_buffer_.data(), bytes)
                != bytes) {
              p->set_exception(Error::INTERNAL_ERROR);
            } else {
              t->read_buffer_.consume(bytes);
              t->is_reading_ = false;
              p->set_value(std::move(result));
            }
            return true;
          });
        });
  }

  cti::continuable<std::vector<uint8_t>> YamuxStream::readSome(size_t bytes) {
    if (bytes == 0) {
      return ERROR_CONTINUABLE(Error::INVALID_ARGUMENT);
    }
    if (!is_readable_) {
      return ERROR_CONTINUABLE(Error::NOT_READABLE);
    }
    if (is_reading_) {
      return ERROR_CONTINUABLE(Error::IS_READING);
    }

    // if there is already enough data in our buffer, read it immediately
    if (read_buffer_.size() != 0) {
      std::vector<uint8_t> result(bytes);
      auto to_read = std::min(read_buffer_.size(), bytes);
      if (boost::asio::buffer_copy(boost::asio::buffer(result),
                                   read_buffer_.data(), to_read)
          != to_read) {
        return ERROR_CONTINUABLE(Error::INTERNAL_ERROR);
      }
      result.resize(to_read);
      read_buffer_.consume(to_read);
      return cti::make_ready_continuable(std::move(result));
    }

    // else, set a callback, which is called each time a new data arrives
    is_reading_ = true;
    return cti::make_continuable<std::vector<uint8_t>>(
        [t = shared_from_this(), bytes](auto &&promise) {
          auto p = SHP_FROM_PROMISE(promise);
          t->yamux_->streamRead(t->stream_id_, [t, p = std::move(p), bytes] {
            if (t->read_buffer_.size() == 0) {
              // not yet
              return false;
            }

            std::vector<uint8_t> result(bytes);
            auto to_read = std::min(t->read_buffer_.size(), bytes);
            if (boost::asio::buffer_copy(boost::asio::buffer(result),
                                         t->read_buffer_.data(), to_read)
                != to_read) {
              p->set_exception(Error::INTERNAL_ERROR);
            } else {
              result.resize(bytes);
              t->read_buffer_.consume(bytes);
              t->is_reading_ = false;
              p->set_value(std::move(result));
            }
            return true;
          });
        });
  }

  cti::continuable<> YamuxStream::reset() {
    if (is_writing_) {
      return ERROR_CONTINUABLE(Error::IS_WRITING);
    }

    is_writing_ = true;
    return cti::make_continuable<void>([t = shared_from_this()](
                                           auto &&promise) {
      auto p = SHP_FROM_PROMISE(promise);
      t->yamux_->streamReset(t->stream_id_, [t, p = std::move(p)](auto &&res) {
        t->is_writing_ = false;
        if (!res) {
          return p->set_exception(res.error());
        }
        t->resetStream();
        p->set_value();
      });
    });
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

  cti::continuable<> YamuxStream::close() {
    if (is_writing_) {
      return ERROR_CONTINUABLE(Error::IS_WRITING);
    }

    is_writing_ = true;
    return cti::make_continuable<void>([t = shared_from_this()](
                                           auto &&promise) {
      auto p = SHP_FROM_PROMISE(promise);
      t->yamux_->streamClose(t->stream_id_, [t, p = std::move(p)](auto &&res) {
        t->is_writing_ = false;
        if (!res) {
          return p->set_exception(res.error());
        }
        p->set_value();
      });
    });
  }

  cti::continuable<> YamuxStream::adjustWindowSize(uint32_t new_size) {
    if (is_writing_) {
      return ERROR_CONTINUABLE(Error::IS_WRITING);
    }

    is_writing_ = true;
    return cti::make_continuable<void>([t = shared_from_this(),
                                        new_size](auto &&promise) {
      auto p = SHP_FROM_PROMISE(promise);
      // delta is sent over the network, so it can be negative as well
      t->yamux_->streamAckBytes(t->stream_id_,
                                t->receive_window_size_ - new_size,
                                [t, p = std::move(p), new_size](auto &&res) {
                                  t->is_writing_ = false;
                                  if (!res) {
                                    return p->set_exception(res.error());
                                  }
                                  t->receive_window_size_ = new_size;
                                  p->set_value();
                                });
    });
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
    s << data.data();
    return outcome::success();
  }

}  // namespace libp2p::connection
