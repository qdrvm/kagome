/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/muxer/yamux/yamux.hpp"

#include <boost/asio/buffers_iterator.hpp>
#include <gsl/span>
#include "libp2p/muxer/yamux/yamux_frame.hpp"
#include "libp2p/muxer/yamux/yamux_stream.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::muxer, Yamux::YamuxErrorStream, e) {
  using ErrorType = libp2p::muxer::Yamux::YamuxErrorStream;
  switch (e) {
    case ErrorType::NO_SUCH_STREAM:
      return "no such stream was found; maybe, it is closed";
    case ErrorType::NOT_WRITABLE:
      return "the stream is closed for writes";
    case ErrorType::NOT_READABLE:
      return "the stream is closed for reads";
    case ErrorType::YAMUX_IS_CLOSED:
      return "this Yamux instance is closed";
  }

  return "unknown";
}

namespace libp2p::muxer {
  Yamux::Yamux(std::shared_ptr<transport::Connection> connection,
               NewStreamHandler stream_handler, YamuxConfig yamux_config,
               kagome::common::Logger logger)
      : connection_{std::move(connection)},
        new_stream_handler_{std::move(stream_handler)},
        is_active_{true},
        config_{yamux_config},
        logger_{std::move(logger)} {
    // client uses odd numbers, server - even
    last_created_stream_id_ = config_.is_server ? 0 : 1;
  }

  Yamux::~Yamux() = default;

  void Yamux::start() {
    startReadingHeader();
  }

  void Yamux::startReadingHeader() {
    if (!is_active_) {
      return;
    }
    if (connection_->isClosed()) {
      closeYamux();
      return;
    }

    connection_->asyncRead(
        read_buffer_, YamuxFrame::kHeaderLength,
        [t = shared_from_this()](const std::error_code &ec, size_t n) {
          t->readingHeaderCompleted(ec, n);
        });
  }

  void Yamux::readingHeaderCompleted(const std::error_code &ec, size_t n) {
    if (ec) {
      logger_->error("cannot read from from the connection: {}", ec.message());
      close();
    }
    if (n != YamuxFrame::kHeaderLength) {
      logger_->error(
          "connection error: read less bytes, than expected in header");
      close();
    }

    if (!processHeader()) {
      startReadingHeader();
    }
  }

  void Yamux::readingDataCompleted(
      const std::error_code &ec, size_t n,
      std::reference_wrapper<YamuxStreamParameters> stream_wrapper) {
    if (ec) {
      logger_->error("cannot read from the connection: {}", ec.message());
      close();
    }

    auto &stream = stream_wrapper.get();
    // if there's a callback, which awaits for the message from this stream,
    // call it; buffer the message otherwise
    kagome::common::Buffer msg(n, 0);
    boost::asio::buffer_copy(boost::asio::buffer(msg.toVector()),
                             read_buffer_.data());
    read_buffer_.consume(n);
    if (!stream.completion_handlers_.empty()) {
      stream.completion_handlers_.front()(std::move(msg));
      stream.completion_handlers_.pop();
    } else {
      stream.buffered_messages_.push(std::move(msg));
    }

    startReadingHeader();
  }

  void Yamux::write(const kagome::common::Buffer &msg,
                    stream::Stream::ErrorCodeCallback cb) {
    outcoming_messages_.push({msg, std::move(cb)});
    if (!is_writing_) {
      startWriting();
    }
  }

  void Yamux::startWriting() {
    if (!is_active_) {
      return;
    }
    if (connection_->isClosed()) {
      closeYamux();
      return;
    }
    is_writing_ = true;

    if (!outcoming_messages_.empty()) {
      const auto &msg_and_callback = outcoming_messages_.front();
      connection_->asyncWrite(
          boost::asio::const_buffer{msg_and_callback.first.toBytes(),
                                    msg_and_callback.first.size()},
          [t = shared_from_this(), &cb = msg_and_callback.second](
              const std::error_code &ec, size_t n) mutable {
            t->writingCompleted(ec, n, cb);
          });
    } else {
      is_writing_ = false;
    }
  }

