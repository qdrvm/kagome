/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PRIMITIVES_BLOCK_HEADER_HPP
#define KAGOME_PRIMITIVES_BLOCK_HEADER_HPP

#include <type_traits>
#include <vector>

#include "common/blob.hpp"
#include "primitives/common.hpp"
#include "primitives/compact_integer.hpp"
#include "primitives/digest.hpp"
#include "storage/trie/types.hpp"

namespace kagome::primitives {
  /**
   * @struct BlockHeader represents header of a block
   */
  struct BlockHeader {
    SCALE_TIE(5);

    BlockHash parent_hash{};  ///< 32-byte Blake2s hash of parent header
    BlockNumber number = 0u;  ///< index of the block in the chain
    storage::trie::RootHash state_root{};  ///< root of the Merkle tree
    common::Hash256 extrinsics_root{};     ///< field for validation integrity
    Digest digest{};                       ///< chain-specific auxiliary data
  };

  struct GenesisBlockHeader {
    const BlockHeader header;
    const BlockHash hash;
  };

}  // namespace kagome::primitives

#endif  // KAGOME_PRIMITIVES_BLOCK_HEADER_HPP
