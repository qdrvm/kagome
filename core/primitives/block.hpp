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

  /**
   * @brief outputs object of type Block to stream
   * @tparam Stream output stream type
   * @param s stream reference
   * @param v value to output
   * @return reference to stream
   */
  template <class Stream>
  Stream &operator<<(Stream &s, const Block &b) {
    return s << b.header << b.extrinsics;
  }

  /**
   * @brief decodes object of type Block from stream
   * @tparam Stream input stream type
   * @param s stream reference
   * @param v value to decode
   * @return reference to stream
   */
  template <class Stream>
  Stream &operator>>(Stream &s, Block &b) {
    return s >> b.header >> b.extrinsics;
  }
}  // namespace kagome::primitives

#endif  // KAGOME_PRIMITIVES_BLOCK_HPP
