/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KAGOME_PRIMITIVES_BLOCK_HPP
#define KAGOME_PRIMITIVES_BLOCK_HPP

#include "primitives/block_header.hpp"
#include "primitives/extrinsic.hpp"

namespace kagome::primitives {
  using BlockBody = std::vector<Extrinsic>;

  /**
   * @brief Block class represents polkadot block primitive
   */
  struct Block {
    BlockHeader header;  ///< block header
    BlockBody body{};    ///< extrinsics collection

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
