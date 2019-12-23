/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/wasm_executor.hpp"

#include <fstream>

#include <binaryen/shell-interface.h>
#include <binaryen/wasm-binary.h>
#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
#include "runtime/impl/basic_wasm_provider.hpp"

using kagome::common::Buffer;
using kagome::runtime::WasmExecutor;

namespace fs = boost::filesystem;

class WasmExecutorTest : public ::testing::Test {
 public:
  void SetUp() override {
    // path to a file with wasm code in wasm/ subfolder
    std::string wasm_path =
        fs::path(__FILE__).parent_path().string() + "/wasm/sumtwo.wasm";
    wasm_provider_ = std::make_shared<test::BasicWasmProvider>(wasm_path);
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
 * @when call is invoked with wasm module with initialised with code with
 * addTwo function
 * @then proper result is returned
 */
TEST_F(WasmExecutorTest, ExecuteModule) {
  wasm::Module module{};
  auto &&state_code = wasm_provider_->getStateCode();
  std::vector<char> data(state_code.begin(), state_code.end());
  wasm::WasmBinaryBuilder parser(module, data, false);
  parser.read();

  auto res = executor_->callInModule(
      module,
      external_interface_,
      "addTwo",
      wasm::LiteralList{wasm::Literal(12), wasm::Literal(34)});
  ASSERT_EQ(res.geti32(), 46);
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
