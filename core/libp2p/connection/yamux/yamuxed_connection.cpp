/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/connection/yamux/yamuxed_connection.hpp"

#include "libp2p/connection/yamux/yamux_frame.hpp"
#include "libp2p/connection/yamux/yamux_stream.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::connection, YamuxedConnection::Error, e) {
  using ErrorType = libp2p::connection::YamuxedConnection::Error;
  switch (e) {
    case ErrorType::NO_SUCH_STREAM:
      return "no such stream was found; maybe, it is closed";
    case ErrorType::YAMUX_IS_CLOSED:
      return "this Yamux instance is closed";
    case ErrorType::FORBIDDEN_CALL:
      return "forbidden method was invoked";
    case ErrorType::OTHER_SIDE_ERROR:
      return "error happened on other side's behalf";
    case ErrorType::INTERNAL_ERROR:
      return "internal error happened";
  }
  return "unknown";
}

#define HANDLE_WRITE_ERROR                               \
  [t = shared_from_this()](auto &&res) {                 \
    if (!res) {                                          \
      t->logger_->error("write finished with error: {}", \
                        res.error().message());          \
      t->last_write_error_ = res.error();                \
    }                                                    \
  }

namespace libp2p::connection {
  YamuxedConnection::YamuxedConnection(
      std::shared_ptr<SecureConnection> connection,
      NewStreamHandler stream_handler, kagome::common::Logger logger)
      : connection_{std::move(connection)},
        new_stream_handler_{std::move(stream_handler)},
        is_active_{true},
        logger_{std::move(logger)} {
    // client uses odd numbers, server - even
    last_created_stream_id_ = connection_->isInitiator() ? 1 : 0;
  }

  outcome::result<void> YamuxedConnection::start() {
    return readerLoop();
  }

  cti::continuable<std::shared_ptr<Stream>> YamuxedConnection::newStream() {
    return cti::make_continuable<std::shared_ptr<Stream>>(
        [t = shared_from_this(), stream_id = getNewStreamId()](auto &&promise) {
          auto p = std::make_shared<std::decay_t<decltype(promise)>>(
              std::forward<decltype(promise)>(promise));
          t->write({newStreamMsg(stream_id),
                    [t, p = std::move(p), stream_id](auto &&res) {
                      if (!res) {
                        p->set_exception(res.error());
                      }
                      auto created_stream =
                          std::make_shared<YamuxStream>(t, stream_id);
                      t->streams_.insert({stream_id, created_stream});
                      p->set_value(std::move(created_stream));
                    }});
        });
  }

  outcome::result<peer::PeerId> YamuxedConnection::localPeer() const {
    return connection_->localPeer();
  }

  outcome::result<peer::PeerId> YamuxedConnection::remotePeer() const {
    return connection_->remotePeer();
  }

  outcome::result<crypto::PublicKey> YamuxedConnection::remotePublicKey()
      const {
    return connection_->remotePublicKey();
  }

  bool YamuxedConnection::isInitiator() const noexcept {
    return connection_->isInitiator();
  }

  outcome::result<multi::Multiaddress> YamuxedConnection::localMultiaddr() {
    return connection_->localMultiaddr();
  }

  outcome::result<multi::Multiaddress> YamuxedConnection::remoteMultiaddr() {
    return connection_->remoteMultiaddr();
  }

  bool YamuxedConnection::isClosed() const {
    return connection_->isClosed();
  }

  outcome::result<void> YamuxedConnection::close() {
    OUTCOME_TRY(connection_->close());
    is_active_ = false;
    for (const auto &stream : streams_) {
      stream.second->resetStream();
    }
    return outcome::success();
  }

  outcome::result<size_t> YamuxedConnection::write(gsl::span<const uint8_t> s) {
    return Error::FORBIDDEN_CALL;
  }

  outcome::result<size_t> YamuxedConnection::writeSome(
      gsl::span<const uint8_t> s) {
    return Error::FORBIDDEN_CALL;
  }

  outcome::result<std::vector<uint8_t>> YamuxedConnection::read(size_t s) {
    return Error::FORBIDDEN_CALL;
  }

