/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/security/plaintext/plaintext.hpp"

#include "libp2p/connection/plaintext/plaintext.hpp"

namespace libp2p::security {
  peer::Protocol Plaintext::getProtocolId() const {
    // TODO(akvinikym) 29.05.19: think about creating SecurityProtocolRegister
    return "/plaintext/1.0.0";
  }

  void Plaintext::secureInbound(
      std::shared_ptr<connection::RawConnection> inbound,
      SecConnCallbackFunc cb) {
    cb(std::make_shared<connection::PlaintextConnection>(std::move(inbound)));
  }

  void Plaintext::secureOutbound(
      std::shared_ptr<connection::RawConnection> outbound, const peer::PeerId &,
      SecConnCallbackFunc cb) {
    cb(std::make_shared<connection::PlaintextConnection>(std::move(outbound)));
  }
}  // namespace libp2p::security
