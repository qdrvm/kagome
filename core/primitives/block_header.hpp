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

#ifndef KAGOME_PRIMITIVES_BLOCK_HEADER_HPP
#define KAGOME_PRIMITIVES_BLOCK_HEADER_HPP

#include <type_traits>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>
#include "common/blob.hpp"
#include "primitives/common.hpp"
#include "primitives/compact_integer.hpp"
#include "primitives/digest.hpp"

namespace kagome::primitives {
  /**
   * @struct BlockHeader represents header of a block
   */
  struct BlockHeader {
    BlockHash parent_hash{};       ///< 32-byte Blake2s hash of parent header
    BlockNumber number = 0u;       ///< index of current block in the chain
    common::Hash256 state_root{};  ///< root of the Merkle tree
    common::Hash256 extrinsics_root{};  ///< field for validation integrity
    Digest digest{};                    ///< chain-specific auxiliary data

    bool operator==(const BlockHeader &rhs) const {
      return std::tie(parent_hash, number, state_root, extrinsics_root, digest)
             == std::tie(rhs.parent_hash,
                         rhs.number,
                         rhs.state_root,
                         rhs.extrinsics_root,
                         rhs.digest);
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
    return s << bh.parent_hash << CompactInteger(bh.number) << bh.state_root
             << bh.extrinsics_root << bh.digest;
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
    CompactInteger number_compact;
    s >> bh.parent_hash >> number_compact >> bh.state_root >> bh.extrinsics_root
        >> bh.digest;
    bh.number = number_compact.convert_to<BlockNumber>();
    return s;
  }
}  // namespace kagome::primitives

#endif  // KAGOME_PRIMITIVES_BLOCK_HEADER_HPP
