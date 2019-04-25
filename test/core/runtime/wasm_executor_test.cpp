/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/wasm_executor.hpp"

#include <fstream>

#include <gtest/gtest.h>
#include "binaryen/wasm-binary.h"
#include "core/extensions/mock_extension.hpp"
#include "runtime/impl/wasm_memory_impl.hpp"

using kagome::extensions::MockExtension;
using kagome::runtime::WasmExecutor;
using kagome::runtime::WasmMemoryImpl;

using ::testing::Return;

class WasmExecutorTest : public ::testing::Test {
 public:
  void SetUp() override {
    extension_ = std::make_shared<MockExtension>();
    memory_ = std::make_shared<WasmMemoryImpl>();

    EXPECT_CALL(*extension_, memory()).WillRepeatedly(Return(memory_));
    executor_ = std::make_shared<WasmExecutor>(extension_);
  }

  std::vector<uint8_t> getSumTwoCode() {
    std::string path = "../../test/core/runtime/wasm/sumtwo.wasm";
    std::ifstream ifd(path, std::ios::binary | std::ios::ate);
    int size = ifd.tellg();
    ifd.seekg(0, std::ios::beg);
    std::vector<char> buffer;
    buffer.resize(size);  // << resize not reserve
    ifd.read(buffer.data(), size);
    return std::vector<uint8_t>(buffer.begin(), buffer.end());
  }

 protected:
  std::shared_ptr<WasmExecutor> executor_;
  std::shared_ptr<MockExtension> extension_;
  std::shared_ptr<WasmMemoryImpl> memory_;
};

/**
 * @given wasm executor
 * @when call is invoked with wasm code with addTwo function
 * @then proper result is returned
 */
TEST_F(WasmExecutorTest, ExecuteCode) {
  auto wasm_sum_two_code = getSumTwoCode();
  auto res =
      executor_->call(wasm_sum_two_code, "addTwo",
                      wasm::LiteralList{wasm::Literal(1), wasm::Literal(2)});
  ASSERT_EQ(res.geti32(), 3);
}

/**
 * @given wasm executor
 * @when call is invoked with wasm module with initiaalized with code with
 * addTwo function
 * @then proper result is returned
 */
TEST_F(WasmExecutorTest, ExecuteModule) {
  auto state_code = getSumTwoCode();

  wasm::Module module{};
  wasm::WasmBinaryBuilder parser(
      module,
      reinterpret_cast<std::vector<char> const &>(state_code),  // NOLINT
      false);
  parser.read();

  auto res = executor_->callInModule(
      module, "addTwo", wasm::LiteralList{wasm::Literal(1), wasm::Literal(2)});
  ASSERT_EQ(res.geti32(), 3);
}
