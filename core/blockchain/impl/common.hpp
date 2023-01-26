/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCKCHAIN_COMMON_HPP
#define KAGOME_BLOCKCHAIN_COMMON_HPP

#include "common/buffer.hpp"
#include "outcome/outcome.hpp"
#include "primitives/block_id.hpp"
#include "storage/spaced_storage.hpp"
#include "storage/trie/types.hpp"

namespace kagome::blockchain {

  /**
   * Convert a block ID (which is either hash or number) into a key, which is a
   * first part of a key, by which the columns are stored in the database
   */
  outcome::result<std::optional<common::BufferOrView>> idToLookupKey(
      storage::SpacedStorage &storage, const primitives::BlockId &id);
}  // namespace kagome::blockchain

#endif  // KAGOME_BLOCKCHAIN_COMMON_HPP
