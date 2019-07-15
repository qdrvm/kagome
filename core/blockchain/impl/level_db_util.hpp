/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_BLOCKCHAIN_IMPL_LEVEL_DB_UTIL_HPP
#define KAGOME_CORE_BLOCKCHAIN_IMPL_LEVEL_DB_UTIL_HPP

#include "common/buffer.hpp"
#include "primitives/block_header.hpp"
#include "primitives/block_id.hpp"

namespace kagome::blockchain {

  /**
   * As LevelDb storage has only one key space, prefixes are used to divide it
   */
  namespace prefix {
    enum Prefix : uint8_t { ID_TO_LOOKUP_KEY, HEADER };
  }

  enum class LevelDbRepositoryError { INVALID_KEY = 1 };

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

  outcome::result<primitives::BlockNumber> lookupKeyToNumber(
      const common::Buffer &key);

  common::Buffer prependPrefix(common::Buffer key,
                               kagome::blockchain::prefix::Prefix key_column);

}  // namespace kagome::blockchain

OUTCOME_HPP_DECLARE_ERROR(kagome::blockchain, LevelDbRepositoryError);

#endif  // KAGOME_CORE_BLOCKCHAIN_IMPL_LEVEL_DB_UTIL_HPP
