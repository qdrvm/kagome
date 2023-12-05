/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */
#include "network/adapters/protobuf_state_response.hpp"

#include <gmock/gmock.h>

#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using kagome::common::Buffer;
using kagome::common::Hash256;
using kagome::network::ProtobufMessageAdapter;
using kagome::network::StateResponse;

struct ProtobufStateResponseAdapterTest : public ::testing::Test {
  using AdapterType = ProtobufMessageAdapter<StateResponse>;

  void SetUp() override {
    response = StateResponse{
        .entries = {{.state_root = "123456"_hash256,
                     .entries = {{.key = "a"_buf, .value = "b"_buf},
                                 {.key = "c"_buf, .value = "d"_buf}},
                     .complete = true}},
        .proof = Buffer()};
  }

  StateResponse response;
};

/**
 * @given sample `StateResponse` instance
 * @when protobuf serialized into buffer
 * @then deserialization `StateResponse` from this buffer will contain exactly
 * the same fields with the same values
 */
TEST_F(ProtobufStateResponseAdapterTest, Serialization) {
  std::vector<uint8_t> data;
  data.resize(AdapterType::size(response));

  AdapterType::write(response, data, data.end());
  StateResponse r2;
  EXPECT_OUTCOME_TRUE(it_read, AdapterType::read(r2, data, data.begin()));

  ASSERT_EQ(it_read, data.end());
}
