/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_OFFCHAIN_OFFCHAINLOCALSTORAGE
#define KAGOME_OFFCHAIN_OFFCHAINLOCALSTORAGE

#include "offchain/offchain_storage.hpp"

namespace kagome::offchain {

  /**
   * Local storage
   * @brief It is revertible and fork-aware. It means that any value set by the
   * offchain worker triggered at a certain block is reverted if that block is
   * reverted as non-canonical. The value is NOT available for the worker that
   * is re-run at the next or any future blocks.
   */
  class OffchainLocalStorage : public OffchainStorage {};

}  // namespace kagome::offchain

#endif  // KAGOME_OFFCHAIN_OFFCHAINLOCALSTORAGE
