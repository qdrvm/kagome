/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CAPABLE_CONNECTION_HPP
#define KAGOME_CAPABLE_CONNECTION_HPP

#include "libp2p/connection/multiaddr_connection.hpp"
#include "libp2p/connection/raw_connection.hpp"
#include "libp2p/connection/secure_connection.hpp"
#include "libp2p/connection/stream.hpp"

namespace libp2p::connection {

  // connection that provides basic libp2p requirements to the connection
  struct CapableConnection : public MultiaddrConnection,
                             public SecureConnection {
    ~CapableConnection() override = default;

    virtual outcome::result<Stream> newStream() = 0;
  };

}  // namespace libp2p::connection

#endif  // KAGOME_CAPABLE_CONNECTION_HPP
