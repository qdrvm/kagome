/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "network/adapters/protobuf_block_request.hpp"

#include <gmock/gmock.h>

#include "testutil/outcome.hpp"

using kagome::network::BlockAttribute;
using kagome::network::BlockAttributes;
using kagome::network::BlocksRequest;
using kagome::network::Direction;
using kagome::network::ProtobufMessageAdapter;

using kagome::primitives::BlockHash;

struct ProtobufBlockRequestAdapterTest : public ::testing::Test {
  using AdapterType = ProtobufMessageAdapter<BlocksRequest>;

  void SetUp() {
    request.max = 10;
    request.direction = Direction::DESCENDING;
    request.fields.load(0x19);

    EXPECT_OUTCOME_TRUE(
        hash_from,
        BlockHash::fromHex("11111403ba5b6a3f3bd0b0604ce439a78244"
                           "c7d43b127ec35cd8325602dd47fd"));
    request.from = hash_from;

    EXPECT_OUTCOME_TRUE(
        hash_to,
        BlockHash::fromHex("22221403ba5b6a3f3bd0b0604ce439a78244"
                           "c7d43b127ec35cd8325602dd47fd"));
    request.to = hash_to;
  }

  BlocksRequest request;
};

/**
 * @given sample `BlocksRequest` instance
 * @when protobuf serialized into buffer
 * @then deserialization `BlocksRequest` from this buffer will contain exactly
 * the same fields with the same values
 */
TEST_F(ProtobufBlockRequestAdapterTest, Serialization) {
  std::vector<uint8_t> data;
  data.resize(AdapterType::size(request));

  AdapterType::write(request, data, data.end());
  BlocksRequest r2;
  EXPECT_OUTCOME_TRUE(it_read, AdapterType::read(r2, data, data.begin()));

  ASSERT_EQ(it_read, data.end());
  ASSERT_EQ(r2.max, request.max);
  ASSERT_EQ(r2.fields, request.fields);
  ASSERT_EQ(r2.from, request.from);
  ASSERT_EQ(r2.to, request.to);
}
