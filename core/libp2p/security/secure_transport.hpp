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

  /**
   * @brief Adaptor, which is used as base class for all security modules (e.g.
   * SECIO, Noise, TLS...)
   */
  struct SecureTransport {
    virtual ~SecureTransport() = default;

    /**
     * @brief Returns a string-identifier, associated with this protocol.
     * @example /tls/1.0.0
     * (https://github.com/libp2p/go-libp2p-tls/blob/master/transport.go#L22)
     */
    virtual const peer::Protocol &getId() const = 0;

    /**
     * @brief Secure the connection, either locally or by communicating with
     * opposing node via inbound connection (receid in listener).
     * @param inbound connection
     * @return secured connection
     */
    virtual outcome::result<std::shared_ptr<SecureConnection>> secureInbound(
        std::shared_ptr<transport::Connection> inbound) = 0;

    /**
     * @brief Secure the connection, either locally or by communicating with
     * opposing node via outbound connection (we are initiator).
     * @param outbound connection
     * @param p remote peer id, we want to establish a secure conn
     * @return secured connection
     */
    virtual outcome::result<std::shared_ptr<SecureConnection>> secureOutbound(
        std::shared_ptr<transport::Connection> outbound,
        const peer::PeerId &p) = 0;
  };

}  // namespace libp2p::security

#endif  // KAGOME_SECURE_TRANSPORT_HPP
