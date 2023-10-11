/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "runtime/common/trie_storage_provider_impl.hpp"

#include "common/buffer.hpp"
#include "mock/core/storage/trie_pruner/trie_pruner_mock.hpp"
#include "runtime/common/runtime_execution_error.hpp"
#include "storage/in_memory/in_memory_storage.hpp"
#include "storage/trie/impl/trie_storage_backend_impl.hpp"
#include "storage/trie/impl/trie_storage_impl.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_factory_impl.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"
#include "storage/trie/serialization/trie_serializer_impl.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"

using kagome::common::Buffer;
using kagome::runtime::RuntimeExecutionError;

class TrieStorageProviderTest : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    auto trie_factory =
        std::make_shared<kagome::storage::trie::PolkadotTrieFactoryImpl>();

    auto codec = std::make_shared<kagome::storage::trie::PolkadotCodec>();

    storage_ = std::make_shared<kagome::storage::InMemoryStorage>();

    auto backend =
        std::make_shared<kagome::storage::trie::TrieStorageBackendImpl>(
            storage_);

    auto serializer =
        std::make_shared<kagome::storage::trie::TrieSerializerImpl>(
            trie_factory, codec, backend);

    auto state_pruner =
        std::make_shared<kagome::storage::trie_pruner::TriePrunerMock>();

    auto trieDb = kagome::storage::trie::TrieStorageImpl::createEmpty(
                      trie_factory, codec, serializer, state_pruner)
                      .value();

    storage_provider_ =
        std::make_shared<kagome::runtime::TrieStorageProviderImpl>(
            std::move(trieDb), serializer);

    ASSERT_OUTCOME_SUCCESS_TRY(storage_provider_->setToPersistentAt(
        serializer->getEmptyRootHash(), std::nullopt));
  }

 protected:
  std::shared_ptr<kagome::storage::BufferStorage> storage_;
  std::shared_ptr<kagome::runtime::TrieStorageProvider> storage_provider_;
};

TEST_F(TrieStorageProviderTest, StartTransaction) {
  ASSERT_OUTCOME_SUCCESS_TRY(storage_provider_->startTransaction());
}

TEST_F(TrieStorageProviderTest, FinishTransactionWithoutStart) {
  ASSERT_OUTCOME_ERROR(storage_provider_->rollbackTransaction(),
                       RuntimeExecutionError::NO_TRANSACTIONS_WERE_STARTED);

  ASSERT_OUTCOME_ERROR(storage_provider_->commitTransaction(),
                       RuntimeExecutionError::NO_TRANSACTIONS_WERE_STARTED);
}

// Concatenate values gotten by keys: A, B, C, D, E
// and compare it with template string
auto checkBatchValues(const kagome::storage::trie::TrieBatch &batch,
                      std::string_view expected_view) {
  std::string actual_view;
  for (std::string_view key : {"A", "B", "C", "D", "E"}) {
    ASSERT_OUTCOME_SUCCESS(
        val, batch.get(Buffer(std::vector<uint8_t>(key.begin(), key.end()))));
    actual_view += val.mut().asString();
  }
  EXPECT_EQ(actual_view, expected_view);
}

