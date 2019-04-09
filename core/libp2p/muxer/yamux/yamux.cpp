/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/muxer/yamux/yamux.hpp"

#include <boost/asio/buffers_iterator.hpp>
#include "libp2p/muxer/yamux/yamux_frame.hpp"
#include "libp2p/muxer/yamux/yamux_stream.hpp"

namespace libp2p::muxer {
  Yamux::Yamux(std::shared_ptr<transport::Connection> connection,
               NewStreamHandler stream_handler, YamuxConfig yamux_config,
               kagome::common::Logger logger)
      : connection_{std::move(connection)},
        new_stream_handler_{std::move(stream_handler)},
        is_active_{true},
        is_server_{yamux_config.is_server},
        logger_{std::move(logger)} {
    // client uses odd numbers, server - even
    last_created_stream_id_ = is_server_ ? 0 : 1;
  }

  Yamux::~Yamux() {
    this->close();
  }

  void Yamux::start() {
    if (!is_active_) {
      return;
    }

    // see, if there's any messages to be written to the connection
    {
      std::lock_guard<std::mutex> lg{outcoming_msgs_mutex_};
      if (!outcoming_messages_.empty()) {
        auto msg_and_callback = outcoming_messages_.front();
        outcoming_messages_.pop();
        connection_->writeAsync(
            msg_and_callback.first,
            [t = shared_from_this(), cb = std::move(msg_and_callback.second)](
                const boost::system::error_code &ec, size_t n) {
              t->writingComplete(ec, n, std::move(cb));
            });
      }
    }
    // if there are no outcoming messages, read something from the connection
    connection_->readAsync(  // read_buffer, YamuxFrame::kHeaderLength,
        [t = shared_from_this()](const boost::system::error_code &ec,
                                 size_t n) {
          t->readingHeaderComplete(ec, n);
        });
  }

  void Yamux::readingHeaderComplete(const boost::system::error_code &ec,
                                    size_t n) {
    if (ec) {
      logger_->error("cannot read from from the connection: {}", ec.value());
      // terminate the Yamux? It's a critical error
    }
    if (n != YamuxFrame::kHeaderLength) {
      logger_->error(
          "connection error: read less bytes, than expected in header");
      // terminate the Yamux?
    }

    processHeader();
    start();
  }

  void Yamux::readingDataComplete(const boost::system::error_code &ec, size_t n,
                                  std::shared_ptr<StreamParameters> stream) {
    if (ec) {
      logger_->error("cannot read from the connection: {}", ec.value());
      // terminate Yamux?
    }

    // if there's a callback, which awaits for the message from this stream,
    // call it; buffer the message otherwise
    auto msg =
        kagome::common::Buffer{boost::asio::buffers_begin(read_buffer_),
                               boost::asio::buffers_begin(read_buffer_) + n};
    std::lock_guard<std::mutex> lg{stream->queues_mutex_};
    if (!stream->completion_handlers_.empty()) {
      stream->completion_handlers_.front()(std::move(msg));
      stream->completion_handlers_.pop();
      return;
    }
    stream->buffered_messages_.push(std::move(msg));
  }

  void Yamux::writingComplete(
      const boost::system::error_code &ec, size_t n,
      stream::Stream::ErrorCodeCallback error_callback) {
    error_callback(ec, n);
  }

  void Yamux::stop() {
    is_active_ = false;
  }

  outcome::result<std::unique_ptr<stream::Stream>> Yamux::newStream() {
    auto stream_id = getNewStreamId();
    // TODO: change to writing loop
    OUTCOME_TRY(connection_->write(newStreamMsg(stream_id)));

    registerNewStream(stream_id);
    return std::make_unique<stream::YamuxStream>(this, stream_id);
  }

  void Yamux::close() {
    (void)connection_->close();
    stop();
  }

  bool Yamux::isClosed() {
    return connection_->isClosed();
  }

