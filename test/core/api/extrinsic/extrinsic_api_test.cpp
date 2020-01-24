/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/extrinsic/impl/extrinsic_api_impl.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "common/blob.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/crypto/hasher_mock.hpp"
#include "mock/core/runtime/tagged_transaction_queue_mock.hpp"
#include "mock/core/transaction_pool/transaction_pool_mock.hpp"
#include "primitives/extrinsic.hpp"
#include "primitives/transaction.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/primitives/mp_utils.hpp"
#include "transaction_pool/transaction_pool_error.hpp"

using namespace kagome::api;
using namespace kagome::crypto;
using namespace kagome::transaction_pool;
using namespace kagome::runtime;

using kagome::blockchain::BlockTree;
using kagome::blockchain::BlockTreeMock;
using kagome::common::Buffer;
using kagome::common::Hash256;
using kagome::primitives::BlockId;
using kagome::primitives::Extrinsic;
using kagome::primitives::Invalid;
using kagome::primitives::Transaction;
using kagome::primitives::TransactionTag;
using kagome::primitives::TransactionValidity;
using kagome::primitives::Unknown;
using kagome::primitives::Valid;

using ::testing::_;
using ::testing::ByRef;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::ReturnRef;

using testutil::createHash256;

struct ExtrinsicSubmissionApiTest : public ::testing::Test {
  template <class T>
  using sptr = std::shared_ptr<T>;

  sptr<HasherMock> hasher;               ///< hasher mock
  sptr<TaggedTransactionQueueMock> ttq;  ///< tagged transaction queue mock
  sptr<TransactionPoolMock> transaction_pool;  ///< transaction pool mock
  sptr<BlockTreeMock> block_tree;              ///< block tree mock instance
  sptr<ExtrinsicApiImpl> api;                  ///< api instance
  sptr<Extrinsic> extrinsic;                   ///< extrinsic instance
  sptr<Valid> valid_transaction;               ///< valid transaction instance
  Hash256 deepest_hash;                        ///< hash of deepest leaf
  sptr<BlockTree::BlockInfo> deepest_leaf;     ///< deepest leaf block info

  void SetUp() override {
    hasher = std::make_shared<HasherMock>();
    ttq = std::make_shared<TaggedTransactionQueueMock>();
    transaction_pool = std::make_shared<TransactionPoolMock>();
    block_tree = std::make_shared<BlockTreeMock>();
    api = std::make_shared<ExtrinsicApiImpl>(
        ttq, transaction_pool, hasher, block_tree);
    extrinsic.reset(new Extrinsic{"12"_hex2buf});
    valid_transaction.reset(new Valid{1, {{2}}, {{3}}, 4});
    deepest_hash = createHash256({1u, 2u, 3u});
    deepest_leaf.reset(new BlockTree::BlockInfo{1u, deepest_hash});
  }
};

/**
 * @given configured extrinsic submission api object
 * @when submit_extrinsic is called
 * @then it is successfully completes returning expected result
 */
TEST_F(ExtrinsicSubmissionApiTest, SubmitExtrinsicSuccess) {
  TransactionValidity tv = *valid_transaction;
  gsl::span<const uint8_t> span = gsl::make_span(extrinsic->data);
  EXPECT_CALL(*hasher, blake2b_256(span)).WillOnce(Return(Hash256{}));
  EXPECT_CALL(*block_tree, deepestLeaf()).WillOnce(Return(*deepest_leaf));
  EXPECT_CALL(*ttq,
              validate_transaction(deepest_leaf->block_number, *extrinsic))
      .WillOnce(Return(tv));
  Transaction tr{*extrinsic,
                 extrinsic->data.size(),
                 Hash256{},
                 valid_transaction->priority,
                 valid_transaction->longevity,
                 valid_transaction->requires,
                 valid_transaction->provides,
                 true};
  EXPECT_CALL(*transaction_pool, submitOne(tr))
      .WillOnce(Return(outcome::success()));

  EXPECT_OUTCOME_TRUE(hash, api->submitExtrinsic(*extrinsic))
  ASSERT_EQ(hash, Hash256{});
}

