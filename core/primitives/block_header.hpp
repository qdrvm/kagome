/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PRIMITIVES_BLOCK_HEADER_HPP
#define KAGOME_PRIMITIVES_BLOCK_HEADER_HPP

#include "common/blob.hpp"
#include "primitives/common.hpp"
#include "primitives/digest.hpp"

namespace kagome::primitives {
  /**
   * @struct BlockHeader represents header of a block
   */
  struct BlockHeader {
    common::Hash256 parent_hash;      ///< 32-byte Blake2s hash of parent header
    BlockNumber number;               ///< index of current block in the chain
    common::Hash256 state_root;       ///< root of the Merkle trie
    common::Hash256 extrinsics_root;  ///< field for validation integrity
    Digest digest;                    ///< chain-specific auxiliary data
  };

}  // namespace kagome::primitives

#endif  // KAGOME_PRIMITIVES_BLOCK_HEADER_HPP