TEST_F(TrieStorageProviderTest, NestedTransactions) {
  /// @given batch with cells A, B, C, D, E with value '-' (means is unchanged)
  auto batch0 = storage_provider_->getCurrentBatch();
  ASSERT_OUTCOME_SUCCESS_TRY(batch0->put("A"_buf, "-"_buf));
  ASSERT_OUTCOME_SUCCESS_TRY(batch0->put("B"_buf, "-"_buf));
  ASSERT_OUTCOME_SUCCESS_TRY(batch0->put("C"_buf, "-"_buf));
  ASSERT_OUTCOME_SUCCESS_TRY(batch0->put("D"_buf, "-"_buf));
  ASSERT_OUTCOME_SUCCESS_TRY(batch0->put("E"_buf, "-"_buf));
  checkBatchValues(*batch0, "-----");

  /// @when 1. start tx 1
  {  // Transaction 1 - will be committed
    ASSERT_OUTCOME_SUCCESS_TRY(storage_provider_->startTransaction());
    auto batch1 = storage_provider_->getCurrentBatch();

    /// @that 1. top level state is not changed, tx1 state like top level state
    checkBatchValues(*batch0, "-----");
    checkBatchValues(*batch1, "-----");

    /// @when 2. change one of values
    ASSERT_OUTCOME_SUCCESS_TRY(batch1->put("A"_buf, "1"_buf));

    /// @that 2. top level state is not changed, tx1 state is changed
    checkBatchValues(*batch0, "-----");
    checkBatchValues(*batch1, "1----");

    {
      /// @when 3. start tx 2
      ASSERT_OUTCOME_SUCCESS_TRY(storage_provider_->startTransaction());
      auto batch2 = storage_provider_->getCurrentBatch();

      /// @that 3. top level and tx1 state are not changed, tx2 state like tx1
      /// state
      checkBatchValues(*batch0, "-----");
      checkBatchValues(*batch1, "1----");
      checkBatchValues(*batch2, "1----");

      /// @when 4. change next value
      ASSERT_OUTCOME_SUCCESS_TRY(batch2->put("B"_buf, "2"_buf));

      /// @that 4. top level and tx1 state are not changed, tx2 state is changed
      checkBatchValues(*batch0, "-----");
      checkBatchValues(*batch1, "1----");
      checkBatchValues(*batch2, "12---");

      {
        /// @when 5. start tx 3
        ASSERT_OUTCOME_SUCCESS_TRY(storage_provider_->startTransaction());
        auto batch3 = storage_provider_->getCurrentBatch();

        /// @that 5. top level, tx1, tx2 state are not changed, tx3 state like
        /// tx2 state
        checkBatchValues(*batch0, "-----");
        checkBatchValues(*batch1, "1----");
        checkBatchValues(*batch2, "12---");
        checkBatchValues(*batch3, "12---");

        /// @when 6. change next value
        ASSERT_OUTCOME_SUCCESS_TRY(batch3->put("C"_buf, "3"_buf));

        /// @that 6. top level, tx1 and tx2 state are not changed, tx3 state is
        /// changed
        checkBatchValues(*batch0, "-----");
        checkBatchValues(*batch1, "1----");
        checkBatchValues(*batch2, "12---");
        checkBatchValues(*batch3, "123--");

        /// @when 7. commit tx3
        ASSERT_OUTCOME_SUCCESS_TRY(storage_provider_->commitTransaction());

        /// @that 7. top level and tx1 state are not changed, tx2 state became
        /// like tx3
        checkBatchValues(*batch0, "-----");
        checkBatchValues(*batch1, "1----");
        checkBatchValues(*batch2, "123--");
        checkBatchValues(*batch3, "123--");
      }

      /// @when 8. change next value
      ASSERT_OUTCOME_SUCCESS_TRY(batch2->put("D"_buf, "2"_buf));

      /// @that 8. top level and tx1 state are not changed, tx2 state is changed
      checkBatchValues(*batch0, "-----");
      checkBatchValues(*batch1, "1----");
      checkBatchValues(*batch2, "1232-");

      /// @when 9. rollback tx2
      ASSERT_OUTCOME_SUCCESS_TRY(storage_provider_->rollbackTransaction());

      /// @that 9. top level and tx1 state are not changed, tx2 state does not
      /// matter anymore
      checkBatchValues(*batch0, "-----");
      checkBatchValues(*batch1, "1----");
    }

    /// @when 10. change next value
    ASSERT_OUTCOME_SUCCESS_TRY(batch1->put("E"_buf, "1"_buf));

    /// @that 10. top level is not changed, tx1 state is changed
    checkBatchValues(*batch0, "-----");
    checkBatchValues(*batch1, "1---1");

    /// @when 11. commit tx3
    ASSERT_OUTCOME_SUCCESS_TRY(storage_provider_->commitTransaction());

    /// @that 11. top level became like tx1 state
    checkBatchValues(*batch0, "1---1");
    checkBatchValues(*batch1, "1---1");
  }
}

