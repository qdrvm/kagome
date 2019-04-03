/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <exception>
#include <optional>

#include <gsl/span>
#include "libp2p/stream/yamux_stream.hpp"
#include "yamux.hpp"

namespace {
  using Buffer = kagome::common::Buffer;
  using StreamId = uint32_t;

  /**
   * Header with optional data, which is sent and accepted with Yamux protocol
   */
  struct YamuxFrame {
    enum class FrameType : uint8_t {
      kData = 0,          // transmit data
      kWindowUpdate = 1,  // update the sender's receive window size
      kPing = 2,          // ping for various purposes
      kGoAway = 3         // close the session
    };
    enum class Flag : uint16_t {
      kSyn = 0,  // start of a new stream
      kAck = 2,  // acknowledge start of a new stream
      kFin = 4,  // half-close of the stream
      kRst = 8   // reset a stream
    };
    static constexpr uint8_t default_version = 0;
    static constexpr uint32_t default_window_size = 256;

    uint8_t version_;
    FrameType type_;
    Flag flag_;
    StreamId stream_id_;
    uint32_t length_;
    Buffer data_;

    /**
     * Get bytes representation of the Yamux frame
     * @return bytes of the frame
     */
    static Buffer frameBytes(uint8_t version, FrameType type, Flag flag,
                             uint32_t stream_id, uint32_t length,
                             const Buffer &data = Buffer{}) {
      return Buffer{}
          .putUint8(version)
          .putUint8(static_cast<uint8_t>(type))
          .putUint16(static_cast<uint16_t>(flag))
          .putUint32(stream_id)
          .putUint32(length)
          .putBuffer(data);
    }
  };

  Buffer newStreamMsg(StreamId stream_id) {
    return YamuxFrame::frameBytes(
        YamuxFrame::default_version, YamuxFrame::FrameType::kWindowUpdate,
        YamuxFrame::Flag::kSyn, stream_id, YamuxFrame::default_window_size);
  }

  Buffer closeStreamMsg(StreamId stream_id) {
    return YamuxFrame::frameBytes(
        YamuxFrame::default_version, YamuxFrame::FrameType::kWindowUpdate,
        YamuxFrame::Flag::kFin, stream_id, YamuxFrame::default_window_size);
  }

  Buffer dataMsg(StreamId stream_id, const Buffer &data) {
    return YamuxFrame::frameBytes(YamuxFrame::default_version,
                                  YamuxFrame::FrameType::kData,
                                  YamuxFrame::Flag::kSyn, stream_id,
                                  static_cast<uint32_t>(data.size()), data);
  }

  std::optional<YamuxFrame> parseFrame(const Buffer &frame_bytes) {
    if (frame_bytes.size() < 12) {
      return {};
    }

    YamuxFrame frame{};

    frame.version_ = frame_bytes[0];

    switch (frame_bytes[1]) {
      case 0:
        frame.type_ = YamuxFrame::FrameType::kData;
        break;
      case 1:
        frame.type_ = YamuxFrame::FrameType::kWindowUpdate;
        break;
      case 2:
        frame.type_ = YamuxFrame::FrameType::kPing;
        break;
      case 3:
        frame.type_ = YamuxFrame::FrameType::kGoAway;
        break;
      default:
        return {};
    }

    switch ((static_cast<uint16_t>(frame_bytes[3]) << 8) | frame_bytes[2]) {
      case 0:
        frame.flag_ = YamuxFrame::Flag::kSyn;
        break;
      case 2:
        frame.flag_ = YamuxFrame::Flag::kAck;
        break;
      case 4:
        frame.flag_ = YamuxFrame::Flag::kFin;
        break;
      case 8:
        frame.flag_ = YamuxFrame::Flag::kRst;
        break;
      default:
        return {};
    }

    frame.stream_id_ = (static_cast<uint16_t>(frame_bytes[7]) << 24)
        | (static_cast<uint16_t>(frame_bytes[6]) << 16)
        | (static_cast<uint16_t>(frame_bytes[5]) << 8)
        | (static_cast<uint16_t>(frame_bytes[4]));

    frame.length_ = (static_cast<uint16_t>(frame_bytes[11]) << 24)
        | (static_cast<uint16_t>(frame_bytes[10]) << 16)
        | (static_cast<uint16_t>(frame_bytes[9]) << 8)
        | (static_cast<uint16_t>(frame_bytes[8]));

    const auto &data_begin = frame_bytes.begin() + 11;
    if (data_begin != frame_bytes.end()) {
      frame.data_ = Buffer{std::vector<uint8_t>(data_begin, frame_bytes.end())};
    }

    return frame;
  }

}  // namespace

