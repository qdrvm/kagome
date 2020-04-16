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

#include "runtime/binaryen/wasm_executor.hpp"

#include <fstream>

#include <binaryen/shell-interface.h>
#include <binaryen/wasm-binary.h>
#include <gtest/gtest.h>
#include <boost/filesystem.hpp>

#include "testutil/runtime/common/basic_wasm_provider.hpp"

using kagome::common::Buffer;
using kagome::runtime::binaryen::WasmExecutor;

namespace fs = boost::filesystem;

class WasmExecutorTest : public ::testing::Test {
 public:
  void SetUp() override {
    // path to a file with wasm code in wasm/ subfolder
    std::string wasm_path =
        fs::path(__FILE__).parent_path().string() + "/wasm/sumtwo.wasm";
    wasm_provider_ = std::make_shared<kagome::runtime::BasicWasmProvider>(wasm_path);
    executor_ = std::make_shared<WasmExecutor>();
  }

 protected:
  std::shared_ptr<WasmExecutor> executor_;
  wasm::ShellExternalInterface external_interface_{};
  std::shared_ptr<kagome::runtime::WasmProvider> wasm_provider_;
};

/**
 * @given wasm executor
 * @when call is invoked with wasm code with addTwo function
 * @then proper result is returned
 */
TEST_F(WasmExecutorTest, ExecuteCode) {
  auto res =
      executor_->call(wasm_provider_->getStateCode(),
                      external_interface_,
                      "addTwo",
                      wasm::LiteralList{wasm::Literal(1), wasm::Literal(2)});
  ASSERT_TRUE(res) << res.error().message();
  ASSERT_EQ(res.value().geti32(), 3);
}

/**
 * @given wasm executor
 * @when call is invoked with invalid or empty state code
 * @then proper error is returned
 */
TEST_F(WasmExecutorTest, ExecuteWithInvalidStateCode) {
  Buffer state_code;

  ASSERT_FALSE(executor_->call(
      state_code, external_interface_, "foo", wasm::LiteralList{}));
  ASSERT_FALSE(executor_->call(Buffer::fromHex("12345A").value(),
                               external_interface_,
                               "foo",
                               wasm::LiteralList{}));
}
