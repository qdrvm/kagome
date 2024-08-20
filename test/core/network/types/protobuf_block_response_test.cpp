/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */
#include "network/adapters/protobuf_block_response.hpp"

#include <gmock/gmock.h>
#include <qtils/test/outcome.hpp>

using kagome::network::BlocksResponse;
using kagome::network::ProtobufMessageAdapter;

using kagome::primitives::BlockData;
using kagome::primitives::BlockHash;
using kagome::primitives::BlockHeader;
using kagome::primitives::Extrinsic;

using kagome::common::Buffer;

struct ProtobufBlockResponseAdapterTest : public ::testing::Test {
  using AdapterType = ProtobufMessageAdapter<BlocksResponse>;

  void SetUp() {
    auto hash =
        EXPECT_OK(BlockHash::fromHex("11111403ba5b6a3f3bd0b0604ce439a78244"
                                     "c7d43b127ec35cd8325602dd47fd"));

    auto parent_hash =
        EXPECT_OK(BlockHash::fromHex("22111403ba5b6a3f3bd0b0604ce439a78244"
                                     "c7d43b127ec35cd8325602dd47fd"));

    auto root_hash =
        EXPECT_OK(BlockHash::fromHex("23648236745b6a3f3bd0b0604ce439a78244"
                                     "c7d43b127ec35cd8325602dd47fd"));

    auto ext_hash =
        EXPECT_OK(BlockHash::fromHex("2364823674278726578628756faad1a78244"
                                     "c7d43b127ec35cd8325602dd47fd"));

    auto ext = EXPECT_OK(Buffer::fromHex("11223344"));
    auto receipt = EXPECT_OK(Buffer::fromHex("55ffddeeaa"));
    auto message_queue = EXPECT_OK(Buffer::fromHex("1a2b3c4d5e6f"));

    response.blocks.emplace_back(BlockData{.hash = hash,
                                           .header =
                                               BlockHeader{
                                                   0,            // number
                                                   parent_hash,  // parent
                                                   root_hash,    // state root
                                                   ext_hash,  // extrinsice root
                                                   {}         // digest
                                               },
                                           .body = std::vector{Extrinsic{ext}},
                                           .receipt = receipt,
                                           .message_queue = message_queue});
  }

  BlocksResponse response;
};

/**
 * @given sample `BlocksResponse` instance
 * @when protobuf serialized into buffer
 * @then deserialization `BlocksResponse` from this buffer will contain exactly
 * the same fields with the same values
 */
TEST_F(ProtobufBlockResponseAdapterTest, Serialization) {
  std::vector<uint8_t> data;
  data.resize(AdapterType::size(response));

  AdapterType::write(response, data, data.end());
  BlocksResponse r2;
  auto it_read = EXPECT_OK(AdapterType::read(r2, data, data.begin()));

  ASSERT_EQ(it_read, data.end());
  for (size_t ix = 0; ix < response.blocks.size(); ++ix) {
    ASSERT_EQ(response.blocks[ix].hash, r2.blocks[ix].hash);
    ASSERT_EQ(response.blocks[ix].header, r2.blocks[ix].header);
    ASSERT_EQ(response.blocks[ix].body, r2.blocks[ix].body);
    ASSERT_EQ(response.blocks[ix].receipt, r2.blocks[ix].receipt);
    ASSERT_EQ(response.blocks[ix].message_queue, r2.blocks[ix].message_queue);
  }
}
