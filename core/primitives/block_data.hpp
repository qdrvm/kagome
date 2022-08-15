/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_PRIMITIVES_BLOCK_DATA_HPP
#define KAGOME_CORE_PRIMITIVES_BLOCK_DATA_HPP

#include <optional>

#include "primitives/block.hpp"
#include "primitives/justification.hpp"

namespace kagome::primitives {

  /**
   * Data, describing the block. Used for example in BlockRequest, where we need
   * to get certain information about the block
   */
  struct BlockData {
    primitives::BlockHash hash;
    std::optional<primitives::BlockHeader> header{};
    std::optional<primitives::BlockBody> body{};
    std::optional<common::Buffer> receipt{};
    std::optional<common::Buffer> message_queue{};
    std::optional<primitives::Justification> justification{};
  };

  struct BlockDataFlags {
    static BlockDataFlags allSet(primitives::BlockHash hash) {
      return BlockDataFlags{std::move(hash), true, true, true, true, true};
    }

    static BlockDataFlags allUnset(primitives::BlockHash hash) {
      return BlockDataFlags{std::move(hash), false, false, false, false, false};
    }

    primitives::BlockHash hash;
    bool header{};
    bool body{};
    bool receipt{};
    bool message_queue{};
    bool justification{};
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