TEST_F(TrieStorageProviderTest, ChildTreeTransactions) {
  ASSERT_OUTCOME_SUCCESS(
      base_batch_1,
      storage_provider_->getMutableChildBatchAt("child_root_1"_buf));
  ASSERT_OUTCOME_SUCCESS_TRY(base_batch_1.get().put("A"_buf, "1"_buf));
  ASSERT_OUTCOME_SUCCESS_TRY(base_batch_1.get().put("B"_buf, "2"_buf));
  ASSERT_OUTCOME_SUCCESS_TRY(base_batch_1.get().put("C"_buf, "3"_buf));
  ASSERT_OUTCOME_SUCCESS_TRY(base_batch_1.get().put("D"_buf, "-"_buf));
  ASSERT_OUTCOME_SUCCESS_TRY(base_batch_1.get().put("E"_buf, "-"_buf));
  checkBatchValues(base_batch_1, "123--");

  ASSERT_OUTCOME_SUCCESS(
      base_batch_2,
      storage_provider_->getMutableChildBatchAt("child_root_2"_buf));
  ASSERT_OUTCOME_SUCCESS_TRY(base_batch_2.get().put("A"_buf, "4"_buf));
  ASSERT_OUTCOME_SUCCESS_TRY(base_batch_2.get().put("B"_buf, "5"_buf));
  ASSERT_OUTCOME_SUCCESS_TRY(base_batch_2.get().put("C"_buf, "6"_buf));
  ASSERT_OUTCOME_SUCCESS_TRY(base_batch_2.get().put("D"_buf, "-"_buf));
  ASSERT_OUTCOME_SUCCESS_TRY(base_batch_2.get().put("E"_buf, "-"_buf));
  checkBatchValues(base_batch_2, "456--");

  // First transaction
  ASSERT_OUTCOME_SUCCESS_TRY(storage_provider_->startTransaction());
  ASSERT_OUTCOME_SUCCESS(
      tr1_batch_1,
      storage_provider_->getMutableChildBatchAt("child_root_1"_buf));
  ASSERT_OUTCOME_SUCCESS(
      tr1_batch_2,
      storage_provider_->getMutableChildBatchAt("child_root_2"_buf));
  checkBatchValues(tr1_batch_1, "123--");
  checkBatchValues(tr1_batch_2, "456--");

  ASSERT_OUTCOME_SUCCESS_TRY(tr1_batch_1.get().put("A"_buf, "a"_buf));
  ASSERT_OUTCOME_SUCCESS_TRY(tr1_batch_2.get().put("A"_buf, "d"_buf));
  checkBatchValues(tr1_batch_1, "a23--");
  checkBatchValues(tr1_batch_2, "d56--");
  checkBatchValues(base_batch_1, "123--");
  checkBatchValues(base_batch_2, "456--");

  // Nested transaction
  ASSERT_OUTCOME_SUCCESS_TRY(storage_provider_->startTransaction());
  ASSERT_OUTCOME_SUCCESS(
      tr2_batch_1,
      storage_provider_->getMutableChildBatchAt("child_root_1"_buf));
  ASSERT_OUTCOME_SUCCESS(
      tr2_batch_2,
      storage_provider_->getMutableChildBatchAt("child_root_2"_buf));
  checkBatchValues(tr2_batch_1, "a23--");
  checkBatchValues(tr2_batch_2, "d56--");

  ASSERT_OUTCOME_SUCCESS_TRY(tr2_batch_1.get().put("A"_buf, "0"_buf));
  ASSERT_OUTCOME_SUCCESS_TRY(tr2_batch_2.get().put("A"_buf, "1"_buf));
  checkBatchValues(tr2_batch_1, "023--");
  checkBatchValues(tr2_batch_2, "156--");
  checkBatchValues(tr1_batch_1, "a23--");
  checkBatchValues(tr1_batch_2, "d56--");
  checkBatchValues(base_batch_1, "123--");
  checkBatchValues(base_batch_2, "456--");

  ASSERT_OUTCOME_SUCCESS_TRY(storage_provider_->commitTransaction());
  checkBatchValues(tr1_batch_1, "023--");
  checkBatchValues(tr1_batch_2, "156--");
  checkBatchValues(base_batch_1, "123--");
  checkBatchValues(base_batch_2, "456--");

  ASSERT_OUTCOME_SUCCESS_TRY(storage_provider_->commitTransaction());
  checkBatchValues(base_batch_1, "023--");
  checkBatchValues(base_batch_2, "156--");

  // Second transaction
  ASSERT_OUTCOME_SUCCESS_TRY(storage_provider_->startTransaction());
  ASSERT_OUTCOME_SUCCESS(
      tr3_batch_1,
      storage_provider_->getMutableChildBatchAt("child_root_1"_buf));
  ASSERT_OUTCOME_SUCCESS(
      tr3_batch_2,
      storage_provider_->getMutableChildBatchAt("child_root_2"_buf));
  checkBatchValues(tr3_batch_1, "023--");
  checkBatchValues(tr3_batch_2, "156--");

  ASSERT_OUTCOME_SUCCESS_TRY(tr3_batch_1.get().put("A"_buf, "a"_buf));
  ASSERT_OUTCOME_SUCCESS_TRY(tr3_batch_2.get().put("A"_buf, "d"_buf));
  checkBatchValues(tr3_batch_1, "a23--");
  checkBatchValues(tr3_batch_2, "d56--");
  checkBatchValues(base_batch_1, "023--");
  checkBatchValues(base_batch_2, "156--");

  ASSERT_OUTCOME_SUCCESS_TRY(storage_provider_->rollbackTransaction());
  checkBatchValues(base_batch_1, "023--");
  checkBatchValues(base_batch_2, "156--");
}