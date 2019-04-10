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
    case ErrorType::kNoSuchStream:
      return "no such stream was found; maybe, it was already closed";
    case ErrorType::kYamuxIsClosed:
      return "this Yamux instance is closed";
  }
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

  Yamux::~Yamux() {
    resetAllStreams();
    closeYamux();
  }

  void Yamux::start() {
    if (!is_active_) {
      return;
    }

    // see, if there's any messages to be written to the connection
    if (!outcoming_messages_.empty()) {
      auto msg_and_callback = outcoming_messages_.front();
      outcoming_messages_.pop();

      const auto &msg = msg_and_callback.first;
      write_buffer_ = boost::asio::const_buffer(msg.toBytes(), msg.size());
      connection_->asyncWrite(
          write_buffer_,
          [t = shared_from_this(), cb = std::move(msg_and_callback.second)](
              const std::error_code &ec, size_t n) mutable {
            t->writingCompleted(ec, n, cb);
          });
      return;
    }

    // if there are no outcoming messages left, read something from the
    // connection
    connection_->asyncRead(
        read_buffer_, YamuxFrame::kHeaderLength,
        [t = shared_from_this()](const std::error_code &ec, size_t n) {
          t->readingHeaderCompleted(ec, n);
        });
  }

  void Yamux::readingHeaderCompleted(const std::error_code &ec, size_t n) {
    if (ec) {
      logger_->error("cannot read from from the connection: {}", ec.value());
      // terminate the Yamux? It's a critical error
    }
    if (n != YamuxFrame::kHeaderLength) {
      logger_->error(
          "connection error: read less bytes, than expected in header");
      // terminate the Yamux?
    }

    if (!processHeader()) {
      start();
    }
  }

  void Yamux::readingDataCompleted(const std::error_code &ec, size_t n,
                                   StreamParameters &stream) {
    if (ec) {
      logger_->error("cannot read from the connection: {}", ec.value());
      // terminate Yamux?
    }

    // if there's a callback, which awaits for the message from this stream,
    // call it; buffer the message otherwise
    std::vector<uint8_t> bytes(n);
    boost::asio::buffer_copy(boost::asio::buffer(bytes), read_buffer_.data());
    read_buffer_.consume(n);
    kagome::common::Buffer msg{std::move(bytes)};
    if (!stream.completion_handlers_.empty()) {
      stream.completion_handlers_.front()(std::move(msg));
      stream.completion_handlers_.pop();
    } else {
      stream.buffered_messages_.push(std::move(msg));
    }

    start();
  }

  void Yamux::writingCompleted(
      const std::error_code &ec, size_t n,
      const stream::Stream::ErrorCodeCallback &error_callback) {
    error_callback(ec, n);
    start();
  }

  void Yamux::stop() {
    is_active_ = false;
  }

  outcome::result<std::unique_ptr<stream::Stream>> Yamux::newStream() {
    if (!is_active_) {
      return YamuxErrorStream::kYamuxIsClosed;
    }

    auto stream_id = getNewStreamId();
    outcoming_messages_.push(
        {newStreamMsg(stream_id), [](auto &&, auto &&) {}});
    streams_.insert(
        std::make_pair(stream_id,
                       std::make_shared<StreamParameters>(StreamParameters{
                           true, true, YamuxFrame::kDefaultWindowSize})));
    return std::make_unique<stream::YamuxStream>(shared_from_this(), stream_id);
  }

  void Yamux::close() {
    closeYamux();
  }

  bool Yamux::isClosed() {
    return connection_->isClosed();
  }

  void Yamux::closeYamux() {
    (void)connection_->close();
    is_active_ = false;
  }

  Yamux::StreamId Yamux::getNewStreamId() {
    last_created_stream_id_ += 2;
    return last_created_stream_id_;
  }

  void Yamux::registerNewStream(StreamId stream_id) {
    streams_.insert({stream_id,
                     std::make_shared<StreamParameters>(StreamParameters{
                         true, true, YamuxFrame::kDefaultWindowSize})});
    new_stream_handler_(
        std::make_unique<stream::YamuxStream>(shared_from_this(), stream_id));
    outcoming_messages_.push(
        {ackStreamMsg(stream_id), [](auto &&, auto &&) {}});
  }

  bool Yamux::processData(std::shared_ptr<Yamux::StreamParameters> stream,
                          const YamuxFrame &frame) {
    auto data_length = frame.length_;
    if (data_length == 0) {
      return false;
    }

    connection_->asyncRead(read_buffer_, data_length,
                           [t = shared_from_this(), stream = std::move(stream)](
                               const std::error_code &ec, size_t n) mutable {
                             t->readingDataCompleted(ec, n, *stream);
                           });
    return true;
  }

  std::optional<std::shared_ptr<Yamux::StreamParameters>> Yamux::processAck(
      StreamId stream_id) {
    // acknowledge of start of a new stream; if we don't have such a stream,
    // a reset should be sent in order to notify the other side about a
    // problem
    auto stream = findStream(stream_id);
    if (!stream) {
      outcoming_messages_.push(
          {resetStreamMsg(stream_id), [](auto &&, auto &&) {}});
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
    outcoming_messages_.push(
        {resetStreamMsg(stream_id), [](auto &&, auto &&) {}});
    streams_.erase(stream_id);
  }

  void Yamux::resetAllStreams() noexcept {
    for (const auto &stream : streams_) {
      // as this function is called from the destructor, we just don't want any
      // exceptions to happen here
      try {
        outcoming_messages_.push(
            {resetStreamMsg(stream.first), [](auto &&, auto &&) {}});
      } catch (const std::exception &e) {
      }
    }
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
      outcoming_messages_.push(
          {goAwayMsg(YamuxFrame::GoAwayError::kProtocolError),
           [](auto &&, auto &&) {}});
      return false;
    }

    auto frame = std::move(*frame_opt);
    switch (frame.type_) {
      case FrameType::kData:
        return processDataFrame(frame);
      case FrameType::kWindowUpdate:
        processWindowUpdateFrame(frame);
        break;
      case FrameType::kPing:
        processPingFrame(frame);
        break;
      case FrameType::kGoAway:
        processGoAwayFrame(frame);
        break;
    }
    return false;
  }

  bool Yamux::processDataFrame(const YamuxFrame &frame) {
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
        return processData(*stream, frame);
      }
      case Flag::kAck: {
        // can be ack of a new stream, just data or both
        if (auto stream = processAck(stream_id)) {
          return processData(*stream, frame);
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
    return false;
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
    outcoming_messages_.push(
        {pingResponseMsg(frame.length_),
         [t = shared_from_this(), stream_id = frame.stream_id_](
             std::error_code error_code, size_t written) {
           if (error_code.value() != 0) {
             t->logger_->error("cannot send ping message to stream with id "
                               + std::to_string(stream_id));
           }
         }});
  }

  void Yamux::processGoAwayFrame(const YamuxFrame &frame) {
    close();
  }

  /// YAMUX STREAM API

  void Yamux::streamReadFrameAsync(
      StreamId stream_id,
      stream::Stream::ReadCompletionHandler completion_handler) {
    if (!is_active_) {
      completion_handler(YamuxErrorStream::kYamuxIsClosed);
      return;
    }

    auto stream_opt = findStream(stream_id);
    if (!stream_opt) {
      completion_handler(YamuxErrorStream::kNoSuchStream);
      return;
    }

    auto stream = *stream_opt;
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
    if (!is_active_) {
      error_callback(YamuxErrorStream::kYamuxIsClosed, 0);
      return;
    }

    outcoming_messages_.push(
        {dataMsg(stream_id, msg), std::move(error_callback)});
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