/**
 * @given configured extrinsic submission api object
 * @when submit_extrinsic is called,
 * but in process extrinsic was recognized as `Invalid`
 * @then method returns failure and extrinsic is not sent
 * to transaction pool
 */
TEST_F(ExtrinsicSubmissionApiTest, SubmitExtrinsicInvalidFail) {
  TransactionValidity tv = Invalid{1u};
  EXPECT_CALL(*block_tree, deepestLeaf()).WillOnce(Return(*deepest_leaf));
  EXPECT_CALL(*ttq,
              validate_transaction(deepest_leaf->block_number, *extrinsic))
      .WillOnce(Return(
          outcome::failure(ExtrinsicApiError::INVALID_STATE_TRANSACTION)));
  EXPECT_CALL(*hasher, blake2b_256(_)).Times(0);
  EXPECT_CALL(*transaction_pool, submitOne(_)).Times(0);
  EXPECT_OUTCOME_FALSE_2(err, api->submitExtrinsic(*extrinsic))
  ASSERT_EQ(err.value(),
            static_cast<int>(ExtrinsicApiError::INVALID_STATE_TRANSACTION));
}

/**
 * @given configured extrinsic submission api object
 * @when submit_extrinsic is called,
 * but in process extrinsic was recognized as `Unknown`
 * @then method returns failure and extrinsic is not sent
 * to transaction pool
 */
TEST_F(ExtrinsicSubmissionApiTest, SubmitExtrinsicUnknownFail) {
  TransactionValidity tv = Unknown{1u};
  EXPECT_CALL(*block_tree, deepestLeaf()).WillOnce(Return(*deepest_leaf));
  EXPECT_CALL(*ttq,
              validate_transaction(deepest_leaf->block_number, *extrinsic))
      .WillOnce(Return(
          outcome::failure(ExtrinsicApiError::UNKNOWN_STATE_TRANSACTION)));
  EXPECT_CALL(*hasher, blake2b_256(_)).Times(0);
  EXPECT_CALL(*transaction_pool, submitOne(_)).Times(0);
  EXPECT_OUTCOME_FALSE_2(err, api->submitExtrinsic(*extrinsic))
  ASSERT_EQ(err.value(),
            static_cast<int>(ExtrinsicApiError::UNKNOWN_STATE_TRANSACTION));
}

/**
 * @given configured extrinsic submission api object
 * @when submit_extrinsic is called,
 * but send to transaction pool fails
 * @then method returns failure
 */
TEST_F(ExtrinsicSubmissionApiTest, SubmitExtrinsicSubmitFail) {
  TransactionValidity tv = *valid_transaction;
  gsl::span<const uint8_t> span = gsl::make_span(extrinsic->data);
  EXPECT_CALL(*hasher, blake2b_256(span)).WillOnce(Return(Hash256{}));
  EXPECT_CALL(*block_tree, deepestLeaf()).WillOnce(Return(*deepest_leaf));
  EXPECT_CALL(*ttq,
              validate_transaction(deepest_leaf->block_number, *extrinsic))
      .WillOnce(Return(tv));
  Transaction tr{*extrinsic,
                 extrinsic->data.size(),
                 Hash256{},
                 valid_transaction->priority,
                 valid_transaction->longevity,
                 valid_transaction->requires,
                 valid_transaction->provides,
                 true};
  EXPECT_CALL(*transaction_pool, submitOne(tr))
      .WillOnce(
          Return(outcome::failure(TransactionPoolError::ALREADY_IMPORTED)));

  EXPECT_OUTCOME_FALSE_2(err, api->submitExtrinsic(*extrinsic))
  ASSERT_EQ(err.value(),
            static_cast<int>(TransactionPoolError::ALREADY_IMPORTED));
}
