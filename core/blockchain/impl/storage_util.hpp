/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/buffer.hpp"
#include "primitives/block_header.hpp"
#include "primitives/block_id.hpp"
#include "storage/spaced_storage.hpp"

/**
 * Storage schema overview
 *
 * A key-value approach is used for block storage.
 * Different parts containing a block are stored in multiple storage spaces but
 * have to be addressed with the same key.
 *
 * A key is the combination of block's number concatenated with its hash. Let's
 * name it as NumHashKey.
 *
 * There is also an auxilary space named Space::kLookupKey where
 * BlockId->NumHashKey mappings are stored. Effectively there are could be two
 * types of mappings: either BlockNumber->NumHashKey or BlockHash->NumHashKey.
 * Anyways, the resulting NumHashKey is good to be used for further manipulating
 * with the Block in other storage spaces.
 */

/**
 * Auxiliary functions to simplify usage of persistant map based storage
 * as a Blockchain storage
 */

namespace kagome::blockchain {

  /**
   * Convert block number into short lookup key (LE representation) for
   * blocks that are in the canonical chain.
   */
  inline common::Buffer blockNumberToKey(primitives::BlockNumber block_number) {
    BOOST_STATIC_ASSERT(std::is_same_v<decltype(block_number), uint32_t>);
    common::Buffer res;
    res.putUint32(block_number);
    return res;
  }

  /**
   * Returns block hash by number if any
   */
  outcome::result<std::optional<primitives::BlockHash>> blockHashByNumber(
      storage::SpacedStorage &storage, primitives::BlockNumber block_number);

  /**
   * Check if an entry is contained in the database
   * @param storage - to get the entry from
   * @param space - key space in the storage  to which the entry belongs
   * @param block_id - id of the block to get entry for
   * @return true if entry exists, false if does not, and error at fail
   */
  outcome::result<bool> hasInSpace(storage::SpacedStorage &storage,
                                   storage::Space space,
                                   const primitives::BlockId &block_id);

  /**
   * Put an entry to key space \param space
   * @param storage to put the entry to
   * @param space keyspace for the entry value
   * @param block_hash block hash that could be used to retrieve the value
   * @param value data to be put to the storage
   * @return storage error if any
   */
  outcome::result<void> putToSpace(storage::SpacedStorage &storage,
                                   storage::Space space,
                                   const primitives::BlockHash &block_hash,
                                   common::BufferOrView &&value);

  /**
   * Get an entry from the database
   * @param storage - to get the entry from
   * @param space - key space in the storage  to which the entry belongs
   * @param block_hash - hash of the block to get entry for
   * @return error, or an encoded entry, if any, or std::nullopt, if none
   */
  outcome::result<std::optional<common::BufferOrView>> getFromSpace(
      storage::SpacedStorage &storage,
      storage::Space space,
      const primitives::BlockHash &block_hash);

  /**
   * Remove an entry from key space \param space and corresponding lookup keys
   * @param storage to put the entry to
   * @param space keyspace for the entry value
   * @param block_hash block hash that could be used to retrieve the value
   * @return storage error if any
   */
  outcome::result<void> removeFromSpace(
      storage::SpacedStorage &storage,
      storage::Space space,
      const primitives::BlockHash &block_hash);

}  // namespace kagome::blockchain
