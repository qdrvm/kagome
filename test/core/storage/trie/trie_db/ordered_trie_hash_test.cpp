/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "storage/trie/impl/ordered_trie_hash.hpp"

#include <gtest/gtest.h>
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

/**
 * @given a set of values, which ordered trie hash we want to calculate
 * @when calling a function that does it
 * @then the function doesn't yield an error
 */
TEST(OrderedTrieHash, DoesntFail) {
  std::vector vals{"aarakocra"_buf, "byzantine"_buf, "crest"_buf};
  EXPECT_OUTCOME_TRUE_1(
      kagome::storage::trie::calculateOrderedTrieHash(vals.begin(), vals.end()))
}

TEST(OrderedTrieHash, EmptyVector) {
  std::vector<kagome::common::Buffer> vals;
  EXPECT_OUTCOME_TRUE(val,
                      kagome::storage::trie::calculateOrderedTrieHash(
                          vals.begin(), vals.end()));
  ASSERT_EQ(kagome::common::hex_lower(val),
            "03170a2e7597b7b7e3d84c05391d139a62b157e78786d8c082f29dcf4c111314");
}

TEST(OrderedTrieHash, OneValueVector) {
  std::vector vals{"budgetary management"_buf};
  EXPECT_OUTCOME_TRUE(val,
                      kagome::storage::trie::calculateOrderedTrieHash(
                          vals.begin(), vals.end()));
  ASSERT_EQ(kagome::common::hex_lower(val),
            "c66a6345c58b3ec0ce9c0a1497553e4078f3d990063ac3e3058db06db358148a");
}

TEST(OrderedTrieHash, TwoValueVector) {
  std::vector vals{"Integrated"_buf, "portal"_buf};
  EXPECT_OUTCOME_TRUE(val,
                      kagome::storage::trie::calculateOrderedTrieHash(
                          vals.begin(), vals.end()));
  ASSERT_EQ(kagome::common::hex_lower(val),
            "ea64d09f9740275ef7faaa3cee5a6a45fc8fe655cf049addbcefa7ba2ba6032d");
}

TEST(OrderedTrieHash, TwoValueVectorErr1) {
  std::vector vals{"budgetary management"_buf, "pricing structure"_buf};
  EXPECT_OUTCOME_TRUE(val,
                      kagome::storage::trie::calculateOrderedTrieHash(
                          vals.begin(), vals.end()));
  ASSERT_EQ(kagome::common::hex_lower(val),
            "a340fba4541947a516c3ae686cf0f3155b1d69f9146e4096c54bc8b45db718f1");
}

TEST(OrderedTrieHash, TwoValueVectorErr2) {
  std::vector vals{"even-keeled"_buf, "Future-proofed"_buf};
  EXPECT_OUTCOME_TRUE(val,
                      kagome::storage::trie::calculateOrderedTrieHash(
                          vals.begin(), vals.end()));
  ASSERT_EQ(kagome::common::hex_lower(val),
            "5147323d593b7bb01fe8ea3e9d5a4bba0497c7f47b5daa121f4a6d791164d60b");
}
