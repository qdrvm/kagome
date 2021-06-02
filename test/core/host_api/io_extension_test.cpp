/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_api/impl/io_extension.hpp"

#include <optional>

#include <gtest/gtest.h>

#include "mock/core/runtime/wasm_memory_mock.hpp"
#include "runtime/wasm_result.hpp"
#include "testutil/literals.hpp"
#include "testutil/prepare_loggers.hpp"

using namespace kagome::host_api;
using ::testing::Return;

using kagome::common::Buffer;
using kagome::runtime::WasmEnum;
using kagome::runtime::WasmLogLevel;
using kagome::runtime::WasmMemoryMock;
using kagome::runtime::WasmPointer;
using kagome::runtime::WasmResult;
using kagome::runtime::WasmSize;

/**
 * It is impossible to test the console output, but at least we can check, that
 * methods do not fail
 */
class IOExtensionTest : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    memory_ = std::make_shared<WasmMemoryMock>();
    io_extension_ = std::make_shared<IOExtension>(memory_);
  }

 protected:
  std::shared_ptr<WasmMemoryMock> memory_;
  std::shared_ptr<IOExtension> io_extension_;

  std::vector<uint8_t> hex_bytes_{"0123456789ABCDEF"_unhex};

  static constexpr uint64_t number_ = std::numeric_limits<uint64_t>::max();

  // 1 @m $t|>i|\Ng
  std::vector<uint8_t> utf8_bytes_{"3120406d2024747c3e697c5c4e67"_unhex};
};

/**
 * @given io_extension
 * @when try to print string 0123456789abcdef using ext_print_hex
 * @then hex encoded for given string is printed
 */
TEST_F(IOExtensionTest, PrintHex) {
  WasmPointer data = 0;
  WasmSize size = hex_bytes_.size();
  Buffer buf(hex_bytes_);

  EXPECT_CALL(*memory_, loadN(data, size)).WillOnce(Return(buf));

  io_extension_->ext_print_hex(data, size);
}

/**
 * @given io_extension
 * @when try to print string 0123456789abcdef using ext_print_hex
 * @then hex encoded for given string is printed
 */
TEST_F(IOExtensionTest, PrintMessage) {
  WasmResult target(0, hex_bytes_.size());
  std::string buf(&hex_bytes_.front(), &hex_bytes_.back());

  EXPECT_CALL(*memory_, loadStr(target.address, target.length))
      .WillRepeatedly(Return(buf));
  io_extension_->ext_logging_log_version_1(
      static_cast<WasmEnum>(WasmLogLevel::WasmLL_Error),
      target.combine(),
      target.combine());
}

/**
 * @given io_extension
 * @when try to get max log level
 * @then log level returned
 * @note somehow HostApi log level is OFF
 */
TEST_F(IOExtensionTest, GetMaxLogLevel) {
  auto res = io_extension_->ext_logging_max_level_version_1();
  ASSERT_EQ(res, static_cast<WasmEnum>(WasmLogLevel::WasmLL_Error));
}

/**
 * @given io_extension
 * @when try to some number using ext_print_num from io_extension
 * @then given number is printed
 */
TEST_F(IOExtensionTest, PrintNum) {
  io_extension_->ext_print_num(number_);
}

/**
 * @given io_extension
 * @when try to print string "1 @m $t|>i|\Ng" represented as byte array
 * @then given utf decoded string is printed
 */
TEST_F(IOExtensionTest, PrintUTF8) {
  WasmPointer data = 0;
  WasmSize size = utf8_bytes_.size();
  std::string buf(utf8_bytes_.begin(), utf8_bytes_.end());

  EXPECT_CALL(*memory_, loadStr(data, size)).WillOnce(Return(buf));
  io_extension_->ext_print_utf8(data, size);
}
