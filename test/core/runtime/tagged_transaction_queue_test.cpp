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

#include "runtime/binaryen/runtime_api/tagged_transaction_queue_impl.hpp"

#include <gtest/gtest.h>
#include "core/runtime/runtime_test.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/runtime/common/basic_wasm_provider.hpp"

using namespace testing;

using kagome::primitives::BlockNumber;
using kagome::primitives::Extrinsic;
using kagome::runtime::TaggedTransactionQueue;
using kagome::runtime::binaryen::TaggedTransactionQueueImpl;

class TTQTest : public RuntimeTest {
 public:
  void SetUp() override {
    RuntimeTest::SetUp();
    ttq_ = std::make_unique<TaggedTransactionQueueImpl>(wasm_provider_,
                                                        extension_factory_);
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
TEST_F(TTQTest, DISABLED_ValidateTransactionSuccess) {
  Extrinsic ext{"01020304AABB"_hex2buf};

  // we test now that the functions above are called sequentially
  // unfortunately, we don't have valid data for validate_transaction to succeed
  EXPECT_OUTCOME_TRUE_1(ttq_->validate_transaction(ext));
}
