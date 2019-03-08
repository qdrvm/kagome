/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MEMORY_EXTENSIONS_HPP
#define KAGOME_MEMORY_EXTENSIONS_HPP

namespace extensions {
  /**
   * Implements extension functions related to memory
   */
  class MemoryExtension {
   public:
    uint8_t *ext_malloc(uint32_t size);

    void ext_free(uint8_t *ptr);
  };
}  // namespace extensions

#endif  // KAGOME_MEMORY_EXTENSIONS_HPP
