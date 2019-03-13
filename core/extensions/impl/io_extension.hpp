/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_IO_EXTENSION_HPP
#define KAGOME_IO_EXTENSION_HPP

#include <cstdint>

#include "common/logger.hpp"

namespace kagome::extensions {
  /**
   * Implements extension functions related to IO
   */
  class IOExtension {
   public:
    IOExtension();

    /**
     * @see Extension::ext_print_hex
     */
    void ext_print_hex(const uint8_t *data, uint32_t length);

    /**
     * @see Extension::ext_print_num
     */
    void ext_print_num(uint64_t value);

    /**
     * @see Extension::ext_print_utf8
     */
    void ext_print_utf8(const uint8_t *utf8_data, uint32_t utf8_length);

   private:
    common::logger::Logger logger_;
  };
}  // namespace kagome::extensions

#endif  // KAGOME_IO_EXTENSION_HPP
