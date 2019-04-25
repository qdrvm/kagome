/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol_muxer/multiselect/message_manager.hpp"

#include <boost/format.hpp>

namespace {
  /// produces protocol string to be sent: '/p2p/PEER_ID/PROTOCOL\n'
  static const boost::format kProtocolTemplate = boost::format("/p2p/%s%s\n");
}  // namespace

namespace libp2p::protocol_muxer {
  using kagome::common::Buffer;

  std::optional<MessageManager::MultiselectMessage>
  MessageManager::parseMessage(const kagome::common::Buffer &bytes) const {
    return {};
  }

  Buffer MessageManager::openingMsg(const peer::PeerId &peer_id) const {
    auto opening_string = kProtocolTemplate % peer_id.getPeerId().

    kagome::common::Buffer{}
        .put(multi::UVarint{kProtocolHeaderString.size()}.toBytes())
        .put(kProtocolHeaderString);

    return multiselect_header_;
  }

  Buffer MessageManager::lsMsg() const {
    return ls_msg_;
  }

  Buffer MessageManager::naMsg() const {
    return na_msg_;
  }

  Buffer MessageManager::protocolMsg(const peer::PeerId &peer_id,
                                     const multi::Multistream &protocol) const {
    auto protocol_string =
        kProtocolTemplate % return Buffer{protocol.getBuffer()};
  }

  Buffer MessageManager::protocolsMsg(
      const peer::PeerId &peer_id,
      gsl::span<const multi::Multistream> protocols) const {
    Buffer msg{};
    for (const auto &proto : protocols) {
      msg.putBuffer(proto.getBuffer());
    }
    return msg;
  }
}  // namespace libp2p::protocol_muxer
