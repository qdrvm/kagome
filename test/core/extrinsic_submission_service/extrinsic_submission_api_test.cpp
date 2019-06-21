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
#include "transaction_pool/transaction_pool.hpp"

using namespace kagome::service;
using namespace kagome::hash;
using namespace kagome::transaction_pool;
using namespace kagome::runtime;

using kagome::common::Buffer;
using kagome::common::Hash256;
using kagome::primitives::BlockId;
using kagome::primitives::Extrinsic;
using kagome::primitives::Transaction;
using kagome::primitives::TransactionTag;
using kagome::primitives::TransactionValidity;
using kagome::primitives::Valid;

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

  Extrinsic ext{"12"_hex2buf};
  Valid valid_transaction{1, {{2}}, {{3}}, 4};
};

/**
 * @given configured extrinsic submission api object
 * @when submit_extrinsic is called
 * @then it is successfully completes returning expected result
 */
TEST_F(ExtrinsicSubmissionApiTest, SubmitOneSuccessful) {
  TransactionValidity tv = valid_transaction;
  std::vector<uint8_t> buf = "12"_unhex;

  gsl::span<const uint8_t> span = gsl::make_span(buf);
  EXPECT_CALL(*hasher, blake2_256(span)).WillOnce(Return(Hash256{}));
  EXPECT_CALL(*ttq, validate_transaction(ext)).WillOnce(Return(tv));
  Transaction tr{ext,
                 ext.data.size(),
                 Buffer(Hash256{}),
                 valid_transaction.priority,
                 valid_transaction.longevity,
                 valid_transaction.requires,
                 valid_transaction.provides,
                 true};
  EXPECT_CALL(*tp, submitOne(tr)).WillOnce(Return(outcome::success()));

  EXPECT_OUTCOME_TRUE(hash, api.submit_extrinsic(ext))
  ASSERT_EQ(hash, Hash256{});
}
