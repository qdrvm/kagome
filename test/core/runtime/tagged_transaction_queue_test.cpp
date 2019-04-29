/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/tagged_transaction_queue_impl.hpp"

#include <gtest/gtest.h>
#include "core/storage/merkle/mock_trie_db.hpp"
#include "extensions/extension_impl.hpp"
#include "primitives/impl/scale_codec_impl.hpp"
#include "runtime/impl/wasm_memory_impl.hpp"
#include "testutil/outcome.hpp"
#include "core/runtime/runtime_fixture.hpp"

using namespace testing;
using kagome::common::Buffer;
using kagome::extensions::ExtensionImpl;
using kagome::primitives::Extrinsic;
using kagome::primitives::ScaleCodecImpl;
using kagome::runtime::TaggedTransactionQueue;
using kagome::runtime::TaggedTransactionQueueImpl;
using kagome::runtime::WasmMemoryImpl;
using kagome::storage::merkle::MockTrieDb;

class TTQFixture : public RuntimeTestFixture {
 public:
  void SetUp() override {
    RuntimeTestFixture::SetUp();

    auto state_code = getRuntimeCode();

    p = std::make_unique<TaggedTransactionQueueImpl>(state_code, extension_, codec_);
  }

 protected:
  std::unique_ptr<TaggedTransactionQueue> p;
};


/**
 * @given initialised tagged transaction queue api
 * @when validating a transaction
 * @then a TransactionValidity structure is obtained after successful call,
 * otherwise an outcome error
 */
TEST_F(TTQFixture, validate_transaction) {
  Extrinsic ext(Buffer::fromHex("01020304AABB").value());

  EXPECT_OUTCOME_TRUE(res, p->validate_transaction(ext));
}
