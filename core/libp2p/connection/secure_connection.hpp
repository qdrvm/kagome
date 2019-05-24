/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SECURE_CONNECTION_HPP
#define KAGOME_SECURE_CONNECTION_HPP

#include "libp2p/connection/raw_connection.hpp"
#include "libp2p/crypto/key.hpp"
#include "libp2p/peer/peer_id.hpp"

namespace libp2p::connection {

  /**
   * @brief Class that represents a connection that is authenticated and
   * encrypted.
   */
  struct SecureConnection : public connection::RawConnection {
    ~SecureConnection() override = default;

    virtual peer::PeerId localPeer() const = 0;
    virtual peer::PeerId remotePeer() const = 0;
    virtual crypto::PublicKey remotePublicKey() const = 0;

    // TODO(warchant): figure out, if it is needed
    // virtual crypto::PrivateKey localPrivateKey() const = 0;
  };

}  // namespace libp2p::connection

#endif  // KAGOME_SECURE_CONNECTION_HPP
