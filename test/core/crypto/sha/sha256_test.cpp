/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
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
      {"", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"},
      {"abc",
       "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"},
      {"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
       "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"},
      {"abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmno"
       "pjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu",
       "cf5b16a778af8380036ce59e7b0492370b249b11e8f07a51afac45037afee9d1"},
      {std::string(1000000, 'a'),
       "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0"}};
};

TEST_F(Sha256Test, Valid) {
  for (const auto &[initial, digest] : test_vectors) {
    ASSERT_EQ(sha256(initial).toHex(), digest);
  }
}
