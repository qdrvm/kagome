/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/muxer/yamux/yamux_frame.hpp"

#include <arpa/inet.h>

namespace libp2p::muxer {
  kagome::common::Buffer YamuxFrame::frameBytes(
      uint8_t version, FrameType type, Flag flag, uint32_t stream_id,
      uint32_t length, const kagome::common::Buffer &data) {
    return kagome::common::Buffer{}
        .putUint8(version)
        .putUint8(static_cast<uint8_t>(type))
        .putUint16(htons(static_cast<uint16_t>(flag)))  // NOLINT
        .putUint32(htonl(stream_id))                    // NOLINT
        .putUint32(htonl(length))                       // NOLINT
        .putBuffer(data);
  }

  kagome::common::Buffer newStreamMsg(YamuxFrame::StreamId stream_id) {
    return YamuxFrame::frameBytes(YamuxFrame::kDefaultVersion,
                                  YamuxFrame::FrameType::kData,
                                  YamuxFrame::Flag::kSyn, stream_id, 0);
  }

  kagome::common::Buffer ackStreamMsg(YamuxFrame::StreamId stream_id) {
    return YamuxFrame::frameBytes(YamuxFrame::kDefaultVersion,
                                  YamuxFrame::FrameType::kData,
                                  YamuxFrame::Flag::kAck, stream_id, 0);
  }

  kagome::common::Buffer closeStreamMsg(YamuxFrame::StreamId stream_id) {
    return YamuxFrame::frameBytes(YamuxFrame::kDefaultVersion,
                                  YamuxFrame::FrameType::kData,
                                  YamuxFrame::Flag::kFin, stream_id, 0);
  }

  kagome::common::Buffer resetStreamMsg(YamuxFrame::StreamId stream_id) {
    return YamuxFrame::frameBytes(YamuxFrame::kDefaultVersion,
                                  YamuxFrame::FrameType::kData,
                                  YamuxFrame::Flag::kRst, stream_id, 0);
  }

  kagome::common::Buffer pingOutMsg(uint32_t value) {
    return YamuxFrame::frameBytes(YamuxFrame::kDefaultVersion,
                                  YamuxFrame::FrameType::kPing,
                                  YamuxFrame::Flag::kSyn, 0, value);
  }

  kagome::common::Buffer pingResponseMsg(uint32_t value) {
    return YamuxFrame::frameBytes(YamuxFrame::kDefaultVersion,
                                  YamuxFrame::FrameType::kPing,
                                  YamuxFrame::Flag::kAck, 0, value);
  }

  kagome::common::Buffer dataMsg(YamuxFrame::StreamId stream_id,
                                 const kagome::common::Buffer &data) {
    return YamuxFrame::frameBytes(YamuxFrame::kDefaultVersion,
                                  YamuxFrame::FrameType::kData,
                                  YamuxFrame::Flag::kSyn, stream_id,
                                  static_cast<uint32_t>(data.size()), data);
  }

  kagome::common::Buffer goAwayMsg(YamuxFrame::GoAwayError error) {
    return YamuxFrame::frameBytes(
        YamuxFrame::kDefaultVersion, YamuxFrame::FrameType::kGoAway,
        YamuxFrame::Flag::kSyn, 0, static_cast<uint32_t>(error));
  }

  std::optional<YamuxFrame> parseFrame(gsl::span<const uint8_t> frame_bytes) {
    if (frame_bytes.size() < YamuxFrame::kHeaderLength) {
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
      case 1:
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

    frame.stream_id_ = (static_cast<uint32_t>(frame_bytes[7]) << 24)
        | (static_cast<uint16_t>(frame_bytes[6]) << 16)
        | (static_cast<uint16_t>(frame_bytes[5]) << 8)
        | (static_cast<uint16_t>(frame_bytes[4]));

    frame.length_ = (static_cast<uint32_t>(frame_bytes[11]) << 24)
        | (static_cast<uint16_t>(frame_bytes[10]) << 16)
        | (static_cast<uint16_t>(frame_bytes[9]) << 8)
        | (static_cast<uint16_t>(frame_bytes[8]));

    const auto &data_begin = frame_bytes.begin() + YamuxFrame::kHeaderLength;
    if (data_begin != frame_bytes.end()) {
      frame.data_ = kagome::common::Buffer{
          std::vector<uint8_t>(data_begin, frame_bytes.end())};
    }

    return frame;
  }
}  // namespace libp2p::muxer
