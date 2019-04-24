/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol_muxer/multiselect/multiselect_communicator.hpp"

#include <string_view>

namespace libp2p::protocol_muxer {
  using kagome::common::Buffer;

  std::optional<MultiselectCommunicator::MultiselectMessage>
  MultiselectCommunicator::parseMessage(
      const kagome::common::Buffer &bytes) const {}

  Buffer MultiselectCommunicator::openingMsg() const {
    return multiselect_header_;
  }

  Buffer MultiselectCommunicator::lsMsg() const {
    return ls_msg_;
  }

  Buffer MultiselectCommunicator::naMsg() const {
    return na_msg_;
  }

  Buffer MultiselectCommunicator::protocolMsg(
      const multi::Multistream &protocol) const {
    return Buffer{protocol.getBuffer()};
  }

  Buffer MultiselectCommunicator::protocolsMsg(
      gsl::span<multi::Multistream> protocols) const {
    Buffer msg{};
    for (const auto &proto : protocols) {
      msg.putBuffer(proto.getBuffer());
    }
    return msg;
  }
}  // namespace libp2p::protocol_muxer