  void Yamux::writingCompleted(
      const std::error_code &ec, size_t n,
      const stream::Stream::ErrorCodeCallback &error_callback) {
    // we have written <MsgLength + HeaderLength> bytes
    error_callback(ec, n - YamuxFrame::kHeaderLength);
    outcoming_messages_.pop();
    startWriting();
  }

  void Yamux::stop() {
    is_active_ = false;
  }

  outcome::result<std::unique_ptr<stream::Stream>> Yamux::newStream() {
    if (!is_active_) {
      return YamuxErrorStream::YAMUX_IS_CLOSED;
    }

    auto stream_id = getNewStreamId();
    write(newStreamMsg(stream_id),
          [t = shared_from_this(), stream_id](auto &&ec, auto && /*unused*/) {
            if (ec) {
              t->logger_->error(
                  "could not write new stream message for stream_id {} with "
                  "error {}",
                  stream_id, ec.message());
            }
          });
    streams_.insert(std::make_pair(
        stream_id,
        YamuxStreamParameters{true, true, YamuxFrame::kDefaultWindowSize}));
    return std::make_unique<stream::YamuxStream>(shared_from_this(), stream_id);
  }

  void Yamux::close() {
    // send a reset message too all streams to notify the other side
    if (!streams_.empty()) {
      auto last_stream_id = streams_.rbegin()->first;
      for (const auto &stream : streams_) {
        write(resetStreamMsg(stream.first),
              [t = shared_from_this(), stream_id = stream.first,
               last_stream_id](auto &&ec, auto && /*unused*/) {
                if (ec) {
                  t->logger_->error(
                      "could not write reset stream message for stream_id {} "
                      "with "
                      "error {}",
                      stream_id, ec.message());
                }
                if (stream_id == last_stream_id) {
                  t->closeYamux();
                }
              });
      }
    } else {
      closeYamux();
    }
  }

  bool Yamux::isClosed() {
    return connection_->isClosed();
  }

  void Yamux::closeYamux() {
    streams_.clear();
    (void)connection_->close();
    is_active_ = false;
  }

  Yamux::StreamId Yamux::getNewStreamId() {
    last_created_stream_id_ += 2;
    return last_created_stream_id_;
  }

  void Yamux::registerNewStream(StreamId stream_id) {
    streams_.insert(
        {stream_id,
         YamuxStreamParameters{true, true, YamuxFrame::kDefaultWindowSize}});
    new_stream_handler_(
        std::make_unique<stream::YamuxStream>(shared_from_this(), stream_id));
    write(ackStreamMsg(stream_id),
          [t = shared_from_this(), stream_id](auto &&ec, auto && /* unused */) {
            if (ec) {
              t->logger_->error(
                  "could not write ack stream message for stream_id {} with "
                  "error {}",
                  stream_id, ec.message());
            }
          });
  }

  bool Yamux::processData(std::reference_wrapper<YamuxStreamParameters> stream,
                          const YamuxFrame &frame) {
    auto data_length = frame.length_;
    if (data_length == 0) {
      return false;
    }

    connection_->asyncRead(read_buffer_, data_length,
                           [t = shared_from_this(), stream](
                               const std::error_code &ec, size_t n) mutable {
                             t->readingDataCompleted(ec, n, stream);
                           });
    return true;
  }

  std::optional<std::reference_wrapper<YamuxStreamParameters>>
  Yamux::processAck(StreamId stream_id) {
    // acknowledge of start of a new stream; if we don't have such a stream,
    // a reset should be sent in order to notify the other side about a
    // problem
    auto stream = findStream(stream_id);
    if (!stream) {
      write(
          resetStreamMsg(stream_id),
          [t = shared_from_this(), stream_id](auto &&ec, auto && /* unused */) {
            if (ec) {
              t->logger_->error(
                  "could not write reset stream message for stream_id {} with "
                  "error {}",
                  stream_id, ec.message());
            }
          });
    }
    return std::ref(stream);
  }

