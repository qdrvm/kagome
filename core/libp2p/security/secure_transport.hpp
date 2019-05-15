/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SECURE_TRANSPORT_HPP
#define KAGOME_SECURE_TRANSPORT_HPP

#include <outcome/outcome.hpp>
#include "libp2p/peer/peer_id.hpp"
#include "libp2p/peer/protocol.hpp"
#include "libp2p/security/secure_connection.hpp"
#include "libp2p/transport/connection.hpp"

namespace libp2p::security {

  struct SecureTransport {
    virtual ~SecureTransport() = default;

    // returns a string, associated with this protocol
    virtual const peer::Protocol getProtocol() const = 0;

    virtual outcome::result<std::shared_ptr<SecureConnection>> secureInbound(
        std::shared_ptr<transport::Connection> inbound) = 0;

    virtual outcome::result<std::shared_ptr<SecureConnection>> secureOutbound(
        std::shared_ptr<transport::Connection> inbound,
        const peer::PeerId &p) = 0;
  };

}  // namespace libp2p::security

#endif  // KAGOME_SECURE_TRANSPORT_HPP
