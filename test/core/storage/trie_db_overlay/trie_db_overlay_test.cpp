/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "mock/core/blockchain/changes_trie_builder_mock.hpp"
#include "mock/core/storage/trie/trie_db_mock.hpp"
#include "scale/scale.hpp"
#include "storage/trie_db_overlay/impl/trie_db_overlay_impl.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using kagome::blockchain::ChangesTrieBuilderMock;
using kagome::common::Buffer;
using kagome::scale::encode;
using kagome::storage::trie::TrieDb;
using kagome::storage::trie::TrieDbMock;
using kagome::storage::trie_db_overlay::TrieDbOverlay;
using kagome::storage::trie_db_overlay::TrieDbOverlayImpl;
using testing::_;
using testing::AnyOf;
using testing::Return;
using testing::ReturnRef;
using testing::Test;

class TrieDbOverlayTest : public Test {
 public:
  TrieDbOverlayTest() : overlay{trie} {}

  void SetUp() override {
    EXPECT_CALL(*trie, get(":extrinsic_index"_buf))
        .WillRepeatedly(Return(Buffer{encode(42).value()}));
  }

 protected:
  std::shared_ptr<TrieDbMock> trie = std::make_shared<TrieDbMock>();
  TrieDbOverlayImpl overlay;
};

TEST_F(TrieDbOverlayTest, CommitsToTrie) {
  EXPECT_OUTCOME_TRUE_1(overlay.put("a"_buf, "1"_buf));
  EXPECT_OUTCOME_TRUE_1(overlay.put("b"_buf, "2"_buf));
  EXPECT_OUTCOME_TRUE_1(overlay.put("c"_buf, "3"_buf));
  EXPECT_CALL(*trie, put(AnyOf("a"_buf, "b"_buf, "c"_buf), _))
      .WillRepeatedly(Return(outcome::success()));
  EXPECT_OUTCOME_TRUE_1(overlay.commit());
}

TEST_F(TrieDbOverlayTest, AsksCacheMissesFromTrie) {
  EXPECT_CALL(*trie, get("a"_buf)).WillOnce(Return("1"_buf));
  EXPECT_OUTCOME_TRUE(res, overlay.get("a"_buf));
  ASSERT_EQ(res, "1"_buf);
  // once in cache, won't ask the trie
  EXPECT_OUTCOME_TRUE_1(overlay.put("a"_buf, "2"_buf));
  EXPECT_OUTCOME_TRUE(res1, overlay.get("a"_buf));
  ASSERT_EQ(res1, "2"_buf);
}

TEST_F(TrieDbOverlayTest, TracksChanges) {
  EXPECT_OUTCOME_TRUE_1(overlay.put("a"_buf, "1"_buf));
  EXPECT_OUTCOME_TRUE_1(overlay.put("b"_buf, "2"_buf));
  EXPECT_OUTCOME_TRUE_1(overlay.put("c"_buf, "3"_buf));

  EXPECT_CALL(*trie, put(AnyOf("a"_buf, "b"_buf, "c"_buf), _))
      .WillRepeatedly(Return(outcome::success()));
  ChangesTrieBuilderMock changes_builder{};
  EXPECT_CALL(changes_builder,
              insertExtrinsicsChange(AnyOf("a"_buf, "b"_buf, "c"_buf),
                                     std::vector{42u}))
      .WillRepeatedly(Return(outcome::success()));
  EXPECT_OUTCOME_TRUE_1(overlay.sinkChangesTo(changes_builder));
}
