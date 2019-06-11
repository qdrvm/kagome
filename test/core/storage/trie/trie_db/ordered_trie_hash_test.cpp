/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/polkadot_trie_db/ordered_trie_hash.hpp"

#include <gtest/gtest.h>
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

TEST(OrderedTrieHash, DoesntFail) {
  std::vector vals {"aarakocra"_buf, "byzantine"_buf, "crest"_buf};
  EXPECT_OUTCOME_TRUE_1(kagome::storage::trie::calculateOrderedTrieHash(
      vals.begin(), vals.end()))
}
