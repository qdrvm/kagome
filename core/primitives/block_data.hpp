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

#ifndef KAGOME_CORE_PRIMITIVES_BLOCK_DATA_HPP
#define KAGOME_CORE_PRIMITIVES_BLOCK_DATA_HPP

#include <boost/optional.hpp>

#include "primitives/block.hpp"
#include "primitives/justification.hpp"

namespace kagome::primitives {

  /**
   * Data, describing the block. Used for example in BlockRequest, where we need
   * to get certain information about the block
   */
  struct BlockData {
    primitives::BlockHash hash;
    boost::optional<primitives::BlockHeader> header{};
    boost::optional<primitives::BlockBody> body{};
    boost::optional<common::Buffer> receipt{};
    boost::optional<common::Buffer> message_queue{};
    boost::optional<primitives::Justification> justification{};

    /**
     * Convert a block data into the block
     * @return block, if at least header exists in this BlockData, nothing
     * otherwise
     */
    boost::optional<primitives::Block> toBlock() const {
      if (!header) {
        return boost::none;
      }
      return body ? primitives::Block{*header, *body}
                  : primitives::Block{*header};
    }
  };

  /**
   * @brief compares two BlockData instances
   * @param lhs first instance
   * @param rhs second instance
   * @return true if equal false otherwise
   */
  inline bool operator==(const BlockData &lhs, const BlockData &rhs) {
    return lhs.hash == rhs.hash && lhs.header == rhs.header
           && lhs.body == rhs.body && lhs.receipt == rhs.receipt
           && lhs.message_queue == rhs.message_queue
           && lhs.justification == rhs.justification;
  }

  /**
   * @brief outputs object of type BlockData to stream
   * @tparam Stream output stream type
   * @param s stream reference
   * @param v value to output
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const BlockData &v) {
    return s << v.hash << v.header << v.body << v.receipt << v.message_queue
             << v.justification;
  }

  /**
   * @brief decodes object of type BlockData from stream
   * @tparam Stream input stream type
   * @param s stream reference
   * @param v value to decode
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, BlockData &v) {
    return s >> v.hash >> v.header >> v.body >> v.receipt >> v.message_queue
           >> v.justification;
  }

}  // namespace kagome::primitives

#endif  // KAGOME_CORE_PRIMITIVES_BLOCK_DATA_HPP
