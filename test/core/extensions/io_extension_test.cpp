/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extensions/impl/io_extension.hpp"

#include <optional>

#include <gtest/gtest.h>
#include "core/runtime/mock_memory.hpp"

using namespace kagome::extensions;
using ::testing::Return;

using kagome::common::Buffer;
using kagome::runtime::MockMemory;
using kagome::runtime::SizeType;
using kagome::runtime::WasmPointer;

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
  std::vector<uint8_t> hex_bytes_{0x01, 0x23, 0x45, 0x67,
                                  0x89, 0xAB, 0xCD, 0xEF};

  // 2^64 - 1
  static constexpr uint64_t number_ = std::numeric_limits<uint64_t>::max();

  // 1 @m $t|>i|\Ng
  std::vector<uint8_t> utf8_bytes_{0x31, 0x20, 0x40, 0x6d, 0x20, 0x24, 0x74,
                                   0x7c, 0x3e, 0x69, 0x7c, 0x5c, 0x4e, 0x67};
};

/**
 * @given io_extension
 * @when try to print string 0123456789abcdef using ext_print_hex
 * @then hex encoded for given string is printed
 */
TEST_F(IOExtensionTest, PrintHex) {
  WasmPointer data = 0;
  SizeType size = hex_bytes_.size();
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
  SizeType size = utf8_bytes_.size();
  Buffer buf(utf8_bytes_);

  EXPECT_CALL(*memory_, loadN(data, size)).WillOnce(Return(buf));
  io_extension_->ext_print_utf8(data, size);
}