  outcome::result<std::vector<uint8_t>> YamuxedConnection::readSome(size_t s) {
    return Error::FORBIDDEN_CALL;
  }

  outcome::result<size_t> YamuxedConnection::read(gsl::span<uint8_t> s) {
    return Error::FORBIDDEN_CALL;
  }

  outcome::result<size_t> YamuxedConnection::readSome(gsl::span<uint8_t> s) {
    return Error::FORBIDDEN_CALL;
  }

  void YamuxedConnection::write(WriteData write_data) {
    write_queue_.push(std::move(write_data));
    if (is_writing_) {
      return;
    }

    is_writing_ = true;
    while (!write_queue_.empty()) {
      const auto &data = write_queue_.front();
      auto write_res = data.some ? connection_->writeSome(data.data)
                                 : connection_->write(data.data);
      if (write_res) {
        data.cb(write_res.value() - YamuxFrame::kHeaderLength);
      } else {
        data.cb(write_res);
      }
      write_queue_.pop();
    }
    is_writing_ = false;
  }

  outcome::result<void> YamuxedConnection::readerLoop() {
    using FrameType = YamuxFrame::FrameType;

    while (is_active_ && !last_write_error_ && !connection_->isClosed()) {
      OUTCOME_TRY(header_bytes, connection_->read(YamuxFrame::kHeaderLength));
      auto header_opt = parseFrame(header_bytes);
      if (!header_opt) {
        // could not parse the frame => client sent some nonsense
        write({goAwayMsg(YamuxFrame::GoAwayError::PROTOCOL_ERROR),
               HANDLE_WRITE_ERROR});
        return Error::OTHER_SIDE_ERROR;
      }

      switch (header_opt->type_) {
        case FrameType::DATA: {
          OUTCOME_TRY(processDataFrame(*header_opt));
          break;
        }
        case FrameType::WINDOW_UPDATE: {
          OUTCOME_TRY(processWindowUpdateFrame(*header_opt));
          break;
        }
        case FrameType::PING: {
          OUTCOME_TRY(processPingFrame(*header_opt));
          break;
        }
        case FrameType::GO_AWAY: {
          OUTCOME_TRY(processGoAwayFrame(*header_opt));
          break;
        }
        default:
          logger_->critical("garbage in parsed frame's type");
          return Error::INTERNAL_ERROR;
      }
    }

    if (last_write_error_) {
      return *last_write_error_;
    }
    return outcome::success();
  }

  outcome::result<void> YamuxedConnection::processDataFrame(
      const YamuxFrame &frame) {
    using Flag = YamuxFrame::Flag;

    auto stream_id = frame.stream_id_;
    switch (frame.flag_) {
      case Flag::SYN: {
        // can be start of a new stream, just data or both
        auto stream = findStream(stream_id);
        if (!stream) {
          // it is at least a new stream request; register it and send ack
          // message
          registerNewStream(
              stream_id, [t = shared_from_this(), frame](auto &&stream_res) {
                if (!stream_res) {
                  t->logger_->error("cannot register new stream: {}",
                                    stream_res.error().message());
                  return;
                }
                if (!t->processData(std::static_pointer_cast<YamuxStream>(
                                        stream_res.value()),
                                    frame)) {
                  t->logger_->error("cannot register new stream: {}",
                                    stream_res.error().message());
                }
              });
          return outcome::success();
        }
        // process data in this frame, if there is one
        return processData(stream, frame);
      }
      case Flag::ACK: {
        // can be ack of a new stream, just data or both
        OUTCOME_TRY(stream, processAck(stream_id));
        if (stream) {
          return processData(stream, frame);
        }
        break;
      }
      case Flag::FIN:
        closeStreamForRead(stream_id);
        break;
      case Flag::RST:
        removeStream(stream_id);
        break;
      default:
        logger_->critical("garbage in parsed frame's flag");
        return Error::INTERNAL_ERROR;
    }
    return outcome::success();
  }

