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
    common::Hash256 state_root;       ///< root of the Merkle tree
    common::Hash256 extrinsics_root;  ///< field for validation integrity
    Digest digest;                    ///< chain-specific auxiliary data
  };

  /**
   * @brief outputs object of type BlockHeader to stream
   * @tparam Stream output stream type
   * @param s stream reference
   * @param v value to output
   * @return reference to stream
   */
  template <class Stream>
  Stream &operator<<(Stream &s, const BlockHeader &bh) {
    return s << bh.parent_hash << bh.number << bh.state_root
             << bh.extrinsics_root << bh.digest;
  }

  /**
   * @brief decodes object of type BlockHeader from stream
   * @tparam Stream input stream type
   * @param s stream reference
   * @param v value to output
   * @return reference to stream
   */
  template <class Stream>
  Stream &operator>>(Stream &s, BlockHeader &bh) {
    return s >> bh.parent_hash >> bh.number >> bh.state_root
        >> bh.extrinsics_root >> bh.digest;
  }
}  // namespace kagome::primitives

#endif  // KAGOME_PRIMITIVES_BLOCK_HEADER_HPP
