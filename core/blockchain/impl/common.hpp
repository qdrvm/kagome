/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCKCHAIN_COMMON_HPP
#define KAGOME_BLOCKCHAIN_COMMON_HPP

#include "common/buffer.hpp"
#include "outcome/outcome.hpp"
#include "primitives/block_id.hpp"
#include "storage/buffer_map_types.hpp"
#include "storage/trie/types.hpp"

namespace kagome::blockchain {
  using ReadableBufferMap =
      storage::face::Readable<common::Buffer, common::Buffer>;

  /// TODO(Harrm): This seriously has to be refactored into
  /// result<optional<...>>, for now we actually have a zoo of such errors over
  /// the blockchain module and it's really hard to pick the right error const
  /// in client code
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
  storage::trie::RootHash trieRoot(
      const std::vector<std::pair<common::Buffer, common::Buffer>> &key_vals);
}  // namespace kagome::blockchain

OUTCOME_HPP_DECLARE_ERROR(kagome::blockchain, Error)

#endif  // KAGOME_BLOCKCHAIN_COMMON_HPP
