/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_OFFCHAIN_OFFCHAINPERSISTENTSTORAGE
#define KAGOME_OFFCHAIN_OFFCHAINPERSISTENTSTORAGE

#include "offchain/offchain_storage.hpp"

namespace kagome::offchain {

  /**
   * Persistent storage
   * @brief It is non-revertible and not fork-aware. It means that any value set
   * by the offchain worker is persisted even if that block (at which the worker
   * is called) is reverted as non-canonical (meaning that the block was
   * surpassed by a longer chain). The value is available for the worker that is
   * re-run at the new (different block with the same block number) and future
   * blocks. This storage can be used by offchain workers to handle forks and
   * coordinate offchain workers running on different forks.
   */
  class OffchainPersistentStorage : public OffchainStorage {};

}  // namespace kagome::offchain

#endif  // KAGOME_OFFCHAIN_OFFCHAINPERSISTENTSTORAGE
