/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
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
using kagome::common::BufferView;
using kagome::primitives::BlockInfo;
using kagome::primitives::Extrinsic;
using kagome::primitives::RuntimeDispatchInfo;
using kagome::primitives::Weight;
using kagome::runtime::TransactionPaymentApiMock;

using testing::_;
using testing::Return;

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
  BlockInfo best_leaf{10, deepest_hash};
  RuntimeDispatchInfo<Weight> expected_result;

  EXPECT_CALL(*block_tree_, bestBlock()).WillOnce(Return(best_leaf));
  EXPECT_CALL(*transaction_payment_api_,
              query_info(deepest_hash, extrinsic, len))
      .WillOnce(Return(expected_result));

  ASSERT_OUTCOME_SUCCESS(result,
                         payment_api_->queryInfo(extrinsic, len, std::nullopt));
  ASSERT_EQ(result, expected_result);
}

TEST_F(PaymentApiTest, DecodeRuntimeDispatchInfo) {
  // clang-format off
  // for extrinsic 0x350284007ef99ee767314ccb4726be579ab3eabd212741b3796db40405ff421c47b0ae8502268965ca1a619e1aec211193906ff60009a2d6b29d61e1f46c4eb6e1646235e0217450f2c129fe9a3adc3d5f585fadab592a5602496f635c3718bc753e9e9f221b550200000105000018c7f5a8530d6aafc1b191156294a9e27bb674128607896f3fd5914282fb196d00
  //  weight:
  //    {
  //      ref_time: 144,460,000
  //      proof_size: 3593
  //     }
  //  class: normal
  //  partialFee: 154146098
  //
  // clang-format on
  Buffer data =
      Buffer::fromHex("8223712225380032153009000000000000000000000000").value();

  auto info = scale::decode<RuntimeDispatchInfo<Weight>>(data).value();

  ASSERT_EQ(info.weight.ref_time, 144460000);
  ASSERT_EQ(info.weight.proof_size, 3593);
  ASSERT_EQ(info.dispatch_class, kagome::primitives::DispatchClass::Normal);
  ASSERT_EQ(info.partial_fee, 154146098);
}
