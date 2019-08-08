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
  using BlockBody = std::vector<Extrinsic>;

  /**
   * @brief Block class represents polkadot block primitive
   */
  struct Block {
    BlockHeader header;  ///< block header
    BlockBody body;      ///< extrinsics collection

    inline bool operator==(const Block &rhs) const {
      return header == rhs.header and body == rhs.body;
    }

    inline bool operator!=(const Block &rhs) const {
      return !operator==(rhs);
    }
  };

  /**
   * @brief outputs object of type Block to stream
   * @tparam Stream output stream type
   * @param s stream reference
   * @param v value to output
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const Block &b) {
    return s << b.header << b.body;
  }

  /**
   * @brief decodes object of type Block from stream
   * @tparam Stream input stream type
   * @param s stream reference
   * @param v value to decode
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, Block &b) {
    return s >> b.header >> b.body;
  }
}  // namespace kagome::primitives

#endif  // KAGOME_PRIMITIVES_BLOCK_HPP
