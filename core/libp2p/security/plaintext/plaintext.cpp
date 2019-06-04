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

  outcome::result<std::shared_ptr<connection::SecureConnection>>
  Plaintext::secureInbound(std::shared_ptr<connection::RawConnection> inbound) {
    return std::make_shared<connection::PlaintextConnection>(
        std::move(inbound));
  }

  outcome::result<std::shared_ptr<connection::SecureConnection>>
  Plaintext::secureOutbound(std::shared_ptr<connection::RawConnection> outbound,
                            const peer::PeerId &) {
    return std::make_shared<connection::PlaintextConnection>(
        std::move(outbound));
  }
}  // namespace libp2p::security