  std::optional<std::reference_wrapper<YamuxStreamParameters>>
  Yamux::findStream(StreamId stream_id) {
    auto stream = streams_.find(stream_id);
    if (stream == streams_.end()) {
      return {};
    }
    return std::ref(stream->second);
  }

  void Yamux::closeStreamForRead(StreamId stream_id) {
    auto stream = findStream(stream_id);
    if (stream) {
      stream->get().is_readable_ = false;
    }
    if (!stream
        || (!stream->get().is_writable_ && !stream->get().is_readable_)) {
      // stream is closed on our side; reset it on the other as well
      removeStream(stream_id);
    }
  }

  void Yamux::closeStreamForWrite(StreamId stream_id) {
    auto stream = findStream(stream_id);
    if (stream) {
      stream->get().is_writable_ = false;
    }
    if (!stream
        || (!stream->get().is_writable_ && !stream->get().is_readable_)) {
      // stream is closed entirely on our side; reset it on the other as well
      removeStream(stream_id);
    } else {
      // tell other side not to wait for messages from us anymore
      write(closeStreamMsg(stream_id),
            [t = shared_from_this(), stream_id](auto &&ec, auto &&n) {
              if (ec) {
                t->logger_->error(
                    "could not write close stream message for stream_id {} "
                    "with error {}",
                    stream_id, ec.message());
              }
            });
    }
  }

  void Yamux::removeStream(StreamId stream_id) {
    write(resetStreamMsg(stream_id),
          [t = shared_from_this(), stream_id](auto &&ec, auto && /* unused */) {
            if (ec) {
              t->logger_->error(
                  "could not write reset stream message for stream_id {} with "
                  "error {}",
                  stream_id, ec.message());
            }
          });
    streams_.erase(stream_id);
  }

  bool Yamux::processHeader() {
    using FrameType = YamuxFrame::FrameType;

    auto frame_opt = parseFrame(
        gsl::make_span(static_cast<const uint8_t *>(read_buffer_.data().data()),
                       YamuxFrame::kHeaderLength));
    read_buffer_.consume(YamuxFrame::kHeaderLength);
    if (!frame_opt) {
      // could not parse the frame => client sent some nonsense, break the
      // connection
      write(goAwayMsg(YamuxFrame::GoAwayError::PROTOCOL_ERROR),
            [t = shared_from_this()](auto &&ec, auto && /* unused */) {
              if (ec) {
                t->logger_->error(
                    "could not write go away message with error {}",
                    ec.message());
              }
            });
      return false;
    }

    auto frame = std::move(*frame_opt);
    switch (frame.type_) {
      case FrameType::DATA:
        return processDataFrame(frame);
      case FrameType::WINDOW_UPDATE:
        processWindowUpdateFrame(frame);
        break;
      case FrameType::PING:
        processPingFrame(frame);
        break;
      case FrameType::GO_AWAY:
        processGoAwayFrame(frame);
        break;
    }
    return false;
  }

  bool Yamux::processDataFrame(const YamuxFrame &frame) {
    using Flag = YamuxFrame::Flag;

    auto stream_id = frame.stream_id_;
    switch (frame.flag_) {
      case Flag::SYN: {
        // can be start of a new stream, just data or both
        auto stream = findStream(stream_id);
        if (!stream) {
          // it is at least a new stream request; register it and send ack
          // message
          registerNewStream(stream_id);
          stream = *findStream(stream_id);
        }
        // process data in this frame, if there is one
        return processData(*stream, frame);
      }
      case Flag::ACK: {
        // can be ack of a new stream, just data or both
        if (auto stream = processAck(stream_id)) {
          return processData(*stream, frame);
        }
        break;
      }
      case Flag::FIN:
        closeStreamForRead(stream_id);
        break;
      case Flag::RST:
        removeStream(stream_id);
        break;
    }
    return false;
  }

