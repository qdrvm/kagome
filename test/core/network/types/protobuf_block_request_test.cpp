/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */
#include "network/adapters/protobuf_block_request.hpp"

#include <gmock/gmock.h>
#include <qtils/test/outcome.hpp>

using kagome::network::BlockAttribute;
using kagome::network::BlocksRequest;
using kagome::network::Direction;
using kagome::network::ProtobufMessageAdapter;
using kagome::network::toBlockAttribute;

using kagome::primitives::BlockHash;

struct ProtobufBlockRequestAdapterTest : public ::testing::Test {
  using AdapterType = ProtobufMessageAdapter<BlocksRequest>;

  void SetUp() {
    request.max = 10;
    request.direction = Direction::DESCENDING;
    request.fields = toBlockAttribute(0x19);

    auto hash_from =
        EXPECT_OK(BlockHash::fromHex("11111403ba5b6a3f3bd0b0604ce439a78244"
                                     "c7d43b127ec35cd8325602dd47fd"));
    request.from = hash_from;
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
  auto it_read = EXPECT_OK(AdapterType::read(r2, data, data.begin()));

  ASSERT_EQ(it_read, data.end());
  ASSERT_EQ(r2.max, request.max);
  ASSERT_EQ(r2.fields, request.fields);
  ASSERT_EQ(r2.from, request.from);
}
