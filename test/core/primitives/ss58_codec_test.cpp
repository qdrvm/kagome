/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "crypto/hasher/hasher_impl.hpp"
#include "primitives/ss58_codec.hpp"
#include "testutil/outcome.hpp"

using kagome::crypto::HasherImpl;

using kagome::primitives::decodeSs58;
using kagome::primitives::encodeSs58;

class Ss58Codec : public testing::Test {
  void SetUp() override {}

 protected:
  HasherImpl hasher_;
};

TEST_F(Ss58Codec, EncodeSs58) {
  EXPECT_OUTCOME_SUCCESS(
      decoded,
      decodeSs58("cnRbTBxzRs4zmzJiqWkuBLMGCkkcq3FidZMvsj7kPgCfvGSsY", hasher_));
}
