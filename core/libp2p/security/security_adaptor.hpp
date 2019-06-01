/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SECURITY_ADAPTOR_HPP
#define KAGOME_SECURITY_ADAPTOR_HPP

#include <memory>

#include <outcome/outcome.hpp>
#include "libp2p/connection/raw_connection.hpp"
#include "libp2p/connection/secure_connection.hpp"
#include "libp2p/peer/peer_id.hpp"
#include "libp2p/peer/protocol.hpp"

namespace libp2p::security {

  /**
   * @brief Adaptor, which is used as base class for all security modules (e.g.
   * SECIO, Noise, TLS...)
   */
  struct SecurityAdaptor {
    virtual ~SecurityAdaptor() = default;

    /**
     * @brief Returns a string-identifier, associated with this protocol.
     * @example /tls/1.0.0
     * (https://github.com/libp2p/go-libp2p-tls/blob/master/transport.go#L22)
     */
    virtual peer::Protocol getProtocolId() const = 0;

    /**
     * @brief Secure the connection, either locally or by communicating with
     * opposing node via inbound connection (receid in listener).
     * @param inbound connection
     * @return secured connection
     */
    virtual outcome::result<std::shared_ptr<connection::SecureConnection>>
    secureInbound(std::shared_ptr<connection::RawConnection> inbound) = 0;

    /**
     * @brief Secure the connection, either locally or by communicating with
     * opposing node via outbound connection (we are initiator).
     * @param outbound connection
     * @param p remote peer id, we want to establish a secure conn
     * @return secured connection
     */
    virtual outcome::result<std::shared_ptr<connection::SecureConnection>>
    secureOutbound(std::shared_ptr<connection::RawConnection> outbound,
                   const peer::PeerId &p) = 0;
  };

}  // namespace libp2p::security

#endif  // KAGOME_SECURITY_ADAPTOR_HPP
