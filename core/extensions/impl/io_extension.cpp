/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <exception>

#include "extensions/impl/io_extension.hpp"

namespace extensions {
  void IOExtension::ext_print_hex(const uint8_t *data, uint32_t length) {
    std::terminate();
  }
  void IOExtension::ext_print_num(uint64_t value) {
    std::terminate();
  }
  void IOExtension::ext_print_utf8(const uint8_t *utf8_data,
                                   uint32_t utf8_length) {
    std::terminate();
  }
}  // namespace extensions