  Yamux::StreamId Yamux::getNewStreamId() {
    last_created_stream_id_ += 2;
    return last_created_stream_id_;
  }

  void Yamux::registerNewStream(StreamId stream_id) {
    streams_.insert(
        {stream_id,
         StreamParameters{true, true, YamuxFrame::kDefaultWindowSize}});
    new_stream_handler_(std::make_unique<stream::YamuxStream>(this, stream_id));
    // TODO: change to writing loop
    connection_->writeAsync(ackStreamMsg(stream_id), [](auto &&, auto &&) {});
  }

  void Yamux::processData(Yamux::StreamParameters &stream,
                          const YamuxFrame &frame) {
    auto data_length = frame.length_;
    if (data_length == 0) {
      return;
    }
    connection_->readAsync(  // read_buffer, data_length,
        [t = shared_from_this()](const boost::system::error_code &ec,
                                 size_t n) { t->readingDataComplete(ec, n); });
  }

  std::optional<std::shared_ptr<Yamux::StreamParameters>> Yamux::processAck(
      StreamId stream_id) {
    // acknowledge of start of a new stream; if we don't have such a stream,
    // a reset should be sent in order to notify the other side about a
    // problem
    auto stream = findStream(stream_id);
    if (!stream) {
      // TODO: change to writing loop
      connection_->writeAsync(resetStreamMsg(stream_id),
                              [](std::error_code, size_t) {});
    }
    return stream;
  }

  std::optional<std::shared_ptr<Yamux::StreamParameters>> Yamux::findStream(
      StreamId stream_id) const {
    auto stream = streams_.find(stream_id);
    if (stream == streams_.end()) {
      return {};
    }
    return stream->second;
  }

  void Yamux::closeStreamForRead(StreamId stream_id) {
    auto stream = findStream(stream_id);
    if (stream) {
      (*stream)->is_readable_ = false;
    }
    if (!stream || (!(*stream)->is_writable_ && !(*stream)->is_readable_)) {
      // stream is closed on our side; reset it on the other as well
      removeStream(stream_id);
    }
  }

  void Yamux::closeStreamForWrite(StreamId stream_id) {
    auto stream = findStream(stream_id);
    if (stream) {
      (*stream)->is_writable_ = false;
    }
    if (!stream || (!(*stream)->is_writable_ && !(*stream)->is_readable_)) {
      // stream is closed on our side; reset it on the other as well
      removeStream(stream_id);
    }
  }

  void Yamux::removeStream(StreamId stream_id) {
    // TODO: change to writing loop
    connection_->writeAsync(resetStreamMsg(stream_id),
                            [](std::error_code, size_t) {});
    streams_.erase(stream_id);
  }

  void Yamux::processHeader() {
    using FrameType = YamuxFrame::FrameType;

    auto frame_opt = parseFrame(
        {boost::asio::buffers_begin(read_buffer_),
         boost::asio::buffers_begin(read_buffer_) + YamuxFrame::kHeaderLength});
    if (!frame_opt) {
      // could not parse the frame => client sent some nonsense, break the
      // connection
      // TODO: change to writing loop
      connection_->writeAsync(
          goAwayMsg(YamuxFrame::GoAwayError::kProtocolError),
          [](auto &&, auto &&) {});
      close();
    }

    auto frame = std::move(*frame_opt);
    switch (frame.type_) {
      case FrameType::kData:
        processDataFrame(frame);
        return;
      case FrameType::kWindowUpdate:
        processWindowUpdateFrame(frame);
        return;
      case FrameType::kPing:
        processPingFrame(frame);
        return;
      case FrameType::kGoAway:
        processGoAwayFrame(frame);
        return;
    }
  }

