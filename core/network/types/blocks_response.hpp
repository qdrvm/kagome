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

#ifndef KAGOME_BLOCKS_RESPONSE_HPP
#define KAGOME_BLOCKS_RESPONSE_HPP

#include <vector>

#include <boost/optional.hpp>
#include "common/buffer.hpp"
#include "primitives/block.hpp"
#include "primitives/block_data.hpp"
#include "primitives/common.hpp"
#include "primitives/justification.hpp"

namespace kagome::network {
  /**
   * Response to the BlockRequest
   */
  struct BlocksResponse {
    primitives::BlocksRequestId id;
    std::vector<primitives::BlockData> blocks{};
  };

  /**
   * @brief compares two BlockResponse instances
   * @param lhs first instance
   * @param rhs second instance
   * @return true if equal false otherwise
   */
  inline bool operator==(const BlocksResponse &lhs, const BlocksResponse &rhs) {
    return lhs.id == rhs.id && lhs.blocks == rhs.blocks;
  }

  /**
   * @brief outputs object of type BlockResponse to stream
   * @tparam Stream output stream type
   * @param s stream reference
   * @param v value to output
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const BlocksResponse &v) {
    return s << v.id << v.blocks;
  }

  /**
   * @brief decodes object of type BlockResponse from stream
   * @tparam Stream input stream type
   * @param s stream reference
   * @param v value to decode
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, BlocksResponse &v) {
    return s >> v.id >> v.blocks;
  }
}  // namespace kagome::network

#endif  // KAGOME_BLOCKS_RESPONSE_HPP
