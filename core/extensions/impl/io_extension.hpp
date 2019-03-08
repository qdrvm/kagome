/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_IO_EXTENSION_HPP
#define KAGOME_IO_EXTENSION_HPP

#include <cstdint>

namespace extensions {
  /**
   * Implements extension functions related to IO
   */
  class IOExtension {
   public:
    void ext_print_hex(const uint8_t *data, uint32_t length);
    void ext_print_num(uint64_t value);
    void ext_print_utf8(const uint8_t *utf8_data, uint32_t utf8_length);
  };
}  // namespace extensions

#endif  // KAGOME_IO_EXTENSION_HPP