  void Yamux::processDataFrame(const YamuxFrame &frame) {
    using Flag = YamuxFrame::Flag;

    auto stream_id = frame.stream_id_;
    switch (frame.flag_) {
      case Flag::kSyn: {
        // can be start of a new stream, just data or both
        auto stream = findStream(stream_id);
        if (!stream) {
          // it is at least a new stream request; register it and send ack
          // message
          registerNewStream(stream_id);
          stream = *findStream(stream_id);
        }
        // process data in this frame, if there is one
        processData(**stream, frame);
        break;
      }
      case Flag::kAck: {
        // can be ack of a new stream, just data or both
        if (auto stream = processAck(stream_id)) {
          processData(**stream, frame);
        }
        break;
      }
      case Flag::kFin:
        closeStreamForRead(stream_id);
        break;
      case Flag::kRst:
        removeStream(stream_id);
        break;
    }
  }

  void Yamux::processWindowUpdateFrame(const YamuxFrame &frame) {
    using Flag = YamuxFrame::Flag;

    auto stream_id = frame.stream_id_;
    switch (frame.flag_) {
      case Flag::kSyn: {
        // can be start of a new stream or update of a window size
        auto stream = findStream(stream_id);
        if (stream) {
          // this stream is already opened => window update
          (*stream)->window_size_ = frame.length_;
        } else {
          // no such stream found => it's a creation of a new stream
          registerNewStream(stream_id);
        }
        break;
      }
      case Flag::kAck: {
        processAck(stream_id);
        break;
      }
      case Flag::kFin: {
        // stream was closed from the other side; still, we can write to it,
        // but new messages will not come
        closeStreamForRead(stream_id);
        break;
      }
      case Flag::kRst:
        // close the stream (but not connection) entirely
        removeStream(stream_id);
        break;
    }
  }

  void Yamux::processPingFrame(const YamuxFrame &frame) {
    // TODO: change to writing loop
    connection_->writeAsync(
        pingResponseMsg(frame.length_),
        [t = shared_from_this(), stream_id = frame.stream_id_](
            std::error_code error_code, size_t written) {
          if (error_code.value() != 0) {
            t->logger_->error("cannot send ping message to stream with id "
                              + std::to_string(stream_id));
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
    auto stream_opt = findStream(stream_id);
    if (!stream_opt) {
      completion_handler(YamuxIOError::kNoSuchStream);
    }

    auto stream = *stream_opt;
    std::lock_guard<std::mutex> lg{stream->queues_mutex_};
    if (!stream->buffered_messages_.empty()) {
      auto msg = stream->buffered_messages_.front();
      stream->buffered_messages_.pop();
      completion_handler(std::move(msg));
      return;
    }
    // if there is no message available, push the callback to the queue of
    // callback; it will be called, when the message appears
    stream->completion_handlers_.push(std::move(completion_handler));
  }

  void Yamux::streamWriteFrameAsync(
      StreamId stream_id, const common::NetworkMessage &msg,
      stream::Stream::ErrorCodeCallback error_callback) {
    auto msg_and_callback =
        std::make_pair(dataMsg(stream_id, msg), std::move(error_callback));
    std::lock_guard<std::mutex> lg{outcoming_msgs_mutex_};
    outcoming_messages_.push(std::move(msg_and_callback));
  }

  void Yamux::streamClose(StreamId stream_id) {
    closeStreamForWrite(stream_id);
  }

  void Yamux::streamReset(StreamId stream_id) {
    removeStream(stream_id);
  }

  bool Yamux::streamIsClosedForWrite(StreamId stream_id) const {
    auto stream = findStream(stream_id);
    if (stream) {
      return !(*stream)->is_writable_;
    }
    return false;
  }

  bool Yamux::streamIsClosedForRead(StreamId stream_id) const {
    auto stream = findStream(stream_id);
    if (stream) {
      return !(*stream)->is_readable_;
    }
    return false;
  }

  bool Yamux::streamIsClosedEntirely(StreamId stream_id) const {
    return !findStream(stream_id).has_value();
  }

}  // namespace libp2p::muxer
