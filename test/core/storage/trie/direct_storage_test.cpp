/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/impl/direct_storage.hpp"

#include <gtest/gtest.h>
#include <qtils/test/outcome.hpp>

#include "mock/core/storage/generic_storage_mock.hpp"
#include "storage/in_memory/in_memory_storage.hpp"
#include "testutil/literals.hpp"
#include "testutil/prepare_loggers.hpp"

using namespace kagome;
using namespace storage;
using namespace storage::trie;

TEST(DirectStorageTest, DirectStorageTest) {
  testutil::prepareLoggers();
  auto direct_storage_db = std::make_shared<InMemoryStorage>();
  auto diff_db = std::make_shared<InMemoryStorage>();
  auto chain_sub_engine =
      std::make_shared<primitives::events::ChainSubscriptionEngine>();
  {
    std::shared_ptr storage =
        DirectStorage::create(direct_storage_db, diff_db, chain_sub_engine)
            .value();

    ASSERT_EQ(storage->getDirectStateRoot(), kEmptyRootHash);
    StateDiff diff1{{"key1"_buf, "val1"_buf}};
    ASSERT_OUTCOME_SUCCESS_void(storage->storeDiff(
        DirectStorage::DiffRoots{.from = kEmptyRootHash, .to = "root1"_hash256},
        std::move(diff1)));
    ASSERT_OUTCOME_SUCCESS_void(storage->updateDirectState("root1"_hash256));
    ASSERT_EQ(storage->getDirectStateRoot(), "root1"_hash256);
    ASSERT_OUTCOME_SUCCESS(view, storage->getViewAt("root1"_hash256));

    ASSERT_EQ(view->get("key1"_buf).value(), "val1"_buf);
    StateDiff diff2{{"key1"_buf, "val2"_buf}};
    ASSERT_OUTCOME_SUCCESS_void(
        storage->storeDiff(DirectStorage::DiffRoots{.from = "root1"_hash256,
                                                    .to = "root2"_hash256},
                           std::move(diff2)));
    ASSERT_OUTCOME_SUCCESS(view2, storage->getViewAt("root2"_hash256));
    ASSERT_EQ(view->get("key1"_buf).value(), "val1"_buf);

    ASSERT_EQ(view2->get("key1"_buf).value(), "val2"_buf);
  }
  {
    std::shared_ptr storage =
        DirectStorage::create(direct_storage_db, diff_db, chain_sub_engine)
            .value();
    ASSERT_OUTCOME_SUCCESS(view, storage->getViewAt("root1"_hash256));

    ASSERT_OUTCOME_SUCCESS(view2, storage->getViewAt("root2"_hash256));
    ASSERT_EQ(view->get("key1"_buf).value(), "val1"_buf);

    ASSERT_EQ(view2->get("key1"_buf).value(), "val2"_buf);
  }
}
