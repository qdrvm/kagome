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
using kagome::primitives::OldWeight;
using kagome::primitives::RuntimeDispatchInfo;
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
  RuntimeDispatchInfo<OldWeight> expected_result;

  EXPECT_CALL(*block_tree_, bestBlock()).WillOnce(Return(best_leaf));
  EXPECT_CALL(*transaction_payment_api_,
              query_info(deepest_hash, extrinsic, len))
      .WillOnce(Return(expected_result));

  ASSERT_OUTCOME_SUCCESS(result,
                         payment_api_->queryInfo(extrinsic, len, std::nullopt));
  ASSERT_EQ(result, expected_result);
}

TEST_F(PaymentApiTest, DecodeRuntimeDispatchInfo) {
  // for extrinsic 0x280403000b30e7f2b98501
  //  weight: 133,706,000
  //  class: Mandatory
  //  partialFee: 0
  //
  // clang-format off
  unsigned char data[22] = {
    0x42, 0xc4, 0xe0, 0x1f,
    0x00, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00};
  // clang-format on

  auto info = scale::decode<RuntimeDispatchInfo<OldWeight>>(data).value();

  ASSERT_EQ(*info.weight, 133706000);
  ASSERT_EQ(info.dispatch_class, kagome::primitives::DispatchClass::Mandatory);
  ASSERT_EQ(*info.partial_fee, 0);
}

// 0x410284000a0e6dfd6e3b14d5f6a379ccb79e4e547c9bb8f0f2793e32528c45eecef15a3b01e4421d5f63c82b99ef6c1d4295e80f7b2ffdbf787d25ea093eff633d231bfe77776a7105600d528c0ba77a44de88916681815d695b7a72dd59235359a924df835503000005000040139d1483af62a1ad4144815f418640fdebafabf2bc922083614fff77c976170700b24f980b
// {
//  weight: 143,322,000
//  class: Normal
//  partialFee: 15.7681 mDOT (157681946)
// }
TEST_F(PaymentApiTest, DecodeRuntimeDispatchInfoWithFee) {
  gsl::span<const uint8_t> data{{
      0x42, 0xae, 0x2b, 0x22, 0x00, 0x00, 0x1a, 0x09, 0x66, 0x09, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  }};

  auto info = scale::decode<RuntimeDispatchInfo<OldWeight>>(data).value();

  ASSERT_EQ(*info.weight, 143322000);
  ASSERT_EQ(info.dispatch_class, kagome::primitives::DispatchClass::Normal);
  ASSERT_EQ(*info.partial_fee, 157681946);
}
