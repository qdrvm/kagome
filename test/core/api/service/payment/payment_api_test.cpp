/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/payment/impl/payment_api_impl.hpp"

#include <gtest/gtest.h>

#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/runtime/transaction_payment_api_mock.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using kagome::api::PaymentApi;
using kagome::api::PaymentApiImpl;
using kagome::blockchain::BlockTreeMock;
using kagome::common::Buffer;
using kagome::primitives::BlockInfo;
using kagome::primitives::Extrinsic;
using kagome::primitives::RuntimeDispatchInfo;
using kagome::primitives::OldWeight;
using kagome::runtime::TransactionPaymentApiMock;

using testing::_;
using testing::Return;

namespace kagome::primitives {
  template<typename Weight>
  bool operator==(const RuntimeDispatchInfo<Weight> &lhs,
                  const RuntimeDispatchInfo<Weight> &rhs) {
    return lhs.weight == rhs.weight && lhs.partial_fee == rhs.partial_fee;
  }
}  // namespace kagome::primitives

class PaymentApiTest : public ::testing::Test {
 public:
  void SetUp() {
    transaction_payment_api_ = std::make_shared<TransactionPaymentApiMock>();
    block_tree_ = std::make_shared<BlockTreeMock>();

    payment_api_ =
        std::make_unique<PaymentApiImpl>(transaction_payment_api_, block_tree_);
  }

 protected:
  std::unique_ptr<PaymentApi> payment_api_;

  std::shared_ptr<TransactionPaymentApiMock> transaction_payment_api_;
  std::shared_ptr<BlockTreeMock> block_tree_;
};

/**
 * @given extrinsic, length, optional block hash
 * @when query extrinsic info for block (or head if nullopt)
 * @then query extrinsic info with transaction payment api and return it
 */
TEST_F(PaymentApiTest, QueryInfo) {
  Extrinsic extrinsic;
  auto len = 22u;
  auto deepest_hash = "12345"_hash256;
  BlockInfo deepest_leaf{10, deepest_hash};
  RuntimeDispatchInfo<OldWeight> expected_result;

  EXPECT_CALL(*block_tree_, deepestLeaf()).WillOnce(Return(deepest_leaf));
  EXPECT_CALL(*transaction_payment_api_,
              query_info(deepest_hash, extrinsic, len))
      .WillOnce(Return(expected_result));

  ASSERT_OUTCOME_SUCCESS(result,
                         payment_api_->queryInfo(extrinsic, len, std::nullopt));
  ASSERT_EQ(result, expected_result);
}
