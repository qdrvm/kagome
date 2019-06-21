/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extrinsics_submission_service/extrinsic_submission_api.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "common/blob.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "primitives/block_id.hpp"
#include "primitives/extrinsic.hpp"
#include "primitives/transaction.hpp"
#include "runtime/tagged_transaction_queue.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "transaction_pool/impl/transaction_pool_impl.hpp"

using namespace kagome::service;
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

class HasherMock : public HasherImpl {
 public:
  MOCK_CONST_METHOD1(blake2_256, Hash256(gsl::span<const uint8_t>));
};

class TaggedTransactionQueueMock : public TaggedTransactionQueue {
 public:
  ~TaggedTransactionQueueMock() override = default;

  MOCK_METHOD1(validate_transaction,
               outcome::result<TransactionValidity>(const Extrinsic &));
};

class TransactionPoolMock : public TransactionPool {
 public:
  ~TransactionPoolMock() override = default;

  // only this one we need here
  MOCK_METHOD1(submitOne, outcome::result<void>(Transaction));
  MOCK_METHOD1(submit, outcome::result<void>(std::vector<Transaction>));
  MOCK_METHOD0(getReadyTransactions, std::vector<Transaction>());
  MOCK_METHOD1(removeStale, std::vector<Transaction>(const BlockId &));
  MOCK_METHOD1(prune, std::vector<Transaction>(const std::vector<Extrinsic> &));
  MOCK_METHOD1(pruneTags,
               std::vector<Transaction>(const std::vector<TransactionTag> &));
  MOCK_CONST_METHOD0(getStatus, Status(void));
};

class ExtrinsicSubmissionApiTest : public ::testing::Test {
  template <class T>
  using sptr = std::shared_ptr<T>;

 protected:
  sptr<HasherMock> hasher{std::make_shared<HasherMock>()};  ///< hasher mock
  sptr<TaggedTransactionQueueMock> ttq{
      std::make_shared<TaggedTransactionQueueMock>()};  ///< ttq mock
  sptr<TransactionPoolMock> tp{
      std::make_shared<TransactionPoolMock>()};  ///< transaction pool mock

  ExtrinsicSubmissionApi api{ttq, tp, hasher};  ///< api instance

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
                 Buffer(Hash256{}),
                 valid_transaction.priority,
                 valid_transaction.longevity,
                 valid_transaction.requires,
                 valid_transaction.provides,
                 true};
  EXPECT_CALL(*tp, submitOne(tr)).WillOnce(Return(outcome::success()));

  EXPECT_OUTCOME_TRUE(hash, api.submit_extrinsic(extrinsic))
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
      .WillOnce(Return(outcome::failure(
          ExtrinsicSubmissionError::INVALID_STATE_TRANSACTION)));
  EXPECT_CALL(*hasher, blake2_256(_)).Times(0);
  EXPECT_CALL(*tp, submitOne(_)).Times(0);
  EXPECT_OUTCOME_FALSE_2(err, api.submit_extrinsic(extrinsic))
  ASSERT_EQ(
      err.value(),
      static_cast<int>(ExtrinsicSubmissionError::INVALID_STATE_TRANSACTION));
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
      .WillOnce(Return(outcome::failure(
          ExtrinsicSubmissionError::UNKNOWN_STATE_TRANSACTION)));
  EXPECT_CALL(*hasher, blake2_256(_)).Times(0);
  EXPECT_CALL(*tp, submitOne(_)).Times(0);
  EXPECT_OUTCOME_FALSE_2(err, api.submit_extrinsic(extrinsic))
  ASSERT_EQ(
      err.value(),
      static_cast<int>(ExtrinsicSubmissionError::UNKNOWN_STATE_TRANSACTION));
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
                 Buffer(Hash256{}),
                 valid_transaction.priority,
                 valid_transaction.longevity,
                 valid_transaction.requires,
                 valid_transaction.provides,
                 true};
  EXPECT_CALL(*tp, submitOne(tr))
      .WillOnce(Return(
          outcome::failure(TransactionPoolImpl::Error::ALREADY_IMPORTED)));

  EXPECT_OUTCOME_FALSE_2(err, api.submit_extrinsic(extrinsic))
  ASSERT_EQ(err.value(),
            static_cast<int>(TransactionPoolImpl::Error::ALREADY_IMPORTED));
}
