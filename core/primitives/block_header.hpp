/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PRIMITIVES_BLOCK_HEADER_HPP
#define KAGOME_PRIMITIVES_BLOCK_HEADER_HPP

#include <vector>

#include "common/blob.hpp"
#include "primitives/common.hpp"
#include "primitives/digest.hpp"

namespace kagome::primitives {
  /**
   * @struct BlockHeader represents header of a block
   */
  struct BlockHeader {
    BlockHash parent_hash;            ///< 32-byte Blake2s hash of parent header
    BlockNumber number = 0u;          ///< index of current block in the chain
    common::Hash256 state_root;       ///< root of the Merkle tree
    common::Hash256 extrinsics_root;  ///< field for validation integrity
    std::vector<Digest> digests;      ///< chain-specific auxiliary data

    bool operator==(const BlockHeader &rhs) const {
      return std::tie(parent_hash, number, state_root, extrinsics_root, digests)
             == std::tie(rhs.parent_hash,
                         rhs.number,
                         rhs.state_root,
                         rhs.extrinsics_root,
                         rhs.digests);
    }

    bool operator!=(const BlockHeader &rhs) const {
      return !operator==(rhs);
    }
  };

  /**
   * @brief outputs object of type BlockHeader to stream
   * @tparam Stream output stream type
   * @param s stream reference
   * @param v value to output
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const BlockHeader &bh) {
    return s << bh.parent_hash << bh.number << bh.state_root
             << bh.extrinsics_root << bh.digests;
  }

  /**
   * @brief decodes object of type BlockHeader from stream
   * @tparam Stream input stream type
   * @param s stream reference
   * @param v value to output
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, BlockHeader &bh) {
    return s >> bh.parent_hash >> bh.number >> bh.state_root
           >> bh.extrinsics_root >> bh.digests;
  }
}  // namespace kagome::primitives

#endif  // KAGOME_PRIMITIVES_BLOCK_HEADER_HPP
