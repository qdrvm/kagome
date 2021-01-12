/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
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
