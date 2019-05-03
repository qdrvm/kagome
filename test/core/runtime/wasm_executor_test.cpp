/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/wasm_executor.hpp"

#include <fstream>

#include <binaryen/wasm-binary.h>
#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
#include "core/extensions/mock_extension.hpp"
#include "runtime/impl/wasm_memory_impl.hpp"
#include "testutil/runtime/wasm_test.hpp"

using kagome::common::Buffer;
using kagome::extensions::MockExtension;
using kagome::runtime::WasmExecutor;
using kagome::runtime::WasmMemoryImpl;

using ::testing::Return;

namespace fs = boost::filesystem;

class WasmExecutorTest: public test::WasmTest {
 public:
  WasmExecutorTest()
      : WasmTest{fs::path(__FILE__).parent_path().string()
                 + "/wasm/sumtwo.wasm"} {}

  void SetUp() override {
    WasmTest::SetUp();
    extension_ = std::make_shared<MockExtension>();
    memory_ = std::make_shared<WasmMemoryImpl>();
    EXPECT_CALL(*extension_, memory()).WillRepeatedly(Return(memory_));
    executor_ = std::make_shared<WasmExecutor>(extension_);
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
  auto res =
      executor_->call(state_code_, "addTwo",
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
  wasm::WasmBinaryBuilder parser(
      module,
      reinterpret_cast<std::vector<char> const &>(state_code_),  // NOLINT
      false);
  parser.read();

  auto res = executor_->callInModule(
      module, "addTwo", wasm::LiteralList{wasm::Literal(1), wasm::Literal(2)});
  ASSERT_EQ(res.geti32(), 3);
}

/**
 * @given wasm executor
 * @when call is invoked with invalid or empty state code
 * @then proper error is returned
 */
TEST_F(WasmExecutorTest, ExecuteWithInvalidStateCode) {
  Buffer state_code;

  ASSERT_FALSE(executor_->call(state_code, "foo", wasm::LiteralList{}));
  ASSERT_FALSE(executor_->call(Buffer::fromHex("12345A").value(), "foo",
                               wasm::LiteralList{}));
}
