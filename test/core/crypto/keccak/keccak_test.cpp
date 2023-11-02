/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "common/blob.hpp"
#include "crypto/keccak/keccak.h"

TEST(Keccak, Correctness) {
  auto keccak_res =
      "4e7e8301f5d206748d1c4f822e3564ddb1124f86591a839f58dfc2f007983b61";

  std::string str = "some";
  kagome::common::Hash256 out;
  EXPECT_EQ(sha3_HashBuffer(256,
                            SHA3_FLAGS ::SHA3_FLAGS_KECCAK,
                            str.c_str(),
                            str.size(),
                            out.data(),
                            32),
            0)
      << "having problems while hashing";
  EXPECT_EQ(kagome::common::hex_lower(out), keccak_res)
      << "hashes are different";
}
