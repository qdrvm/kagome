/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <optional>

#include <gsl/span>
#include "libp2p/muxer/yamux/yamux.hpp"
#include "libp2p/muxer/yamux/yamux_frame.hpp"
#include "libp2p/stream/yamux_stream.hpp"

namespace libp2p::muxer {
  using stream::YamuxStream;

  Yamux::Yamux(transport::Connection &connection, YamuxConfig config)
      : connection_{connection}, is_server_{config.is_server} {
    last_created_stream_id = is_server_ ? 0 : 1;

    // Listener for all incoming frames should be created. Maybe, like this?
    // std::thread({
    //   while (true) {
    //     connection_.read(12);
    //     parseFrame(frame); if not parsed => sendGoAway(); closeConnection();
    //     processFrame();
    //   }
  }

  Yamux::~Yamux() {
    resetAllStreams();
  }

  std::unique_ptr<stream::Stream> Yamux::newStream() {
    auto stream_id = getNewStreamId();

    connection_.write(newStreamMsg(stream_id));

    // according to docs, we are not required to wait until ACK is received,
    // relying on the reliable underneath connection, so the stream can be
    // created and passed to the client immediately, even though we will need to
    // handle RST flag, if received later
    readable_streams_.insert(stream_id);
    writable_streams_.insert(stream_id);
    return std::make_unique<YamuxStream>(*this, stream_id);
  }

  outcome::result<multi::Multiaddress> Yamux::getRemoteMultiaddr() const {
    return connection_.getRemoteMultiaddr();
  }

  bool Yamux::processFrame(const YamuxFrame &frame) {
    using FrameType = YamuxFrame::FrameType;
    using Flag = YamuxFrame::Flag;

    auto frame_stream_id = frame.stream_id_;

    if (frame.type_ == FrameType::kGoAway) {
      // connection must be entirely closed
      closeConnection();
      return false;
    }

    if (frame.type_ == FrameType::kPing) {
      // response with ping as well; length field contains a "ping number"
      connection_.write(pingResponseMsg(frame.length_));
      return true;
    }

    if (frame.type_ == FrameType::kWindowUpdate) {
      switch (frame.flag_) {
        case Flag::kSyn: {
          // used to open a stream or update window size
          auto stream_window = streams_windows_.find(frame_stream_id);
          if (stream_window != streams_windows_.end()) {
            // this stream is already opened => window update
            stream_window->second = frame.length_;
          } else {
            // create a new stream and pass it to the listener?
            /// new_streams_signal.trigger(new YamuxStream{});
          }
          break;
        }
        case Flag::kAck:
          // acknowledge of start of a new stream; don't have to do anything?
          break;
        case Flag::kFin:
          // stream was closed from the other side; still, we can write to it,
          // but new messages will not come
          closeStreamForRead(frame_stream_id);
          break;
        case Flag::kRst:
          // close the stream (but not connection) entirely
          removeStream(frame_stream_id);
          break;
      }
      return true;
    }

    switch (frame.flag_) {
      case Flag::kSyn:
      case Flag::kAck: {
        // data received
        auto data_res = connection_.read(frame.length_);
        if (!data_res) {
          // underlying connection failed; just return?
          return true;
        }
        // place a received data to the stream's buffer
        auto stream_buffer = streams_buffers_.find(frame_stream_id);
        if (stream_buffer == streams_buffers_.end()) {
          // the stream is closed for read; discard the message
          return true;
        }
        stream_buffer->second.push(std::move(data_res.value()));
        break;
      }
      case Flag::kFin:
        closeStreamForRead(frame_stream_id);
        break;
      case Flag::kRst:
        removeStream(frame_stream_id);
        break;
    }
    return true;
  }

  std::optional<common::NetworkMessage> Yamux::findMsgInBuffer(
      StreamId stream_id) {
    auto stream_buffer_entry = streams_buffers_.find(stream_id);
    if (stream_buffer_entry == streams_buffers_.end()) {
      return {};
    }

    auto &stream_buffer = stream_buffer_entry->second;
    if (stream_buffer.empty()) {
      return {};
    }

    /// FIXME: this copy should be avoided
    auto msg = stream_buffer.front();
    stream_buffer.pop();
    return msg;
  }

  void Yamux::closeConnection() {
    resetAllStreams();
    writable_streams_.clear();
    readable_streams_.clear();
    streams_windows_.clear();
    streams_buffers_.clear();
    connection_.close();
  }

  void Yamux::removeStream(StreamId stream_id) {
    writable_streams_.erase(stream_id);
    readable_streams_.erase(stream_id);
    streams_buffers_.erase(stream_id);
  }

  void Yamux::closeStreamForRead(StreamId stream_id) {
    readable_streams_.erase(stream_id);
  }

  void Yamux::closeStreamForWrite(StreamId stream_id) {
    writable_streams_.erase(stream_id);
  }

  void Yamux::resetAllStreams() {
    for (auto stream : writable_streams_) {
      resetStream(stream);
    }
  }

  void Yamux::closeStream(StreamId stream_id) {
    connection_.write(closeStreamMsg(stream_id));
    writable_streams_.erase(stream_id);
  }

  void Yamux::resetStream(StreamId stream_id) {
    connection_.write(resetStreamMsg(stream_id));
    removeStream(stream_id);
  }

  outcome::result<common::NetworkMessage> Yamux::readFrame(StreamId stream_id) {
    if (!streamCanRead(stream_id)) {
      return YamuxError::kNoReads;
    }

    // we should somehow block here until message for this stream appears or the
    // stream is closed
    if (auto msg = findMsgInBuffer(stream_id)) {
      return *msg;
    }
  }

  outcome::result<void> Yamux::writeFrame(StreamId stream_id,
                                          const common::NetworkMessage &msg) {
    if (!streamCanWrite(stream_id)) {
      return YamuxError::kNoWrites;
    }

    return connection_.write(dataMsg(stream_id, msg));
  }

  Yamux::StreamId Yamux::getNewStreamId() {
    last_created_stream_id += 2;  // use either odd or even numbers
    return last_created_stream_id;
  }

  bool Yamux::streamCanRead(StreamId stream_id) const {
    return readable_streams_.find(stream_id) != readable_streams_.end();
  }

  bool Yamux::streamCanWrite(StreamId stream_id) const {
    return writable_streams_.find(stream_id) != writable_streams_.end();
  }

}  // namespace libp2p::muxer
