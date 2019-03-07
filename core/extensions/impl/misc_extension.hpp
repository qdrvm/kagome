/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MISC_EXTENSION_HPP
#define KAGOME_MISC_EXTENSION_HPP

namespace extensions {
  /**
   * Implements miscellaneous extension functions
   */
  class MiscExtension {
   public:
    uint64_t ext_chain_id() {
      std::terminate();
    }
  };
}  // namespace extensions

#endif  // KAGOME_MISC_EXTENSION_HPP
