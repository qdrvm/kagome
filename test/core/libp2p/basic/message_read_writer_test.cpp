/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/basic/message_read_writer.hpp"

#include <gtest/gtest.h>
#include "mock/libp2p/connection/raw_connection_mock.hpp"

using namespace libp2p;
using namespace connection;

class MessageReadWriterTest : public testing::Test {
 public:
  std::shared_ptr<RawConnectionMock> conn_mock_;
};

TEST_F(MessageReadWriterTest, Read) {}

TEST_F(MessageReadWriterTest, Write) {}
