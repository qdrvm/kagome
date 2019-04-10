/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/muxer/yamux/yamux_frame.hpp"

#include <gtest/gtest.h>

using namespace libp2p::muxer;
using namespace kagome::common;

class YamuxFrameTest : public ::testing::Test {
 public:
  static constexpr size_t data_length = 6;
  static constexpr Yamux::StreamId default_stream_id = 1;
  static constexpr uint32_t default_ping_value = 337;

  Buffer data{0x12, 0x34, 0x45, 0x67, 0x89, 0xAB};
  Buffer data_frame_bytes =
      Buffer{}
          .putUint8(YamuxFrame::kDefaultVersion)
          .putUint8(static_cast<uint8_t>(YamuxFrame::FrameType::kData))
          .putUint16(htons(static_cast<uint16_t>(YamuxFrame::Flag::kSyn)))
          .putUint32(htonl(default_stream_id))
          .putUint32(htonl(data_length))
          .putBuffer(data);

  /**
   * Check that all frame's fields are as expected
   */
  void checkFrame(std::optional<YamuxFrame> frame_opt, uint8_t version,
                  YamuxFrame::FrameType type, YamuxFrame::Flag flag,
                  Yamux::StreamId stream_id, uint32_t length,
                  const Buffer &frame_data) {
    ASSERT_TRUE(frame_opt);
    auto frame = *frame_opt;
    ASSERT_EQ(frame.version_, version);
    ASSERT_EQ(frame.type_, type);
    ASSERT_EQ(frame.flag_, flag);
    ASSERT_EQ(frame.stream_id_, stream_id);
    ASSERT_EQ(frame.length_, length);
    ASSERT_EQ(frame.data_, frame_data);
  }
};

TEST_F(YamuxFrameTest, ParseFrameSuccess) {
  auto frame_opt = parseFrame(data_frame_bytes.toVector());
  checkFrame(frame_opt, YamuxFrame::kDefaultVersion,
             YamuxFrame::FrameType::kData, YamuxFrame::Flag::kSyn,
             default_stream_id, data_length, data);
}

TEST_F(YamuxFrameTest, ParseFrameFailure) {
  auto frame_opt = parseFrame(data.toVector());

  ASSERT_FALSE(frame_opt);
}

TEST_F(YamuxFrameTest, NewStreamMsg) {
  auto frame_bytes = newStreamMsg(default_stream_id);
  auto frame_opt = parseFrame(frame_bytes.toVector());
  checkFrame(frame_opt, YamuxFrame::kDefaultVersion,
             YamuxFrame::FrameType::kData, YamuxFrame::Flag::kSyn,
             default_stream_id, 0, Buffer{});
}

TEST_F(YamuxFrameTest, AckStreamMsg) {
  auto frame_bytes = ackStreamMsg(default_stream_id);
  auto frame_opt = parseFrame(frame_bytes.toVector());
  checkFrame(frame_opt, YamuxFrame::kDefaultVersion,
             YamuxFrame::FrameType::kData, YamuxFrame::Flag::kAck,
             default_stream_id, 0, Buffer{});
}

TEST_F(YamuxFrameTest, CloseStreamMsg) {
  auto frame_bytes = closeStreamMsg(default_stream_id);
  auto frame_opt = parseFrame(frame_bytes.toVector());
  checkFrame(frame_opt, YamuxFrame::kDefaultVersion,
             YamuxFrame::FrameType::kData, YamuxFrame::Flag::kFin,
             default_stream_id, 0, Buffer{});
}

TEST_F(YamuxFrameTest, ResetStreamMsg) {
  auto frame_bytes = resetStreamMsg(default_stream_id);
  auto frame_opt = parseFrame(frame_bytes.toVector());
  checkFrame(frame_opt, YamuxFrame::kDefaultVersion,
             YamuxFrame::FrameType::kData, YamuxFrame::Flag::kRst,
             default_stream_id, 0, Buffer{});
}

TEST_F(YamuxFrameTest, PingOutMsg) {
  auto frame_bytes = pingOutMsg(default_ping_value);
  auto frame_opt = parseFrame(frame_bytes.toVector());
  checkFrame(frame_opt, YamuxFrame::kDefaultVersion,
             YamuxFrame::FrameType::kPing, YamuxFrame::Flag::kSyn, 0,
             default_ping_value, Buffer{});
}

TEST_F(YamuxFrameTest, PingResponseMsg) {
  auto frame_bytes = pingResponseMsg(default_ping_value);
  auto frame_opt = parseFrame(frame_bytes.toVector());
  checkFrame(frame_opt, YamuxFrame::kDefaultVersion,
             YamuxFrame::FrameType::kPing, YamuxFrame::Flag::kAck, 0,
             default_ping_value, Buffer{});
}

TEST_F(YamuxFrameTest, DataMsg) {
  auto frame_bytes = dataMsg(default_stream_id, data);
  auto frame_opt = parseFrame(frame_bytes.toVector());
  checkFrame(frame_opt, YamuxFrame::kDefaultVersion,
             YamuxFrame::FrameType::kData, YamuxFrame::Flag::kSyn,
             default_stream_id, data_length, data);
}

TEST_F(YamuxFrameTest, GoAwayMsg) {
  auto frame_bytes = goAwayMsg(YamuxFrame::GoAwayError::kProtocolError);
  auto frame_opt = parseFrame(frame_bytes.toVector());
  checkFrame(frame_opt, YamuxFrame::kDefaultVersion,
             YamuxFrame::FrameType::kGoAway, YamuxFrame::Flag::kSyn, 0,
             static_cast<uint32_t>(YamuxFrame::GoAwayError::kProtocolError),
             Buffer{});
}
