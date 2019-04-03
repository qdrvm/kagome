/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include "common/buffer.hpp"
#include "crypto/sha/sha256.hpp"

using namespace kagome::common;
using namespace kagome::crypto;

class Sha256Test : public ::testing::Test {
 public:
  /// NIST test vectors https://www.di-mgt.com.au/sha_testvectors.html
  const std::vector<std::pair<std::string, std::string>> test_vectors{
      {"", "E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855"},
      {"abc",
       "BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD"},
      {"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
       "248D6A61D20638B8E5C026930C3E6039A33CE45964FF2167F6ECEDD419DB06C1"},
      {"abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmno"
       "pjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu",
       "CF5B16A778AF8380036CE59E7B0492370B249B11E8F07A51AFAC45037AFEE9D1"},
      {std::string(1000000, 'a'),
       "CDC76E5C9914FB9281A1C7E284D73E67F1809A48A497200E046D39CCC7112CD0"}};
};

TEST_F(Sha256Test, Valid) {
  for (const auto &[initial, digest] : test_vectors) {
    ASSERT_EQ(sha256(initial).toHex(), digest);
  }
}
