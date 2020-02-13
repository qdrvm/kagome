/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_GOSSIP_MESSAGE_HPP
#define KAGOME_GOSSIP_MESSAGE_HPP

#include "common/buffer.hpp"

namespace kagome::network {
  /**
   * Message, which is passed over the Gossip protocol
   */
  struct GossipMessage {
    enum class Type : uint8_t {
      BLOCK_ANNOUNCE = 1,
      PRECOMMIT,
      PREVOTE,
      PRIMARY_PROPOSE,

      UNKNOWN = 0
    };

    Type type{};
    common::Buffer data;
  };

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const GossipMessage &m) {
    return s << static_cast<uint8_t>(m.type) << m.data;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, GossipMessage &m) {
    return s >> m.type >> m.data;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, GossipMessage::Type &t) {
    uint8_t type{};
    s >> type;
    if (type > 4) {
      t = GossipMessage::Type::UNKNOWN;
    } else {
      t = static_cast<GossipMessage::Type>(type);
    }
    return s;
  }
}  // namespace kagome::network

#endif  // KAGOME_GOSSIP_MESSAGE_HPP
