/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/extrinsic/impl/extrinsic_api_impl.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "common/blob.hpp"
#include "mock/crypto/hasher.hpp"
#include "mock/runtime/tagged_transaction_queue_mock.hpp"
#include "mock/transaction_pool/transaction_pool_mock.hpp"
#include "primitives/block_id.hpp"
#include "primitives/extrinsic.hpp"
#include "primitives/transaction.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "transaction_pool/impl/transaction_pool_impl.hpp"
#include "transaction_pool/transaction_pool_error.hpp"

using namespace kagome::api;
using namespace kagome::hash;
using namespace kagome::transaction_pool;
using namespace kagome::runtime;

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
using kagome::transaction_pool::TransactionPoolImpl;

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;

class ExtrinsicSubmissionApiTest : public ::testing::Test {
  template <class T>
  using sptr = std::shared_ptr<T>;

 protected:
  sptr<HasherMock> hasher{std::make_shared<HasherMock>()};  ///< hasher mock
  sptr<TaggedTransactionQueueMock> ttq{
      std::make_shared<TaggedTransactionQueueMock>()};  ///< ttq mock
  sptr<TransactionPoolMock> tp{
      std::make_shared<TransactionPoolMock>()};  ///< transaction pool mock

  ExtrinsicApiImpl api{ttq, tp, hasher};  ///< api instance

  Extrinsic extrinsic{"12"_hex2buf};
  Valid valid_transaction{1, {{2}}, {{3}}, 4};
};

/**
 * @given configured extrinsic submission api object
 * @when submit_extrinsic is called
 * @then it is successfully completes returning expected result
 */
TEST_F(ExtrinsicSubmissionApiTest, SubmitExtrinsicSuccess) {
  TransactionValidity tv = valid_transaction;
  gsl::span<const uint8_t> span = gsl::make_span(extrinsic.data);
  EXPECT_CALL(*hasher, blake2_256(span)).WillOnce(Return(Hash256{}));
  EXPECT_CALL(*ttq, validate_transaction(extrinsic)).WillOnce(Return(tv));
  Transaction tr{extrinsic,
                 extrinsic.data.size(),
                 Hash256{},
                 valid_transaction.priority,
                 valid_transaction.longevity,
                 valid_transaction.requires,
                 valid_transaction.provides,
                 true};
  EXPECT_CALL(*tp, submitOne(tr)).WillOnce(Return(outcome::success()));

  EXPECT_OUTCOME_TRUE(hash, api.submitExtrinsic(extrinsic))
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

  EXPECT_CALL(*ttq, validate_transaction(extrinsic))
      .WillOnce(Return(
          outcome::failure(ExtrinsicApiError::INVALID_STATE_TRANSACTION)));
  EXPECT_CALL(*hasher, blake2_256(_)).Times(0);
  EXPECT_CALL(*tp, submitOne(_)).Times(0);
  EXPECT_OUTCOME_FALSE_2(err, api.submitExtrinsic(extrinsic))
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

  EXPECT_CALL(*ttq, validate_transaction(extrinsic))
      .WillOnce(Return(
          outcome::failure(ExtrinsicApiError::UNKNOWN_STATE_TRANSACTION)));
  EXPECT_CALL(*hasher, blake2_256(_)).Times(0);
  EXPECT_CALL(*tp, submitOne(_)).Times(0);
  EXPECT_OUTCOME_FALSE_2(err, api.submitExtrinsic(extrinsic))
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
  TransactionValidity tv = valid_transaction;
  gsl::span<const uint8_t> span = gsl::make_span(extrinsic.data);
  EXPECT_CALL(*hasher, blake2_256(span)).WillOnce(Return(Hash256{}));
  EXPECT_CALL(*ttq, validate_transaction(extrinsic)).WillOnce(Return(tv));
  Transaction tr{extrinsic,
                 extrinsic.data.size(),
                 Hash256{},
                 valid_transaction.priority,
                 valid_transaction.longevity,
                 valid_transaction.requires,
                 valid_transaction.provides,
                 true};
  EXPECT_CALL(*tp, submitOne(tr))
      .WillOnce(Return(
          outcome::failure(TransactionPoolError::ALREADY_IMPORTED)));

  EXPECT_OUTCOME_FALSE_2(err, api.submitExtrinsic(extrinsic))
  ASSERT_EQ(err.value(),
            static_cast<int>(TransactionPoolError::ALREADY_IMPORTED));
}
