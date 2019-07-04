/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/basic/message_read_writer.hpp"

#include <gtest/gtest.h>
#include "libp2p/multi/uvarint.hpp"
#include "mock/libp2p/basic/protobuf_message_mock.hpp"
#include "mock/libp2p/connection/raw_connection_mock.hpp"
#include "testutil/gmock_actions.hpp"

using namespace libp2p;
using namespace basic;
using namespace connection;
using namespace multi;

using kagome::common::Buffer;
using testing::_;
using testing::Return;

class MessageReadWriterTest : public testing::Test {
 public:
  std::shared_ptr<RawConnectionMock> conn_mock_ =
      std::make_shared<RawConnectionMock>();

  std::shared_ptr<MessageReadWriter<ProtobufMessageMock>> msg_rw_ =
      std::make_shared<MessageReadWriter<ProtobufMessageMock>>();

  static constexpr uint64_t kMsgLength = 4;
  UVarint len_varint_ = UVarint{kMsgLength};

  Buffer msg_bytes_{0x11, 0x22, 0x33, 0x44};

  Buffer msg_with_varint_bytes_ =
      Buffer{}.put(len_varint_.toBytes()).put(msg_bytes_);

  bool operation_completed_ = false;
};

ACTION_P(ReadPut, buf) {
  ASSERT_GE(arg0.size(), buf.size());
  for (auto i = 0u; i < buf.size(); ++i) {
    arg0[i] = buf[i];
  }
  arg2(buf.size());
}

ACTION_P(CheckParse, buf) {
  for (auto i = 0u; i < buf.size(); ++i) {
    EXPECT_EQ(static_cast<const uint8_t *>(arg0)[i], buf[i]);
  }
  return true;
}

TEST_F(MessageReadWriterTest, Read) {
  EXPECT_CALL(*conn_mock_, read(_, 1, _))
      .WillOnce(ReadPut(len_varint_.toBytes()));
  EXPECT_CALL(*conn_mock_, read(_, kMsgLength, _))
      .WillOnce(ReadPut(msg_bytes_));

  ProtobufMessageMock msg;
  EXPECT_CALL(msg, ParseFromArray(_, kMsgLength))
      .WillOnce(CheckParse(msg_bytes_));

  msg_rw_->read(conn_mock_, msg, [this](auto &&res) {
    ASSERT_TRUE(res);
    operation_completed_ = true;
  });

  ASSERT_TRUE(operation_completed_);
}

ACTION_P2(CheckWrite, buf, varint) {
  ASSERT_EQ(arg0.size(), buf.size());

  // it's hard to check, that message bytes were copied, but at least check
  // varint is at the beginning
  for (auto i = 0u; i < varint.size(); ++i) {
    ASSERT_EQ(arg0[i], varint.toBytes()[i]);
  }
  arg2(buf.size());
}

TEST_F(MessageReadWriterTest, Write) {
  ProtobufMessageMock msg;
  EXPECT_CALL(msg, ByteSize()).Times(3).WillRepeatedly(Return(kMsgLength));
  EXPECT_CALL(msg, SerializeToArray(_, kMsgLength));

  EXPECT_CALL(*conn_mock_, write(_, kMsgLength + 1, _))
      .WillOnce(CheckWrite(msg_with_varint_bytes_, len_varint_));

  msg_rw_->write(conn_mock_, msg, [this](auto &&res) {
    ASSERT_TRUE(res);
    operation_completed_ = true;
  });

  ASSERT_TRUE(operation_completed_);
}