  outcome::result<void> YamuxedConnection::processWindowUpdateFrame(
      const YamuxFrame &frame) {
    using Flag = YamuxFrame::Flag;

    auto stream_id = frame.stream_id_;
    auto window_delta = frame.length_;
    switch (frame.flag_) {
      case Flag::SYN: {
        // can be start of a new stream or update of a window size
        if (auto stream = findStream(stream_id)) {
          // this stream is already opened => delta window update
          processWindowUpdate(stream, frame.length_);
          break;
        }
        // no such stream found => it's a creation of a new stream
        registerNewStream(
            stream_id,
            [t = shared_from_this(), window_delta](auto &&stream_res) {
              if (!stream_res) {
                t->logger_->error("cannot register new stream: {}",
                                  stream_res.error().message());
                return;
              }
              t->processWindowUpdate(
                  std::static_pointer_cast<YamuxStream>(stream_res.value()),
                  window_delta);
            });
        break;
      }
      case Flag::ACK: {
        if (auto stream = findStream(stream_id)) {
          processWindowUpdate(stream, window_delta);
          break;
        }
        // if no such stream found, some error happened - reset the stream on
        // the other side just in case
        write({resetStreamMsg(stream_id), HANDLE_WRITE_ERROR});
        break;
      }
      case Flag::FIN: {
        if (auto stream = findStream(stream_id)) {
          processWindowUpdate(stream, window_delta);
        }
        closeStreamForRead(stream_id);
        break;
      }
      case Flag::RST:
        removeStream(stream_id);
        break;
      default:
        logger_->critical("garbage in parsed frame's flag");
        return Error::INTERNAL_ERROR;
    }
    return outcome::success();
  }

  outcome::result<void> YamuxedConnection::processPingFrame(
      const YamuxFrame &frame) {
    write({pingResponseMsg(frame.length_), HANDLE_WRITE_ERROR});
    return outcome::success();
  }

  outcome::result<void> YamuxedConnection::processGoAwayFrame(
      const YamuxFrame &frame) {
    OUTCOME_TRY(close());
    return outcome::success();
  }

  std::shared_ptr<YamuxStream> YamuxedConnection::findStream(
      StreamId stream_id) {
    auto stream = streams_.find(stream_id);
    if (stream == streams_.end()) {
      return nullptr;
    }
    return stream->second;
  }

  void YamuxedConnection::registerNewStream(StreamId stream_id,
                                            StreamResultHandler cb) {
    write({ackStreamMsg(stream_id),
           [t = shared_from_this(), stream_id, cb = std::move(cb)](auto &&res) {
             if (!res) {
               t->last_write_error_ = res.error();
               return cb(res.error());
             }
             auto new_stream = std::make_shared<YamuxStream>(t, stream_id);
             t->streams_[stream_id] = new_stream;
             t->new_stream_handler_(new_stream);
             cb(std::move(new_stream));
           }});
  }

  outcome::result<void> YamuxedConnection::processData(
      const std::shared_ptr<YamuxStream> &stream, const YamuxFrame &frame) {
    auto data_length = frame.length_;
    if (data_length == 0) {
      return outcome::success();
    }

    // read the data, commit it to the stream and call handler, if exists
    OUTCOME_TRY(data_bytes, connection_->read(data_length));
    OUTCOME_TRY(stream->commitData(std::move(data_bytes)));
    if (auto stream_read_handler =
            streams_read_handlers_.find(frame.stream_id_);
        stream_read_handler != streams_read_handlers_.end()) {
      if (stream_read_handler->second()) {
        // if handler returns true, it means that it should be removed
        streams_read_handlers_.erase(stream_read_handler);
      }
    }

    return outcome::success();
  }

  outcome::result<std::shared_ptr<YamuxStream>> YamuxedConnection::processAck(
      StreamId stream_id) {
    // acknowledge of start of a new stream; if we don't have such a stream,
    // a reset should be sent in order to notify the other side about the
    // problem
    if (auto stream = findStream(stream_id)) {
      return stream;
    }
    write({resetStreamMsg(stream_id), HANDLE_WRITE_ERROR});
    return nullptr;
  }

