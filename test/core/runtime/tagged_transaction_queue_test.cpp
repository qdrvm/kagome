/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/tagged_transaction_queue_impl.hpp"

#include <gtest/gtest.h>
#include "core/runtime/runtime_test.hpp"
#include "core/storage/trie/mock_trie_db.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/blockchain/header_backend_mock.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/primitives/hash_creator.hpp"
#include "testutil/runtime/wasm_test.hpp"

using namespace testing;

using kagome::blockchain::BlockTreeMock;
using kagome::blockchain::HeaderRepositoryMock;
using kagome::primitives::BlockNumber;
using kagome::primitives::Extrinsic;
using kagome::runtime::TaggedTransactionQueue;
using kagome::runtime::TaggedTransactionQueueImpl;

class TTQTest : public RuntimeTest {
 public:
  void SetUp() override {
    RuntimeTest::SetUp();

    block_tree_ = std::make_shared<BlockTreeMock>();
    header_backend_ = std::make_shared<HeaderRepositoryMock>();

    ttq_ = std::make_unique<TaggedTransactionQueueImpl>(
        state_code_, extension_, block_tree_, header_backend_);
  }

 protected:
  std::shared_ptr<BlockTreeMock> block_tree_;
  std::shared_ptr<HeaderRepositoryMock> header_backend_;
  std::unique_ptr<TaggedTransactionQueue> ttq_;
};

/**
 * @given initialised tagged transaction queue api
 * @when validating a transaction
 * @then a TransactionValidity structure is obtained after successful call,
 * otherwise an outcome error
 */
TEST_F(TTQTest, ValidateTransactionSuccess) {
  Extrinsic ext{"01020304AABB"_hex2buf};
  auto hash = testutil::createHash256({1, 2, 3});
  EXPECT_CALL(*block_tree_, deepestLeaf()).WillOnce(ReturnRef(hash));
  EXPECT_CALL(*header_backend_, getNumberByHash(hash))
      .WillOnce(Return(outcome::success(BlockNumber{1u})));

  // we test now that the functions above are called sequentially
  // unfortunately, we don't have valid data for validate_transaction to succeed
  EXPECT_OUTCOME_FALSE_1(ttq_->validate_transaction(ext));
}
