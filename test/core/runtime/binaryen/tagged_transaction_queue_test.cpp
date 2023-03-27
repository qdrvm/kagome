/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/tagged_transaction_queue.hpp"

#include <gtest/gtest.h>

#include "core/runtime/binaryen/binaryen_runtime_test.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "testutil/outcome.hpp"
#include "testutil/runtime/common/basic_code_provider.hpp"

using namespace testing;

using kagome::blockchain::BlockTree;
using kagome::blockchain::BlockTreeMock;
using kagome::primitives::BlockNumber;
using kagome::primitives::Extrinsic;
using kagome::primitives::TransactionSource;
using kagome::runtime::TaggedTransactionQueue;
using kagome::runtime::TaggedTransactionQueueImpl;

class TTQTest : public BinaryenRuntimeTest {
 public:
  void SetUp() override {
    BinaryenRuntimeTest::SetUp();

    const auto injector = boost::di::make_injector(
        boost::di::bind<BlockTree>.template to<BlockTreeMock>());

    boost::di::extension::lazy<std::shared_ptr<BlockTree>> block_tree(injector);

    ttq_ = std::make_unique<TaggedTransactionQueueImpl>(executor_, block_tree);
  }

 protected:
  std::unique_ptr<TaggedTransactionQueue> ttq_;
};

/**
 * @given initialised tagged transaction queue api
 * @when validating a transaction
 * @then a TransactionValidity structure is obtained after successful call,
 * otherwise an outcome error
 */
TEST_F(TTQTest, DISABLED_ValidateTransactionSuccess) {
  Extrinsic ext{"01020304AABB"_hex2buf};

  // we test now that the functions above are called sequentially
  // unfortunately, we don't have valid data for validate_transaction to succeed
  EXPECT_OUTCOME_TRUE_1(
      ttq_->validate_transaction(TransactionSource::External, ext));
}
