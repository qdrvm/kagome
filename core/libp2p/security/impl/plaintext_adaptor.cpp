/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/security/impl/plaintext_adaptor.hpp"

#include "libp2p/connection/security_conn_impl/plaintext_connection.hpp"

namespace libp2p::security {
  const peer::Protocol PlaintextAdaptor::kProtocolId = "/plaintext/1.0.0";

  const peer::Protocol &PlaintextAdaptor::getProtocolId() const {
    return kProtocolId;
  }

  outcome::result<std::shared_ptr<connection::SecureConnection>>
  PlaintextAdaptor::secureInbound(
      std::shared_ptr<connection::RawConnection> inbound) {
    return std::make_shared<connection::PlaintextConnection>(
        std::move(inbound));
  }

  outcome::result<std::shared_ptr<connection::SecureConnection>>
  PlaintextAdaptor::secureOutbound(
      std::shared_ptr<connection::RawConnection> outbound,
      const peer::PeerId &) {
    return std::make_shared<connection::PlaintextConnection>(
        std::move(outbound));
  }
}  // namespace libp2p::security
