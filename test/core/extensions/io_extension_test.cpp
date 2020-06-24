/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extensions/impl/io_extension.hpp"

#include <optional>

#include <gtest/gtest.h>
#include "core/runtime/mock_memory.hpp"
#include "testutil/literals.hpp"

using namespace kagome::extensions;
using ::testing::Return;

using kagome::common::Buffer;
using kagome::runtime::MockMemory;
using kagome::runtime::WasmPointer;
using kagome::runtime::WasmSize;

/**
 * It is impossible to test the console output, but at least we can check, that
 * methods do not fail
 */
class IOExtensionTest : public ::testing::Test {
 public:
  void SetUp() override {
    memory_ = std::make_shared<MockMemory>();
    io_extension_ = std::make_shared<IOExtension>(memory_);
  }

 protected:
  std::shared_ptr<MockMemory> memory_;
  std::shared_ptr<IOExtension> io_extension_;

  // 0123456789abcdef
  std::vector<uint8_t> hex_bytes_{"0123456789ABCDEF"_unhex};

  // 2^64 - 1
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
  Buffer buf(utf8_bytes_);

  EXPECT_CALL(*memory_, loadN(data, size)).WillOnce(Return(buf));
  io_extension_->ext_print_utf8(data, size);
}
