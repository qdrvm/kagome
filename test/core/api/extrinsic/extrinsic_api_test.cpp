/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
using kagome::primitives::BlockInfo;
using kagome::primitives::Extrinsic;
using kagome::primitives::InvalidTransaction;
using kagome::primitives::Transaction;
using kagome::primitives::TransactionTag;
using kagome::primitives::TransactionValidity;
using kagome::primitives::UnknownTransaction;
using kagome::primitives::ValidTransaction;

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
  sptr<ValidTransaction> valid_transaction;    ///< valid transaction instance
  Hash256 deepest_hash;                        ///< hash of deepest leaf
  sptr<BlockInfo> deepest_leaf;                ///< deepest leaf block info

  void SetUp() override {
    hasher = std::make_shared<HasherMock>();
    ttq = std::make_shared<TaggedTransactionQueueMock>();
    transaction_pool = std::make_shared<TransactionPoolMock>();
    block_tree = std::make_shared<BlockTreeMock>();
    api = std::make_shared<ExtrinsicApiImpl>(
        ttq, transaction_pool, hasher, block_tree);
    extrinsic.reset(new Extrinsic{"12"_hex2buf});
    valid_transaction.reset(new ValidTransaction{1, {{2}}, {{3}}, 4, true});
    deepest_hash = createHash256({1u, 2u, 3u});
    deepest_leaf.reset(new BlockInfo{1u, deepest_hash});
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
  EXPECT_CALL(*ttq, validate_transaction(*extrinsic)).WillOnce(Return(tv));
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
  TransactionValidity tv = InvalidTransaction{1u};
  EXPECT_CALL(*ttq, validate_transaction(*extrinsic))
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
  TransactionValidity tv = UnknownTransaction{1u};
  EXPECT_CALL(*ttq, validate_transaction(*extrinsic))
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
  EXPECT_CALL(*ttq, validate_transaction(*extrinsic)).WillOnce(Return(tv));
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
