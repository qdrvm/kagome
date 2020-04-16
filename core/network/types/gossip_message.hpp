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

#ifndef KAGOME_GOSSIP_MESSAGE_HPP
#define KAGOME_GOSSIP_MESSAGE_HPP

#include "common/buffer.hpp"

namespace kagome::network {

  inline const size_t kMaxMessageTypes = 80;

  /**
   * Message, which is passed over the Gossip protocol
   */
  struct GossipMessage {
    enum class Type : uint8_t {
      STATUS = 0,
      BLOCK_REQUEST,
      BLOCK_ANNOUNCE,
      TRANSACTIONS,
      CONSENSUS,
      UNKNOWN = 99
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

    if (type > kMaxMessageTypes) {
      t = GossipMessage::Type::UNKNOWN;
    } else {
      t = static_cast<GossipMessage::Type>(type);
    }
    return s;
  }
}  // namespace kagome::network

#endif  // KAGOME_GOSSIP_MESSAGE_HPP
