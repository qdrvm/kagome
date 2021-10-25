/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_BLOCKCHAIN_IMPL_PERSISTENT_MAP_UTIL_HPP
#define KAGOME_CORE_BLOCKCHAIN_IMPL_PERSISTENT_MAP_UTIL_HPP

#include "common/buffer.hpp"
#include "primitives/block_header.hpp"
#include "primitives/block_id.hpp"
#include "storage/buffer_map_types.hpp"

/**
 * Auxiliary functions to simplify usage of persistant map based storage
 * as a Blockchain storage
 */

namespace kagome::blockchain {

  /**
   * Storage has only one key space, prefixes are used to divide it
   */
  namespace prefix {
    enum Prefix : uint8_t {
      // mapping of block id to a storage lookup key
      ID_TO_LOOKUP_KEY = 3,

      // block headers
      HEADER = 4,

      // body of the block (extrinsics)
      BLOCK_DATA = 5,

      // justification of the finalized block
      JUSTIFICATION = 6,

      // node of a trie db
      TRIE_NODE = 7
    };
  }

  /**
   * Errors that might occur during work with storage
   */
  enum class KeyValueRepositoryError { INVALID_KEY = 1 };

  /**
   * Concatenate \param key_column with \param key
   * @return key_column|key
   */
  common::Buffer prependPrefix(const common::Buffer &key,
                               prefix::Prefix key_column);

  /**
   * Put an entry to key space \param prefix and corresponding lookup keys to
   * ID_TO_LOOKUP_KEY space
   * @param map to put the entry to
   * @param prefix keyspace for the entry value
   * @param num block number that could be used to retrieve the value
   * @param block_hash block hash that could be used to retrieve the value
   * @param value data to be put to the storage
   * @return storage error if any
   */
  outcome::result<void> putWithPrefix(storage::BufferStorage &map,
                                      prefix::Prefix prefix,
                                      primitives::BlockNumber num,
                                      common::Hash256 block_hash,
                                      const common::Buffer &value);

  /**
   * Get an entry from the database
   * @param storage - to get the entry from
   * @param prefix - key space in the storage  to which the entry belongs
   * @param block_id - id of the block to get entry for
   * @return error, or an encoded entry, if any, or boost::none, if none
   */
  outcome::result<boost::optional<common::Buffer>> getWithPrefix(
      const storage::BufferStorage &storage,
      prefix::Prefix prefix,
      const primitives::BlockId &block_id);

  /**
   * Convert block number into short lookup key (LE representation) for
   * blocks that are in the canonical chain.
   *
   * In the current database schema, this kind of key is only used for
   * lookups into an index, NOT for storing header data or others.
   */
  common::Buffer numberToIndexKey(primitives::BlockNumber n);

  /**
   * Convert number and hash into long lookup key for blocks that are
   * not in the canonical chain.
   */
  common::Buffer numberAndHashToLookupKey(primitives::BlockNumber number,
                                          const common::Hash256 &hash);

  /**
   * Convert lookup key to a block number
   */
  outcome::result<primitives::BlockNumber> lookupKeyToNumber(
      const common::Buffer &key);

}  // namespace kagome::blockchain

OUTCOME_HPP_DECLARE_ERROR(kagome::blockchain, KeyValueRepositoryError);

#endif  // KAGOME_CORE_BLOCKCHAIN_IMPL_PERSISTENT_MAP_UTIL_HPP
