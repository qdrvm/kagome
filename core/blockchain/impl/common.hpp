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
      storage::face::Readable<common::Buffer, common::Buffer>;

  /**
   * Convert a block ID into a key, which is a first part of a key, by which the
   * columns are stored in the database
   */
  outcome::result<std::optional<common::BufferOrView>> idToLookupKey(
      const ReadableBufferStorage &map, const primitives::BlockId &id);
}  // namespace kagome::blockchain

#endif  // KAGOME_BLOCKCHAIN_COMMON_HPP
