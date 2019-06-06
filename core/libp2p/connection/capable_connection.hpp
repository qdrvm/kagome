/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CAPABLE_CONNECTION_HPP
#define KAGOME_CAPABLE_CONNECTION_HPP

#include "libp2p/connection/secure_connection.hpp"
#include "libp2p/connection/stream.hpp"

namespace libp2p::connection {

  // connection that provides basic libp2p requirements to the connection
  struct CapableConnection : public SecureConnection {
    ~CapableConnection() override = default;

    /**
     * Start to process incoming messages for this connection
     * @return nothing or error; returns when the connection closes
     */
    virtual outcome::result<void> start() = 0;

    /**
     * @brief Opens new stream using this connection.
     */
    virtual outcome::result<std::shared_ptr<Stream>> newStream() = 0;
  };

}  // namespace libp2p::connection

#endif  // KAGOME_CAPABLE_CONNECTION_HPP