  void YamuxedConnection::processWindowUpdate(
      const std::shared_ptr<YamuxStream> &stream, uint32_t window_delta) {
    stream->send_window_size_ += window_delta;
    if (auto window_update_sub =
            streams_window_updates_subs_.find(stream->stream_id_);
        window_update_sub != streams_window_updates_subs_.end()) {
      if (window_update_sub->second()) {
        // if handler returns true, it means that it should be removed
        streams_window_updates_subs_.erase(window_update_sub);
      }
    }
  }

  void YamuxedConnection::closeStreamForRead(StreamId stream_id) {
    if (auto stream = findStream(stream_id)) {
      if (!stream->is_writable_) {
        removeStream(stream_id);
        return;
      }
      stream->is_readable_ = false;
    }
  }

  void YamuxedConnection::closeStreamForWrite(
      StreamId stream_id, std::function<void(outcome::result<void>)> cb) {
    if (auto stream = findStream(stream_id)) {
      write({closeStreamMsg(stream_id),
             [t = shared_from_this(), cb = std::move(cb), stream,
              stream_id](auto &&write_res) {
               if (!write_res) {
                 t->logger_->error("cannot write close stream msg: {}",
                                   write_res.error().message());
                 t->last_write_error_ = write_res.error();
                 return cb(write_res.error());
               }
               if (!stream->is_readable_) {
                 t->removeStream(stream_id);
               } else {
                 stream->is_writable_ = false;
               }
               cb(outcome::success());
             }});
    } else {
      cb(Error::NO_SUCH_STREAM);
    }
  }

  void YamuxedConnection::removeStream(StreamId stream_id) {
    if (auto stream = findStream(stream_id)) {
      streams_.erase(stream_id);
      stream->resetStream();
    }
  }

  YamuxedConnection::StreamId YamuxedConnection::getNewStreamId() {
    last_created_stream_id_ += 2;
    return last_created_stream_id_;
  }

  /// YAMUX STREAM API

  void YamuxedConnection::streamAddWindowUpdateHandler(
      StreamId stream_id, ReadWriteCompletionHandler handler) {
    streams_window_updates_subs_[stream_id] = std::move(handler);
  }

  void YamuxedConnection::streamWrite(
      StreamId stream_id, gsl::span<const uint8_t> msg, bool some,
      std::function<void(outcome::result<size_t>)> cb) {
    if (!is_active_) {
      return cb(Error::YAMUX_IS_CLOSED);
    }

    auto stream_opt = findStream(stream_id);
    if (!stream_opt) {
      return cb(Error::NO_SUCH_STREAM);
    }

    write({dataMsg(stream_id, msg), std::move(cb), some});
  }

  void YamuxedConnection::streamRead(StreamId stream_id,
                                     ReadWriteCompletionHandler handler) {
    streams_read_handlers_[stream_id] = std::move(handler);
  }

  void YamuxedConnection::streamAckBytes(
      StreamId stream_id, uint32_t bytes,
      std::function<void(outcome::result<void>)> cb) {
    write({windowUpdateMsg(stream_id, bytes),
           [t = shared_from_this(), cb = std::move(cb)](auto &&write_res) {
             if (!write_res) {
               t->logger_->error("cannot write ack bytes msg: {}",
                                 write_res.error().message());
               t->last_write_error_ = write_res.error();
               return cb(write_res.error());
             }
             cb(outcome::success());
           }});
  }

  void YamuxedConnection::streamClose(
      StreamId stream_id, std::function<void(outcome::result<void>)> cb) {
    closeStreamForWrite(stream_id, std::move(cb));
  }

  void YamuxedConnection::streamReset(
      StreamId stream_id, std::function<void(outcome::result<void>)> cb) {
    write({resetStreamMsg(stream_id),
           [t = shared_from_this(), cb = std::move(cb),
            stream_id](auto &&write_res) {
             if (!write_res) {
               t->logger_->error("cannot write reset stream msg: {}",
                                 write_res.error().message());
               t->last_write_error_ = write_res.error();
               return cb(write_res.error());
             }
             t->removeStream(stream_id);
             cb(outcome::success());
           }});
  }

}  // namespace libp2p::connection
