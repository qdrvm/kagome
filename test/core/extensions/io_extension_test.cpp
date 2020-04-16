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

#include "extensions/impl/io_extension.hpp"

#include <optional>

#include <gtest/gtest.h>
#include "testutil/literals.hpp"
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
