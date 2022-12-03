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

  using ReadableBufferStorage =
      storage::face::ReadableStorage<common::BufferView, common::Buffer>;

  /**
   * Convert a block ID into a key, which is a first part of a key, by which the
   * columns are stored in the database
   */
  outcome::result<std::optional<common::BufferOrView>> idToLookupKey(
      const ReadableBufferStorage &map, const primitives::BlockId &id);

  /**
   * Instantiate empty merkle trie, insert \param key_vals pairs and \return
   * Buffer containing merkle root of resulting trie
   */
  storage::trie::RootHash trieRoot(
      const std::vector<std::pair<common::Buffer, common::Buffer>> &key_vals);
}  // namespace kagome::blockchain

#endif  // KAGOME_BLOCKCHAIN_COMMON_HPP
