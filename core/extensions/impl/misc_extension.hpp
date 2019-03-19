/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MISC_EXTENSION_HPP
#define KAGOME_MISC_EXTENSION_HPP

#include <cstdint>

namespace kagome::extensions {
  /**
   * Implements miscellaneous extension functions
   */
  class MiscExtension {
   public:
    MiscExtension() = default;
    explicit MiscExtension(uint64_t chain_id);

    /**
     * @return id (a 64-bit unsigned integer) of the current chain
     */
    uint64_t ext_chain_id();

   private:
    const uint64_t chain_id_ = 42;
  };
}  // namespace extensions

#endif  // KAGOME_MISC_EXTENSION_HPP