namespace libp2p::muxer {
  using stream::YamuxStream;

  Yamux::Yamux(transport::Connection &connection, YamuxConfig config)
      : connection_{connection}, is_server_{config.is_server} {
    last_created_stream_id = is_server_ ? 0 : 1;
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

  void Yamux::closeStream(StreamId stream_id) {
    connection_.write(closeStreamMsg(stream_id));
    writable_streams_.erase(stream_id);
  }

  outcome::result<common::NetworkMessage> Yamux::readFrame() {
    // read 12 bytes
    // parse frame
    // if data => read length and return
    // if service => processService(msg)
  }

  outcome::result<void> Yamux::writeFrame(const common::NetworkMessage &msg) {
    // just write the msg
  }

  outcome::result<kagome::common::Buffer> Yamux::read(StreamId stream_id,
                                                      uint32_t to_read) {
    if (!streamCanRead(stream_id)) {
      return ReadWriteError::kStreamError;
    }

    OUTCOME_TRY(frame, connection_.read(to_read));
    if (auto msg = parseFrame(frame)) {
      return msg->data_;
    }
    return ReadWriteError::kConnectionError;
  }

  outcome::result<kagome::common::Buffer> Yamux::readSome(StreamId stream_id,
                                                          uint32_t to_read) {
    if (!streamCanRead(stream_id)) {
      return ReadWriteError::kStreamError;
    }
    return connection_.readSome(to_read);
  }

  void Yamux::readAsync(
      StreamId stream_id,
      std::function<basic::Readable::BufferResultCallback> callback) noexcept {
    if (!streamCanRead(stream_id)) {
      return ReadWriteError::kStreamError;
    }
    connection_.readAsync(std::move(callback));
  }

  outcome::result<void> Yamux::writeSome(StreamId stream_id,
                                         const kagome::common::Buffer &msg) {
    if (!streamCanWrite(stream_id)) {
      return ReadWriteError::kStreamError;
    }
    return connection_.writeSome(dataMsg(stream_id, msg));
  }

  outcome::result<void> Yamux::write(StreamId stream_id,
                                     const kagome::common::Buffer &msg) {
    if (!streamCanWrite(stream_id)) {
      return ReadWriteError::kStreamError;
    }
    return connection_.write(dataMsg(stream_id, msg));
  }

  void Yamux::writeAsync(
      StreamId stream_id, const kagome::common::Buffer &msg,
      std::function<basic::Writable::ErrorCodeCallback> handler) noexcept {
    if (!streamCanWrite(stream_id)) {
      return ReadWriteError::kStreamError;
    }
    connection_.writeAsync(dataMsg(stream_id, msg), std::move(handler));
  }

  outcome::result<common::NetworkMessage> Yamux::read(
      StreamId stream_id) const {
    const auto &stream_buffer = stream_buffers_.find(stream_id);
    if (stream_buffer == stream_buffers_.end()) {
      return ReadWriteError::kStreamError;
    }

    return stream_buffer->second[0];
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

  outcome::result<multi::Multiaddress> Yamux::getRemoteMultiaddr() const {
    return connection_.getRemoteMultiaddr();
  }

}  // namespace libp2p::muxer
