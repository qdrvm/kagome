/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PRIMITIVES_BLOCK_HEADER_HPP
#define KAGOME_PRIMITIVES_BLOCK_HEADER_HPP

#include "common/blob.hpp"
#include "common/buffer.hpp"
#include "primitives/common.hpp"
#include "primitives/digest.hpp"

namespace kagome::primitives {
  /**
   * @class BlockHeader represents header of a block
   */
  class BlockHeader {
    using Buffer = common::Buffer;

   public:
    /**
     * @brief BlockHeader class constructor
     * @param parent_hash parent hash
     * @param number index of block in the chain
     * @param state_root state root
     * @param extrinsics_root extrinsics root
     * @param digest digest collection
     */
    BlockHeader(common::Hash256 parent_hash, BlockNumber number,
                common::Hash256 state_root, common::Hash256 extrinsics_root,
                Buffer digest);

    /**
     * @return parent hash const reference
     */
    const common::Hash256 &parentHash() const;

    /**
     * @return number
     */
    BlockNumber number() const;

    /**
     * @return state root const reference
     */
    const common::Hash256 &stateRoot() const;

    /**
     * @return extrinsics root const reference
     */
    const common::Hash256 &extrinsicsRoot() const;

    /**
     * @return digest const reference
     */
    const Digest &digest() const;

   private:
    common::Hash256 parent_hash_;  ///< 32-byte Blake2s hash of parent header
    BlockNumber number_;           ///< index of current block in the chain
    common::Hash256 state_root_;   ///< root of the Merkle trie
    common::Hash256 extrinsics_root_;  ///< field for validation integrity
    Digest digest_;                    ///< chain-specific auxiliary data
  };

}  // namespace kagome::primitives

#endif  // KAGOME_PRIMITIVES_BLOCK_HEADER_HPP
