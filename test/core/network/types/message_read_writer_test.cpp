/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "network/helpers/message_read_writer.hpp"

#include <gmock/gmock.h>

#include "mock/core/network/adapter_mock.hpp"
#include "testutil/outcome.hpp"

using AdapterType_0 = kagome::network::AdapterMock;
using AdapterMockPtrType = std::shared_ptr<AdapterType_0>;

struct AdapterWrapper {
  static AdapterMockPtrType obj;

  template <typename T>
  static size_t size(const T &t) {
    return obj->m_size(t);
  }

  template <typename T>
  static std::vector<uint8_t>::iterator write(
      const T &t,
      std::vector<uint8_t> &out,
      std::vector<uint8_t>::iterator loaded) {
    return obj->m_write(t, out, loaded);
  }

  template <typename T>
  static libp2p::outcome::result<std::vector<uint8_t>::const_iterator> read(
      T &out,
      const std::vector<uint8_t> &src,
      std::vector<uint8_t>::const_iterator from) {
    return obj->m_read(out, src, from);
  }
};

AdapterMockPtrType AdapterWrapper::obj;
using ::testing::Return;

struct MessageReadWriterTestTest : public ::testing::Test {
  kagome::network::Dummy d;
  using AdapterType = AdapterWrapper;

  void SetUp() {
    AdapterWrapper::obj = ptr;
  }

  void TearDown() {
    AdapterWrapper::obj.reset();
  }

  std::shared_ptr<kagome::network::AdapterMock> ptr =
      std::make_shared<kagome::network::AdapterMock>();
};

TEST_F(MessageReadWriterTestTest, CallOrder) {
  using namespace kagome::network;
  using Last = MessageReadWriter<AdapterType, NoSink>;

  std::vector<uint8_t> data;
  data.resize(10);

  EXPECT_CALL(*ptr, m_size(d)).WillRepeatedly(Return(5));
  EXPECT_CALL(*ptr, m_write(d, data, data.end()))
      .WillOnce(Return(data.begin()));
  ASSERT_EQ(Last::write(d, data), data.begin());
}

TEST_F(MessageReadWriterTestTest, CallOrder_2) {
  using namespace kagome::network;
  using Last = MessageReadWriter<AdapterType, NoSink>;
  using First = MessageReadWriter<AdapterType, Last>;

  std::vector<uint8_t> data;
  data.resize(10);

  EXPECT_CALL(*ptr, m_size(d)).WillRepeatedly(Return(5));
  EXPECT_CALL(*ptr, m_write(d, data, data.end()))
      .WillOnce(Return(data.end() - 5));
  EXPECT_CALL(*ptr, m_write(d, data, data.end() - 5))
      .WillOnce(Return(data.begin()));
  ASSERT_EQ(First::write(d, data), data.begin());
}
