/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */
#include "network/adapters/protobuf_state_request.hpp"

#include <gmock/gmock.h>

#include <qtils/test/outcome.hpp>

using kagome::network::ProtobufMessageAdapter;
using kagome::network::StateRequest;

using kagome::common::Buffer;
using kagome::primitives::BlockHash;

struct ProtobufStateRequestAdapterTest : public ::testing::Test {
  using AdapterType = ProtobufMessageAdapter<StateRequest>;

  void SetUp() {
    ASSERT_OUTCOME_SUCCESS(
        hash_from,
        BlockHash::fromHex("11111403ba5b6a3f3bd0b0604ce439a78244"
                           "c7d43b127ec35cd8325602dd47fd"));
    request.hash = hash_from;
    request.start = {Buffer::fromString("bua"), Buffer::fromString("b")};
    request.no_proof = true;
  }

  StateRequest request;
};

/**
 * @given sample `StateRequest` instance
 * @when protobuf serialized into buffer
 * @then deserialization `StateRequest` from this buffer will contain exactly
 * the same fields with the same values
 */
TEST_F(ProtobufStateRequestAdapterTest, Serialization) {
  std::vector<uint8_t> data;
  data.resize(AdapterType::size(request));

  AdapterType::write(request, data, data.end());
  StateRequest r2;
  ASSERT_OUTCOME_SUCCESS(it_read, AdapterType::read(r2, data, data.begin()));

  ASSERT_EQ(it_read, data.end());
  ASSERT_EQ(r2.hash, request.hash);
  ASSERT_EQ(r2.start, request.start);
  ASSERT_EQ(r2.no_proof, request.no_proof);
}