  void Yamux::processWindowUpdateFrame(const YamuxFrame &frame) {
    using Flag = YamuxFrame::Flag;

    auto stream_id = frame.stream_id_;
    switch (frame.flag_) {
      case Flag::SYN: {
        // can be start of a new stream or update of a window size
        auto stream = findStream(stream_id);
        if (stream) {
          // this stream is already opened => window update
          stream->get().window_size_ = frame.length_;
        } else {
          // no such stream found => it's a creation of a new stream
          registerNewStream(stream_id);
        }
        break;
      }
      case Flag::ACK: {
        processAck(stream_id);
        break;
      }
      case Flag::FIN: {
        // stream was closed from the other side; still, we can write to it,
        // but new messages will not come
        closeStreamForRead(stream_id);
        break;
      }
      case Flag::RST:
        // close the stream (but not connection) entirely
        removeStream(stream_id);
        break;
    }
  }

  void Yamux::processPingFrame(const YamuxFrame &frame) {
    write(pingResponseMsg(frame.length_),
          [t = shared_from_this(), stream_id = frame.stream_id_](auto &&ec,
                                                                 auto &&n) {
            if (ec) {
              t->logger_->error(
                  "cannot send ping message to stream with id {} and error {}",
                  std::to_string(stream_id), ec.message());
            }
          });
  }

  void Yamux::processGoAwayFrame(const YamuxFrame &frame) {
    close();
  }

  /// YAMUX STREAM API

  void Yamux::streamReadFrameAsync(
      StreamId stream_id,
      stream::Stream::ReadCompletionHandler completion_handler) {
    if (!is_active_) {
      completion_handler(YamuxErrorStream::YAMUX_IS_CLOSED);
      return;
    }

    auto stream_opt = findStream(stream_id);
    if (!stream_opt) {
      completion_handler(YamuxErrorStream::NO_SUCH_STREAM);
      return;
    }

    auto &stream = stream_opt->get();
    if (!stream.is_readable_) {
      completion_handler(YamuxErrorStream::NOT_READABLE);
      return;
    }

    if (!stream.buffered_messages_.empty()) {
      auto msg = stream.buffered_messages_.front();
      stream.buffered_messages_.pop();
      completion_handler(std::move(msg));
      return;
    }
    // if there is no message available, push the callback to the queue of
    // callback; it will be called, when the message appears
    stream.completion_handlers_.push(std::move(completion_handler));
  }

  void Yamux::streamWriteFrameAsync(
      StreamId stream_id, const kagome::common::Buffer &msg,
      stream::Stream::ErrorCodeCallback error_callback) {
    if (!is_active_) {
      error_callback(YamuxErrorStream::YAMUX_IS_CLOSED, 0);
      return;
    }

    auto stream_opt = findStream(stream_id);
    if (!stream_opt) {
      error_callback(YamuxErrorStream::NO_SUCH_STREAM, 0);
      return;
    }

    if (!stream_opt->get().is_writable_) {
      error_callback(YamuxErrorStream::NOT_WRITABLE, 0);
      return;
    }

    write(dataMsg(stream_id, msg), std::move(error_callback));
  }

  void Yamux::streamClose(StreamId stream_id) {
    closeStreamForWrite(stream_id);
  }

  void Yamux::streamReset(StreamId stream_id) {
    removeStream(stream_id);
  }

  bool Yamux::streamIsClosedForWrite(StreamId stream_id) {
    auto stream = findStream(stream_id);
    if (stream) {
      return !stream->get().is_writable_;
    }
    return false;
  }

  bool Yamux::streamIsClosedForRead(StreamId stream_id) {
    auto stream = findStream(stream_id);
    if (stream) {
      return !stream->get().is_readable_;
    }
    return false;
  }

  bool Yamux::streamIsClosedEntirely(StreamId stream_id) {
    return !findStream(stream_id).has_value();
  }

}  // namespace libp2p::muxer
