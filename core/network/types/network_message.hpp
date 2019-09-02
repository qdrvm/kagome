/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_MESSAGE_HPP
#define KAGOME_NETWORK_MESSAGE_HPP

#include "common/buffer.hpp"

namespace kagome::network {
  /**
   * Message, which is sent over the network
   */
  struct NetworkMessage {
    enum class Type { BLOCK_ANNOUNCE, BLOCKS_REQUEST, BLOCKS_RESPONSE };

    Type type;
    common::Buffer body;
  };

  /**
   * @brief outputs object of type NetworkMessage to stream
   * @tparam Stream output stream type
   * @param s stream reference
   * @param v value to output
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const NetworkMessage &b) {
    return s << b.type << b.body;
  }

  /**
   * @brief decodes object of type NetworkMessage from stream
   * @tparam Stream input stream type
   * @param s stream reference
   * @param v value to decode
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, NetworkMessage &b) {
    return s >> b.type >> b.body;
  }
}  // namespace kagome::network

#endif  // KAGOME_NETWORK_MESSAGE_HPP
