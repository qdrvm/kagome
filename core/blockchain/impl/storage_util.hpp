/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_BLOCKCHAIN_IMPL_PERSISTENT_MAP_UTIL_HPP
#define KAGOME_CORE_BLOCKCHAIN_IMPL_PERSISTENT_MAP_UTIL_HPP

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
   * Stores the mapping of block number to its full key form - NumHashKey
   */
  outcome::result<void> assignNumberToHash(storage::SpacedStorage &storage,
                                           const primitives::BlockInfo &block);

  /**
   * Put an entry to key space \param space and corresponding lookup keys to
   * kLookupKey space
   * @param storage to put the entry to
   * @param space keyspace for the entry value
   * @param num block number that could be used to retrieve the value
   * @param block_hash block hash that could be used to retrieve the value
   * @param value data to be put to the storage
   * @return storage error if any
   */
  outcome::result<void> putToSpace(storage::SpacedStorage &storage,
                                   storage::Space space,
                                   common::Hash256 block_hash,
                                   common::BufferOrView &&value);

  /**
   * Chech if an entry from the database
   * @param storage - to get the entry from
   * @param space - key space in the storage  to which the entry belongs
   * @param block_id - id of the block to get entry for
   * @return true if entry exists, false if does not, and error at fail
   */
  outcome::result<bool> hasInSpace(storage::SpacedStorage &storage,
                                   storage::Space space,
                                   const primitives::BlockId &block_id);

  /**
   * Get an entry from the database
   * @param storage - to get the entry from
   * @param space - key space in the storage  to which the entry belongs
   * @param block_id - id of the block to get entry for
   * @return error, or an encoded entry, if any, or std::nullopt, if none
   */
  outcome::result<std::optional<common::BufferOrView>> getFromSpace(
      storage::SpacedStorage &storage,
      storage::Space space,
      const primitives::BlockId &block_id);

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

}  // namespace kagome::blockchain

#endif  // KAGOME_CORE_BLOCKCHAIN_IMPL_PERSISTENT_MAP_UTIL_HPP
