/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MISC_EXTENSION_HPP
#define KAGOME_MISC_EXTENSION_HPP

#include <cstdint>

#include "macro/nodiscard.hpp"

namespace kagome::extensions {
  /**
   * Implements miscellaneous extension functions
   */
  class MiscExtension {
   public:
    MiscExtension() = default;
    ~MiscExtension() = default;
    explicit MiscExtension(uint64_t chain_id);

    /**
     * @return id (a 64-bit unsigned integer) of the current chain
     */
    NODISCARD uint64_t ext_chain_id() const;

   private:
    const uint64_t chain_id_ = 42;
  };
}  // namespace kagome::extensions

#endif  // KAGOME_MISC_EXTENSION_HPP
