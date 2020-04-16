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

#include "runtime/binaryen/runtime_api/offchain_worker_impl.hpp"

#include <gtest/gtest.h>

#include "core/runtime/runtime_test.hpp"
#include "extensions/impl/extension_impl.hpp"
#include "primitives/common.hpp"
#include "runtime/binaryen/runtime_api/offchain_worker_impl.hpp"
#include "runtime/binaryen/wasm_memory_impl.hpp"
#include "testutil/outcome.hpp"

using ::testing::_;
using ::testing::Return;

namespace fs = boost::filesystem;

class OffchainWorkerTest : public RuntimeTest {
  using OffchainWorker = kagome::runtime::OffchainWorker;
  using OffchainWorkerImpl = kagome::runtime::binaryen::OffchainWorkerImpl;
  using BlockNumber = kagome::primitives::BlockNumber;

 public:
  void SetUp() override {
    RuntimeTest::SetUp();

    api_ = std::make_shared<OffchainWorkerImpl>(wasm_provider_,
                                                extension_factory_);
  }

  BlockNumber createBlockNumber() const {
    return BlockNumber{0u};
  }

 protected:
  std::shared_ptr<OffchainWorker> api_;
};

/**
 * @given initialized offchain worker api
 * @when offchain_worker() is invoked
 * @then successful result is returned
 */
TEST_F(OffchainWorkerTest, OffchainWorkerCallSuccess) {
  ASSERT_TRUE(api_->offchain_worker(createBlockNumber()));
}
