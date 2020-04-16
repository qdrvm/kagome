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

#ifndef KAGOME_BLOCK_ANNOUNCE_HPP
#define KAGOME_BLOCK_ANNOUNCE_HPP

#include "primitives/block_header.hpp"

namespace kagome::network {
  /**
   * Announce a new complete block on the network
   */
  struct BlockAnnounce {
    primitives::BlockHeader header;
  };

  /**
   * @brief compares two BlockAnnounce instances
   * @param lhs first instance
   * @param rhs second instance
   * @return true if equal false otherwise
   */
  inline bool operator==(const BlockAnnounce &lhs, const BlockAnnounce &rhs) {
    return lhs.header == rhs.header;
  }
  inline bool operator!=(const BlockAnnounce &lhs, const BlockAnnounce &rhs) {
    return !(lhs == rhs);
  }

  /**
   * @brief outputs object of type BlockAnnounce to stream
   * @tparam Stream output stream type
   * @param s stream reference
   * @param v value to output
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const BlockAnnounce &v) {
    return s << v.header;
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
  Stream &operator>>(Stream &s, BlockAnnounce &v) {
    return s >> v.header;
  }
}  // namespace kagome::network

#endif  // KAGOME_BLOCK_ANNOUNCE_HPP
