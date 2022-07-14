/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "network/adapters/uvar.hpp"

#include <gmock/gmock.h>

#include "testutil/outcome.hpp"

using kagome::network::UVarMessageAdapter;

struct UVarAdapterTest : public ::testing::Test {
  struct Dummy {
  } d;
};

/**
 * @given zero data buffer
 * @when we add uvar field
 * @then the only one byte with 0-value must be written
 */
TEST_F(UVarAdapterTest, ZeroDataTest) {
  std::vector<uint8_t> data;

  data.resize(UVarMessageAdapter<Dummy>::size(d));
  auto it = UVarMessageAdapter<Dummy>::write(d, data, data.end());

  ASSERT_EQ(data.size() - std::distance(data.begin(), it), 1);
  ASSERT_EQ(*it, 0);

  EXPECT_OUTCOME_TRUE(it_read, UVarMessageAdapter<Dummy>::read(d, data, it));
  ASSERT_EQ(it_read, data.end());
}

/**
 * @given data buffer with 7f data size
 * @when we add uvar field
 * @then the only one byte with 7f-value must be written
 */
TEST_F(UVarAdapterTest, DataSize_7f) {
  std::vector<uint8_t> data;
  data.resize(0x7f + UVarMessageAdapter<Dummy>::size(d));

  auto from = data.begin() + UVarMessageAdapter<Dummy>::size(d);
  auto it = UVarMessageAdapter<Dummy>::write(d, data, from);

  ASSERT_EQ(data.size() - std::distance(data.begin(), it), 0x80);
  ASSERT_EQ(*it, 0x7f);

  EXPECT_OUTCOME_TRUE(it_read, UVarMessageAdapter<Dummy>::read(d, data, it));
  ASSERT_EQ(it_read.base() - data.begin().base(),
            (uint64_t)UVarMessageAdapter<Dummy>::size(Dummy{}));
}

/**
 * @given 1 byte data buffer
 * @when we add uvar field
 * @then the only one byte with 1-value must be written and total data becomes 2
 */
TEST_F(UVarAdapterTest, DataSize_1) {
  std::vector<uint8_t> data;
  data.resize(0x1 + UVarMessageAdapter<Dummy>::size(d));
  data.back() = 99;

  auto from = data.begin() + UVarMessageAdapter<Dummy>::size(d);
  auto it = UVarMessageAdapter<Dummy>::write(d, data, from);

  ASSERT_EQ(data.size() - std::distance(data.begin(), it), 2);
  ASSERT_EQ(*it, 0x1);

  EXPECT_OUTCOME_TRUE(it_read, UVarMessageAdapter<Dummy>::read(d, data, it));
  ASSERT_EQ(it_read.base() - data.begin().base(),
            (uint64_t)UVarMessageAdapter<Dummy>::size(Dummy{}));
}

/**
 * @given fd-size data buffer
 * @when we add uvar field
 * @then the only two bytes with 0x1 and 0xfd values must be written, the result
 * buffer must be 0xff size
 */
TEST_F(UVarAdapterTest, DataSize_fd) {
  std::vector<uint8_t> data;
  data.resize(0xfd + UVarMessageAdapter<Dummy>::size(d));

  auto from = data.begin() + UVarMessageAdapter<Dummy>::size(d);
  auto it = UVarMessageAdapter<Dummy>::write(d, data, from);

  ASSERT_EQ(data.size() - std::distance(data.begin(), it), 0xff);
  ASSERT_EQ(*it, 0xfd);
  ASSERT_EQ(*(it + 1), 0x1);

  EXPECT_OUTCOME_TRUE(it_read, UVarMessageAdapter<Dummy>::read(d, data, it));
  ASSERT_EQ(it_read.base() - data.begin().base(),
            (uint64_t)UVarMessageAdapter<Dummy>::size(Dummy{}));
}
