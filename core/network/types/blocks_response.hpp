/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCKS_RESPONSE_HPP
#define KAGOME_BLOCKS_RESPONSE_HPP

#include <vector>

#include <optional>
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
    primitives::BlocksRequestId id{0ull};
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
