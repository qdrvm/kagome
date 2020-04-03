/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "blockchain/impl/changes_trie_builder_impl.hpp"
#include "storage/trie/impl/in_memory_trie_db_factory.hpp"
#include "storage/trie_db_overlay/impl/trie_db_overlay_impl.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using kagome::blockchain::ChangesTrieBuilderImpl;
using kagome::storage::trie::InMemoryTrieDbFactory;
using kagome::storage::trie_db_overlay::TrieDbOverlayImpl;

TEST(ChangesTrieTest, IntegrationWithOverlay) {
  auto factory = std::make_shared<InMemoryTrieDbFactory>();
  TrieDbOverlayImpl overlay(factory->makeTrieDb());
  EXPECT_OUTCOME_TRUE_1(overlay.put("abc"_buf, "123"_buf));
  EXPECT_OUTCOME_TRUE_1(overlay.put("cde"_buf, "345"_buf));

  auto changes_trie_builder = ChangesTrieBuilderImpl({}, {}, factory);
  EXPECT_OUTCOME_TRUE_1(overlay.sinkChangesTo(changes_trie_builder));
  auto hash = changes_trie_builder.finishAndGetHash();
  ASSERT_EQ(
      hash,
      "3dcfd23a93747fbf8fbe0e01cebe349122681f2dd4d2c381f81e6c7374e2d4e1"_unhex);
}
