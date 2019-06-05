/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/tagged_transaction_queue_impl.hpp"

#include <gtest/gtest.h>
#include "core/runtime/runtime_test.hpp"
#include "core/storage/merkle/mock_trie_db.hpp"
#include "extensions/extension_impl.hpp"
#include "runtime/impl/wasm_memory_impl.hpp"
#include "testutil/outcome.hpp"
#include "testutil/runtime/wasm_test.hpp"

using namespace testing;
using kagome::common::Buffer;
using kagome::extensions::ExtensionImpl;
using kagome::primitives::Extrinsic;
using kagome::runtime::TaggedTransactionQueue;
using kagome::runtime::TaggedTransactionQueueImpl;
using kagome::runtime::WasmMemoryImpl;
using kagome::storage::merkle::MockTrieDb;

class TTQTest: public RuntimeTest {
 public:
  void SetUp() override {
    RuntimeTest::SetUp();

    ttq_ = std::make_unique<TaggedTransactionQueueImpl>(state_code_, extension_);
  }

 protected:
  std::unique_ptr<TaggedTransactionQueue> ttq_;
};

/**
 * @given initialised tagged transaction queue api
 * @when validating a transaction
 * @then a TransactionValidity structure is obtained after successful call,
 * otherwise an outcome error
 */
TEST_F(TTQTest, validate_transaction) {
  Extrinsic ext{Buffer::fromHex("01020304AABB").value()};

  ASSERT_FALSE(ttq_->validate_transaction(ext));
}
