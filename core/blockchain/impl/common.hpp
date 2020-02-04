/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCKCHAIN_COMMON_HPP
#define KAGOME_BLOCKCHAIN_COMMON_HPP

#include <outcome/outcome.hpp>
#include "common/buffer.hpp"
#include "primitives/block_id.hpp"
#include "storage/buffer_map.hpp"

namespace kagome::blockchain {
  using PersistentBufferMap = storage::PersistentBufferMap;
  using ReadableBufferMap =
      storage::face::ReadableMap<common::Buffer, common::Buffer>;

  enum class Error {
    // it's important to convert storage errors of this type to this one to
    // enable a user to discern between cases when a header with provided id
    // is not found and when an internal storage error occurs
    BLOCK_NOT_FOUND = 1
  };

  /**
   * Convert a block ID into a key, which is a first part of a key, by which the
   * columns are stored in the database
   */
  outcome::result<common::Buffer> idToLookupKey(const ReadableBufferMap &map,
                                                const primitives::BlockId &id);

  /**
   * Instantiate empty merkle trie, insert \param key_vals pairs and \return
   * Buffer containing merkle root of resulting trie
   */
  common::Buffer trieRoot(
      const std::vector<std::pair<common::Buffer, common::Buffer>> &key_vals);
}  // namespace kagome::blockchain

OUTCOME_HPP_DECLARE_ERROR(kagome::blockchain, Error)

#endif  // KAGOME_BLOCKCHAIN_COMMON_HPP
