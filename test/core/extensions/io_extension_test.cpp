/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extensions/impl/io_extension.hpp"
#include <gtest/gtest.h>

using namespace kagome::extensions;

/**
 * It is impossible to test the console output, but at least we can check, that
 * methods do not fail
 */
class IOExtensionTest : public ::testing::Test {
 public:
  IOExtension io_extension{};

  // 0123456789abcdef
  static constexpr std::array<uint8_t, 8> hex_bytes{0x01, 0x23, 0x45, 0x67,
                                                    0x89, 0xAB, 0xCD, 0xEF};

  // 2^64 - 1
  static constexpr uint64_t number = std::numeric_limits<uint64_t>::max();

  // 1 @m $t|>i|\Ng
  static constexpr std::array<uint8_t, 14> utf8_bytes{
      0x31, 0x20, 0x40, 0x6d, 0x20, 0x24, 0x74,
      0x7c, 0x3e, 0x69, 0x7c, 0x5c, 0x4e, 0x67};
};

TEST_F(IOExtensionTest, PrintHex) {
  io_extension.ext_print_hex(hex_bytes.data(), hex_bytes.size());
}

TEST_F(IOExtensionTest, PrintNum) {
  io_extension.ext_print_num(number);
}

TEST_F(IOExtensionTest, PrintUTF8) {
  io_extension.ext_print_utf8(utf8_bytes.data(), utf8_bytes.size());
}
