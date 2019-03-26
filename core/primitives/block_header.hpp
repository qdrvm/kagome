/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PRIMITIVES_BLOCK_HEADER_HPP
#define KAGOME_PRIMITIVES_BLOCK_HEADER_HPP

#include "common/buffer.hpp"

using namespace kagome::common;

namespace kagome::primitives {
  /**
   * @class BlockHeader represents header of a block
   */
  class BlockHeader {
   public:
    /**
     * @brief BlockHeader class constructor
     * @param parent_hash parent hash
     * @param state_root state root
     * @param extrinsics_root extrinsics root
     * @param digest digest collection
     */
    BlockHeader(Buffer parent_hash, size_t number, Buffer state_root,
                Buffer extrinsics_root, Buffer digest);

    /**
     * @return parent hash const reference
     */
    const Buffer &parentHash() const;

    /**
     * @return number
     */
    uint64_t number() const;

    /**
     * @return state root const reference
     */
    const Buffer &stateRoot() const;

    /**
     * @return extrinsics root const reference
     */
    const Buffer &extrinsicsRoot() const;

    /**
     * @return digest const reference
     */
    const Buffer &digest() const;

   private:
    Buffer parent_hash_;  ///< 32-byte Blake2s hash of the header of the parent
    uint64_t number_;     ///< index of current block in the chain
    Buffer stateRoot_;    ///< root of the Merkle trie
    Buffer extrinsics_root_;  ///< field for validation integrity
    Buffer digest_;           ///< chain-specific auxiliary data
  };

}  // namespace kagome::primitives

#endif  // KAGOME_PRIMITIVES_BLOCK_HEADER_HPP
