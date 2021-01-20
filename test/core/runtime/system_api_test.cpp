/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/runtime_api/system_impl.hpp"

#include <gtest/gtest.h>

#include "common/mp_utils.hpp"
#include "core/runtime/runtime_test.hpp"
#include "crypto/sr25519_types.hpp"
#include "runtime/binaryen/wasm_memory_impl.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::Invoke;

using kagome::runtime::System;
using kagome::runtime::binaryen::SystemImpl;

namespace fs = boost::filesystem;

class SystemApiTest : public RuntimeTest {
 public:
  void SetUp() override {
    RuntimeTest::SetUp();

    api_ = std::make_shared<SystemImpl>(wasm_provider_, runtime_manager_);
  }

 protected:
  std::shared_ptr<System> api_;
};

std::array<uint8_t, 32> raw_account_id_from_substrate{
    212, 53,  147, 199, 21,  253, 211, 28,  97,  20,  26,
    189, 4,   169, 159, 214, 130, 44,  133, 88,  133, 76,
    205, 227, 154, 86,  132, 231, 165, 109, 162, 125};

/**
 * @given initialized Metadata api
 * @when metadata() is invoked
 * @then successful result is returned
 */
TEST_F(SystemApiTest, account_nonce) {
  kagome::crypto::Sr25519PublicKey pk{
      kagome::common::Blob(raw_account_id_from_substrate)};
  auto pk_hex = pk.toHex();
  uint32_t pk_int{};
  auto shift = 24;
  for (auto byte : pk) {
    pk_int |= byte;
    pk_int <<= shift;
    shift -= 8;
  }
  EXPECT_CALL(*storage_provider_mock_, setToEphemeral())
      .WillOnce(Return(outcome::success()));
  EXPECT_CALL(*storage_provider_mock_, getCurrentBatch())
      .WillOnce(testing::Invoke([]() {
        auto batch =
            std::make_unique<kagome::storage::trie::PersistentTrieBatchMock>();
        EXPECT_CALL(*batch, get(_))
            .WillOnce(Invoke([](auto& key) {

              return Buffer{
                kagome::scale::encode(static_cast<uint64_t>(42)).value()};
            }));
        return batch;
      }));

  // uint32_t pk_int_1 = std::stoul(pk_hex, nullptr, 16);
  // ASSERT_EQ(pk_int, pk_int_1);
  EXPECT_OUTCOME_TRUE(nonce, api_->account_nonce(pk))
}
