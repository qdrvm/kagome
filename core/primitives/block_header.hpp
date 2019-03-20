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
     * @param parentHash parent hash
     * @param stateRoot state root
     * @param extrinsicsRoot extrinsics root
     * @param digest digest collection
     */
    BlockHeader(Buffer parentHash, Buffer stateRoot, Buffer extrinsicsRoot,
                std::vector<Buffer> digest);

    /**
     * @return parent hash const reference
     */
    const Buffer &parentHash() const;

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
    const std::vector<Buffer> &digest() const;

   private:
    Buffer parentHash_;           ///< parent hash
    Buffer stateRoot_;            ///< state root
    Buffer extrinsicsRoot_;       ///< extrinsics root
    std::vector<Buffer> digest_;  ///< digest collection
  };

}  // namespace kagome::primitives

#endif  // KAGOME_PRIMITIVES_BLOCK_HEADER_HPP
