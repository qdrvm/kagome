/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PRIMITIVES_BLOCK_HPP
#define KAGOME_PRIMITIVES_BLOCK_HPP

#include "common/buffer.hpp"
#include "primitives/block_header.hpp"
#include "primitives/extrinsic.hpp"

namespace kagome::primitives {
  /**
   * @brief Block class represents polkadot block primitive
   */
  struct Block {
    BlockHeader header;                 ///< block header
    std::vector<Extrinsic> extrinsics;  ///< extrinsics collection
  };

}  // namespace kagome::primitives

namespace kagome::scale {
  template <class Stream>
  Stream &operator<<(Stream &s, const primitives::Block &b) {
    return s << b.header << b.extrinsics;
  }
}  // namespace kagome::scale

#endif  // KAGOME_PRIMITIVES_BLOCK_HPP
